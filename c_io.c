/******************************* console **********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Console I/O
//
// Basic routines: outputting text to the console, main console functions:
//                 drawer, responder, ticker, init
//

/* includes ************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "c_io.h"
#include "c_handle.h"
#include "c_runcmd.h"
#include "c_cmdlst.h"
#include "c_net.h"

#include "d_event.h"
#include "d_main.h"
#include "doomdef.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_over.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "doomstat.h"
#include "w_wad.h"
#include "s_sound.h"

/* defines *************************/
#define MESSAGES 384
        // keep the last 32 typed commands
#define HISTORY 32

#define C_SCREENHEIGHT (SCREENHEIGHT<<hires)
#define C_SCREENWIDTH (SCREENWIDTH<<hires)

/* externs *************************/

extern const char* shiftxform;

/* prototypes **********************/

void C_ScrollUp();
void C_EasterEgg();     // shhh!

/* local variables *****************/
        // the messages (what you see in the console window)
unsigned char messages[MESSAGES][LINELENGTH];
int message_pos=0;      // position in the history (last line in window)
int message_last=0;     // the last message
        // the messages history(what you type in)
unsigned char history[HISTORY][LINELENGTH];
int history_last=0;
int history_current=0;

char* inputprompt = FC_GRAY "$" FC_RED;
int c_height=100;     // the height of the console
int c_speed=10;       // pixels/tic it moves
int current_target=0;
int current_height=0;
int consoleactive=0;
boolean c_showprompt;
char *backdrop;
char inputtext[INPUTLENGTH];
char *input_point;      // left-most point you see of the command line
                        // for scrolling command line
int pgup_down=0, pgdn_down=0;

/* functions ***********************/

void C_InitBackdrop()
{
        patch_t *patch;
        char *lumpname;
        byte *oldscreen;

        switch(gamemode)
        {
                case commercial: case retail: lumpname = "INTERPIC";break;
                case registered: lumpname = "PFUB2"; break;
                default: lumpname = "TITLEPIC"; break;
        }

        if(backdrop) Z_Free(backdrop);
        backdrop = Z_Malloc(C_SCREENHEIGHT*C_SCREENWIDTH, PU_STATIC, 0);

        oldscreen = screens[1]; screens[1] = backdrop;  // hack to write to
                                                        // backdrop
        patch = W_CacheLumpName(lumpname, PU_CACHE);
        V_DrawPatch(0, 0, 1, patch);

        screens[1] = oldscreen;
}

        // input_point is the leftmost point of the inputtext which
        // we see. This function is called every time the inputtext
        // changes to decide where input_point should be.
void C_UpdateInputPoint()
{
        for(input_point=inputtext;
                V_StringWidth(input_point) > SCREENWIDTH-20; input_point++);
}

        // initialise the console
void C_Init()
{
        C_InitBackdrop();

        // sf: stupid american spellings =)
        C_NewAlias("color", "colour %opt");
        C_NewAlias("centermsg", "centremsg %opt");
        C_NewAlias("colourtest",
                "echo "
                FC_BRICK "M" FC_TAN "U" FC_GRAY "L" FC_GREEN "T" FC_BROWN "I"
                FC_GOLD "C" FC_RED "O" FC_BLUE "L" FC_ORANGE "O" FC_YELLOW "U"
                FC_BRICK "R" FC_TAN "E" FC_GRAY "D" FC_GREEN "!");

        C_AddCommands();
        C_UpdateInputPoint();
}

// put smmu into console mode

void C_SetConsole()
{
        gamestate = GS_CONSOLE;
        gameaction = ga_nothing;
        consoleactive = 1;
        current_height = SCREENHEIGHT;
        current_target = SCREENHEIGHT;

        C_Update();
        C_ClearBuffer(c_script);
        S_StopMusic();  // stop music if any
        S_StopSounds(); // and sounds
        G_StopDemo();
}

        // called every tic
void C_Ticker()
{
        c_showprompt = true;

        if(gamestate != GS_CONSOLE)
        {
            if(current_height != current_target)
                redrawsbar = true;

            if(abs(current_height-current_target)>=c_speed)
               current_height +=
               current_target<current_height ? -c_speed : c_speed;
            else
               current_height = current_target;
        }
        else   current_target = current_height;

        if(!current_height) consoleactive = 0;

        if(consoleactive)       // no moving console when fullscreen
        {
            if(pgdn_down) message_pos++;
            if(pgup_down) message_pos--;
        }

        if(message_pos < 0) message_pos = 0;
        if(message_pos > message_last) message_pos = message_last;

        C_RunBuffer(c_typed);   // run the typed commands
}

void C_AddToHistory(char *s)
{
        char *t;

                     // display the command in console
        C_Printf("%s%s\n", inputprompt, s);

        t = s;                  // check for nothing typed
        while(*t==' ') t++;     // or just spaces
        if(!*t) return; 

                // add it to the history
                // 6/8/99 maximum linelength to prevent segfaults
        strncpy(history[history_last], s, LINELENGTH-3);
        history_last++;

                // scroll the history if neccesary
        while(history_last > HISTORY)
        {
                int i;
                for(i=0; i<HISTORY; i++)
                        strcpy(history[i], history[i+1]);
                history_last--;
        }
        history_current = history_last;
        *history[history_last] = 0;
}

#define KEYD_CONSOLE '`'

        // respond to keyboard input/events
int C_Responder(event_t* ev)
{
        static int shiftdown;
        char ch;

        if(ev->data1 == KEYD_RSHIFT)
        {
                shiftdown = ev->type==ev_keydown;
                return consoleactive;
        }
        if(ev->data1 == KEYD_PAGEUP)
        {
                pgup_down = ev->type==ev_keydown;
                return consoleactive;
        }
        if(ev->data1 == KEYD_PAGEDOWN)
        {
                pgdn_down = ev->type==ev_keydown;
                return consoleactive;
        }

        if(ev->type != ev_keydown) return 0;

        if(ev->data1 == KEYD_CONSOLE)
        {
                current_target =
                        current_target == c_height ? 0 : c_height;
                consoleactive = true;
                return true;
        }

        if(!consoleactive) return false;
        if(current_target < current_height) return false; // not til its stopped moving

        if(ev->data1 == KEYD_TAB)
        {
                strcpy(inputtext, shiftdown ? C_NextTab(inputtext) :
                                C_PrevTab(inputtext));
                C_UpdateInputPoint(); // update scrolling
                return true;
        }
        if(ev->data1 == KEYD_ENTER)
        {
                char *tempcmdstr = strdup(inputtext);

                C_AddToHistory(inputtext);      // add to history

                inputtext[0] = 0;       // clear inputtext
                cmdtype = c_typed;
                if(!strcmp(tempcmdstr, "r0x0rz delux0rz")) C_EasterEgg();
                C_RunTextCmd(tempcmdstr);
                C_InitTab();
                C_UpdateInputPoint();   // reset scrolling

                free(tempcmdstr);
                return true;
        }

        if(ev->data1 == KEYD_UPARROW)
        {
                history_current--;
                if(history_current < 0)
                    history_current = 0;
                strcpy(inputtext, history[history_current]);
                C_InitTab();
                C_UpdateInputPoint();   // update scrolling
                return true;
        }
        if(ev->data1 == KEYD_DOWNARROW)
        {
                history_current++;
                if(history_current > history_last)
                    history_current = history_last;

                        // the last history is an empty string
                strcpy(inputtext, (history_current == history_last) ?
                               "" : (char*)(history[history_current]) );
                C_InitTab();
                C_UpdateInputPoint();   // update scrolling
                return true;
        }
        if(ev->data1 == KEYD_BACKSPACE)
        {
                if(strlen(inputtext) > 0)
                        inputtext[strlen(inputtext)-1] = '\0';
                C_InitTab();
                C_UpdateInputPoint();   // update scrolling
                return true;
        }

                // none of these, probably just a normal character

        ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?

        if(ch>31 && ch<127 && strlen(inputtext) < INPUTLENGTH-3)
        {
                sprintf(inputtext, "%s%c", inputtext, ch);
                C_InitTab();
                C_UpdateInputPoint();   // update scrolling
                return true;
        }
        return false;
}


       // draw the console
void C_Drawer()
{
        int y;
        int count;
        static int oldscreenheight;
        unsigned char tempstr[LINELENGTH];

        if(!consoleactive) return;

        if(oldscreenheight != C_SCREENHEIGHT)
        {
                C_InitBackdrop();       // re-init to the new screen size
                oldscreenheight = C_SCREENHEIGHT;
        }

                /////// BACKDROP FIRST ////////
        memcpy(screens[0],
          backdrop + (C_SCREENHEIGHT-(current_height<<hires))*C_SCREENWIDTH,
                (current_height<<hires)*C_SCREENWIDTH);

        if(gamestate == GS_CONSOLE) current_height = SCREENHEIGHT;

                //////////// TEXT /////////////
        y = c_showprompt ? current_height-8 : current_height;
        count = message_pos;
        
        while(1)
        {
                y-=8; count--;
                if(count < 0) break;
                if(y < 0) break;
                V_WriteText(messages[count], 0, y);
        }

                /////// NOW THE INPUT LINE ///////

        if(current_height > 8 && c_showprompt)    // off the screen ?
        {
                if(message_pos == message_last)
                        sprintf(tempstr, "%s%s_", inputprompt, input_point);
                else
                        sprintf(tempstr, "V V V V V V V V V V V V V V V V");
                V_WriteText(tempstr, 0, current_height-8);
        }
}

        // write a line of text to the console
        // kind of redundant now, #defined as c_puts also
void C_WriteText(unsigned char *s, ...)
{
        va_list args;
        unsigned char tempstr[500];
        va_start(args, s);

        vsprintf(tempstr, s, args);
        va_end(args);

        C_Printf("%s\n", tempstr);
}

static void C_AddChar(unsigned char c)
{
        char *end;

        if( (c>31 && c<127) || c>=128)  // >=128 for colours
        {
             if(V_StringWidth(messages[message_last]) > SCREENWIDTH-9)
             {
                // might possibly over-run, go onto next line
                 C_ScrollUp();
             }

             end = messages[message_last] + strlen(messages[message_last]);
             *end = c; end++;
             *end = 0;
        }
        if(c == '\a') // alert
        {
                S_StartSound(NULL, sfx_tink);   // 'tink'!
        }
        if(c == '\n')
        {
                C_ScrollUp();
        }
}

void C_Printf(unsigned char *s, ...)
{
        va_list args;
        unsigned char tempstr[1024];

        va_start(args, s);
        vsprintf(tempstr, s, args);
        va_end(args);

        for(s = tempstr; *s; s++)
                C_AddChar(*s);
}

        // scroll it up
void C_ScrollUp()
{
        if(message_last == message_pos) message_pos++;
        message_last++;

        if(message_last >= MESSAGES)       // past the end of the string
        {
                int i;      // cut off the oldest 128 messages
                for(i=0; i<MESSAGES-128; i++)
                        strcpy(messages[i], messages[i+128]);
                message_last-=128;      // move the message boundary
                message_pos-=128;       // move the view
        }

        messages[message_last] [0] = 0;  // new line is empty
}

        // make the console go up
void C_Popup()
{
        current_target = 0;
}

        // make the console instantly go away
void C_InstaPopup()
{
        current_target = 0;
        current_height = 0;
        consoleactive = false;
}

// updates the screen without actually waiting for d_display

void C_Update()
{
        C_Drawer();
        I_FinishUpdate ();
}

void C_DrawBackdrop()
{
        memcpy(screens[0], backdrop, C_SCREENWIDTH * C_SCREENHEIGHT);
}

void C_Seperator()
{
        C_Printf("{|||||||||||||||||||||||||||||}\n");
}

void C_EasterEgg(){char *oldscreen;int x,y;extern unsigned char egg[];for(x
=0;x<C_SCREENWIDTH;x++)for(y=0;y<C_SCREENHEIGHT;y++){unsigned char *src=egg
+((y%44)*42)+(x%42);if(*src!=247)backdrop[y*C_SCREENWIDTH+x]=*src;}oldscreen
=screens[0];screens[0]=backdrop;HU_WriteText(FC_BROWN"my hair looks much too"
"\n dark in this pic.\noh well, have fun!\n      -- fraggle",160,168);screens
[0]=oldscreen;}


