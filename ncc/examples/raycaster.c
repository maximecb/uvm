#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/math.h>
#include <uvm/graphics.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

// Horizontal FOV angle in degrees
#define FOV_X 90

#define MAP_WIDTH 10
#define MAP_HEIGHT 10

// The player spawns facing towards +x
// ----> +y
// |
// v
// +x
u8 map[MAP_HEIGHT][MAP_WIDTH] = {
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 1, 0, 1, 1 },
    { 1, 0, 1, 1, 1, 1, 0, 0, 0, 1 },
    { 1, 0, 1, 0, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 1, 0, 0, 1, 0, 0, 0, 1 },
};

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Player position and camera direction
float pos_x = 1.5f;
float pos_y = 1.5f;
float dir = 0.0f;
float dir_x = 1.0f;
float dir_y = 0.0f;

// Draw a line in a top-down view
void draw_world_line(
    int view_width,
    int view_height,
    float x0,
    float y0,
    float x1,
    float y1,
    u32 color,
)
{
    draw_line(
        (u32*)frame_buffer,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        (int)(x0 * view_width / MAP_WIDTH),
        (int)(y0 * view_height / MAP_HEIGHT),
        (int)(x1 * view_width / MAP_WIDTH),
        (int)(y1 * view_height / MAP_HEIGHT),
        color
    );
}

float cast_ray(float cam_x, float cam_y, float dx, float dy)
{
    // Current position along the ray
    float x = cam_x;
    float y = cam_y;

    // Current distance along the ray
    float ray_dist = 0;

    while (true)
    {
        if (x < 0 || y < 0)
        {
            break;
        }

        int i = (int)x;
        int j = (int)y;

        if (i >= MAP_HEIGHT || j >= MAP_WIDTH)
        {
            break;
        }

        // If we're inside of a wall, stop
        if (map[i][j])
        {
            break;
        }

        // There's 4 possible cell sides we could exit out of,
        // and we have to determine if we're going left or right
        // We can compute which direction the ray is facing,
        // and how close the edge is.

        float dst_x = 100000;
        if (dx > 0)
        {
            dst_x = (int)x + 1 - x;
        }
        else if (dx < 0)
        {
            // Negative fractional part
            dst_x = (int)x - x;
        }

        float lx = dst_x / dx;
        assert(lx >= 0);

        float dst_y = 100000;
        if (dy > 0)
        {
            dst_y = (int)y + 1 - y;
        }
        else if (dy < 0)
        {
            // Negative fractional part
            dst_y = (int)y - y;
        }

        float ly = dst_y / dy;
        assert(ly >= 0);

        // Pick the closest intersection
        // Add a small constant to make sure we're not stuck in the edge
        float l = MIN(lx, ly) + 0.0001f;
        //assert(l > 0);

        // Update the current ray start position
        ray_dist = ray_dist + l;
        x = cam_x + (ray_dist * dx);
        y = cam_y + (ray_dist * dy);
    }

    return ray_dist;
}

// Paint a column of the screen
void paint_column(int col_idx, float dx, float dy, float frame_dst, float ray_dst)
{
    if (ray_dst == 0)
    {
        return;
    }

    // Compute the intersection coordinates
    float x = pos_x + dx * ray_dst;
    float y = pos_y + dy * ray_dst;

    int i = (int)x;
    int j = (int)y;

    u32 wall_color;
    if (x < 0 || y < 0)
    {
        wall_color = COLOR_GREY;
    }
    else if (i >= MAP_HEIGHT || j >= MAP_WIDTH)
    {
        wall_color = COLOR_GREY;
    }
    else
    {
        if (fabsf(x - (int)x) < fabsf(y - (int)y))
            wall_color = rgb32(255, 0, 0);
        else
            wall_color = rgb32(180, 0, 0);
    }

    float eye_z = 0.6f;
    float wall_min_z = 0.0f;
    float wall_max_z = 1.0f;

    // Compute the frame size at the ray distance
    float half_w = (ray_dst / frame_dst) * tanf(DEG2RAD(FOV_X / 2));
    float half_h = half_w * FRAME_HEIGHT / FRAME_WIDTH;
    float max_frame_z = eye_z + half_h;
    float min_frame_z = eye_z - half_h;

    // Compute the wall top coordinate relative to the top of the frame
    float wall_top = CLAMP((max_frame_z - wall_max_z) / (2 * half_h), 0.0f, 1.0f);
    float wall_bot = CLAMP((max_frame_z - wall_min_z) / (2 * half_h), 0.0f, 1.0f);

    int wall_min_y = (int)(wall_top * (FRAME_HEIGHT - 1));
    int wall_max_y = (int)(wall_bot * (FRAME_HEIGHT - 1));

    //printf("wall_top=%f, wall_bot=%f\n", wall_top, wall_bot);

    // Paint the ceiling
    for (int y = 0; y < wall_min_y; ++y)
    {
        frame_buffer[y][col_idx] = rgb32(50, 50, 50);
    }

    // Paint the wall strip
    for (int y = wall_min_y; y < wall_max_y; ++y)
    {
        frame_buffer[y][col_idx] = wall_color;
    }

    // Paint the floor
    for (int y = wall_max_y; y < FRAME_HEIGHT; ++y)
    {
        frame_buffer[y][col_idx] = rgb32(100, 100, 100);
    }
}

void anim_callback()
{
    u64 frame_start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / 4);

    // Compute the left vector (parallel with the image plane)
    float left_x = -dir_y;
    float left_y = dir_x;

    // Half of the image frame width
    // With the image plane being one unit away
    float half_w = tanf(DEG2RAD(FOV_X / 2));

    // Leftmost and rightmost coordinates on the image frame
    float x0 = pos_x + dir_x + (half_w * left_x);
    float y0 = pos_y + dir_y + (half_w * left_y);
    float x1 = pos_x + dir_x - (half_w * left_x);
    float y1 = pos_y + dir_y - (half_w * left_y);

    printf("x=%f, y=%f, dx=%f, dy=%f\n", pos_x, pos_y, dir_x, dir_y);

    /*
    for (int i = 0; i < MAP_HEIGHT; ++i)
    {
        for (int j = 0; j < MAP_WIDTH; ++j)
        {
            if (map[i][j] == 0)
                continue;

            fill_rect(
                (u32*)frame_buffer,
                FRAME_WIDTH,
                FRAME_HEIGHT,
                i * 400 / MAP_WIDTH,
                j * 400 / MAP_HEIGHT,
                400 / MAP_WIDTH,
                400 / MAP_HEIGHT,
                COLOR_RED
            );
        }
    }

    // Draw the player position and direction
    draw_world_line(
        400,
        400,
        pos_x - 0.1f,
        pos_y,
        pos_x + 0.1f,
        pos_y,
        COLOR_GREEN,
    );
    draw_world_line(
        400,
        400,
        pos_x,
        pos_y - 0.1f,
        pos_x,
        pos_y + 0.1f,
        COLOR_GREEN,
    );
    draw_world_line(
        400,
        400,
        pos_x,
        pos_y,
        pos_x + 0.4f * dir_x,
        pos_y + 0.4f * dir_y,
        COLOR_YELLOW,
    );
    draw_world_line(
        400,
        400,
        pos_x,
        pos_y,
        pos_x + 0.4f * left_x,
        pos_y + 0.4f * left_y,
        COLOR_BLUE,
    );

    // Draw image plane
    draw_world_line(
        400,
        400,
        x0,
        y0,
        x1,
        y1,
        COLOR_ORANGE,
    );
    */

    // For each column of the frame
    for (int i = 0; i < FRAME_WIDTH; ++i)
    {
        // Starting position of the ray
        float rx = x0 + (x1 - x0) * i / FRAME_WIDTH;
        float ry = y0 + (y1 - y0) * i / FRAME_WIDTH;

        // Ray direction
        float dx = rx - pos_x;
        float dy = ry - pos_y;
        float l = sqrtf(dx*dx + dy*dy);
        dx = dx / l;
        dy = dy / l;

        float ray_dst = cast_ray(pos_x, pos_y, dx, dy);
        paint_column(i, dx, dy, l, ray_dst);

        /*
        draw_world_line(
            400,
            400,
            pos_x,
            pos_y,
            pos_x + ray_dst * dx,
            pos_y + ray_dst * dy,
            COLOR_PURPLE,
        );
        */
    }

    u64 frame_end_time = time_current_ms();
    printf("render time %d ms\n", frame_end_time - frame_start_time);

    window_draw_frame(0, frame_buffer);

    // Schedule a fixed rate update for the next frame (30fps)
    fixed_rate_update(frame_start_time, 1000 / 30, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    if (keycode == KEY_ESCAPE)
    {
        exit(0);
    }

    else if (keycode == KEY_LEFT)
    {
        dir = dir + DEG2RAD(20);
        if (dir >= 2 * M_PI_F)
            dir = dir - 2 * M_PI_F;

        dir_x = cosf(dir);
        dir_y = sinf(dir);
    }

    else if (keycode == KEY_RIGHT)
    {
        dir = dir - DEG2RAD(20);
        if (dir < 0)
            dir = dir + 2 * M_PI_F;

        dir_x = cosf(dir);
        dir_y = sinf(dir);
    }

    else if (keycode == KEY_UP)
    {
        pos_x = pos_x + 0.2f * dir_x;
        pos_y = pos_y + 0.2f * dir_y;
    }

    else if (keycode == KEY_DOWN)
    {
        pos_x = pos_x - 0.2f * dir_x;
        pos_y = pos_y - 0.2f * dir_y;
    }
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Ray-Casting Example", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
