#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tools.h"
#include "io.h"
#include "lcd.h"
#include "lcd/lcd_l2f50.h"
#include "lcd/lcd_ls020.h"
#include "lcd/lcd_lph88.h"
#include "lcd/lcd_mio283qt.h"
#include "lcd/font_8x8.h"
#ifndef LOADER
# include "lcd/font_8x12.h"
# include "lcd/font_clock.h"
#endif


unsigned int lcd_checkbit(unsigned long *data, unsigned int nr)
{
  return (data[nr/32] & (0x80000000UL>>(nr&31))); // (data[nr/32] & (1<<(nr&31)))
}


void lcd_img32(int x, unsigned int y, const unsigned char *img, unsigned int color, unsigned int bgcolor)
{
  int i, x0, y0, x1, y1;
  unsigned long *ptr, start, end;

  if((x <= -32) || (x >= LCD_WIDTH))
  {
    return;
  }

  ptr = (unsigned long*)img;

  if(x < 0)
  {
    x0    = 0;
    y0    = y;
    x1    = x+31;
    y1    = y+31;
    start = -x;
    end   = 32;
  }
  else if(x > (LCD_WIDTH-32))
  {
    x0    = x;
    y0    = y;
    x1    = LCD_WIDTH-1;
    y1    = y+31;
    start = 0;
    end   = (x1-x0)+1;
  }
  else
  {
    x0    = x;
    y0    = y;
    x1    = x+31;
    y1    = y+31;
    start = 0;
    end   = 32;
  }

  lcd_setarea(x0, y0, x1, y1);

  lcd_drawstart();

#ifdef LCD_L2F50
  unsigned int w, h;

  for(w=start; w<end; w++)
  {
    for(h=w; h<(32*32); h+=32)
    {
      if(lcd_checkbit(ptr, h))
      {
        lcd_draw(color);
      }
      else
      {
        lcd_draw(bgcolor);
      }
    }
  }

#else
  unsigned long data, mask;

  start = 1UL<<(31-start);
  if(end <= 31)
  {
    end = 1UL<<(31-end);
  }
  else
  {
    end = 0;
  }

  for(i=32; i!=0; i--) //height
  {
    data = *ptr++;
    //data = ((data&0xFF000000UL)>>24)|((data&0x00FF0000UL)>>8)|((data&0x0000FF00UL)<<8)|((data&0x000000FFUL)<<24); //swap32
    for(mask=start; mask>end; mask>>=1) //width
    {
      if(data & mask)
      {
        lcd_draw(color);
      }
      else
      {
        lcd_draw(bgcolor);
      }
    }
  }
#endif

  lcd_drawstop();

  return;
}


void lcd_putlinebr(unsigned int x, unsigned int y, const unsigned char *s, unsigned int font, unsigned int size, unsigned int color, unsigned int bgcolor)
{
  unsigned int start_x=x, font_height;
  char c;

  switch(font)
  {
    case SMALLFONT:  font_height=SMALLFONT_HEIGHT-1;  break;
#ifndef LOADER
    case NORMALFONT: font_height=NORMALFONT_HEIGHT-1; break;
    case TIMEFONT:   font_height=TIMEFONT_HEIGHT-1;   break;
#endif
  }

  lcd_fillrect(0, y, x-1, (y+(font_height*size)), bgcolor); //clear before text

  while(*s)
  {
    c = *s++;
    if(c == '\n') //new line
    {
      lcd_fillrect(x, y, (LCD_WIDTH-1), (y+(font_height*size)), bgcolor); //clear after text
      x  = start_x;
      y += (font_height*size)+2;
      lcd_fillrect(0, y, x-1, (y+(font_height*size)), bgcolor); //clear before text
      continue;
    }

    x = lcd_putc(x, y, c, font, size, color, bgcolor);
    if(x >= LCD_WIDTH) //new line
    {
      lcd_fillrect(x, y, (LCD_WIDTH-1), (y+(font_height*size)), bgcolor); //clear after text
      x  = start_x;
      y += (font_height*size)+2;
      lcd_fillrect(0, y, x-1, (y+(font_height*size)), bgcolor); //clear before text
      x = lcd_putc(x, y, c, font, size, color, bgcolor);
    }
  }

  return;
}


void lcd_putline(unsigned int x, unsigned int y, const unsigned char *s, unsigned int font, unsigned int size, unsigned int color, unsigned int bgcolor)
{
  unsigned int last_x=x, font_height;

  switch(font)
  {
    case SMALLFONT:  font_height=SMALLFONT_HEIGHT-1;  break;
#ifndef LOADER
    case NORMALFONT: font_height=NORMALFONT_HEIGHT-1; break;
    case TIMEFONT:   font_height=TIMEFONT_HEIGHT-1;   break;
#endif
  }

  lcd_fillrect(0, y, x-1, (y+(font_height*size)), bgcolor); //clear before text

  while(*s)
  {
    x = lcd_putc(x, y, *s++, font, size, color, bgcolor);
    if(x >= LCD_WIDTH)
    {
      break;
    }
    last_x = x;
  }

  lcd_fillrect(last_x, y, (LCD_WIDTH-1), (y+(font_height*size)), bgcolor); //clear after text

  return;
}


unsigned int lcd_puts(unsigned int x, unsigned int y, const unsigned char *s, unsigned int font, unsigned int size, unsigned int color, unsigned int bgcolor)
{
  while(*s)
  {
    x = lcd_putc(x, y, *s++, font, size, color, bgcolor);
    if(x >= LCD_WIDTH)
    {
      break;
    }
  }

  return x;
}


unsigned int lcd_putc(unsigned int x, unsigned int y, unsigned int c, unsigned int font, unsigned int size, unsigned int color, unsigned int bgcolor)
{
  unsigned int ret, i, j, width, height, w, h, wh;
  unsigned long *ptr;

  switch(font)
  {
    case SMALLFONT:
      c     -= SMALLFONT_START;
      ptr    = (unsigned long*)&SMALLFONT_NAME[c*(SMALLFONT_WIDTH*SMALLFONT_HEIGHT/8)];
      width  = SMALLFONT_WIDTH;
      height = SMALLFONT_HEIGHT;
      break;
#ifndef LOADER
    case NORMALFONT:
      c     -= NORMALFONT_START;
      ptr    = (unsigned long*)&NORMALFONT_NAME[c*(NORMALFONT_WIDTH*NORMALFONT_HEIGHT/8)];
      width  = NORMALFONT_WIDTH;
      height = NORMALFONT_HEIGHT;
      break;
    case TIMEFONT:
      c     -= TIMEFONT_START;
      ptr    = (unsigned long*)&TIMEFONT_NAME[c*(TIMEFONT_WIDTH*TIMEFONT_HEIGHT/8)];
      width  = TIMEFONT_WIDTH;
      height = TIMEFONT_HEIGHT;
      break;
#endif
  }

  ret = x+(width*size);
  if(ret >= LCD_WIDTH)
  {
    return LCD_WIDTH;
  }

  if(size <= 1)
  {
    lcd_setarea(x, y, x+(+width-1), y+(height-1));
    lcd_drawstart();

#ifdef LCD_L2F50
    wh = (width*height);
    for(w=0; w<width; w++)
    {
# ifdef LCD_MIRROR
      for(h=w+(wh-width); h<wh; h-=width)
# else
      for(h=w; h<wh; h+=width)
# endif
      {
        if(lcd_checkbit(ptr, h))
        {
          lcd_draw(color);
        }
        else
        {
          lcd_draw(bgcolor);
        }
      }
    }
#else
    unsigned long data, mask;
    for(wh=(width*height)/32; wh!=0; wh--)
    {
      data = *ptr++;
      //data = ((data&0xFF000000UL)>>24)|((data&0x00FF0000UL)>>8)|((data&0x0000FF00UL)<<8)|((data&0x000000FFUL)<<24); //swap32
      for(mask=0x80000000UL; mask!=0UL; mask>>=1)
      {
        if(data & mask)
        {
          lcd_draw(color);
        }
        else
        {
          lcd_draw(bgcolor);
        }
      }
    }
#endif

    lcd_drawstop();
  }
  else
  {
    lcd_setarea(x, y, x+(width*size)-1, y+(height*size)-1);
    lcd_drawstart();

#ifdef LCD_L2F50
    wh = (width*height);
    for(w=0; w<width; w++)
    {
      for(i=size; i!=0; i--)
      {
# ifdef LCD_MIRROR
        for(h=w+(wh-width); h<wh; h-=width)
# else
        for(h=w; h<wh; h+=width)
# endif
        {
          if(lcd_checkbit(ptr, h))
          {
            for(j=size; j!=0; j--)
            {
              lcd_draw(color);
            }
          }
          else
          {
            for(j=size; j!=0; j--)
            {
              lcd_draw(bgcolor);
            }
          }
        }
      }
    }
#else
    unsigned int bit;
    wh = (width*height);
    for(h=0; h<wh; h+=width)
    {
      for(i=size; i!=0; i--)
      {
        bit = h;
        for(w=0; w<width; w++)
        {
          if(lcd_checkbit(ptr, bit++))
          {
            for(j=size; j!=0; j--)
            {
              lcd_draw(color);
            }
          }
          else
          {
            for(j=size; j!=0; j--)
            {
              lcd_draw(bgcolor);
            }
          }
        }
      }
    }
#endif

    lcd_drawstop();
  }

  return ret;
}


void lcd_fillrect(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int color)
{
  unsigned int wh, tmp;

  if(x0 > x1)
  {
    tmp = x0;
    x0  = x1;
    x1  = tmp;
  }
  if(y0 > y1)
  {
    tmp = y0;
    y0  = y1;
    y1  = tmp;
  }

  if((x1 >= LCD_WIDTH) ||
     (y1 >= LCD_HEIGHT))
  {
    return;
  }

  lcd_setarea(x0, y0, x1, y1);

  lcd_drawstart();
  for(wh=((1+(x1-x0))*(1+(y1-y0))); wh!=0; wh--)
  {
    lcd_draw(color);
  }
  lcd_drawstop();

  return;
}


void lcd_drawrect(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int color)
{
  lcd_fillrect(x0, y0, x0, y1, color);
  lcd_fillrect(x0, y1, x1, y1, color);
  lcd_fillrect(x1, y0, x1, y1, color);
  lcd_fillrect(x0, y0, x1, y0, color);

  return;
}


void lcd_fillcircle(unsigned int x0, unsigned int y0, unsigned int radius, unsigned int color)
{
  int err, x, y;

  err = -radius;
  x   = radius;
  y   = 0;

  lcd_setarea(0, 0, (LCD_WIDTH-1), (LCD_HEIGHT-1));

  while(x >= y)
  {
    lcd_drawline(x0 - x, y0 + y, x0 + x, y0 + y, color);
    lcd_drawline(x0 - x, y0 - y, x0 + x, y0 - y, color);
    lcd_drawline(x0 - y, y0 + x, x0 + y, y0 + x, color);
    lcd_drawline(x0 - y, y0 - x, x0 + y, y0 - x, color);

    err += y;
    y++;
    err += y;
    if(err >= 0)
    {
      x--;
      err -= x;
      err -= x;
    }
  }

  return;
}


void lcd_drawcircle(unsigned int x0, unsigned int y0, unsigned int radius, unsigned int color)
{
  int err, x, y;

  err = -radius;
  x   = radius;
  y   = 0;

  lcd_setarea(0, 0, (LCD_WIDTH-1), (LCD_HEIGHT-1));

  while(x >= y)
  {
    lcd_drawpixel(x0 + x, y0 + y, color);
    lcd_drawpixel(x0 - x, y0 + y, color);
    lcd_drawpixel(x0 + x, y0 - y, color);
    lcd_drawpixel(x0 - x, y0 - y, color);
    lcd_drawpixel(x0 + y, y0 + x, color);
    lcd_drawpixel(x0 - y, y0 + x, color);
    lcd_drawpixel(x0 + y, y0 - x, color);
    lcd_drawpixel(x0 - y, y0 - x, color);

    err += y;
    y++;
    err += y;
    if(err >= 0)
    {
      x--;
      err -= x;
      err -= x;
    }
  }

  return;
}


void lcd_drawline(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int color)
{
  int dx, dy, dx2, dy2, stepx, stepy, err;

  if((x0 == x1) ||
     (y0 == y1)) //horizontal or vertical line
  {
    lcd_fillrect(x0, y0, x1, y1, color);
  }
  else
  {
    //calculate direction
    dx = x1 - x0;
    dy = y1 - y0;
    if(dx < 0) { dx = -dx; stepx = -1; } else { stepx = +1; }
    if(dy < 0) { dy = -dy; stepy = -1; } else { stepy = +1; }
    dx2 = dx << 1;
    dy2 = dy << 1;
    //draw line
    lcd_setarea(0, 0, (LCD_WIDTH-1), (LCD_HEIGHT-1));
    lcd_drawpixel(x0, y0, color);
    if(dx > dy)
    {
      err = dy2 - dx;
      while(x0 != x1)
      {
        if(err >= 0)
        {
          err -= dx2;
          y0  += stepy;
        }
        err += dy2;
        x0  += stepx;
        lcd_drawpixel(x0, y0, color);
      }
    }
    else
    {
      err = dx2 - dy;
      while(y0 != y1)
      {
        if(err >= 0)
        {
          err -= dy2;
          x0  += stepx;
        }
        err += dx2;
        y0  += stepy;
        lcd_drawpixel(x0, y0, color);
      }
    }
  }

  return;
}


void lcd_drawpixel(unsigned int x, unsigned int y, unsigned int color)
{
  if((x >= LCD_WIDTH) ||
     (y >= LCD_HEIGHT))
  {
    return;
  }

  lcd_setcursor(x, y);

  lcd_drawstart();
  lcd_draw(color);
  lcd_drawstop();

  return;
}


void lcd_clear(unsigned int color)
{
  unsigned int i;

  lcd_setarea(0, 0, (LCD_WIDTH-1), (LCD_HEIGHT-1));

  lcd_drawstart();
  for(i=(LCD_WIDTH*LCD_HEIGHT); i!=0; i--)
  {
    lcd_draw(color);
  }
  lcd_drawstop();

  return;
}


void lcd_init(void)
{
  pwm_led(50); //led backlight on

  ssi_speed(2000000); //2 MHz
  lcd_reset();
  ssi_speed(0); //ssi speed up (0 = default speed)

  return;
}
