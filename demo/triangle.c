#include <demo/triangle.h>
#include <math.h>

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/xenio/tty.h>
#include <xencore/timer/sleep.h>
#include <xencore/xenmem/bitmap.h>

struct DemoTriangleState demo_triangle_init(void)
{
    // Enable double-buffering
    if (fb_get_size() > 0) {
        bool fail = false;
        void *fb_buffer = alloc_page();
        if (fb_buffer == NULL) fail = true;
        
        size_t fb_pages = fb_get_size() / PAGE_SIZE_2MB;
        for (size_t i = 0; i < fb_pages; ++i) {
            void *page = alloc_page();
            if (page == NULL) {
                fail = true;
                break;
            }
        }

        if (!fail) {
            fb_init_buffer(fb_buffer);
        } else {
            tty_printf("[Kernel] Failed to allocate memory for double-buffering.\n");
        }
    }

    // Create state
    struct DemoTriangleState state = {
        0.0f,
        fb_color_rgb(0.1f, 0.1f, 0.1f),
        fb_color_rgb(0.1f, 0.7f, 0.2f),
        {  0.0f,  0.5f },
        { -0.5f, -0.5f },
        {  0.5f, -0.5f }
    };

    return state;
}

void demo_triangle_tick(struct DemoTriangleState *state)
{
    fb_clear(state->clear_color);

    // Rotate vertices
    const float angle = state->angle;
    matrix2x2 rotation_matrix = {
        .m = {
            {cos(angle), -sin(angle)},
            {sin(angle),  cos(angle)}
        }
    };

    vector2 rotated_v1 = lib_matrix2x2_mul_vector2(rotation_matrix, state->v1);
    vector2 rotated_v2 = lib_matrix2x2_mul_vector2(rotation_matrix, state->v2);
    vector2 rotated_v3 = lib_matrix2x2_mul_vector2(rotation_matrix, state->v3);

    // Correct aspect ratio
    const float width = fb_get_width();
    const float height = fb_get_height();
    const float aspect = (float)width / (float)height;
    rotated_v1.x /= aspect;
    rotated_v2.x /= aspect;
    rotated_v3.x /= aspect;

    rotated_v1.x = (rotated_v1.x + 1.0f) * (width / 2.0f);
    rotated_v1.y = (rotated_v1.y + 1.0f) * (height / 2.0f);
    rotated_v2.x = (rotated_v2.x + 1.0f) * (width / 2.0f);
    rotated_v2.y = (rotated_v2.y + 1.0f) * (height / 2.0f);
    rotated_v3.x = (rotated_v3.x + 1.0f) * (width / 2.0f);
    rotated_v3.y = (rotated_v3.y + 1.0f) * (height / 2.0f);

    // Render
    fb_triangle_fill(
        state->triangle_color,
        rotated_v1.x, rotated_v1.y,
        rotated_v2.x, rotated_v2.y,
        rotated_v3.x, rotated_v3.y
    );

    fb_present();
    ksleep(1);

    // Update state
    float new_angle = angle + 0.01f;
    if (new_angle > 2.0f * M_PI) {
        new_angle -= 2.0f * M_PI;
    }
    state->angle = new_angle;
}
