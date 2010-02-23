/*---------------------------------------------------
 * WebRadio (c) Andreas Watterott (www.watterott.net)
 *---------------------------------------------------
 * For more information, see readme.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "lmi/inc/hw_types.h"
#include "lmi/inc/hw_memmap.h"
#include "lmi/driverlib/sysctl.h"
#include "lmi/driverlib/gpio.h"
#include "lmi/driverlib/systick.h"
#include "fatfs/ff.h"
#include "debug.h"
#include "tools.h"
#include "main.h"
#include "io.h"
#include "lcd.h"
#include "lcd/font_8x8.h"
#include "lcd/font_8x12.h"
#include "lcd/font_clock.h"
#include "mmc_io.h"
#include "mmc.h"
#include "vs.h"
#include "eth.h"
#include "eth/ntp.h"
#include "eth/ssdp.h"
#include "buffer.h"
#include "menu.h"
#include "alarm.h"
#include "settings.h"
#include "chucknorris.h"


volatile unsigned int status=0, standby_active=0;
volatile long on_time=0;
volatile unsigned int ms_time=0;
unsigned long sec_time=0;
TIME time;
char date_str[14] = {'T','h',' ','0','1','.','0','1','.','1','9','7','0',0}; //Th 01.01.1970
char clock_str[9] = {'0','0',':','0','0',':','0','0',0}; //00:00:00

const char day_tab[7][3] = 
{
  {"Su"},{"Mo"},{"Tu"},{"We"},{"Th"},{"Fr"},{"Sa"} //English
  //{"So"},{"Mo"},{"Di"},{"Mi"},{"Do"},{"Fr"},{"Sa"} //German
};

const char clock_tab[60][3] = 
{
  {"00"},{"01"},{"02"},{"03"},{"04"},{"05"},{"06"},{"07"},{"08"},{"09"},
  {"10"},{"11"},{"12"},{"13"},{"14"},{"15"},{"16"},{"17"},{"18"},{"19"},
  {"20"},{"21"},{"22"},{"23"},{"24"},{"25"},{"26"},{"27"},{"28"},{"29"},
  {"30"},{"31"},{"32"},{"33"},{"34"},{"35"},{"36"},{"37"},{"38"},{"39"},
  {"40"},{"41"},{"42"},{"43"},{"44"},{"45"},{"46"},{"47"},{"48"},{"49"},
  {"50"},{"51"},{"52"},{"53"},{"54"},{"55"},{"56"},{"57"},{"58"},{"59"}
};


void systick(void) //100 Hz
{
  static unsigned long sec=1;
  unsigned int s;

  disk_timerproc();
  keys_timerservice();

  //1 Hz
  if(--sec == 0)
  {
    sec = 100;
    on_time++;
    sec_time++;

    s  = status;
    s |= SEC_CHANGED;
    if(++time.s == 60)
    {
      time.s = 0;
      s |= MIN_CHANGED;
      if(++time.m == 60)
      {
        time.m = 0;
        s |= HOUR_CHANGED;
        if(++time.h == 24)
        {
          time.h = 0;
          s |= DAY_CHANGED;
        }
        clock_str[0] = clock_tab[time.h][0];
        clock_str[1] = clock_tab[time.h][1];
      }
      clock_str[3] = clock_tab[time.m][0];
      clock_str[4] = clock_tab[time.m][1];
    }
    clock_str[6] = clock_tab[time.s][0];
    clock_str[7] = clock_tab[time.s][1];
    status = s;
  }

  return;
}


char* getclock(void)
{
  return clock_str;
}


char* getdate(void)
{
  return date_str;
}


void gettime(TIME* t)
{
  t->year  = time.year;
  t->month = time.month;
  t->day   = time.day;
  t->wday  = time.wday;
  t->h     = time.h;
  t->m     = time.m;
  t->s     = time.s;

  return;
}


void settime(unsigned long s)
{
  TIME t;

  SysTickIntDisable();
  sectotime(s-1, &t);
  sec_time   = s;
  time.year  = t.year;
  time.month = t.month;
  time.day   = t.day;
  time.wday  = t.wday;
  time.h     = t.h;
  time.m     = t.m;
  time.s     = t.s;
  SysTickIntEnable();

  date_str[0] = day_tab[time.wday][0];
  date_str[1] = day_tab[time.wday][1];
  date_str[3] = clock_tab[time.day][0];
  date_str[4] = clock_tab[time.day][1];
  date_str[6] = clock_tab[time.month][0];
  date_str[7] = clock_tab[time.month][1];
  itoa(time.year, &date_str[9], 10);

  clock_str[0] = clock_tab[time.h][0];
  clock_str[1] = clock_tab[time.h][1];
  clock_str[3] = clock_tab[time.m][0];
  clock_str[4] = clock_tab[time.m][1];
  clock_str[6] = clock_tab[time.s][0];
  clock_str[7] = clock_tab[time.s][1];

  DEBUGOUT("Set time: %s %s\n", getclock(), getdate());

  return;
}


long getdeltatime(long t)
{
  long s;

  s = on_time;

  return (s-t);
}


long getontime(void)
{
  long s;

  s = on_time;

  return s;
}


unsigned int standby_isactive(void)
{
  return standby_active;
}


unsigned int standby(unsigned int param)
{
  unsigned int i, alarm;
  unsigned long t;
  char tmp[32];

  DEBUGOUT("Standby\n");

  standby_active = 1;

  vs_stop();
  vs_setvolume(0); //0 -> analog power off
  delay_ms(10);
  USB_OFF();

  //draw clock
  lcd_clear(RGB(0,0,0));
  tmp[0] = clock_str[0];
  tmp[1] = clock_str[1];
  tmp[2] = clock_str[2];
  tmp[3] = clock_str[3];
  tmp[4] = clock_str[4];
  tmp[5] = 0;
  lcd_puts((LCD_WIDTH/2)-((5*(TIMEFONT_WIDTH*2))/2), (LCD_HEIGHT/2)-((TIMEFONT_HEIGHT*2)/2), tmp, TIMEFONT, 2, RGB(255,255,255), RGB(0,0,0));

  //try to get time from ntp
#ifndef DEBUG
  t = ntp_gettime();
  if(t){ settime(t); }
#endif

  cpu_speed(1); //low speed

  for(alarm=0; alarm == 0;)
  {
    eth_service();

    i = status;
    status &= ~i;
    if(i & MIN_CHANGED)
    {
      if(i & DAY_CHANGED)
      {
        settime(sec_time);
      }
      i = alarm_check(&time);
      if(i == 1) //alarm: play
      {
        alarm = i;
      }
      tmp[0] = clock_str[0];
      tmp[1] = clock_str[1];
      tmp[3] = clock_str[3];
      tmp[4] = clock_str[4];
      lcd_puts((LCD_WIDTH/2)-((5*(TIMEFONT_WIDTH*2))/2), (LCD_HEIGHT/2)-((TIMEFONT_HEIGHT*2)/2), tmp, TIMEFONT, 2, RGB(255,255,255), RGB(0,0,0));
    }

    if(keys_sw() || (ir_cmd() == SW_POWER))
    {
      if(alarm == 0)
      {
        pwm_led(80);
        daytime(tmp, &time);
        lcd_puts(10, 10, tmp, NORMALFONT, 1, RGB(255,255,255), RGB(0,0,0));

        chucknorris_rfact();
        pwm_led(100);
      }
      break;
    }
  }

  //re-mount card
  fs_unmount();
  fs_mount();

  USB_ON();

  cpu_speed(0); //high speed

  //clear cmds
  keys_sw();
  keys_steps();
  ir_cmd();

  standby_active = 0;

  if(alarm != 0)
  {
    menu_alarm(alarm);
  }

  return 0;
}


int main()
{
  unsigned int i, alarm;
  unsigned long l;

  //init debug output
  debug_init();

  DEBUGOUT("\n"APPNAME" v"APPVERSION" ("__DATE__" "__TIME__")\n");
  DEBUGOUT("Hardware: "LM3S_NAME", "LCD_NAME"\n");

  //get reset cause
  l = SysCtlResetCauseGet();
  if(l)
  {
    SysCtlResetCauseClear(l);
    DEBUGOUT("Reset: %i\n", l);
  }

  //init brown-out reset
  DEBUGOUT("Init BOR...\n");
  init_bor(1);

  //init pins
  DEBUGOUT("Init Pins...\n");
  init_pins();

  //init systick
  DEBUGOUT("Init SysTick...\n");
  SysTickDisable();
  SysTickPeriodSet(SysCtlClockGet() / 100); //100 Hz
  SysTickIntRegister(systick);
  SysTickIntEnable();
  SysTickEnable();

  //init peripherals
  DEBUGOUT("Init Peripherals...\n");
  init_periph();

  //low speed and enable interrupts
  DEBUGOUT("Low speed and IRQs on...\n");
  cpu_speed(1);

  //init lcd
  DEBUGOUT("Init LCD...\n");
  lcd_init();

  //show start-up screen
  lcd_clear(DEFAULT_BGCOLOR);
  lcd_fillrect( 0, 0, LCD_WIDTH-1, 10, DEFAULT_EDGECOLOR);
  lcd_puts(30, 2, APPNAME" v"APPVERSION, SMALLFONT, 1, DEFAULT_BGCOLOR, DEFAULT_EDGECOLOR);
  lcd_fillrect( 0, LCD_HEIGHT-1-13, LCD_WIDTH-1, LCD_HEIGHT-1, DEFAULT_EDGECOLOR);
  lcd_puts(20, LCD_HEIGHT-1-10, "www.watterott.net", SMALLFONT, 1, DEFAULT_BGCOLOR, DEFAULT_EDGECOLOR);
  lcd_puts(10, 20, "HW:"LM3S_NAME","LCD_NAME, SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR);
  if(l) //l = reset cause
  {
    i = lcd_puts(10,  35, "Reset:", SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR) + 4;
    if(l & SYSCTL_CAUSE_LDO) { i = lcd_puts(i, 35, "LDO", SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR) + 4; }
    if(l & SYSCTL_CAUSE_SW)  { i = lcd_puts(i, 35, "SW",  SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR) + 4; }
    if(l & SYSCTL_CAUSE_WDOG){ i = lcd_puts(i, 35, "WD",  SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR) + 4; }
    if(l & SYSCTL_CAUSE_BOR) { i = lcd_puts(i, 35, "BOR", SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR) + 4; }
    if(l & SYSCTL_CAUSE_POR) { i = lcd_puts(i, 35, "POR", SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR) + 4; }
    if(l & SYSCTL_CAUSE_EXT) { i = lcd_puts(i, 35, "EXT", SMALLFONT, 1, DEFAULT_EDGECOLOR, DEFAULT_BGCOLOR) + 4; }
  }

  i = 52; //msg y start

  //init mmc & mount filesystem
  lcd_puts(10,  i, "Init Memory Card...", SMALLFONT, 1, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR); i += 10;
  fs_mount();

  //init fram
  lcd_puts(10,  i, "Init F-RAM...", SMALLFONT, 1, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR);
  l = fm_init();
  if(l)
  {
    char tmp[8];
    sprintf(tmp, "%ikb", (unsigned int)(unsigned long)(l/1024UL));
    lcd_puts(120,  i, tmp, SMALLFONT, 1, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR);
  }
  i += 10;

  //init ethernet
  lcd_puts(10,  i, "Init Ethernet...", SMALLFONT, 1, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR); i += 10;
  eth_init();

  //load settings
  lcd_puts(10, i, "Load Settings...", SMALLFONT, 1, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR); i += 10;
  settings_read();

  //advertise UPnP device
  lcd_puts(10, i, "SSDP: Advertise...", SMALLFONT, 1, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR); i += 10;
  ssdp_advertise();

  //set clock
  lcd_puts(10, i, "NTP: Get Time...", SMALLFONT, 1, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR); i += 10;
#ifdef DEBUG
  settime(1);
#else
  l = ntp_gettime();
  if(l)
  {
    settime(l);
  }
  else
  {
    settime(timetosec(0, 0, 0, COMPILE_DAY, COMPILE_MONTH, COMPILE_YEAR));
  }
#endif

  //cpu high speed
  cpu_speed(0);

  //init menu
  menu_init();

  //usb power on
  USB_ON();

  //check alarm
  alarm = alarm_check(&time);
  menu_alarm(alarm);

  DEBUGOUT("Ready...\n");

  for(;;)
  {
    eth_service();

    i = status;
    status &= ~i;
#ifdef DEBUG
//    if(i & SEC_CHANGED)
//    {
//      DEBUGOUT("buf: %i / rx %i\n", buf_free(), eth_rxfree());
//    }
#endif
    if(i & MIN_CHANGED)
    {
      if(i & DAY_CHANGED)
      {
        settime(sec_time);
      }
      alarm = alarm_check(&time);
      if(alarm == 2)
      {
        standby(0);
        i |= DRAWALL;
      }
      else if(alarm != 0)
      {
        menu_alarm(alarm);
      }
    }
    menu_service(i);
  }
}