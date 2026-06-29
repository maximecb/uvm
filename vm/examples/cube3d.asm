#
# A rotating, solid-shaded 3D cube rendered in UVM assembly.
#
# Each frame the eight cube vertices are rotated by a 3x3 matrix built from
# sin/cos of two running angles, projected with a perspective divide, and the
# six faces are drawn as filled triangles. Back-facing polygons are culled
# (the cube is convex, so culling alone gives correct visibility with no depth
# buffer), and each visible face is flat-shaded by the dot product of its
# rotated normal with a fixed light direction, so the cube reads as a solid
# 3D object rather than a flat silhouette.
#
# Press Escape (or close the window) to quit.
#
# Window: 640 x 480, BGRA frame buffer. Colours are written as 0xAARRGGBB
# literals, which land in memory as B,G,R,A.
#

.data;

# 640 * 480 * 4 (BGRA)
.align 4;
PIXEL_BUFFER:
.zero 1228800;

# Buffer for window events
EVENT:
.zero 256;

WINDOW_TITLE:
.stringz "UVM 3D Cube";

# Cube vertices: 8 points at the corners of a [-1, 1] cube (f32 x,y,z).
.align 4;
CUBE_VERTS:
.f32 -1; .f32 -1; .f32 -1;   # v0
.f32  1; .f32 -1; .f32 -1;   # v1
.f32  1; .f32  1; .f32 -1;   # v2
.f32 -1; .f32  1; .f32 -1;   # v3
.f32 -1; .f32 -1; .f32  1;   # v4
.f32  1; .f32 -1; .f32  1;   # v5
.f32  1; .f32  1; .f32  1;   # v6
.f32 -1; .f32  1; .f32  1;   # v7

# Outward unit normal for each of the 6 faces (f32 x,y,z).
CUBE_NORMS:
.f32  1; .f32  0; .f32  0;   # face 0  +X
.f32 -1; .f32  0; .f32  0;   # face 1  -X
.f32  0; .f32  1; .f32  0;   # face 2  +Y
.f32  0; .f32 -1; .f32  0;   # face 3  -Y
.f32  0; .f32  0; .f32  1;   # face 4  +Z
.f32  0; .f32  0; .f32 -1;   # face 5  -Z

# Vertex indices for each face, 4 per face, CCW when seen from outside so the
# cross product of the first edges matches the stored normal above.
FACE_IDX:
.u8 1; .u8 2; .u8 6; .u8 5;   # face 0  +X
.u8 0; .u8 4; .u8 7; .u8 3;   # face 1  -X
.u8 3; .u8 7; .u8 6; .u8 2;   # face 2  +Y
.u8 0; .u8 1; .u8 5; .u8 4;   # face 3  -Y
.u8 5; .u8 6; .u8 7; .u8 4;   # face 4  +Z
.u8 0; .u8 3; .u8 2; .u8 1;   # face 5  -Z

# Base colour for each face (0xAARRGGBB).
.align 4;
FACE_COLOR:
.u32 0xFFE04040;   # red
.u32 0xFF40E040;   # green
.u32 0xFF4060E0;   # blue
.u32 0xFFE0E040;   # yellow
.u32 0xFFE040E0;   # magenta
.u32 0xFF40E0E0;   # cyan

# Projected screen coordinates of the 8 vertices, filled in each frame (i32).
.align 4;
SXP:
.zero 32;
SYP:
.zero 32;

.code;

#
# Main-frame locals (all indices are get_local / set_local indices):
#   L0  running   1 while the loop runs
#   L1  angX      rotation angle about X (f32, radians)
#   L2  angY      rotation angle about Y (f32, radians)
#   L3  ca        cos(angX)        L4  sa  sin(angX)
#   L5  cb        cos(angY)        L6  sb  sin(angY)
#   L7..L15       rotation matrix r00,r01,r02, r10,r11,r12, r20,r21,r22 (f32)
#   L16 i         loop index (vertices, then faces)
#   L17 base      scratch address
#   L18 mx  L19 my  L20 mz   model x/y/z (also reused for normal nx/ny/nz)
#   L21 rx  L22 ry  L23 rz   rotated x/y/z
#   L24 vz        rotated z + camera distance
#   L25 Nx  L26 Ny  L27 Nz   rotated face normal
#   L28 inten     light intensity, 0..256 (integer)
#   L29 col       shaded face colour (u32)
#   L30..L37      face vertices' screen coords ax0,ay0, ax1,ay1, ax2,ay2, ax3,ay3
#   L38 fbase     base offset into FACE_IDX for this face
#   L39 idx       scratch vertex index
#
push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0;
push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0;
push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0;
push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0;

push 1; set_local 0;     # running = 1

# Create the window
push 640;
push 480;
push WINDOW_TITLE;
push 0;
syscall window_create;
pop;

MAIN_LOOP:

# =====================================================================
# Input: drain pending events, quit on EVENT_QUIT or Escape
# =====================================================================
POLL:
push EVENT;
syscall window_poll_event;
jz POLL_DONE;

push EVENT; load_u16;                 # kind
dup; push $EVENT_QUIT;    eq_u64; jnz EV_QUIT;
dup; push $EVENT_KEYDOWN; eq_u64; jnz EV_KEY;
pop;
jmp POLL;

EV_QUIT:
pop;
push 0; set_local 0;
jmp POLL_DONE;

EV_KEY:
pop;
push EVENT; push 4; add_u64; load_u16;   # key
push $KEY_ESCAPE; eq_u64; jnz KEY_ESC;
jmp POLL;

KEY_ESC:
push 0; set_local 0;
jmp POLL_DONE;

POLL_DONE:
get_local 0; jz EXIT;

# =====================================================================
# Advance angles and build the rotation matrix R = Ry(angY) * Rx(angX)
# =====================================================================
get_local 1; push_f32 0.013; add_f32; set_local 1;   # angX += 0.013
get_local 2; push_f32 0.02;  add_f32; set_local 2;   # angY += 0.02

get_local 1; cos_f32; set_local 3;   # ca
get_local 1; sin_f32; set_local 4;   # sa
get_local 2; cos_f32; set_local 5;   # cb
get_local 2; sin_f32; set_local 6;   # sb

# r00 = cb
get_local 5; set_local 7;
# r01 = sb*sa
get_local 6; get_local 4; mul_f32; set_local 8;
# r02 = sb*ca
get_local 6; get_local 3; mul_f32; set_local 9;
# r10 = 0
push_f32 0; set_local 10;
# r11 = ca
get_local 3; set_local 11;
# r12 = -sa
push_f32 0; get_local 4; sub_f32; set_local 12;
# r20 = -sb
push_f32 0; get_local 6; sub_f32; set_local 13;
# r21 = cb*sa
get_local 5; get_local 4; mul_f32; set_local 14;
# r22 = cb*ca
get_local 5; get_local 3; mul_f32; set_local 15;

# =====================================================================
# Clear the frame to a dark background
# =====================================================================
push PIXEL_BUFFER; push_u32 0xFF202830; push 307200; syscall memset32;

# =====================================================================
# Transform and project the 8 vertices into SXP / SYP
# =====================================================================
push 0; set_local 16;
VLOOP:
get_local 16; push 8; ge_i64; jnz VLOOP_END;

get_local 16; push 12; mul_u64; push CUBE_VERTS; add_u64; set_local 17;   # base
get_local 17;                  load_u32; set_local 18;   # mx
get_local 17; push 4; add_u64; load_u32; set_local 19;   # my
get_local 17; push 8; add_u64; load_u32; set_local 20;   # mz

# rx = r00*mx + r01*my + r02*mz
get_local 7; get_local 18; mul_f32; get_local 8; get_local 19; mul_f32; add_f32; get_local 9; get_local 20; mul_f32; add_f32; set_local 21;
# ry = r10*mx + r11*my + r12*mz
get_local 10; get_local 18; mul_f32; get_local 11; get_local 19; mul_f32; add_f32; get_local 12; get_local 20; mul_f32; add_f32; set_local 22;
# rz = r20*mx + r21*my + r22*mz
get_local 13; get_local 18; mul_f32; get_local 14; get_local 19; mul_f32; add_f32; get_local 15; get_local 20; mul_f32; add_f32; set_local 23;

# vz = rz + 4.5 (camera distance)
get_local 23; push_f32 4.5; add_f32; set_local 24;

# sx = 320 + 450*rx/vz
push_f32 320; push_f32 450; get_local 21; mul_f32; get_local 24; div_f32; add_f32; f32_to_i32;
get_local 16; push 4; mul_u64; push SXP; add_u64; swap; store_u32;
# sy = 240 - 450*ry/vz   (screen y grows downward)
push_f32 240; push_f32 450; get_local 22; mul_f32; get_local 24; div_f32; sub_f32; f32_to_i32;
get_local 16; push 4; mul_u64; push SYP; add_u64; swap; store_u32;

get_local 16; push 1; add_u64; set_local 16;
jmp VLOOP;
VLOOP_END:

# =====================================================================
# Draw each visible face as two shaded triangles
# =====================================================================
push 0; set_local 16;
FLOOP:
get_local 16; push 6; ge_i64; jnz FLOOP_END;

# Gather the 4 projected face vertices into L30..L37
get_local 16; push 4; mul_u64; push FACE_IDX; add_u64; set_local 38;
get_local 38;                 load_u8; set_local 39;
get_local 39; push 4; mul_u64; push SXP; add_u64; load_u32; set_local 30;
get_local 39; push 4; mul_u64; push SYP; add_u64; load_u32; set_local 31;
get_local 38; push 1; add_u64; load_u8; set_local 39;
get_local 39; push 4; mul_u64; push SXP; add_u64; load_u32; set_local 32;
get_local 39; push 4; mul_u64; push SYP; add_u64; load_u32; set_local 33;
get_local 38; push 2; add_u64; load_u8; set_local 39;
get_local 39; push 4; mul_u64; push SXP; add_u64; load_u32; set_local 34;
get_local 39; push 4; mul_u64; push SYP; add_u64; load_u32; set_local 35;
get_local 38; push 3; add_u64; load_u8; set_local 39;
get_local 39; push 4; mul_u64; push SXP; add_u64; load_u32; set_local 36;
get_local 39; push 4; mul_u64; push SYP; add_u64; load_u32; set_local 37;

# Back-face cull using the projected (perspective-correct) winding. The signed
# area of the first triangle, (ax1-ax0)*(ay2-ay0) - (ax2-ax0)*(ay1-ay0), is
# positive for front-facing polygons; skip the face when it is <= 0. The cube
# is convex, so this alone gives correct visibility with no depth buffer.
get_local 32; get_local 30; sub_u64; get_local 35; get_local 31; sub_u64; mul_u64;
get_local 34; get_local 30; sub_u64; get_local 33; get_local 31; sub_u64; mul_u64;
sub_u64;
push 0; le_i64; jnz FNEXT;

# Rotate this face's normal by R (for lighting)
get_local 16; push 12; mul_u64; push CUBE_NORMS; add_u64; set_local 17;
get_local 17;                  load_u32; set_local 18;   # nx
get_local 17; push 4; add_u64; load_u32; set_local 19;   # ny
get_local 17; push 8; add_u64; load_u32; set_local 20;   # nz
get_local 7; get_local 18; mul_f32; get_local 8; get_local 19; mul_f32; add_f32; get_local 9; get_local 20; mul_f32; add_f32; set_local 25;   # Nx
get_local 10; get_local 18; mul_f32; get_local 11; get_local 19; mul_f32; add_f32; get_local 12; get_local 20; mul_f32; add_f32; set_local 26;  # Ny
get_local 13; get_local 18; mul_f32; get_local 14; get_local 19; mul_f32; add_f32; get_local 15; get_local 20; mul_f32; add_f32; set_local 27;  # Nz

# Lighting: dot = N . L, with L = (-1, 1, -2) normalised
get_local 25; push_f32 -0.40825; mul_f32;
get_local 26; push_f32  0.40825; mul_f32; add_f32;
get_local 27; push_f32 -0.81650; mul_f32; add_f32;
# clamp the dot product to >= 0
dup; push_f32 0; lt_f32; jz DOT_OK;
    pop; push_f32 0;
DOT_OK:
# shade = 0.25 (ambient) + 0.75 * dot
push_f32 0.75; mul_f32; push_f32 0.25; add_f32;
# inten = shade * 256, clamped to 256
push_f32 256; mul_f32; f32_to_i32;
dup; push 256; gt_i64; jz INT_OK;
    pop; push 256;
INT_OK:
set_local 28;

# col = SHADE(FACE_COLOR[i], inten)
get_local 16; push 4; mul_u64; push FACE_COLOR; add_u64; load_u32;
get_local 28;
call SHADE, 2;
set_local 29;

# Triangulate the quad as (v0,v1,v2) and (v0,v2,v3)
get_local 30; get_local 31; get_local 32; get_local 33; get_local 34; get_local 35; get_local 29; call FILL_TRI, 7; pop;
get_local 30; get_local 31; get_local 34; get_local 35; get_local 36; get_local 37; get_local 29; call FILL_TRI, 7; pop;

FNEXT:
get_local 16; push 1; add_u64; set_local 16;
jmp FLOOP;
FLOOP_END:

# Present the frame and pace to ~60 FPS
push 0; push PIXEL_BUFFER; syscall window_draw_frame;
push 16; syscall thread_sleep;

jmp MAIN_LOOP;

EXIT:
push 0;
syscall exit;

#
# SHADE(u32 base, i64 inten) -> u32
#
# Scale each colour channel of base by inten/256 (inten in 0..256) and return
# the result with a fully opaque alpha. Used for per-face flat shading.
#
# Locals: L0 r, L1 g, L2 b
#
SHADE:
push 0; push 0; push 0;
# r = ((base >> 16) & 0xFF) * inten >> 8
get_arg 0; push 16; rshift_u64; push 255; and_u64; get_arg 1; mul_u64; push 8; rshift_u64;
dup; push 255; gt_i64; jz SH_R;
    pop; push 255;
SH_R:
set_local 0;
# g
get_arg 0; push 8; rshift_u64; push 255; and_u64; get_arg 1; mul_u64; push 8; rshift_u64;
dup; push 255; gt_i64; jz SH_G;
    pop; push 255;
SH_G:
set_local 1;
# b
get_arg 0; push 255; and_u64; get_arg 1; mul_u64; push 8; rshift_u64;
dup; push 255; gt_i64; jz SH_B;
    pop; push 255;
SH_B:
set_local 2;
# recombine: 0xFF000000 | (r<<16) | (g<<8) | b
push_u32 0xFF000000;
get_local 0; push 16; lshift_u64; or_u64;
get_local 1; push 8;  lshift_u64; or_u64;
get_local 2; or_u64;
ret;

#
# FILL_TRI(i64 x0, y0, x1, y1, x2, y2, u32 color) -> 0
#
# Fill a triangle into PIXEL_BUFFER using one horizontal memset32 span per
# scanline, clipped to the 640x480 screen. The three vertices are first sorted
# by y; each scanline is then bounded by the long edge (top->bottom vertex) on
# one side and one of the two short edges on the other.
#
# Locals:
#   L0 ax L1 ay   top vertex      L2 bx L3 by   middle vertex
#   L4 cx L5 cy   bottom vertex   L6 y          current scanline
#   L7 xa         x on the long edge      L8 xb   x on the active short edge
#   L9 yend       clipped bottom scanline
#   L10 left L11 right             clipped span bounds
#
FILL_TRI:
push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0; push 0;

get_arg 0; set_local 0; get_arg 1; set_local 1;
get_arg 2; set_local 2; get_arg 3; set_local 3;
get_arg 4; set_local 4; get_arg 5; set_local 5;

# Sort the three vertices so ay <= by <= cy
get_local 1; get_local 3; gt_i64; jz FT_S1;
    get_local 0; get_local 2; set_local 0; set_local 2;
    get_local 1; get_local 3; set_local 1; set_local 3;
FT_S1:
get_local 3; get_local 5; gt_i64; jz FT_S2;
    get_local 2; get_local 4; set_local 2; set_local 4;
    get_local 3; get_local 5; set_local 3; set_local 5;
FT_S2:
get_local 1; get_local 3; gt_i64; jz FT_S3;
    get_local 0; get_local 2; set_local 0; set_local 2;
    get_local 1; get_local 3; set_local 1; set_local 3;
FT_S3:

# Degenerate (zero height) triangle: nothing to draw
get_local 5; get_local 1; eq_u64; jnz FT_RET;

# y = max(ay, 0)
get_local 1; set_local 6;
get_local 6; push 0; lt_i64; jz FT_YC;
    push 0; set_local 6;
FT_YC:
# yend = min(cy, 480)
get_local 5; set_local 9;
get_local 9; push 480; gt_i64; jz FT_YE;
    push 480; set_local 9;
FT_YE:

FT_LOOP:
get_local 6; get_local 9; ge_i64; jnz FT_RET;

# xa = ax + (cx-ax)*(y-ay)/(cy-ay)   (long edge, cy-ay > 0)
get_local 4; get_local 0; sub_u64;
get_local 6; get_local 1; sub_u64;
mul_u64;
get_local 5; get_local 1; sub_u64;
div_i64;
get_local 0; add_u64;
set_local 7;

# xb from the short edge that covers this scanline
get_local 6; get_local 3; lt_i64; jz FT_BOT;
    # top edge a->b: by-ay > 0 here
    get_local 2; get_local 0; sub_u64;
    get_local 6; get_local 1; sub_u64;
    mul_u64;
    get_local 3; get_local 1; sub_u64;
    div_i64;
    get_local 0; add_u64;
    set_local 8;
    jmp FT_SPAN;
FT_BOT:
    # bottom edge b->c: cy-by > 0 here
    get_local 4; get_local 2; sub_u64;
    get_local 6; get_local 3; sub_u64;
    mul_u64;
    get_local 5; get_local 3; sub_u64;
    div_i64;
    get_local 2; add_u64;
    set_local 8;
FT_SPAN:

# left = min(xa,xb), right = max(xa,xb)
get_local 7; get_local 8; lt_i64; jz FT_LR;
    get_local 7; set_local 10; get_local 8; set_local 11;
    jmp FT_LR2;
FT_LR:
    get_local 8; set_local 10; get_local 7; set_local 11;
FT_LR2:

# Clip the span to [0, 640)
get_local 10; push 0; lt_i64; jz FT_CL;
    push 0; set_local 10;
FT_CL:
get_local 11; push 640; gt_i64; jz FT_CR;
    push 640; set_local 11;
FT_CR:

# Skip empty spans
get_local 11; get_local 10; le_i64; jnz FT_NEXT;

# addr = PIXEL_BUFFER + (y*640 + left) * 4
get_local 6; push 640; mul_u64; get_local 10; add_u64; push 4; mul_u64; push PIXEL_BUFFER; add_u64;
get_arg 6;
get_local 11; get_local 10; sub_u64;
syscall memset32;

FT_NEXT:
get_local 6; push 1; add_u64; set_local 6;
jmp FT_LOOP;

FT_RET:
push 0;
ret;
