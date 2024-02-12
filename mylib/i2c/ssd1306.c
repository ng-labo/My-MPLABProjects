#include "defines.h"
#include "i2c_local.h"
#include "mylib/i2c.h"
#include "mcc_generated_files/i2c1.h"

#include "font5x7.h"
#include "ssd1306.h"

#include <stdio.h>
#include <stdarg.h>

#define I2C_SSD1306 (0x78>>1)
#ifdef I2C_SSD1306

uint8_t frameBuff[MAX_PAGE][MAX_COLUMN+1]={
    {SSD1306_DAT, 0},
    {SSD1306_DAT, 0},
    {SSD1306_DAT, 0},
    {SSD1306_DAT, 0},
#ifdef SSD1306_128_64
    {SSD1306_DAT, 0},
    {SSD1306_DAT, 0},
    {SSD1306_DAT, 0},
    {SSD1306_DAT, 0},
#endif
};

// ============================================================
// graphics library stuff

// This is the 'raw' display w/h - never changes
static struct GFXPARAMS {
    int16_t width;
    int16_t height;
    int16_t cursor_x;
    int16_t cursor_y;
    uint16_t textcolor;
    uint16_t textbgcolor;
    uint8_t textsize;
    uint8_t rotation;
    uint8_t wrap; // If set, 'wrap' text at right edge of display
} gfxParams = {
    DISPLAYWIDTH,
    DISPLAYHEIGHT,
    0,
    0,
    WHITE,
    WHITE,
    1,
    0,
    1
};

void gfx_setCursor( int16_t x, int16_t y ){
    gfxParams.cursor_x = x;
    gfxParams.cursor_y = y;
}

void gfx_setTextSize( uint8_t size ){
    gfxParams.textsize = (size > 0) ? size : 1;
}
/*
void gfx_setTextColor( uint8_t color ){
    // For 'transparent' background, we'll set the bg
    // to the same as fg instead of using a flag
    textcolor = textbgcolor = color;
}

void gfx_setTextBg( uint8_t color ){
    textbgcolor = color;
}*/

void gfx_setTextWrap( uint8_t w ){
    gfxParams.wrap = w;
}

static uint8_t gfx_rotation_adjust_0(int16_t* px, int16_t* py ) {
    return (*px<0||*px>=DISPLAYWIDTH||*py<0||*py>=DISPLAYHEIGHT);
}
static uint8_t gfx_rotation_adjust_2(int16_t* px, int16_t* py ) {
    *px = DISPLAYWIDTH - *px - 1;
    *py = DISPLAYHEIGHT - *py - 1;
    return (*px<0||*px>=DISPLAYWIDTH||*py<0||*py>=DISPLAYHEIGHT);
}
#ifdef HAS_ROT_1_3
static uint8_t gfx_rotation_adjust_1(int16_t* px, int16_t* py ) {
    int16_t y0 = *py;
    *py = *px;
    *px = DISPLAYWIDTH - y0 - 1;
    return (*px<0||*px>=DISPLAYWIDTH||*py<0||*py>=DISPLAYHEIGHT);
}
static uint8_t gfx_rotation_adjust_3(int16_t* px, int16_t* py ) {
    int16_t y0 = *py;
    *py = DISPLAYHEIGHT - *px - 1;
    *px = y0;
    return (*px<0||*px>=DISPLAYWIDTH||*py<0||*py>=DISPLAYHEIGHT);
}
#endif

static uint8_t (*gfx_rotation_adjust)();

/*
static void gfx_rotation_adjust( int16_t* px, int16_t* py ){

    int16_t y0 = *py;
    
    switch( gfxParams.rotation ){
        case 1:
            *py = *px;
            *px = DISPLAYWIDTH - y0 - 1;
            break;
        case 2:
            *px = DISPLAYWIDTH - *px - 1;
            *py = DISPLAYHEIGHT - *py - 1;
            break;
        case 3:
            *py = DISPLAYHEIGHT - *px - 1;
            *px = y0;
            break;
    }
}*/
void gfx_setRotation( uint8_t x ){
    
    gfxParams.rotation = (x & 3);
    switch( gfxParams.rotation ){
        case 0:
            gfx_rotation_adjust = gfx_rotation_adjust_0;
            gfxParams.width = DISPLAYWIDTH;
            gfxParams.height = DISPLAYHEIGHT;
            break;
        case 2:
            gfx_rotation_adjust = gfx_rotation_adjust_2;
            gfxParams.width = DISPLAYWIDTH;
            gfxParams.height = DISPLAYHEIGHT;
            break;
#ifdef HAS_ROT_1_3
        case 1:
            gfx_rotation_adjust = gfx_rotation_adjust_1;
            gfxParams.width = DISPLAYHEIGHT;
            gfxParams.height = DISPLAYWIDTH;
            break;
        case 3:
            gfx_rotation_adjust = gfx_rotation_adjust_3;
            gfxParams.width = DISPLAYHEIGHT;
            gfxParams.height = DISPLAYWIDTH;
            break;
#endif
    }
}


void gfx_drawPixel( int16_t x, int16_t y, uint16_t color ){
    if(gfx_rotation_adjust( &x, &y )) return;
    display_setPixel(x, y, color);
}

// helper function for gfx_drawLine, handles special cases of horizontal and vertical lines
static void gfx_hvLine( int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color ){
    
    if( x1 != x2 && y1 != y2 ){
        // neither vertical nor horizontal
        return;
    }    
    
    // bounds check by gfx_rotation_adjust() return
    if(gfx_rotation_adjust( &x1, &y1 )) return;
    if(gfx_rotation_adjust( &x2, &y2 )) return;
    
    // ensure coords are from left to right and top to bottom
    if( (x1 == x2 && y2 < y1) || (y1 == y2 && x2 < x1) ){
        // swap as needed
        int16_t t = x1; x1 = x2; x2 = t;
        t = y1; y1 = y2; y2 = t;
    }
    
    display_line( x1, y1, x2, y2, color );
}

inline int16_t my_abs(int16_t a) {
	return (a < 0) ? -a : a;
}

// always use this function for line drawing
void gfx_drawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color ){
 
    if( x0 == x1 || y0 == y1 ){
        // vertical and horizontal lines can be drawn faster
        gfx_hvLine( x0, y0, x1, y1, color );
        return;
    }
    
    int16_t t;
    
    int16_t steep = my_abs(y1 - y0) > my_abs(x1 - x0);
    if( steep ){
        t = x0; x0 = y0; y0 = t;
        t = x1; x1 = y1; y1 = t;
    }
    if( x0 > x1 ){
        t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }
    int16_t dx, dy;
    dx = x1 - x0;
    dy = my_abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = ( y0 < y1 ) ? 1 : -1;
    /*if( y0 < y1 ){
        ystep = 1;
    }else{
        ystep = -1;
    }*/
    for( ; x0<=x1; x0++ ){
        if( steep ){
            gfx_drawPixel( y0, x0, color );
        }else{
            gfx_drawPixel( x0, y0, color );
        }
        err -= dy;
        if( err < 0 ){
            y0 += ystep;
            err += dx;
        }
    }
}
/*
void gfx_drawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color ){
    
    gfx_drawLine( x, y, x+w-1, y, color );
    gfx_drawLine( x, y+h-1, x+w-1, y+h-1, color );
    gfx_drawLine( x, y, x, y+h-1, color );
    gfx_drawLine( x+w-1, y, x+w-1, y+h-1, color );
}
*/
void gfx_fillRect( int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color ){
    int16_t i = 0;
    if( h > w ){
        const int16_t yh = y + h - 1;
        for( i = x ; i < x+w ; i++ ){
            gfx_drawLine( i, y, i, yh, color );
        }
    }else{
        const int16_t xw = x + w - 1;
        for( i = y ; i < y+h ; i++ ){
            gfx_drawLine( x, i, xw, i, color );
        }
    }
}

/*
// circle outline
void gfx_drawCircle( int16_t x0, int16_t y0, int16_t r,uint16_t color ){

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    gfx_drawPixel( x0 , y0+r, color );
    gfx_drawPixel( x0 , y0-r, color );
    gfx_drawPixel( x0+r, y0 , color );
    gfx_drawPixel( x0-r, y0 , color );
    while( x < y ){
        if( f >= 0 ){
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        gfx_drawPixel( x0 + x, y0 + y, color );
        gfx_drawPixel( x0 - x, y0 + y, color );
        gfx_drawPixel( x0 + x, y0 - y, color );
        gfx_drawPixel( x0 - x, y0 - y, color );
        gfx_drawPixel( x0 + y, y0 + x, color );
        gfx_drawPixel( x0 - y, y0 + x, color );
        gfx_drawPixel( x0 + y, y0 - x, color );
        gfx_drawPixel( x0 - y, y0 - x, color );
    }
}

void gfx_drawTriangle( int16_t x0, int16_t y0,int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color ){
    
    gfx_drawLine( x0, y0, x1, y1, color );
    gfx_drawLine( x1, y1, x2, y2, color );
    gfx_drawLine( x2, y2, x0, y0, color );
}
*/
// Draw a character
void gfx_drawRawChar(const int16_t x, const int16_t y, const unsigned char *font, const uint8_t fontsz_width, const uint8_t fontsz_height) {
    int16_t i = 0;
    int16_t cx = x;
    for( i = 0 ; i < fontsz_width ; i++, cx++, font++ ){
        unsigned char line = *font;
        int16_t j = 0;
        int16_t cy = y; 
        for( j = 0; j < fontsz_height ; j++, cy++ ){
            gfx_drawPixel( cx, cy, (line & 0x1 ) ? WHITE : BLACK );
            line >>= 1;
        }
    }    
}
void gfx_drawChar(const int16_t x, const int16_t y,const uint8_t c, const uint8_t color, const uint8_t bg, const uint8_t size) {
    if( (x >= gfxParams.width) || // Clip right
        (y >= gfxParams.height) || // Clip bottom
        ((x + 6 * size - 1) < 0) || // Clip left
        ((y + 8 * size - 1) < 0)) // Clip top
        return;

    if(size==1){
        gfx_drawRawChar(x, y, &font5x7[c*5], FONT_WIDTH, FONT_HEIGHT);
        /*
        int16_t i = 0;
        int16_t cx = x;
        uint8_t* pfnt = &font[c*5];
        for( i = 0 ; i < 5 ; i++, cx++, pfnt++ ){
            uint8_t line = *pfnt;
            int16_t j = 0;
            int16_t cy = y; 
            for( j = 0; j < 8 ; j++, cy++ ){
                if( line & 0x1 ){
                    gfx_drawPixel( cx, cy, color );
                } else if( bg != color ){
                    gfx_drawPixel( cx, cy, bg );
                }
                line >>= 1;
            }
            if( bg != color ){
                cy = y; 
                for( j = 0; j < 8 ; j++, cy++ ){
                    gfx_drawPixel( cx, cy, bg );
                }
            }
        }*/
    }else{
        int8_t i = 0;
        for( i = 0 ; i < 6 ; i++ ){
            uint8_t line = (i==5) ? 0x0 :  font5x7[(c*5)+i];
            int8_t j = 0;
            int16_t cx = x + (i * size);
            for( j = 0; j < 8 ; j++ ){
                if( line & 0x1 ){
                    gfx_fillRect(cx, y+(j*size), size, size, color );
                } else if( bg != color ){
                    gfx_fillRect(cx, y+j*size, size, size, bg );
                }
                line >>= 1;
            }
        }
    }
}

void gfx_write( uint8_t ch ){
    if( ch == '\n' ){
        gfxParams.cursor_y += gfxParams.textsize * 8;
        gfxParams.cursor_x = 0;
    }else if( ch == '\r' ){
        // skip em
    }else{
        gfx_drawChar(gfxParams.cursor_x, gfxParams.cursor_y, ch, gfxParams.textcolor, gfxParams.textbgcolor, gfxParams.textsize);
        gfxParams.cursor_x += gfxParams.textsize * 6;
        if( gfxParams.wrap && (gfxParams.cursor_x > (gfxParams.width - gfxParams.textsize * 6)) ){
            gfxParams.cursor_y += gfxParams.textsize * 8;
            gfxParams.cursor_x = 0;
        }
    }
}

I2C1_MESSAGE_STATUS write_ssd1306(uint8_t* data, uint8_t len) {
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    I2C1_MasterWrite( data, len, SSD1306_ADDR, &status);
    __delay_ms(5); // ? SSD1306 

    uint16_t c = I2C_TO_CNT;
    while (status == I2C1_MESSAGE_PENDING && c--){
        __delay_us(100);
    }
    return (status == I2C1_MESSAGE_COMPLETE);    
}

// see data sheet page 25 for Graphic Display Data RAM organization
// 8 pages, each page a row of DISPLAYWIDTH bytes
// start address of of row: y/8*DISPLAYWIDTH
// x pos in row: == x 
// #define GDDRAM_ADDRESS(X,Y) ((_displaybuf)+((Y)/8)*(FRAME_BUFFER_WIDTH)+(X+1))
#define GDDRAM_ADDRESS(X,Y) (&frameBuff[Y >> 3][X + 1])

// lower 3 bit of y determine vertical pixel position (pos 0...7) in GDDRAM byte
// (y&0x07) == position of pixel on row (page). LSB is top, MSB bottom
#define GDDRAM_PIXMASK(Y) (1 << ((Y)&0x07))

#define PIXEL_ON(X,Y) (*GDDRAM_ADDRESS(X,Y) |= GDDRAM_PIXMASK(Y))
#define PIXEL_OFF(X,Y) (*GDDRAM_ADDRESS(X,Y) &= ~GDDRAM_PIXMASK(Y))
#define PIXEL_TOGGLE(X,Y) (*GDDRAM_ADDRESS(X,Y) ^= GDDRAM_PIXMASK(Y)) 

#define display_write_buf( buf, size ) write_ssd1306(buf, size)

// call before first use of other functions
void display_init( void ){
    uint8_t cmdbuf[] = {
        SSD1306_CMD,
        SSD1306_DISPLAYOFF,
        SSD1306_SETDISPLAYCLOCKDIV,
        0x80,
        SSD1306_SETMULTIPLEX,
        DISPLAYHEIGHT-1,
        SSD1306_SETDISPLAYOFFSET,
        0x00,
        SSD1306_SETSTARTLINE | 0x0,
        SSD1306_CHARGEPUMP,
        0x14,
        SSD1306_MEMORYMODE,
        0x00,
        SSD1306_SEGREMAP | 0x1,
        SSD1306_COMSCANDEC,
#ifdef SSD1306_128_64
        SSD1306_SETCOMPINS,
        0x12,
        SSD1306_SETCONTRAST,
        0xcf,
#endif
#ifdef SSD1306_128_32
        SSD1306_SETCOMPINS,
        0x02,
        SSD1306_SETCONTRAST,
        0x8f,
#endif
        SSD1306_SETPRECHARGE,
        0xf1,
        SSD1306_SETVCOMDETECT,
        0x40,
        SSD1306_DISPLAYALLON_RESUME,
        SSD1306_NORMALDISPLAY,
        SSD1306_DISPLAYON
    };
    
    display_write_buf( cmdbuf, sizeof(cmdbuf) ); 
}

// used by gfx_ functions. Needs to be implemented by display_
void display_setPixel( int16_t x, int16_t y, uint8_t color ){
    
    if( (x < 0) || (x >= DISPLAYWIDTH) || (y < 0) || (y >= DISPLAYHEIGHT) )
        return;

    switch( color ){
        case WHITE: 
            PIXEL_ON(x, y);
            break;
        case BLACK:
            PIXEL_OFF(x, y);
            break;
        case INVERSE: 
            PIXEL_TOGGLE(x, y);
            break;
    }
}

void display_clear(void){
    uint8_t page, j;
    for(page = 0; page<MAX_PAGE; page++){
        frameBuff[page][0] = SSD1306_DAT;
        uint8_t* d = &frameBuff[page][1];
        for(j = 0; j<MAX_COLUMN; j++) *d++ = 0;
    }
}

// contrast: 0 ...255
void display_contrast( uint8_t contrast ){
        
    uint8_t cmdbuf[] = {
        SSD1306_CMD,  
        SSD1306_SETCONTRAST,
        0   // contrast
    };
    cmdbuf[2] = contrast;
    display_write_buf( cmdbuf, sizeof(cmdbuf) ); 
}

// invert <> 0 for inverse display, invert == 0 for normal display
#ifdef HAS_INVERT
void display_invert( uint8_t invert ){
    uint8_t cmdbuf[] = {
        SSD1306_CMD,  
        0
    };
    cmdbuf[1] = invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY;
    display_write_buf(cmdbuf, sizeof(cmdbuf)); 
}
#endif

void display_update(void) {
    uint8_t cmdbuf[] = {
        SSD1306_CMD,
        SSD1306_MEMORYMODE,
        0,
        SSD1306_COLUMNADDR,
        0,              // start
        DISPLAYWIDTH-1, // end
        SSD1306_PAGEADDR, // SSD1306_SETPAGESTART didnt work in my case
        0,
        1
    };
    uint8_t page;
    for(page = 0; page < MAX_PAGE; page++){
        cmdbuf[7] = page;
        cmdbuf[8] = page + 1;
        display_write_buf(cmdbuf, sizeof(cmdbuf)); 
        frameBuff[page][0] = SSD1306_DAT;
        display_write_buf(frameBuff[page], sizeof(frameBuff[page]));
    }
}

void display_update_page(uint8_t page) {
    uint8_t cmdbuf[] = {
        SSD1306_CMD,
        SSD1306_MEMORYMODE,
        0,
        SSD1306_COLUMNADDR,
        0,
        DISPLAYWIDTH-1,
        SSD1306_PAGEADDR,
        page,
        page+1
    };
    display_write_buf(cmdbuf, sizeof(cmdbuf)); 
    frameBuff[page][0] = SSD1306_DAT;
    display_write_buf(frameBuff[page], sizeof(frameBuff[page]));
}

// draws horizontal or vertical line
// Note: no check for valid coords, this needs to be done by caller
// should only be called from gfx_hvline which is doing all validity checking
void display_line( int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color ){

    if( x1 == x2 ){
        // vertical
        uint8_t* pstart = GDDRAM_ADDRESS(x1, y1);
        uint8_t* pend = GDDRAM_ADDRESS(x2, y2);       
        uint8_t* ptr = pstart;             
        
        while( ptr <= pend ){
            
            uint8_t mask;
            if( ptr == pstart ){
                // top
                uint8_t lbit = y1 % 8;
                // bottom (line can be very short, all inside this one byte)
                uint8_t ubit = lbit + y2 - y1;
                if( ubit >= 7 )
                    ubit = 7;
                mask = ((1 << (ubit-lbit+1)) - 1) << lbit;    
            } else if( ptr == pend ){
                // top is always bit 0, that makes it easy
                // bottom
                mask = (1 << (y2 % 8)) - 1;    
            }

            if( ptr == pstart || ptr == pend ){
                switch( color ){
                    case WHITE:     *ptr |= mask; break;
                    case BLACK:     *ptr &= ~mask; break;
                    case INVERSE:   *ptr ^= mask; break;
                };  
            } else {
                switch( color ){
                    case WHITE:     *ptr = 0xff; break;
                    case BLACK:     *ptr = 0x00; break;
                    case INVERSE:   *ptr ^= 0xff; break;
                };  
            }
            
            ptr += (DISPLAYWIDTH+1); // mod kane4d
        }
    }else{
        // horizontal
        uint8_t* pstart = GDDRAM_ADDRESS(x1, y1);
        uint8_t* pend = pstart + x2 - x1;
        uint8_t pixmask = GDDRAM_PIXMASK(y1);    

        uint8_t* ptr = pstart;
        switch( color ){
            case WHITE:
                while( ptr <= pend ) *ptr++ |= pixmask;
                break;
            case BLACK:
                while( ptr <= pend ) *ptr++ &= ~pixmask;
                break;
            case INVERSE:
                while( ptr <= pend ) *ptr++ ^= pixmask;
                break;
        }
    }
}

inline unsigned reverse_bits(uint8_t v) {
    // swap odd and even bits
    v = ((v >> 1) & 0x55) | ((v & 0x55) << 1);
    // swap consecutive pairs
    v = ((v >> 2) & 0x33) | ((v & 0x33) << 2);
    // swap nibbles ... 
    v = ((v >> 4)         | (v<< 4));
    return v;
}

void display_putchar5x7(uint8_t unit, uint8_t page, const char *p, uint8_t rot) {
    const uint8_t w = 8; //, h = 16;
    char* lp = (char*) p;
    while(*lp) lp++;
    int l = lp - p;
    if(l==0){
        return;
    }
    if(l>16)l=16;
    uint8_t cmdbuf[] = {
        SSD1306_CMD,
        SSD1306_MEMORYMODE,
        0,
        SSD1306_PAGEADDR,
        page,
        page,
        SSD1306_COLUMNADDR,
        rot == 0 ? unit * w : 127 - (unit+l) * w -1,
        rot == 0 ? (unit+l) * w -1 : 127 - unit * w
    };
    display_write_buf(cmdbuf, sizeof(cmdbuf));
    uint8_t datbuf[129];
    uint8_t* d = datbuf;
    *d++ = SSD1306_DAT;
    int i, j;
    if(rot==0) {
        j = 0;
        while(*p&&j<17){
            uint8_t* pf = (uint8_t*) &font5x7[((int) *p++) * 5];
            for(i=0;i<5;i++) *d++ = *pf++;
            for(i=0;i<3;i++) *d++ = 0;
            j++;
        }
    } else {
        d = &datbuf[l*8];
        j = 0;
        while(*p&&j<17){
            uint8_t* pf = (uint8_t*) &font5x7[((int) *p++) * 5];
            for(i=0;i<5;i++) *d-- = reverse_bits(*pf++);
            for(i=0;i<3;i++) *d-- = 0;
            j++;
        }
    }
    display_write_buf(datbuf, 1 + j * 8);
}

#ifdef FONT1216NUMBER
void display_putchar12x16_rot0(uint8_t unit, uint8_t page, const uint8_t *font) {
     const uint8_t w = 12; //, h = 16;
    uint8_t cmdbuf[] = {
        SSD1306_CMD,
        SSD1306_MEMORYMODE,
        0,
        SSD1306_PAGEADDR,
        page,
        page + 1,
        SSD1306_COLUMNADDR,
        unit * w,
        (unit+1) * w -1
    };
    display_write_buf( cmdbuf, sizeof(cmdbuf) );
    uint8_t datbuf[25];
    uint8_t* d = datbuf;
    *d++ = SSD1306_DAT;
    int i=0;
    for(;i<24;i++) *d++ = *font++;
    display_write_buf( datbuf, sizeof(datbuf) );
}

void display_putchar12x16_rot2(uint8_t unit, uint8_t page, const uint8_t *font) {
     const uint8_t w = 12; //, h = 16;
     uint8_t cmdbuf[] = {
        SSD1306_CMD,
        SSD1306_MEMORYMODE,
        0,
        SSD1306_PAGEADDR,
        MAX_PAGE - page - 2,
        MAX_PAGE - page - 1,
        SSD1306_COLUMNADDR,
        127 - ((unit+1) * w - 1),
        127 - unit * w
    };
    display_write_buf(cmdbuf, sizeof(cmdbuf));
    uint8_t datbuf[25];
    int i=0;
    uint8_t* d = &datbuf[24];
    for(i=0;i<24;i++) {
        *d-- = reverse_bits(*font++);
    }
    *d = SSD1306_DAT;
    display_write_buf( datbuf, sizeof(datbuf) );
}

void display_putchar12x16(uint8_t unit, uint8_t page, const uint8_t *font, uint8_t rot) {
    if(rot==2) display_putchar12x16_rot2(unit, page, font);
    else display_putchar12x16_rot0(unit, page, font);
}
#endif
#ifdef HAS_CHARTSCROLL
void display_chartscroll(uint8_t rot){
    if(rot&0x01)return;
    int page, j;
    if(rot==0){
        for(page=1;page<MAX_PAGE;page++){
            uint8_t* d = &frameBuff[page][1];
            uint8_t* s = &frameBuff[page][9];
            for(j=0;j<120;j++) *d++ = *s++;
            for(j=0;j<8;j++) *d++ = 0;
        }
    }else{
        for(page=0;page<MAX_PAGE-1;page++){
            uint8_t* d = &frameBuff[page][128];
            uint8_t* s = &frameBuff[page][120];
            for(j=0;j<120;j++) *d-- = *s--;
            for(j=0;j<8;j++) *d-- = 0;
        }
    }    
}
#endif
void display_textscroll(uint8_t rot){
    if(rot&0x01){
        int i, j;
        if(rot==1){
            for(i=0;i<MAX_PAGE-1;i++){
                uint8_t* d = &frameBuff[i][128];
                uint8_t* s = &frameBuff[i][120];
                for(j=0;j<120;j++) *d-- = *s--;
                for(j=0;j<8;j++) *d-- = 0;
            }
        }else{
            for(i=0;i<MAX_PAGE-1;i++){
                uint8_t* d = &frameBuff[i][1];
                uint8_t* s = &frameBuff[i][9];
                for(j=0;j<120;j++) *d++ = *s++;
                for(j=0;j<8;j++) *d++ = 0;
            }
        }
    }else{
        int i, j;
        /*if(rot==0){
            
        }else{
            
        }*/
        for(i=0;i<MAX_PAGE-1;i++){
            uint8_t* d = rot==0 ? &frameBuff[i][1] : &frameBuff[MAX_PAGE-1-i][1];
            uint8_t* s = rot==0 ? &frameBuff[i+1][1] : &frameBuff[MAX_PAGE-2-i][1];
            for(j=0;j<128;j++) *d++=*s++;
        }
        uint8_t* d = rot==0 ? &frameBuff[MAX_PAGE-1][1] : &frameBuff[0][1];
        for(j=0;j<128;j++) *d++ = 0;
    }
}

void OledInit(uint8_t rot) {
    gfx_setCursor(0, 0);
    gfx_setTextSize(1);
    gfx_setRotation(rot);
    gfx_setTextWrap(1);

    display_init();
#ifdef HAS_INVERT
    display_invert(0);
#endif
    display_clear();
    
    display_update();
}

void OledPutPrint(uint8_t cx, uint8_t cy, const char* format, ...) {
    if(gfxParams.rotation&0x01){
        // not implement yet
        return;
    }
    
    char buf[64];
    va_list argList;

    va_start(argList, format);
    vsprintf(buf, format, argList);
    va_end(argList);

    uint16_t tmpx = gfxParams.cursor_x, tmpy = gfxParams.cursor_y;
    gfxParams.cursor_x = cx;
    gfxParams.cursor_y = cy;
    gfxParams.wrap = 0;
    register const uint8_t *cp = (const uint8_t*) buf;
    while(*cp) gfx_write(*cp++);
    if(gfxParams.rotation==2) cy = (DISPLAYHEIGHT-8) - cy;
    //display_update();
    display_update_page(cy>>3);
    gfxParams.wrap = 1;
    gfxParams.cursor_x = tmpx;
    gfxParams.cursor_y = tmpy;
}

void OledPrint(const char *format, ...) {
    char buf[64];
    va_list argList;

    va_start(argList, format);
    vsprintf(buf, format, argList);
    va_end(argList);

    uint8_t *cp = (uint8_t*) buf;
    while(*cp) {
        if(gfxParams.cursor_y>=gfxParams.height){
            display_textscroll(gfxParams.rotation);
            gfxParams.cursor_y = gfxParams.height - 8;
        }
        gfx_write(*cp++);
    }

    display_update();
}

void OledClear() {
    display_clear();
    display_update();
    gfxParams.cursor_x = 0;
    gfxParams.cursor_y = 0;
}

void OledShutdown() {
    uint8_t cmdbuf[] = {
        SSD1306_CMD,
        SSD1306_DISPLAYOFF
    };
    display_write_buf(cmdbuf, sizeof(cmdbuf));
}

#endif // I2C_SSD1306


