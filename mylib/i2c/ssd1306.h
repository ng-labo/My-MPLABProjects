#ifndef SSD1306_H
#define	SSD1306_H

#include "i2c_local.h"

#ifndef SSD1306_ADDR
#define SSD1306_ADDR  (0x78 >> 1)
#endif
// https://github.com/derkst/Cypress-PSOC-OLED

#define DISPLAYWIDTH 128
#ifdef SSD1306_128_64
#define DISPLAYHEIGHT 64
#endif // SSD1306_128_64
#ifdef SSD1306_128_32
#define DISPLAYHEIGHT 32
#endif // SSD1306_128_32
  
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SETPAGESTART 0xB0

// I2C result status
#define TRANSFER_CMPLT    (0x00u)
#define TRANSFER_ERROR    (0xFFu)

//control byte, Co bit = 0, D/C# = 0 (command)
#define SSD1306_CMD (0b00000000)
//control byte, Co bit = 0 (continue), D/C# = 1
#define SSD1306_DAT (0b01000000)    

#ifdef SSD1306_128_64
#define MAX_PAGE (8u)
#endif
#ifdef SSD1306_128_32
#define MAX_PAGE (4u)
#endif
#define MAX_COLUMN (128u)
    
typedef enum{
    SCROLL_RIGHT = 0x26,
    SCROLL_LEFT = 0x2A
}SCROLL_DIR;

typedef enum{
    SCROLL_SPEED_0 = 0x03,  // slowest
    SCROLL_SPEED_1 = 0x02,
    SCROLL_SPEED_2 = 0x01,
    SCROLL_SPEED_3 = 0x06,
    SCROLL_SPEED_4 = 0x00,
    SCROLL_SPEED_5 = 0x05,
    SCROLL_SPEED_6 = 0x04,
    SCROLL_SPEED_7 = 0x07   // fastest
}SCROLL_SPEED;

typedef enum{
    SCROLL_PAGE_0 = 0,
    SCROLL_PAGE_1,
    SCROLL_PAGE_2,
    SCROLL_PAGE_3,
    SCROLL_PAGE_4,
    SCROLL_PAGE_5,
    SCROLL_PAGE_6,
    SCROLL_PAGE_7   
}SCROLL_AREA;

#define BLACK 0
#define WHITE 1
#define INVERSE 2   

void gfx_drawPixel(int16_t x, int16_t y, uint16_t color);
void gfx_drawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color );
//void gfx_setTextColor( uint8_t color );
//void gfx_setTextBg( uint8_t background );
void gfx_write(const uint8_t ch);
void gfx_print(const char* s);
void gfx_println(const char* s);
//void gfx_drawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color );
void gfx_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color );
//void gfx_drawCircle( int16_t x0, int16_t y0, int16_t r,uint8_t color );
//void gfx_drawTriangle( int16_t x0, int16_t y0,int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color );
void gfx_drawRawChar(const int16_t x, const int16_t y, const unsigned char *font, const uint8_t fontsz_width, const uint8_t fontsz_height);
void gfx_drawChar(const int16_t x, const int16_t y, const uint8_t c, const uint8_t color, const uint8_t bg, const uint8_t size);
void gfx_setCursor( int16_t x, int16_t y );
void gfx_setTextSize( uint8_t size );
void gfx_setRotation(uint8_t x);
void gfx_setTextWrap(uint8_t wrap);

void display_init(void);
void display_setPixel( int16_t x, int16_t y, uint8_t color );
void display_contrast( uint8_t contrast );
#ifdef HAS_INVERT
void display_invert( uint8_t invert );
#endif
void display_update(void);
void display_update_page(uint8_t page);
void display_clear(void);
void display_line( int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color );
//void display_stopscroll(void);
//void display_scroll( SCROLL_AREA start, SCROLL_AREA end, SCROLL_DIR dir, SCROLL_SPEED speed );
void display_putchar5x7(uint8_t col, uint8_t page, const char *p, uint8_t rot);
#ifdef FONT1216NUMBER
void display_putchar12x16_rot0(uint8_t col, uint8_t page, const uint8_t *font);
void display_putchar12x16_rot2(uint8_t col, uint8_t page, const uint8_t *font);
void display_putchar12x16(uint8_t col, uint8_t page, const uint8_t *font, uint8_t rot);
#endif
#ifdef HAS_CHARTSCROLL
void display_chartscroll(uint8_t rot);
#endif
void display_textscroll(uint8_t rot);

#endif	/* SSD1306_H */


