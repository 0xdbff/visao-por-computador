#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef loop
#define loop for (;;)
#endif

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#define RETURN_NULL_ON_ERROR(msg) \
    do {                          \
        LOG(msg);                 \
        if (file) fclose(file);   \
        return NULL;              \
    } while (0)
#else
#define RETURN_NULL_ON_ERROR(msg) \
    do {                          \
        if (file) fclose(file);   \
        return NULL;              \
    } while (0)
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

/// Image definition structure
typedef struct image {
    /// Image ptr to first section of allocated data (byte)
    uint8_t* data;
    /// Image dimensions
    uint32_t width, height;
    /// biggest possible value
    uint32_t max_value;
    /// Number of image channels
    uint8_t channels;
    /// Bytes per pixel
    uint8_t Bpp;
    /// Image type
    Netpbm type;
} Image;  // 34 bytes struct

/// Return the number of bytes needed to store a channel
static inline uint8_t bytes_per_channel(int precision) {
    return (uint8_t)ceil(log2(precision) / 8.0);
}

/// Free an image allocation.
static inline Image* free_image(Image* img) {
    if (img) free(img);
    return NULL;
}

/// Allocate a new image.
static inline Image* new_image(size_t width, size_t height, uint8_t channels,
                               size_t precision, Netpbm type) {
    Image* img = (Image*)malloc(sizeof(Image));
    if (img == NULL) return NULL;

    img->width = width;
    img->height = height;
    img->channels = channels;
    img->max_value = precision;
    img->Bpp = bytes_per_channel(precision) * channels;
    img->data = (uint8_t*)malloc(img->width * img->height * img->Bpp);
    img->type = type;

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
#ifdef DEBUG
        LOG("Not a valid Netpbm file!");
#endif
    }
    return type;
}

static Image* pbm_img_from_fp(FILE* file) {
    uint32_t width, height;

    skip_whitespace_and_comments(file);
    if (fscanf(file, "%u", &width) != 1) {
        RETURN_NULL_ON_ERROR("Not a valid pbm file!");
    }
    skip_whitespace_and_comments(file);
    if (fscanf(file, "%u", &height) != 1) {
        RETURN_NULL_ON_ERROR("Not a valid pbm file!");
    }
    fgetc(file);

    Image* img = new_image(width, height, 1, 1, PBM);
    if (img == NULL) {
        RETURN_NULL_ON_ERROR("Cannot allocate memory!");
    }

    size_t data_size = (img->width * img->height + 7) / 8;
    if (fread(img->data, 1, data_size, file) != data_size) {
        RETURN_NULL_ON_ERROR("Not a valid pbm file!");
    }

    fclose(file);
    return img;
}

/// Read a image PBM, PGM, PPM, PAM
static Image* read_image(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) return NULL;

    // Read the header
    char signature[3] = {0};
    uint32_t width, height;

    // Read signature
    if (fscanf(file, "%2s", signature) != 1) {
        RETURN_NULL_ON_ERROR("Not a valid pbm file!\nAborting!");
    }

    Netpbm type = eval_type(signature);
    if (type == INVALID_TYPE) {
        RETURN_NULL_ON_ERROR("Aborting!");
    }

    if (type == PBM) {
        Image* img = pbm_img_from_fp(file);
        if (!img) {
            return NULL;
        }
        return img;
    }

    fclose(file);
    return NULL;
}

#ifdef __cplusplus
}
#endif
