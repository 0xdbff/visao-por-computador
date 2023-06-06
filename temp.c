
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB;

typedef struct {
    float h;
    float s;
    float v;
} HSV;

HSV RGBtoHSV(RGB rgb) {
    float r = (float)rgb.r / 255.0f;
    float g = (float)rgb.g / 255.0f;
    float b = (float)rgb.b / 255.0f;

    float max = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    float min = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    float diff = max - min;

    HSV h;

    if (max == min) {
        h.h = 0;
    } else if (max == r) {
        h.h = (float)(60.0 * ((g - b) / diff));  // 0
    } else if (max == g) {
        h.h = (float)(60.0 * (2.0 + (b - r) / diff));  // 120
    } else {
        h.h = (float)(60.0 * (4.0 + (r - g) / diff));  // 240
    }

    if (h.h < 0.0) {
        h.h += 360.f;
    }

    h.v = max;

    if (max == 0.0f) {
        h.s = 0;
    } else {
        h.s = diff / max;
    }

    return h;
}
