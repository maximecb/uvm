#include <stdio.h>
#include <stdlib.h>
#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/math.h>
#include <uvm/3dmath.h>
#include <uvm/graphics.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600

#define remap(v, a0, a1, b0, b1) (b0 + (b1 - b0) * ((v) - a0) / (a1 - a0))

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Cube vertices in [-1, 1]
float verts[8][3] = {
    {-1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f },
    { 1.0f,-1.0f, 1.0f },
    {-1.0f,-1.0f, 1.0f },
    {-1.0f, 1.0f,-1.0f },
    { 1.0f, 1.0f,-1.0f },
    { 1.0f,-1.0f,-1.0f },
    {-1.0f,-1.0f,-1.0f },
};

vec3 cube_pos = { 0.0f, 0.0f, -6.0f };

// Cube rotation angle
float angle = 0.0f;

// Perspective projection matrix
mat44 persp;

// Translation matrix for the cube
mat44 trans;

// Draw a line between two 3D points
void draw_line3d(vec3 v0, vec3 v1, u32 color)
{
    vec3 v_tmp;
    mat44_transform(persp, v0, v_tmp);
    int x0 = (int)remap(v_tmp[0], -1.0f, 1.0f, 0.0f, (float)FRAME_WIDTH);
    int y0 = (int)remap(v_tmp[1], -1.0f, 1.0f, 0.0f, (float)FRAME_HEIGHT);
    //printf("x0=%d, y0=%d\n", x0, y0);

    mat44_transform(persp, v1, v_tmp);
    int x1 = (int)remap(v_tmp[0], -1.0f, 1.0f, 0.0f, (float)FRAME_WIDTH);
    int y1 = (int)remap(v_tmp[1], -1.0f, 1.0f, 0.0f, (float)FRAME_HEIGHT);

    // TODO: we should handle coordinates that are outside the screen

    draw_line(
        (u32*)frame_buffer,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        (u32)x0,
        (u32)y0,
        (u32)x1,
        (u32)y1,
        color
    );
}

// Transform and draw a 3D line
void trans_line3d(mat44 trans, vec3 _v0, vec3 _v1)
{
    vec3 v0;
    vec3 v1;
    mat44_transform(trans, _v0, v0);
    mat44_transform(trans, _v1, v1);
    draw_line3d(v0, v1, COLOR_PURPLE);
}

void anim_callback()
{
    u64 start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, 800 * 600);

    angle = angle + 0.01f;

    // Rotation matrices for the cube
    mat44 rotx;
    mat44 roty;
    mat44_rotx(angle, rotx);
    mat44_roty(angle, roty);

    mat44 rot;
    mat44 m_cube;
    mat44_mul(roty, rotx, rot);
    mat44_mul(rot, trans, m_cube);

    trans_line3d(m_cube, verts[0], verts[1]);
    trans_line3d(m_cube, verts[1], verts[2]);
    trans_line3d(m_cube, verts[2], verts[3]);
    trans_line3d(m_cube, verts[3], verts[0]);

    trans_line3d(m_cube, verts[4], verts[5]);
    trans_line3d(m_cube, verts[5], verts[6]);
    trans_line3d(m_cube, verts[6], verts[7]);
    trans_line3d(m_cube, verts[7], verts[4]);

    trans_line3d(m_cube, verts[0], verts[4]);
    trans_line3d(m_cube, verts[1], verts[5]);
    trans_line3d(m_cube, verts[2], verts[6]);
    trans_line3d(m_cube, verts[3], verts[7]);

    window_draw_frame(0, frame_buffer);

    u64 end_time = time_current_ms();
    printf("render time: %dms\n", end_time - start_time);

    // Schedule a fixed rate update for the next frame (60fps)
    fixed_rate_update(start_time, 1000 / 60, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    if (keycode == KEY_ESCAPE)
    {
        exit(0);
    }
}

int main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Rotating 3D Cube Example", 0);
    window_on_keydown(0, keydown);
    time_delay_cb(0, anim_callback);
    enable_event_loop();

    // Setup the perspective projection matrix
    perspective(
        DEG2RAD(40.0f),
        (float)FRAME_WIDTH / (float)FRAME_HEIGHT,
        0.1f,   // near,
        100.0f, // far,
        persp
    );

    // Translation matrix for the cube
    mat44_translate(cube_pos, trans);

    return 0;
}
