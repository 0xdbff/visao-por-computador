#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

/// 8 bit RGB struct
typedef struct rgb_8 {
    uint8_t r;  // red
    uint8_t g;  // green
    uint8_t b;  // blue
}
#ifdef ALIGN_MEM
__attribute__((aligned(4)))
#endif
RGB_8bit;  // 3-4 bytes (define ALIGN_MEM for better CPU performance, with
           // increased memory usage)

/// 16 bit RGB struct
typedef struct rgb_16 {
    uint16_t r;  // red
    uint16_t g;  // green
    uint16_t b;  // blue
}
#ifdef ALIGN_MEM
__attribute__((aligned(8)))
#endif
RGB_16bit;  // 6-8 bytes (define ALIGN_MEM for better CPU performance, with
            // increased memory usage)

/// Image types
typedef enum netpbm {
    /// Binary    P4, 1 bit
    PBM,
    /// Grayscale P5, 8 or 16 bit
    PGM,
    /// RGB       P6, 8 or 16 bit
    PPM,
    /// PAM       P7, dymamic (multiples of uint8_t)
    PAM,
    /// Invalid type
    INVALID_TYPE
} Netpbm;

typedef enum ImageTp { BLACK, WHITE, GRAYSCALE, RGB, RGB16, HSV } ImageTp;

/// Image definition structure
typedef struct imageNpbm {
    /// Image ptr to first section of allocated data (byte)
    uint8_t* data;
    /// Image dimensions
    uint32_t width, height;
    /// biggest possible value
    uint32_t max_value;
    /// Number of image channels
    uint8_t channels;
    /// Size of the image data
    size_t data_size;
    /// Bits per pixel
    uint8_t bpp;
    /// Image type
    Netpbm type;
} ImageNpbm;  // 32 bytes struct

/// Return the number of bytes needed to store a channel
static inline uint8_t bytes_per_channel(uint32_t precision) {
    return (uint8_t)ceil(log2(precision) / 8.0);
}

/// Free an image allocation.
static inline ImageNpbm* free_image(ImageNpbm* img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
    return NULL;
}

/// Allocate a new image.
static inline ImageNpbm* alloc_image(size_t width, size_t height,
                                     uint8_t channels, uint32_t precision,
                                     Netpbm type) {
    ImageNpbm* img = (ImageNpbm*)malloc(sizeof(ImageNpbm));
    img->data = NULL;
    // size_t data_size;
    if (img == NULL) return NULL;

    img->width = width;
    img->height = height;
    img->channels = channels;
    img->max_value = precision;
    img->type = type;

    switch (type) {
        case PBM:
            img->data_size = (width * height + 7) / 8;
            break;
        case PGM:
        case PPM:
            img->data_size =
                width * height * channels * bytes_per_channel(precision);
            break;
        case PAM:
            img->data_size =
                width * height * (channels + 1) * bytes_per_channel(precision);
            break;
        default:
            free_image(img);
            return NULL;
    }
    img->data = (uint8_t*)calloc(img->data_size, sizeof(uint8_t));
    return img->data == NULL ? free_image(img) : img;
}

static inline void skip_whitespace_and_comments(FILE* file) {
    int c;
    do {
        while (isspace(c = getc(file)))
            ;

        if (c == '#') {
            while ((c = getc(file)) != '\n' && c != EOF)
                ;
        }
    } while (c == '#');
    ungetc(c, file);
}

/// Evaluate the type of the image (Netpbm) with a signature.
static inline Netpbm eval_type(const char* signature) {
    Netpbm type = INVALID_TYPE;
    if (strcmp(signature, "P4") == 0)
        type = PBM;
    else if (strcmp(signature, "P5") == 0)
        type = PGM;
    else if (strcmp(signature, "P6") == 0)
        type = PPM;
    else if (strcmp(signature, "P7") == 0)
        type = PAM;
    else {
        LOG("Not a valid Netpbm file!");
    }
    return type;
}

static ImageNpbm* pbm_img_from_fp(FILE* file) {
    ImageNpbm* img = NULL;
    uint32_t width, height;

    skip_whitespace_and_comments(file);
    if (fscanf(file, "%u", &width) != 1) {
        LOG("Not a valid pbm file!");
        return img;
    }
    skip_whitespace_and_comments(file);
    if (fscanf(file, "%u", &height) != 1) {
        LOG("Not a valid pbm file!");
        return img;
    }
    fgetc(file);

    img = alloc_image(width, height, 1, 1, PBM);
    if (!img) {
        LOG("Cannot allocate memory!");
        return img;
    }

    size_t data_size = (img->width * img->height + 7) / 8;
    if (fread(img->data, 1, data_size, file) != data_size) {
        LOG("Not a valid pbm file!");
        free_image(img);
        return NULL;
    }

    return img;
}

/// Read a image PBM, PGM, PPM
static ImageNpbm* read_image(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) return NULL;

    ImageNpbm* img = NULL;

    // Read the header
    char signature[3] = {0};
    uint32_t width, height;

    // Read signature
    if (fscanf(file, "%2s", signature) != 1) {
        if (file) fclose(file);
        LOG("Not a valid pbm file!\nAborting!");
        return img;
    }

    Netpbm type = eval_type(signature);
    if (type == INVALID_TYPE) {
        if (file) fclose(file);
        LOG("Aborting!");
        return img;
    }

    if (type == PBM) {
        img = pbm_img_from_fp(file);
        if (!img) {
            if (file) fclose(file);
            LOG("Aborting!");
            return img;
        }
        return img;
    }

    if (file) fclose(file);
    return NULL;
}

static uint8_t write_pbm_image(const char* filepath, const ImageNpbm* img) {
    assert(img->type == PBM);

    FILE* file = fopen(filepath, "wb");
    if (!file) return false;

    fprintf(file, "P4\n%u %u\n", img->width, img->height);

    size_t row_bytes = (img->width + 7) / 8;
    uint8_t padding_bits = 8 - ((img->width % 8) ? (img->width % 8) : 8);

    for (uint32_t i = 0; i < img->height; i++) {
        for (uint32_t j = 0; j < row_bytes; j++) {
            uint8_t byte = img->data[i * row_bytes + j];

            // If this is the last byte in the row and there are padding bits,
            // pad with 0s
            if (j == row_bytes - 1 && padding_bits != 0) {
                byte &= (0xFF << padding_bits);
            }

            if (fwrite(&byte, 1, 1, file) != 1) {
                fclose(file);
                return false;
            }
        }
    }

    fclose(file);
    return true;
}

static ImageNpbm* new_image(uint32_t width, uint32_t height, Netpbm type,
                            ImageTp c_type) {
    uint8_t channels;
    int precision;
    size_t data_size;

    switch (type) {
        case PBM:
            channels = 1;
            precision = 1;
            data_size = (width * height + 7) / 8;

            if (c_type != (BLACK | WHITE)) return NULL;
            break;
        case PGM:
            channels = 1;
            precision = 255;
            data_size =
                width * height * channels * bytes_per_channel(precision);
            if (c_type != (BLACK | WHITE | GRAYSCALE)) return NULL;
            break;
        case PPM:
            channels = 3;
            precision = 255;
            data_size =
                width * height * channels * bytes_per_channel(precision);
            break;
        default:
            return NULL;
    }

    ImageNpbm* img = alloc_image(width, height, channels, precision, type);
    if (!img) return NULL;

    // For PBM, set all bits to 0 for white pixels.
    memset(img->data, (type == PBM) ? 0x00 : precision, data_size);

    // For PBM images with width not a multiple of 8, set padding bits to 0.
    if (type == PBM && width % 8 != 0) {
        uint8_t padding_bits = 8 - (width % 8);
        for (uint32_t i = 0; i < height; i++) {
            uint32_t row_end_byte = ((i + 1) * width - 1) / 8;
            img->data[row_end_byte] &= ~(0xFF >> (8 - padding_bits));
        }
    }

    return img;
}

#ifdef __cplusplus
}
#endif
