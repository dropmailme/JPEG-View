#define W1 2841
#define W2 2676
#define W3 2408
#define W4 1609
#define W5 1108
#define W6 565

struct ImageComp {
    int compID;  
    int compX, compY; 
    int width, height;
    int step; 
    int qtsel; 
    int ACTabSel, DCTabSel; 
    int DClast; 
    unsigned char *pixels; 
};

struct VLC {
    unsigned char bits, code;
};

struct Info {
    int error;
    const unsigned char *pos; 
    int size;
    int length;
    int width, height; 
    int mbwidth, mbheight; 
    int mbsizex, mbsizey; 
    int ncomp;
    ImageComp comp[3]; 
    int qtused, qtavail; 
    unsigned char qtab[4][64];
    VLC vlctab[4][65536]; 
    int buf, bufbits; 
    int block[64]; 
    int rstinterval;
    unsigned char *rgb; 
};

unsigned char FixBit(const int x) {
    if (x < 0) {
        return 0;
    } else if (x > 0xFF) {
        return 0xFF;
    } else {
        return (unsigned char) x;
    }
}
    
int IDCTableRow(int* blockCode) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;
    int tmp = blockCode[0] << 3;
    if (!((x1 = blockCode[4] << 11) | (x2 = blockCode[6]) | (x3 = blockCode[2]) | (x4 = blockCode[1]) | (x5 = blockCode[7]) | (x6 = blockCode[5]) | (x7 = blockCode[3]))) {
        blockCode[0] = tmp;
        blockCode[1] = tmp;
        blockCode[2] = tmp;
        blockCode[3] = tmp;
        blockCode[4] = tmp;
        blockCode[5] = tmp;
        blockCode[6] = tmp;
        blockCode[7] = tmp;
        return 0;
    }

    x0 = (blockCode[0] << 11) + 128;
    x8 = W6 * (x4 + x5);
    x4 = x8 + (W1 - W6) * x4;
    x5 = x8 - (W1 + W6) * x5;
    x8 = W3 * (x6 + x7);
    x6 = x8 - (W3 - W4) * x6;
    x7 = x8 - (W3 + W4) * x7;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W5 * (x3 + x2);
    x2 = x1 - (W2 + W5) * x2;
    x3 = x1 + (W2 - W5) * x3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;
    x4 = (181 * (x4 - x5) + 128) >> 8;

    blockCode[0] = (x7 + x1) >> 8;
    blockCode[1] = (x3 + x2) >> 8;
    blockCode[2] = (x0 + x4) >> 8;
    blockCode[3] = (x8 + x6) >> 8;
    blockCode[4] = (x8 - x6) >> 8;
    blockCode[5] = (x0 - x4) >> 8;
    blockCode[6] = (x3 - x2) >> 8;
    blockCode[7] = (x7 - x1) >> 8;
}

int IDCTableCol(const int* blockCode, unsigned char *out, int stride) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;
    if (!((x1 = blockCode[8*4] << 8) | (x2 = blockCode[8*6]) | (x3 = blockCode[8*2]) | (x4 = blockCode[8*1]) | (x5 = blockCode[8*7]) | (x6 = blockCode[8*5]) | (x7 = blockCode[8*3]))) {
        x1 = FixBit(((blockCode[0] + 32) >> 6) + 128);
        for (x0 = 8;  x0;  --x0) {
            *out = (unsigned char) x1;
            out += stride;
        }
        return 0 ;
    }

    x0 = (blockCode[0] << 8) + 8192;
    x8 = W6 * (x4 + x5) + 4;
    x4 = (x8 + (W1 - W6) * x4) >> 3;
    x5 = (x8 - (W1 + W6) * x5) >> 3;
    x8 = W3 * (x6 + x7) + 4;
    x6 = (x8 - (W3 - W4) * x6) >> 3;
    x7 = (x8 - (W3 + W4) * x7) >> 3;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W5 * (x3 + x2) + 4;
    x2 = (x1 - (W2 + W5) * x2) >> 3;
    x3 = (x1 + (W2 - W5) * x3) >> 3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;
    x4 = (181 * (x4 - x5) + 128) >> 8;

    *out = FixBit(((x7 + x1) >> 14) + 128);  
    out += stride;
    
    *out = FixBit(((x3 + x2) >> 14) + 128);  
    out += stride;
    
    *out = FixBit(((x0 + x4) >> 14) + 128);  
    out += stride;
    
    *out = FixBit(((x8 + x6) >> 14) + 128);  
    out += stride;
    
    *out = FixBit(((x8 - x6) >> 14) + 128);  
    out += stride;
    
    *out = FixBit(((x0 - x4) >> 14) + 128);  
    out += stride;
    
    *out = FixBit(((x3 - x2) >> 14) + 128);  
    out += stride;
    *out = FixBit(((x7 - x1) >> 14) + 128);
}
