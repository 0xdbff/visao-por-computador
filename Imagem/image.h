#ifndef IMAGE_H
#define IMAGE_H

#define DEBUG

#include <stdlib.h>

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math.h"

#define LOOP for (;;)

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

/// 8 bit RGB struct
typedef struct rgb_8 {
  uint8_t r; // red
  uint8_t g; // green
  uint8_t b; // blue
} RGB_8bit;  // 3 bytes

/// 16 bit RGB struct
typedef struct rgb_16 {
  uint16_t r; // red
  uint16_t g; // green
  uint16_t b; // blue
} RGB_16bit;  // 6 bytes

/// Image types
typedef enum netpbm {
  /// Binary    P4, 1 bit
  PBM,
  /// Grayscale P5, 8 or 16 bit
  PGM,
  /// RGB       P6, 8 or 16 bit
  PPM,
  /// PAM       P7, dymamic
  PAM,
  /// Invalid type
  INVALID_TYPE
} Netpbm;

/// Image definition structure
typedef struct image {
  /// Image ptr to first section of allocated data (byte)
  uint8_t *data;
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
} Image; // 34 bytes struct

/// Return the number of bytes needed to store a channel
static uint8_t bytes_per_channel(int precision) {
  return (uint8_t)ceil(log2(precision) / 8.0);
}

/// Free an image allocation.
static inline Image *free_image(Image *img) {
  if (img)
    free(img);
  return NULL;
}

/// Allocate a new image.
static inline Image *new_image(size_t width, size_t height, uint8_t channels,
                               size_t precision) {
  Image *img = (Image *)malloc(sizeof(Image));
  if (img == NULL)
    return NULL;

  img->width = width;
  img->height = height;
  img->channels = channels;
  img->max_value = precision;
  img->Bpp = bytes_per_channel(precision) * channels;
  img->data = (uint8_t *)malloc(img->width * img->height * img->Bpp);

  return img->data == NULL ? free_image(img) : img;
}

static inline void read_pbm() {}

static inline void read_pgm_ppm() {}

/// Read a image PBM, PGM, PPM
static inline Image *read_image(const char *filepath) {
  FILE *file = fopen(filepath, "rb");
  if (!file)
    return NULL;

  // Read the header
  char signature[3] = {0};
  uint32_t width, height, max_val;

  // Read signature
  if (fscanf(file, "%2s ", signature) != 1) {
    fclose(file);
    return NULL;
  }

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
    LOG("NOT A VALID FILETYPE!");
    fclose(file);
    return NULL;
  }

  // Read header
  if (type == PBM) {
    if (fscanf(file, "%u %u\n", &width, &height) != 2) {
      fclose(file);
      return NULL;
    }
  } else if (type == (PBM | PPM)) {
    if (fscanf(file, "%u %u %u\n", &width, &height, &max_val) != 3) {
      fclose(file);
      return NULL;
    }
  } else { // PAM
    // !TODO
  }

  // Allocate the image
  Image *img = new_image(width, height, 1, 1);
  if (!img) {
    fclose(file);
    return NULL;
  }

  // Read the data
  size_t num_bytes = (size_t)ceil(width / 8.0);
  for (int y = 0; y < height; y++) {
    uint8_t *row = img->data + y * img->width;
    for (int x = 0; x < width; x += 8) {
      uint8_t byte;
      fread(&byte, 1, 1, file);
      for (int i = 0; i < 8 && x + i < width; i++) {
        row[x + i] = (byte >> (7 - i)) & 1;
      }
    }
  }

  fclose(file);
  return img;
}

#endif
