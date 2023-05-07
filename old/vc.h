#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VC_H
#define VC_H

#define VC_DEBUG

#define _CRT_SECURE_NO_WARNINGS

#define loop for (;;)

typedef struct ivc {
  unsigned char *data;
  int width, height;
  int channels;     // Binário/Cinzentos=1; RGB=3
  int levels;       // Binário=1; Cinzentos [1,255]; RGB [1,255]
  int bytesperline; // width * channels
} IVC;

IVC *vc_image_new(int width, int height, int channels, int levels);
void vc_image_free(IVC *image);

IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);

#endif
