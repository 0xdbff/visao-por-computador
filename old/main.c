#include "vc.h"

void a() {
  IVC *image = vc_image_new(256, 256, 1, 255);

  for (size_t x = 0; x < image->width; x++) {
    for (size_t y = 0; y < image->height; y++) {
      int pos = y * image->bytesperline + x * image->channels;
      image->data[pos] = x;
    }
  }
  vc_write_image("imagem2.pbm", image);

  vc_image_free(image);
}

void b()

{
  IVC *image = vc_image_new(256, 256, 1, 255);

  for (size_t i = 0; i < (256 * 256); i++) {
    image->data[i] = i / 256;
  }

  vc_write_image("imagem.pbm", image);

  vc_image_free(image);
}

void c() {
  IVC *image = vc_image_new(256, 256, 1, 255);

  for (size_t x = 0; x < image->width; x++) {
    for (size_t y = 0; y < image->height; y++) {
      int pos = y * image->bytesperline + x * image->channels;
      image->data[pos] = (x + y) / 2;
    }
  }
  vc_write_image("imagem3.pbm", image);

  vc_image_free(image);
}

IVC *create_rgb() {
  IVC *image = vc_image_new(256, 256, 3, 255);

  for (size_t x = 0; x < image->width; x++) {
    for (size_t y = 0; y < image->height; y++) {
      int pos = y * image->bytesperline + x * image->channels;
      image->data[pos] = 0xff;
      image->data[pos + 1] = 0x00;
      image->data[pos + 2] = 0x88;
    }
  }

  return image;
}

/// Dois apontadores
IVC *create_grey_from_rgb(IVC *original) {
  IVC *image =
      vc_image_new(original->width, original->height, 1, original->levels);

  if (image == NULL)
    return NULL;

  for (size_t x = 0; x < original->width; x++) {
    for (size_t y = 0; y < original->height; y++) {
      int pos_original = y * original->bytesperline + x * original->channels;
      int pos_new = y * image->bytesperline + x * image->channels;

      image->data[pos_new] =
          (uint8_t)(((float)original->data[pos_original]) * 0.299 +
                    ((float)original->data[pos_original + 1]) * 0.587 +
                    ((float)original->data[pos_original + 2]) * 0.114);
    }
  }

  return image;
}

int main() {

  IVC *image = vc_read_image("");

  IVC *image2 = vc_image_new(image->width, image->height, 1, image->levels);

  vc_write_image("imagem-rgb.pbm", image);

  IVC *grey = create_grey_from_rgb(image);

  vc_write_image("imagem-gray.pbm", grey);
  return 0;
}
