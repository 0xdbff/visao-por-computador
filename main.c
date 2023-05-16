#include "image.h"

int main() {
    uint32_t width = 25;
    uint32_t height = 25;

    ImageNpbm* white_image = create_white_image(width, height, PBM);
    if (!white_image) {
        LOG("Failed to create white PBM image\n");
        return 1;
    }

    if (!write_pbm_image("white_image.pbm", white_image)) {
        LOG("Failed to write white PBM image to file\n");
        return 1;
    }

    free_image(white_image);
    return 0;
}
