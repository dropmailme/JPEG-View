#include <stdlib.h>
#include <string.h>
#include "AlexLoi.Dev.JPEG.STR.h"

#define Sucsess 0
#define Error 1

class Decoder {
    
public:
    Decoder(const unsigned char* data, size_t size, void *(*malFunc)(size_t) = malloc, void (*freeFunc)(void*) = free); //Конструктор декодера и выделения необходимой памяти || Вынесен за пределы класса
    int Width();
    int Height();
    unsigned char* Image();
    size_t ImageSize();
    int ErrorDecode;
    
private:
    Info pix; 
    char ZigZagArr[64];
    void *(*AllocMem)(size_t);
    void (*FreeMem)(void*); 
   
    void Skip(int count) { 
        pix.pos += count;
        pix.size -= count;
        pix.length -= count;
        
        if (pix.size < 0) { 
            pix.error = Error;
        }
    }

    int GetBits(int bits) {
        unsigned char newByte;
        if (!bits) {
          return 0;  
        } 

        while (pix.bufbits < bits) {
            if (pix.size <= 0) {
                pix.buf = (pix.buf << 8) | 0xFF;
                pix.bufbits += 8;
                continue;
            }

            newByte = *pix.pos++;
            pix.size--;
            pix.bufbits += 8;
            pix.buf = (pix.buf << 8) | newByte;

            if (newByte == 0xFF) {
                if (pix.size) {
                    unsigned char marker = *pix.pos++;
                    pix.size--;

                    switch (marker) {
                        case 0:    
                            break;
                        case 0xD9: 
                            pix.size = 0; 
                            break;
                        default:
                            if ((marker & 0xF8) != 0xD0) {
                                pix.error = Error;
                            } else {
                                pix.buf = (pix.buf << 8) | marker;
                                pix.bufbits += 8;
                            }
                    }
                } else {
                    pix.error = Error;
                }
            }
        }
        return (pix.buf >> (pix.bufbits - bits)) & ((1 << bits) - 1);
    }

    void GetLength() {

        if (pix.size < 2){
            pix.error = Error; 
        }
        pix.length = ((pix.pos[0] << 8) | pix.pos[1]);

        if (pix.length > pix.size){
            pix.error = Error; 
        } 

        Skip(2);
    }

    void SOFDec() {
        int i;
        int compXMax = 0; 
        int compYMax = 0;

        ImageComp* c;
        GetLength();

        pix.height = ((pix.pos[1] << 8) | pix.pos[2]);
        pix.width = ((pix.pos[3] << 8) | pix.pos[4]);
        pix.ncomp = pix.pos[5];

        Skip(6);
        
        for (i = 0, c = pix.comp;  i < pix.ncomp;  i++, c++) {
            c->compID = pix.pos[0];

            if (!(c->compX = pix.pos[1] >> 4)) { 
                pix.error = Error; 
            } else if (c->compX & (c->compX - 1)) {
                pix.error = Error; 
            } else if (!(c->compY = pix.pos[1] & 15)) {
                pix.error = Error;
            } else if (c->compY & (c->compY - 1)) { 
                pix.error = Error;
            } else if ((c->qtsel = pix.pos[2]) & 0xFC) { 
                pix.error = Error; 
            }    
            
            Skip(3);
            pix.qtused |= 1 << c->qtsel;
            if (c->compX > compXMax) { 
                compXMax = c->compX;
            } 
            if (c->compY > compYMax) {
                compYMax = c->compY;
            }
        }

        pix.mbsizex = compXMax << 3;
        pix.mbsizey = compYMax << 3;
        pix.mbwidth = (pix.width + pix.mbsizex - 1) / pix.mbsizex;
        pix.mbheight = (pix.height + pix.mbsizey - 1) / pix.mbsizey;
        
        for (i = 0, c = pix.comp;  i < pix.ncomp;  i++, c++) {
            c->width = (pix.width * c->compX + compXMax - 1) / compXMax;
            c->step = (c->width + 7) & 0x7FFFFFF8;
            c->height = (pix.height * c->compY + compYMax - 1) / compYMax;
            c->step = pix.mbwidth * pix.mbsizex * c->compX / compXMax;

            if (!(c->pixels = (unsigned char*)AllocMem(c->step * (pix.mbheight * pix.mbsizey * c->compY / compYMax)))) {
                pix.error = Error;
            } 
        }

        if (pix.ncomp == 3) {
            pix.rgb = (unsigned char*)AllocMem(pix.width * pix.height * pix.ncomp);
            if (!pix.rgb) {
                pix.error = Error;
            }
        }
        Skip(pix.length);
    }

    void DHTDec() {
        int cLen, cr, rem, spr, i, j;
        VLC *vlc;

        unsigned char counts[16];
        GetLength();
        
        while (pix.length >= 17) {
            i = pix.pos[0];
            i = (i | (i >> 3)) & 3; 

            for (cLen = 1;  cLen <= 16;  cLen++) {
                counts[cLen - 1] = pix.pos[cLen];
            }

            Skip(17);
            
            vlc = &pix.vlctab[i][0];
            rem = 65536;
            spr = 65536;
            
            for (cLen = 1;  cLen <= 16;  cLen++) {
                spr >>= 1;
                cr = counts[cLen - 1];

                rem -= cr << (16 - cLen);

                for (i = 0;  i < cr;  ++i) {                
                    for (j = spr;  j;  --j) {
                        vlc->bits = (unsigned char) cLen;
                        vlc->code = pix.pos[i];
                        ++vlc;
                    }
                }

                Skip(cr);
            }
            while (rem--) {
                vlc->bits = 0;
                ++vlc;
            }
        }
    }

    void DQTDec() {
        int i;
        unsigned char *tmp;
        GetLength();

        while (pix.length >= 65) {
            i = pix.pos[0];
         
            pix.qtavail |= 1 << i;
            tmp = &pix.qtab[i][0];
         
            for (i = 0;  i < 64;  i++) {
                tmp[i] = pix.pos[i + 1];
            }
            Skip(65);
        } 
    }

    int GetVLC(VLC* vlc, unsigned char* code) {
        int value = GetBits(16);
        int bits = vlc[value].bits;
        
        if (pix.bufbits < bits) {
            GetBits(bits);
        }
        pix.bufbits -= bits;

        value = vlc[value].code;
        if (code) {
          *code = (unsigned char) value;  
        } 
        bits = value & 15;
        
        value = GetBits(bits);
        pix.bufbits -= bits;

        if (value < (1 << (bits - 1))) {
            value += ((-1) << bits) + 1;
        }
        return value;
    }

    void BlockDec(ImageComp* c, unsigned char* out) {
        unsigned char code;
        int value, coef = 0;
        memset(pix.block, 0, sizeof(pix.block));

        c->DClast += GetVLC(&pix.vlctab[c->DCTabSel][0], NULL);
        pix.block[0] = (c->DClast) * pix.qtab[c->qtsel][0];
 
        while (coef < 63) {
            value = GetVLC(&pix.vlctab[c->ACTabSel][0], &code);
            if (!code) { 
                break;
            }
            
            coef += (code >> 4) + 1;
            pix.block[(int) ZigZagArr[coef]] = value * pix.qtab[c->qtsel][coef];
        } 
 
        for (coef = 0;  coef < 64;  coef += 8){
            IDCTableRow(&pix.block[coef]);
        }
        for (coef = 0;  coef < 8;  coef++){
            IDCTableCol(&pix.block[coef], &out[coef], c->step);
        }
    }

    void Decode() {
        int i, mbx, mby, sbx, sby;
        int rstcount = pix.rstinterval, nextrst = 0;
        ImageComp* c;
        GetLength();

        Skip(1);
        for (i = 0, c = pix.comp;  i < pix.ncomp;  i++, c++) {
            c->DCTabSel = pix.pos[1] >> 4;
            c->ACTabSel = (pix.pos[1] & 1) | 2;
            Skip(2);
        }

        Skip(pix.length);
        for (mby = 0;  mby < pix.mbheight;  mby++) {
           for (mbx = 0;  mbx < pix.mbwidth;  mbx++) {
                for (i = 0, c = pix.comp;  i < pix.ncomp;  i++, c++){
                    for (sby = 0;  sby < c->compY;  ++sby){
                        for (sbx = 0;  sbx < c->compX;  ++sbx) {
                            BlockDec(c, &c->pixels[((mby * c->compY + sby) * c->step + mbx * c->compX + sbx) << 3]);
                        }
                    } 
                }
                if (pix.rstinterval && !(--rstcount)) {
                    i = GetBits(16); 
                    nextrst = (nextrst + 1) & 7;
                    rstcount = pix.rstinterval;

                    for (i = 0;  i < 3;  ++i) {
                        pix.comp[i].DClast = 0;
                    }
                }
            }
        }
        pix.error = Error;
    }

    #define Mark4A -9
    #define Mark4B 111
    #define Mark4C 29
    #define Mark4D -3
    #define Mark3A 28
    #define Mark3B 109
    #define Mark3C -9
    #define Mark3X 104
    #define Mark3Y 27
    #define Mark3Z -3
    #define Mark2A 139
    #define Mark2B -11

    void CvTableH(ImageComp* c) {
        int xmax = c->width - 3;

        unsigned char *out, *lin, *lout;
        int x, y;
        
        out = (unsigned char*)AllocMem((c->width * c->height) << 1);

        lin = c->pixels;
        lout = out;

        for (y = c->height;  y;  --y) {
            lout[0] = FixBit(((Mark2A * lin[0] + Mark2B * lin[1])+ 64) >> 7);
            lout[1] = FixBit(((Mark3X * lin[0] + Mark3Y * lin[1] + Mark3Z * lin[2])+ 64) >> 7);
            lout[2] = FixBit(((Mark3A * lin[0] + Mark3B * lin[1] + Mark3C * lin[2])+ 64) >> 7);

            for (x = 0;  x < xmax;  ++x) {
                lout[(x << 1) + 3] = FixBit(((Mark4A * lin[x] + Mark4B * lin[x + 1] + Mark4C * lin[x + 2] + Mark4D * lin[x + 3])+ 64) >> 7);
                lout[(x << 1) + 4] = FixBit(((Mark4D * lin[x] + Mark4C * lin[x + 1] + Mark4B * lin[x + 2] + Mark4A * lin[x + 3])+ 64) >> 7);
            }

            lin += c->step;
            lout += c->width << 1;
            lout[-3] = FixBit(((Mark3A * lin[-1] + Mark3B * lin[-2] + Mark3C * lin[-3])+ 64) >> 7);
            lout[-2] = FixBit(((Mark3X * lin[-1] + Mark3Y * lin[-2] + Mark3Z * lin[-3])+ 64) >> 7);
            lout[-1] = FixBit(((Mark2A * lin[-1] + Mark2B * lin[-2])+ 64) >> 7);
        }

        c->width <<= 1;
        c->step = c->width;
        FreeMem(c->pixels);
        c->pixels = out;
    }

    void CvTableV(ImageComp* c) {
        int w = c->width;
        int s1 = c->step;
        int s2 = 2*s1;

        unsigned char *out, *cin, *cout;
        int x, y;
        out = (unsigned char*)AllocMem((c->width * c->height) << 1);
        
        for (x = 0;  x < w;  ++x) {
            cin = &c->pixels[x];
            cout = &out[x];
            *cout = FixBit(((Mark2A * cin[0] + Mark2B * cin[s1])+ 64) >> 7);  
            cout += w;
            *cout = FixBit(((Mark3X * cin[0] + Mark3Y * cin[s1] + Mark3Z * cin[s2])+ 64) >> 7);  
            cout += w;
            *cout = FixBit(((Mark3A * cin[0] + Mark3B * cin[s1] + Mark3C * cin[s2])+ 64) >> 7);  
            cout += w;
            cin += s1;

            for (y = c->height - 3;  y;  --y) {
                *cout = FixBit(((Mark4A * cin[-s1] + Mark4B * cin[0] + Mark4C * cin[s1] + Mark4D * cin[s2])+ 64) >> 7);  
                cout += w;
                *cout = FixBit(((Mark4D * cin[-s1] + Mark4C * cin[0] + Mark4B * cin[s1] + Mark4A * cin[s2])+ 64) >> 7);  
                cout += w;
                cin += s1;
            }
            cin += s1;
            *cout = FixBit(((Mark3A * cin[0] + Mark3B * cin[-s1] + Mark3C * cin[-s2])+ 64) >> 7);  
            cout += w;
            *cout = FixBit(((Mark3X * cin[0] + Mark3Y * cin[-s1] + Mark3Z * cin[-s2])+ 64) >> 7);  
            cout += w;
            *cout = FixBit(((Mark2A * cin[0] + Mark2B * cin[-s1])+ 64) >> 7);
        }

        c->height <<= 1;
        c->step = c->width;
        FreeMem(c->pixels);
        c->pixels = out;
    }

    void RGB() {
        int i;
        ImageComp* c;

        for (i = 0, c = pix.comp;  i < pix.ncomp;  i++, c++) {
            while ((c->width < pix.width) || (c->height < pix.height)) {
                if (c->width < pix.width) {
                    CvTableH(c);
                }    
                if (c->height < pix.height) {
                    CvTableV(c);
                }                
            }
        }

        if (pix.ncomp == 3) {
            int rgbX, rgbY;
            unsigned char *prgb = pix.rgb;
            unsigned char *pY  = pix.comp[0].pixels;
            unsigned char *pCb = pix.comp[1].pixels;
            unsigned char *pCr = pix.comp[2].pixels;
            
            for (rgbY = pix.height;  rgbY;  rgbY--) {
                for (rgbX = 0;  rgbX < pix.width;  rgbX++) {
                    register int Y = pY[rgbX] << 8;
                    register int Cb = pCb[rgbX] - 128;
                    register int Cr = pCr[rgbX] - 128;
                    *prgb++ = FixBit((Y + 359 * Cr + 128) >> 8);
                    *prgb++ = FixBit((Y -  88 * Cb - 183 * Cr + 128) >> 8);
                    *prgb++ = FixBit((Y + 454 * Cb + 128) >> 8);
                }
      
                pY += pix.comp[0].step;
                pCb += pix.comp[1].step;
                pCr += pix.comp[2].step;
            }
        }
    }

    int Decode(const unsigned char* jpeg, const int size) {
        pix.pos = (const unsigned char*) jpeg;
        pix.size = size & 0x7FFFFFFF;

        Skip(2);
        while (!pix.error) {
            Skip(2);
            switch (pix.pos[-1]) {
                case 0xC0: 
                    SOFDec();  
                    break;
                case 0xC4: 
                    DHTDec();  
                    break;
                case 0xDB: 
                    DQTDec();  
                    break;
                case 0xDA: 
                    Decode(); 
                    break;
                default:
                    if ((pix.pos[-1] & 0xF0) == 0xE0){
                        GetLength();
                        Skip(pix.length);
                    } else {
                        return Error;
                    }
            }
        }        
        RGB();
        ErrorDecode = pix.error; 
        return pix.error;
    }
};

Decoder::Decoder(const unsigned char* data, size_t size, void *(*malFunc)(size_t), void (*freeFunc)(void*)) : AllocMem(malFunc) , FreeMem(freeFunc) {
    char tmp[64] = {0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };
    memcpy(ZigZagArr, tmp, sizeof(ZigZagArr));
    memset(&pix, 0, sizeof(Info));
    Decode(data, size);
}

int Decoder::Width() { 
    return pix.width; 
}

int Decoder::Height() { 
    return pix.height; 
}

unsigned char* Decoder::Image() { 
    return (pix.ncomp == 1) ? pix.comp[0].pixels : pix.rgb; 
}

size_t Decoder::ImageSize() { 
    return pix.width * pix.height * pix.ncomp; 
}