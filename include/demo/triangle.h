#ifndef _DEMO_TRIANGLE_H
#define _DEMO_TRIANGLE_H

#include <xencore/xenlib/math.h>
#include <xencore/graphics/framebuffer.h>

struct DemoTriangleState {
    float angle;
    fb_color_t clear_color;
    fb_color_t triangle_color;
    vector2 v1;
    vector2 v2;
    vector2 v3;
};

struct DemoTriangleState demo_triangle_init(void);
void demo_triangle_tick(struct DemoTriangleState *state);

#endif
