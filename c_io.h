// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//

#ifndef __C_IO_H__
#define __C_IO_H__

#include "doomstat.h"
#include "d_event.h"
        // for text colours:
#include "v_video.h"

#define INPUTLENGTH 512
#define LINELENGTH 96

void C_Init();
void C_Ticker();
void C_Drawer();
int C_Responder(event_t* ev);
void C_Update();

void C_Printf(unsigned char *s, ...);
void C_WriteText(unsigned char *s, ...);
#define C_Puts C_WriteText     /*** basically the same **/
void C_Seperator();

void C_SetConsole();
void C_Popup();
void C_InstaPopup();

        // sf 9/99: made a #define
#define consoleactive (current_height || gamestate==GS_CONSOLE)

extern int c_height;     // the height of the console
extern int c_speed;       // pixels/tic it moves
extern int current_height;
extern int current_target;
#define c_moving (current_height != current_target)
extern boolean c_showprompt;

#endif
