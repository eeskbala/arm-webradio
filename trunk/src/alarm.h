#ifndef _ALARM_H_
#define _ALARM_H_


//----- DEFINES -----
#define ALARMTIMES                     (8)
#define ALARM_FILE                     "ALARM.INI"
#define ALARM_FILEITEMS                (3)


typedef struct
{
  unsigned int wdays;  //Bit0=Sunday, Bit1=Monday, Bit2=Tueday...
  unsigned int h;
  unsigned int m;
  unsigned int on;
} ALARMTIME;


//----- PROTOTYPES -----
unsigned int                           alarm_check(TIME *time);
unsigned int                           alarm_getfile(char *dst, unsigned int nr);
unsigned int                           alarm_getvol(void);
void                                   alarm_gettime(unsigned int item, char *time);
unsigned int                           alarm_gettimes(void);
void                                   alarm_load(void);

void                                   alarm_closeitem(void);
unsigned int                           alarm_openitem(unsigned int item);
void                                   alarm_getitem(unsigned int item, char *name);
unsigned int                           alarm_items(void);
void                                   alarm_init(void);


#endif //_ALARM_H_
