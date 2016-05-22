#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AlexLoi.Dev.JPEG.h"

int main(int argc, char* argv[]) {
    size_t size;
    unsigned char *buf;
    FILE *f;

    if (argc < 1) {
        printf("Usage: %s <input.jpg>\n", argv[0]);
        return 2;
    }
    int i = 1;

    do {
        f = fopen(argv[i], "rb");
        if (!f) {
            printf("Error opening the input file.\n");
            return 1;
        }

        fseek(f, 0, SEEK_END);
        size = ftell(f);
        buf = (unsigned char*)malloc(size);
        fseek(f, 0, SEEK_SET);
        size_t read = fread(buf, 1, size, f);
        fclose(f);

        Decoder dec(buf, size);

        if (dec.ErrorDecode != 1) {
            printf("Error decoding image. \n");
            return -1;
        }
        
        f = fopen("AlexLoi.Dev.JPEG.ppm", "wb");
        if (!f) {
            printf("Error opening the output file.\n");
            return 1;
        }
        fprintf(f, "P6\n%d %d\n255\n", dec.Width(), dec.Height());
        fwrite(dec.Image(), 1, dec.ImageSize(), f);
        fclose(f);

        system("./AlexView");
        system("rm AlexLoi.Dev.JPEG.ppm");          
        i++;
    } while (i < argc);

    return 0;
}
