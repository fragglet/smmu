#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"
#include "v_video.h"

#define MAXHUDMESSAGES 16

#define MAXWIDGETS 16

typedef struct textwidget_s textwidget_t;

struct textwidget_s
{
        int x, y;       // co-ords on screen
        char *message;
        void (*handler)();      // controller function
        int cleartic;   // gametic in which to clear the widget (0=never)
};

extern int show_vpo;
extern int obituaries;
extern int death_colour;

void HU_Init();
void HU_Drawer();
void HU_Ticker();
boolean HU_Responder(event_t *ev);
void HU_NewLevel();

void HU_Start();
void HU_End();

void HU_WriteText(unsigned char *s, int x, int y);
void HU_playermsg(char *s);
void HU_centremsg();
void HU_Erase();

#define CROSSHAIRS 3
extern int crosshairnum;       // 0= none
void HU_CrossHairDraw();
void HU_CrossHairInit();
void HU_CrossHairTick();
void HU_CrossHairConsole();

#endif
