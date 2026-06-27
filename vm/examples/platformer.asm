#
# A small platformer game written directly in UVM assembly.
#
# Controls:
#   Left / A   : move left
#   Right / D  : move right
#   Space/Up/W : jump (only while standing on a platform)
#   Escape     : quit
#
# Goal: climb the platforms and touch the gold flag at the top.
#
# Physics is done in 1/256-pixel fixed point so that gravity and
# velocities have sub-pixel precision. Pixel coordinates are obtained
# by shifting fixed-point values right by 8 bits.
#
# Window: 640 x 480, BGRA frame buffer.
#

.data;

# 640 * 480 * 4 (BGRA)
PIXEL_BUFFER:
.zero 1228800;

# Buffer for window events
EVENT:
.zero 256;

WINDOW_TITLE:
.stringz "UVM Platformer";

WIN_STR:
.stringz "You reached the goal!\n";

# Platform table: 5 platforms, each is { x, y, w, h } in pixels (i32).
.align 4;
PLATFORMS:
# Ground
.i32 0; .i32 448; .i32 640; .i32 32;
# P1
.i32 100; .i32 376; .i32 120; .i32 16;
# P2
.i32 300; .i32 304; .i32 120; .i32 16;
# P3
.i32 120; .i32 232; .i32 120; .i32 16;
# P4 (goal platform)
.i32 340; .i32 176; .i32 150; .i32 16;

.code;

#
# Local variables (all fixed point unless noted):
#   L0  px         player x
#   L1  py         player y
#   L2  vx         player x velocity
#   L3  vy         player y velocity
#   L4  on_ground  1 if standing on a platform (integer flag)
#   L5  key_left   1 if left is held (integer flag)
#   L6  key_right  1 if right is held (integer flag)
#   L7  running    1 while the game loop runs (integer flag)
#   L8  i          platform loop index (integer)
#   L9  won        1 while overlapping the goal (integer flag)
#   L10 gx         scratch: platform x / base address
#   L11 gy         scratch: platform y
#   L12 gw         scratch: platform w
#   L13 gh         scratch: platform h
#   L14 jump_held  1 while a jump key is held (edge detection)
#
push 10240;     # L0  px = 40 * 256
push 89600;     # L1  py = 350 * 256
push 0;         # L2  vx
push 0;         # L3  vy
push 0;         # L4  on_ground
push 0;         # L5  key_left
push 0;         # L6  key_right
push 1;         # L7  running
push 0;         # L8  i
push 0;         # L9  won
push 0;         # L10 gx
push 0;         # L11 gy
push 0;         # L12 gw
push 0;         # L13 gh
push 0;         # L14 jump_held (1 while a jump key is held)

# Create the window
push 640;
push 480;
push WINDOW_TITLE;
push 0;
syscall window_create;
pop;

MAIN_LOOP:

# =====================================================================
# Input: drain all pending events
# =====================================================================
POLL:
push EVENT;
syscall window_poll_event;
jz POLL_DONE;            # no event available -> stop polling

# kind = *(u16*)EVENT
push EVENT;
load_u16;

# EVENT_QUIT == 0
dup; push 0; eq_u64; jnz EV_QUIT;
# EVENT_KEYDOWN == 1
dup; push 1; eq_u64; jnz EV_KEYDOWN;
# EVENT_KEYUP == 2
dup; push 2; eq_u64; jnz EV_KEYUP;
# Anything else: ignore
pop;
jmp POLL;

EV_QUIT:
pop;                     # drop kind
push 0; set_local 7;     # running = 0
jmp POLL_DONE;

EV_KEYDOWN:
pop;                     # drop kind
push EVENT; push 4; add_u64; load_u16;   # key = *(u16*)(EVENT+4)
# Left: KEY_LEFT (16001) or A (65)
dup; push 16001; eq_u64; jnz KD_LEFT;
dup; push 65;    eq_u64; jnz KD_LEFT;
# Right: KEY_RIGHT (16002) or D (68)
dup; push 16002; eq_u64; jnz KD_RIGHT;
dup; push 68;    eq_u64; jnz KD_RIGHT;
# Jump: SPACE (32), KEY_UP (16003) or W (87)
dup; push 32;    eq_u64; jnz KD_JUMP;
dup; push 16003; eq_u64; jnz KD_JUMP;
dup; push 87;    eq_u64; jnz KD_JUMP;
# Escape (27): quit
dup; push 27;    eq_u64; jnz KD_ESC;
pop;
jmp POLL;

KD_LEFT:
pop; push 1; set_local 5; jmp POLL;
KD_RIGHT:
pop; push 1; set_local 6; jmp POLL;
KD_ESC:
pop; push 0; set_local 7; jmp POLL_DONE;
KD_JUMP:
pop;
get_local 14; jnz POLL;        # key repeat while held: ignore
push 1; set_local 14;          # latch the held state on the rising edge
get_local 4; jz POLL;          # ignore jump if not on the ground
push -1550; set_local 3;       # vy = JUMP velocity (upwards)
push 0; set_local 4;           # on_ground = 0
jmp POLL;

EV_KEYUP:
pop;                     # drop kind
push EVENT; push 4; add_u64; load_u16;   # key
dup; push 16001; eq_u64; jnz KU_LEFT;
dup; push 65;    eq_u64; jnz KU_LEFT;
dup; push 16002; eq_u64; jnz KU_RIGHT;
dup; push 68;    eq_u64; jnz KU_RIGHT;
dup; push 32;    eq_u64; jnz KU_JUMP;
dup; push 16003; eq_u64; jnz KU_JUMP;
dup; push 87;    eq_u64; jnz KU_JUMP;
pop;
jmp POLL;

KU_LEFT:
pop; push 0; set_local 5; jmp POLL;
KU_RIGHT:
pop; push 0; set_local 6; jmp POLL;
KU_JUMP:
pop; push 0; set_local 14; jmp POLL;     # release: allow the next jump

POLL_DONE:

# Quit if requested
get_local 7; jz EXIT;

# =====================================================================
# Compute horizontal velocity from held keys
# =====================================================================
push 0; set_local 2;     # vx = 0
get_local 6; jz SKIP_R;
    get_local 2; push 720; add_u64; set_local 2;   # moving right
SKIP_R:
get_local 5; jz SKIP_L;
    get_local 2; push 720; sub_u64; set_local 2;   # moving left
SKIP_L:

# =====================================================================
# Horizontal movement + collision
# =====================================================================
get_local 0; get_local 2; add_u64; set_local 0;    # px += vx

# Clamp to the screen: px in [0, (640-24) * 256] = [0, 157696]
get_local 0; push 0; lt_i64; jz HCLAMP_R;
    push 0; set_local 0;
HCLAMP_R:
get_local 0; push 157696; gt_i64; jz HCLAMP_DONE;
    push 157696; set_local 0;
HCLAMP_DONE:

push 0; set_local 8;     # i = 0
HLOOP:
get_local 8; push 5; ge_i64; jnz HLOOP_END;

# Load platform i into L10..L13 as fixed point
get_local 8; push 16; mul_u64; push PLATFORMS; add_u64;        # base addr
dup; load_u32; push 8; lshift_u64; set_local 10;               # gx
dup; push 4;  add_u64; load_u32; push 8; lshift_u64; set_local 11;  # gy
dup; push 8;  add_u64; load_u32; push 8; lshift_u64; set_local 12;  # gw
push 12; add_u64; load_u32; push 8; lshift_u64; set_local 13;       # gh

# overlap? AABB_OVERLAP(px, py, PW, PH, gx, gy, gw, gh)
get_local 0; get_local 1; push 6144; push 8192;
get_local 10; get_local 11; get_local 12; get_local 13;
call AABB_OVERLAP, 8;
jz HNEXT;

# Resolve: if vx > 0 push player to the left of the platform,
#          if vx < 0 push player to the right.
get_local 2; push 0; gt_i64; jz HRES_NEG;
    get_local 10; push 6144; sub_u64; set_local 0;   # px = gx - PW
    push 0; set_local 2;                             # vx = 0
    jmp HNEXT;
HRES_NEG:
get_local 2; push 0; lt_i64; jz HNEXT;
    get_local 10; get_local 12; add_u64; set_local 0; # px = gx + gw
    push 0; set_local 2;

HNEXT:
get_local 8; push 1; add_u64; set_local 8;
jmp HLOOP;
HLOOP_END:

# =====================================================================
# Vertical movement + collision
# =====================================================================
# vy += GRAVITY, capped at MAX_FALL
get_local 3; push 60; add_u64; set_local 3;
get_local 3; push 2400; gt_i64; jz VCAP_DONE;
    push 2400; set_local 3;
VCAP_DONE:

get_local 1; get_local 3; add_u64; set_local 1;    # py += vy
push 0; set_local 4;                                # assume airborne

push 0; set_local 8;     # i = 0
VLOOP:
get_local 8; push 5; ge_i64; jnz VLOOP_END;

# Load platform i into L10..L13 as fixed point
get_local 8; push 16; mul_u64; push PLATFORMS; add_u64;
dup; load_u32; push 8; lshift_u64; set_local 10;
dup; push 4;  add_u64; load_u32; push 8; lshift_u64; set_local 11;
dup; push 8;  add_u64; load_u32; push 8; lshift_u64; set_local 12;
push 12; add_u64; load_u32; push 8; lshift_u64; set_local 13;

get_local 0; get_local 1; push 6144; push 8192;
get_local 10; get_local 11; get_local 12; get_local 13;
call AABB_OVERLAP, 8;
jz VNEXT;

# Resolve: if vy > 0 we landed (snap to top), if vy < 0 we hit a ceiling.
get_local 3; push 0; gt_i64; jz VRES_NEG;
    get_local 11; push 8192; sub_u64; set_local 1;   # py = gy - PH
    push 0; set_local 3;                             # vy = 0
    push 1; set_local 4;                             # on_ground = 1
    jmp VNEXT;
VRES_NEG:
get_local 3; push 0; lt_i64; jz VNEXT;
    get_local 11; get_local 13; add_u64; set_local 1; # py = gy + gh
    push 0; set_local 3;

VNEXT:
get_local 8; push 1; add_u64; set_local 8;
jmp VLOOP;
VLOOP_END:

# Respawn if the player falls off the bottom of the screen
get_local 1; push 8; rshift_i64; push 500; gt_i64; jz NO_RESPAWN;
    push 10240; set_local 0;
    push 89600; set_local 1;
    push 0; set_local 2;
    push 0; set_local 3;
NO_RESPAWN:

# =====================================================================
# Goal check
# =====================================================================
# Goal flag rectangle in pixels: x=405, y=144, w=16, h=32 -> * 256
get_local 0; get_local 1; push 6144; push 8192;
push 103680; push 36864; push 4096; push 8192;
call AABB_OVERLAP, 8;
jz GOAL_OUT;
    get_local 9; jnz GOAL_DONE;     # already announced this touch
    push 1; set_local 9;
    push WIN_STR; syscall print_str;
    jmp GOAL_DONE;
GOAL_OUT:
    push 0; set_local 9;
GOAL_DONE:

# =====================================================================
# Render
# =====================================================================
# Clear to sky blue (0xFF87CEEB in BGRA-as-u32)
push PIXEL_BUFFER; push_u32 0xFF87CEEB; push 307200; syscall memset32;

# Draw platforms (green)
push 0; set_local 8;
RLOOP:
get_local 8; push 5; ge_i64; jnz RLOOP_END;
get_local 8; push 16; mul_u64; push PLATFORMS; add_u64; set_local 10;  # base
get_local 10;            load_u32;   # x
get_local 10; push 4;  add_u64; load_u32;   # y
get_local 10; push 8;  add_u64; load_u32;   # w
get_local 10; push 12; add_u64; load_u32;   # h
push_u32 0xFF3CB44B;     # green
call FILL_RECT, 5; pop;
get_local 8; push 1; add_u64; set_local 8;
jmp RLOOP;
RLOOP_END:

# Draw the goal as a little door
push 405; push 144;
call DRAW_DOOR, 2; pop;

# Draw the player as a little guy. Convert fixed point -> pixels.
get_local 0; push 8; rshift_i64;
get_local 1; push 8; rshift_i64;
call DRAW_GUY, 2; pop;

# Present the frame
push 0; push PIXEL_BUFFER; syscall window_draw_frame;

# ~60 FPS
push 16; syscall thread_sleep;

jmp MAIN_LOOP;

EXIT:
push 0;
syscall exit;

#
# FILL_RECT(i64 x, i64 y, i64 w, i64 h, u32 color)
#
# Draw a filled, screen-clipped rectangle into PIXEL_BUFFER.
# Coordinates are in pixels. Returns 0.
#
# Locals:
#   L0 x0   clipped left
#   L1 y0   clipped top
#   L2 x1   clipped right
#   L3 y1   clipped bottom
#   L4 rw   clipped width
#   L5 row  current row
#
FILL_RECT:
push 0; push 0; push 0; push 0; push 0; push 0;

# x0 = max(0, x)
get_arg 0;
dup; push 0; lt_i64; jz FR_X0;
    pop; push 0;
FR_X0:
set_local 0;

# y0 = max(0, y)
get_arg 1;
dup; push 0; lt_i64; jz FR_Y0;
    pop; push 0;
FR_Y0:
set_local 1;

# x1 = min(640, x + w)
get_arg 0; get_arg 2; add_u64;
dup; push 640; gt_i64; jz FR_X1;
    pop; push 640;
FR_X1:
set_local 2;

# y1 = min(480, y + h)
get_arg 1; get_arg 3; add_u64;
dup; push 480; gt_i64; jz FR_Y1;
    pop; push 480;
FR_Y1:
set_local 3;

# Reject empty rectangles
get_local 0; get_local 2; ge_i64; jnz FR_RET;
get_local 1; get_local 3; ge_i64; jnz FR_RET;

# rw = x1 - x0
get_local 2; get_local 0; sub_u64; set_local 4;

# row = y0
get_local 1; set_local 5;

FR_LOOP:
get_local 5; get_local 3; ge_i64; jnz FR_RET;
# addr = PIXEL_BUFFER + (row * 640 + x0) * 4
get_local 5; push 640; mul_u64;
get_local 0; add_u64;
push 4; mul_u64;
push PIXEL_BUFFER; add_u64;
# memset32(addr, color, rw)
get_arg 4;
get_local 4;
syscall memset32;
get_local 5; push 1; add_u64; set_local 5;
jmp FR_LOOP;

FR_RET:
push 0;
ret;

#
# AABB_OVERLAP(ax, ay, aw, ah, bx, by, bw, bh) -> 1 if the two
# axis-aligned boxes overlap, else 0. All arguments share the same units.
#
AABB_OVERLAP:
# ax < bx + bw
get_arg 0; get_arg 4; get_arg 6; add_u64; lt_i64;
# ax + aw > bx
get_arg 0; get_arg 2; add_u64; get_arg 4; gt_i64;
and_u64;
# ay < by + bh
get_arg 1; get_arg 5; get_arg 7; add_u64; lt_i64;
and_u64;
# ay + ah > by
get_arg 1; get_arg 3; add_u64; get_arg 5; gt_i64;
and_u64;
ret;

#
# DRAW_GUY(i64 x, i64 y)
#
# Draw a little character inside the 24 x 32 player box whose top-left
# corner is at the given pixel coordinates. Built out of FILL_RECT parts:
# hair, head, eyes, shirt, arms, legs and shoes. Returns 0.
#
DRAW_GUY:
# hair
get_arg 0; push 6;  add_u64; get_arg 1; push 0;  add_u64; push 12; push 3;  push_u32 0xFF5A3A1A; call FILL_RECT, 5; pop;
# head
get_arg 0; push 6;  add_u64; get_arg 1; push 3;  add_u64; push 12; push 8;  push_u32 0xFFF0C8A0; call FILL_RECT, 5; pop;
# left eye
get_arg 0; push 9;  add_u64; get_arg 1; push 6;  add_u64; push 2;  push 2;  push_u32 0xFF202020; call FILL_RECT, 5; pop;
# right eye
get_arg 0; push 13; add_u64; get_arg 1; push 6;  add_u64; push 2;  push 2;  push_u32 0xFF202020; call FILL_RECT, 5; pop;
# shirt / torso
get_arg 0; push 4;  add_u64; get_arg 1; push 11; add_u64; push 16; push 10; push_u32 0xFFE63C3C; call FILL_RECT, 5; pop;
# left arm
get_arg 0; push 1;  add_u64; get_arg 1; push 11; add_u64; push 3;  push 9;  push_u32 0xFFF0C8A0; call FILL_RECT, 5; pop;
# right arm
get_arg 0; push 20; add_u64; get_arg 1; push 11; add_u64; push 3;  push 9;  push_u32 0xFFF0C8A0; call FILL_RECT, 5; pop;
# left leg
get_arg 0; push 5;  add_u64; get_arg 1; push 21; add_u64; push 6;  push 9;  push_u32 0xFF2A50A0; call FILL_RECT, 5; pop;
# right leg
get_arg 0; push 13; add_u64; get_arg 1; push 21; add_u64; push 6;  push 9;  push_u32 0xFF2A50A0; call FILL_RECT, 5; pop;
# left shoe
get_arg 0; push 5;  add_u64; get_arg 1; push 29; add_u64; push 6;  push 3;  push_u32 0xFF3A2A1A; call FILL_RECT, 5; pop;
# right shoe
get_arg 0; push 13; add_u64; get_arg 1; push 29; add_u64; push 6;  push 3;  push_u32 0xFF3A2A1A; call FILL_RECT, 5; pop;
push 0;
ret;

#
# DRAW_DOOR(i64 x, i64 y)
#
# Draw a little wooden door inside the 16 x 32 goal box whose top-left
# corner is at the given pixel coordinates: outer frame, recessed inner
# panel, two sub-panels and a gold knob. Returns 0.
#
DRAW_DOOR:
# frame
get_arg 0; push 0;  add_u64; get_arg 1; push 0;  add_u64; push 16; push 32; push_u32 0xFF8B5A2B; call FILL_RECT, 5; pop;
# recessed inner panel
get_arg 0; push 2;  add_u64; get_arg 1; push 2;  add_u64; push 12; push 28; push_u32 0xFF6B3F1A; call FILL_RECT, 5; pop;
# upper sub-panel
get_arg 0; push 4;  add_u64; get_arg 1; push 4;  add_u64; push 8;  push 10; push_u32 0xFF8B5A2B; call FILL_RECT, 5; pop;
# lower sub-panel
get_arg 0; push 4;  add_u64; get_arg 1; push 16; add_u64; push 8;  push 12; push_u32 0xFF8B5A2B; call FILL_RECT, 5; pop;
# door knob
get_arg 0; push 11; add_u64; get_arg 1; push 18; add_u64; push 2;  push 3;  push_u32 0xFFFFD700; call FILL_RECT, 5; pop;
push 0;
ret;
