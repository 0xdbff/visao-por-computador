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

    HSV hsv;

    if (max == min) {
        hsv.h = 0;
    } else if (max == r) {
        hsv.h = 60 * ((g - b) / diff);
    } else if (max == g) {
        hsv.h = (float)(60.0 * (2.0 + (b - r) / diff));
    } else {
        hsv.h = (float)(60.0 * (4.0 + (r - g) / diff));
    }

    if (hsv.h < 0.0f) {
        hsv.h += 360.0f;
    }

    hsv.v = max;

    if (max == 0.0f) {
        hsv.s = 0;
    } else {
        hsv.s = diff / max;
    }

    return hsv;
}

int main() {
    RGB rgb = {.r = 127, .g = 200, .b = 100};
    HSV hsv = RGBtoHSV(rgb);
    printf("HSV Values: \nHue: %.2f\nSaturation: %.2f\nValue: %.2f\n", hsv.h,
           hsv.s, hsv.v);
    return 0;
}
