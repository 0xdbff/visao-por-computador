#include "vc.h"
IVC *vc_image_new(int width, int height, int channels, int levels) {
    IVC *image = (IVC *)malloc(sizeof(IVC));
    if (image == NULL) return NULL;
    if ((levels <= 0) || (levels > 255)) return NULL;

    image->width = width;
    image->height = height;
    image->channels = channels;
    image->levels = levels;
    image->bytesperline = image->width * image->channels;
    image->data = (unsigned char *)malloc(image->width * image->height *
                                          image->channels * sizeof(char));

    if (image->data == NULL) {
        vc_image_free(image);
        return NULL;
    }

    return image;
}

void vc_image_free(IVC *image) {
    if (image) free(image);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char *netpbm_get_token(FILE *file, char *tok, int len) {
    char *t;
    int c;

    loop {
        while (isspace(c = getc(file)))
            ;
        if (c != '#') break;
        do c = getc(file);
        while ((c != '\n') && (c != EOF));
        if (c == EOF) break;
    }

    t = tok;

    if (c != EOF) {
        do {
            *t++ = c;
            c = getc(file);
        } while ((!isspace(c)) && (c != '#') && (c != EOF) &&
                 (t - tok < len - 1));

        if (c == '#') ungetc(c, file);
    }

    *t = 0;

    return tok;
}

long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit,
                              int width, int height) {
    int x, y;
    int countbits;
    long int pos, counttotalbytes;
    unsigned char *p = databit;

    *p = 0;
    countbits = 1;
    counttotalbytes = 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = width * y + x;

            if (countbits <= 8) {
                // Numa imagem PBM:
                // 1 = Preto
                // 0 = Branco
                //*p |= (datauchar[pos] != 0) << (8 - countbits);

                // Na nossa imagem:
                // 1 = Branco
                // 0 = Preto
                *p |= (datauchar[pos] == 0) << (8 - countbits);

                countbits++;
            }
            if ((countbits > 8) || (x == width - 1)) {
                p++;
                *p = 0;
                countbits = 1;
                counttotalbytes++;
            }
        }
    }

    return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar,
                          int width, int height) {
    int x, y;
    int countbits;
    long int pos;
    unsigned char *p = databit;

    countbits = 1;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pos = width * y + x;

            if (countbits <= 8) {
                // Numa imagem PBM:
                // 1 = Preto
                // 0 = Branco
                // datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

                // Na nossa imagem:
                // 1 = Branco
                // 0 = Preto
                datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

                countbits++;
            }
            if ((countbits > 8) || (x == width - 1)) {
                p++;
                countbits = 1;
            }
        }
    }
}

IVC *vc_read_image(char *filename) {
    FILE *file = NULL;
    IVC *image = NULL;
    unsigned char *tmp;
    char tok[20];
    long int size, sizeofbinarydata;
    int width, height, channels;
    int levels = 255;
    int v;

    // Abre o ficheiro
    if ((file = fopen(filename, "rb")) != NULL) {
        // Efectua a leitura do header
        netpbm_get_token(file, tok, sizeof(tok));

        if (strcmp(tok, "P4") == 0) {
            channels = 1;
            levels = 1;
        }  // Se PBM (Binary [0,1])
        else if (strcmp(tok, "P5") == 0)
            channels = 1;  // Se PGM (Gray [0,MAX(level,255)])
        else if (strcmp(tok, "P6") == 0)
            channels = 3;  // Se PPM (RGB [0,MAX(level,255)])
        else {
            printf(
                "ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or "
                "PPM "
                "file.\n\tBad magic number!\n");

            fclose(file);
            return NULL;
        }

        if (levels == 1)  // PBM
        {
            if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d",
                       &width) != 1 ||
                sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d",
                       &height) != 1) {
                printf(
                    "ERROR -> vc_read_image():\n\tFile is not a valid PBM "
                    "file.\n\tBad size!\n");

                fclose(file);
                return NULL;
            }

            // Aloca memória para imagem
            image = vc_image_new(width, height, channels, levels);
            if (image == NULL) return NULL;

            sizeofbinarydata =
                (image->width / 8 + ((image->width % 8) ? 1 : 0)) *
                image->height;
            tmp = (unsigned char *)malloc(sizeofbinarydata);
            if (tmp == NULL) return 0;

            printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels,
                   image->width, image->height, levels);

            if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata,
                           file)) != sizeofbinarydata) {
                printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");

                vc_image_free(image);
                fclose(file);
                free(tmp);
                return NULL;
            }

            bit_to_unsigned_char(tmp, image->data, image->width, image->height);

            free(tmp);
        } else  // PGM ou PPM
        {
            if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d",
                       &width) != 1 ||
                sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d",
                       &height) != 1 ||
                sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d",
                       &levels) != 1 ||
                levels <= 0 || levels > 255) {
                printf(
                    "ERROR -> vc_read_image():\n\tFile is not a valid PGM or "
                    "PPM "
                    "file.\n\tBad size!\n");

                fclose(file);
                return NULL;
            }

            // Aloca memória para imagem
            image = vc_image_new(width, height, channels, levels);
            if (image == NULL) return NULL;

            printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels,
                   image->width, image->height, levels);

            size = image->width * image->height * image->channels;

            if ((v = fread(image->data, sizeof(unsigned char), size, file)) !=
                size) {
                printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");

                vc_image_free(image);
                fclose(file);
                return NULL;
            }
        }

        fclose(file);
    } else {
        printf("ERROR -> vc_read_image():\n\tFile not found.\n");
    }

    return image;
}

int vc_write_image(char *filename, IVC *image) {
    FILE *file = NULL;
    unsigned char *tmp;
    long int totalbytes, sizeofbinarydata;

    if (image == NULL) return 0;

    if ((file = fopen(filename, "wb")) != NULL) {
        if (image->levels == 1) {
            sizeofbinarydata =
                (image->width / 8 + ((image->width % 8) ? 1 : 0)) *
                    image->height +
                1;
            tmp = (unsigned char *)malloc(sizeofbinarydata);
            if (tmp == NULL) return 0;

            fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

            totalbytes = unsigned_char_to_bit(image->data, tmp, image->width,
                                              image->height);
            printf("Total = %ld\n", totalbytes);
            if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) !=
                totalbytes) {
#ifdef VC_DEBUG
                fprintf(stderr,
                        "ERROR -> vc_read_image():\n\tError writing PBM, PGM "
                        "or PPM file.\n");
#endif

                fclose(file);
                free(tmp);
                return 0;
            }

            free(tmp);
        } else {
            fprintf(file, "%s %d %d 255\n",
                    (image->channels == 1) ? "P5" : "P6", image->width,
                    image->height);

            if (fwrite(image->data, image->bytesperline, image->height, file) !=
                image->height) {
#ifdef VC_DEBUG
                fprintf(stderr,
                        "ERROR -> vc_read_image():\n\tError writing PBM, PGM "
                        "or PPM file.\n");
#endif

                fclose(file);
                return 0;
            }
        }

        fclose(file);

        return 1;
    }

    return 0;
}

void a() {
    IVC *image = vc_image_new(25, 25, 1, 255);

    for (size_t x = 0; x < image->width; x++) {
        for (size_t y = 0; y < image->height; y++) {
            int pos = y * image->bytesperline + x * image->channels;
            image->data[pos] = 0;
        }
    }
    vc_write_image("imagem2.pbm", image);

    vc_image_free(image);
}

// void b()

// {
//   IVC *image = vc_image_new(256, 256, 1, 255);
//
//   for (size_t i = 0; i < (256 * 256); i++) {
//     image->data[i] = i / 256;
//   }
//
//   vc_write_image("imagem.pbm", image);
//
//   vc_image_free(image);
// }
//
// void c() {
//   IVC *image = vc_image_new(256, 256, 1, 255);
//
//   for (size_t x = 0; x < image->width; x++) {
//     for (size_t y = 0; y < image->height; y++) {
//       int pos = y * image->bytesperline + x * image->channels;
//       image->data[pos] = (x + y) / 2;
//     }
//   }
//   vc_write_image("imagem3.pbm", image);
//
//   vc_image_free(image);
// }
//
// IVC *create_rgb() {
//   IVC *image = vc_image_new(256, 256, 3, 255);
//
//   for (size_t x = 0; x < image->width; x++) {
//     for (size_t y = 0; y < image->height; y++) {
//       int pos = y * image->bytesperline + x * image->channels;
//       image->data[pos] = 0xff;
//       image->data[pos + 1] = 0x00;
//       image->data[pos + 2] = 0x88;
//     }
//   }
//
//   return image;
// }

/// Dois apontadores
// IVC *create_grey_from_rgb(IVC *original) {
//   IVC *image =
//       vc_image_new(original->width, original->height, 1, original->levels);
//
//   if (image == NULL)
//     return NULL;
//
//   for (size_t x = 0; x < original->width; x++) {
//     for (size_t y = 0; y < original->height; y++) {
//       int pos_original = y * original->bytesperline + x * original->channels;
//       int pos_new = y * image->bytesperline + x * image->channels;
//
//       image->data[pos_new] =
//           (uint8_t)(((float)original->data[pos_original]) * 0.299 +
//                     ((float)original->data[pos_original + 1]) * 0.587 +
//                     ((float)original->data[pos_original + 2]) * 0.114);
//     }
//   }
//
//   return image;
// }

int main() {
    a();
    //
    // IVC *image = vc_read_image("");
    //
    // IVC *image2 = vc_image_new(image->width, image->height, 1,
    // image->levels);
    //
    // vc_write_image("imagem-rgb.pbm", image);
    //
    // IVC *grey = create_grey_from_rgb(image);
    //
    // vc_write_image("imagem-gray.pbm", grey);
    // return 0;
}
