/****************************** heads up **********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Heads up display
//
//      Re-written. Displays the messages, etc
//

/* includes ************************/

#include <stdio.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_runcmd.h"
#include "d_deh.h"
#include "d_event.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_stuff.h"
#include "hu_over.h"
#include "p_info.h"
#include "p_map.h"
#include "p_setup.h"
#include "r_draw.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"

/* defines ************************/

#define HU_TITLE  (*mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2 (*mapnames2[gamemap-1])
#define HU_TITLEP (*mapnamesp[gamemap-1])
#define HU_TITLET (*mapnamest[gamemap-1])

/* prototypes *********************/

void HU_WarningsInit();
void HU_WarningsDrawer();

void HU_WidgetsInit();
void HU_WidgetsTick();
void HU_WidgetsDraw();
void HU_WidgetsErase();

void HU_MessageTick();
void HU_MessageDraw();
void HU_MessageClear();
void HU_MessageErase();

void HU_CentreMessageClear();
boolean HU_ChatRespond(event_t *ev);

/* globals ************************/

// the global widget list

char *chat_macros[10];
const char* shiftxform;
const char english_shiftxform[];
boolean chat_on;
int obituaries = 0;
int death_colour = CR_BRICK;       // the colour of death messages

// main message list
unsigned char *levelname;

/* externs ***********************/
//
// Builtin map names.
// The actual names can be found in DStrings.h.
//
// Ty 03/27/98 - externalized map name arrays - now in d_deh.c
// and converted to arrays of pointers to char *
// See modified HUTITLEx macros
extern char **mapnames[];
extern char **mapnames2[];
extern char **mapnamesp[];
extern char **mapnamest[];

/* Functions **********************/

void HU_Start()
{
        HU_MessageClear();
        HU_CentreMessageClear();
}

void HU_End()
{
}

void HU_Init()
{
        shiftxform = english_shiftxform;

        HU_CrossHairInit();
        HU_FragsInit();
        HU_WarningsInit();
        HU_WidgetsInit();
        HU_LoadFont();
}

void HU_Drawer()
{
        HU_MessageDraw();
        HU_CrossHairDraw();
        HU_FragsDrawer();
        HU_WarningsDrawer();
        HU_WidgetsDraw();
        HU_OverlayDraw();
}

void HU_Ticker()
{
        HU_CrossHairTick();
        HU_WidgetsTick();
        HU_MessageTick();
}

boolean HU_Responder(event_t *ev)
{
        return HU_ChatRespond(ev);
}

void HU_NewLevel()
{
   extern char *gamemapname;

        // determine the level name        
        // there are a number of sources from which it can come from,
        // getting the right one is the tricky bit =)
        
        // if commerical mode, OLO loaded and inside the confines of the
        // new level names added, use the olo level name

  if(gamemode == commercial && olo_loaded
        && (gamemap-1>=olo.levelwarp && gamemap-1<=olo.lastlevel) )
        levelname = olo.levelname[gamemap-1];

        // info level name from level lump (p_info.c) ?
  
  else if(*info_levelname) levelname = info_levelname;

        // not a new level or dehacked level names ?

  else if(!newlevel || deh_loaded)
  {
    if(isMAPxy(gamemapname))
      levelname = gamemission == pack_tnt ? HU_TITLET :
                  gamemission == pack_plut ? HU_TITLEP : HU_TITLE2;
    else if(isExMy(gamemapname))
      levelname = HU_TITLE;
    else
      levelname = gamemapname;
  }
  else        //  otherwise just put "new level"
  {
        static char newlevelstr[50];

        sprintf(newlevelstr, "%s: new level", gamemapname);
        levelname = newlevelstr;
  }

        // print the new level name into the console

  C_Printf("\n");
  C_Seperator();
  C_Printf("%c  %s\n\n", 128+CR_GRAY, levelname);
  C_Update();
}

        // erase text that can be trashed by small screens
void HU_Erase()
{
        if(!viewwindowx || automapactive)
                return;

        HU_MessageErase();
        HU_WidgetsErase();
        HU_FragsErase();
}

/*****************
 NORMAL MESSAGES
 *****************/

// 'picked up a clip' etc.
// seperate from the widgets (below)

char hu_messages[MAXHUDMESSAGES][256];
int hud_msg_lines;   // number of message lines in window up to 16
int current_messages;   // the current number of messages
int hud_msg_timer;   // timer used for review messages
int hud_msg_scrollup;// whether message list scrolls up
int message_timer;   // timer used for normal messages
int scrolltime;         // the gametic when the message list next needs
                        // to scroll up

void HU_playermsg(char *s)
{
        if(current_messages == hud_msg_lines)  // display full
        {
                int i;
                // scroll up
                for(i=0; i<hud_msg_lines-1; i++)
                        strcpy(hu_messages[i], hu_messages[i+1]);
                strcpy(hu_messages[hud_msg_lines-1], s);
        }
        else            // add one to the end
        {
                strcpy(hu_messages[current_messages], s);
                current_messages++;
        }
        scrolltime = gametic + (message_timer * 35) / 1000;
}

        // erase the text before drawing
void HU_MessageErase()
{
        int y;

        for(y=0; y<8*hud_msg_lines; y++)
                R_VideoErase(y*SCREENWIDTH, SCREENWIDTH);
}

void HU_MessageDraw()
{
        int i;

        for(i=0; i<current_messages; i++)
               V_WriteText(hu_messages[i], 0, i*8);
}

void HU_MessageClear()
{
        current_messages = 0;
}

void HU_MessageTick()
{
        int i;

        if(!hud_msg_scrollup) return;   // messages dont scroll

        if(gametic >= scrolltime)
        {
                for(i=0; i<current_messages-1; i++)
                        strcpy(hu_messages[i], hu_messages[i+1]);
                current_messages = current_messages ? current_messages-1 : 0;
                scrolltime = gametic + (message_timer * 35) / 1000;
        }
}


/******************
        CROSSHAIR
******************/

patch_t *crosshairs[CROSSHAIRS];
patch_t *crosshair=NULL;
char *crosshairpal;
char *targetcolour, *notargetcolour, *friendcolour;
int crosshairnum;       // 0= none

void HU_CrossHairDraw()
{
        int drawx, drawy;

        if(!crosshair) return;
        if(viewcamera || automapactive) return;

        // where to draw??

        drawx = SCREENWIDTH/2 - crosshair->width/2;
        drawy = scaledviewheight == SCREENHEIGHT ? SCREENHEIGHT/2 :
                        (SCREENHEIGHT-ST_HEIGHT)/2;

        if(bfglook == 2 && players[displayplayer].readyweapon == wp_bfg)
          drawy += (players[displayplayer].updownangle * scaledviewheight)/100;

        drawy -= crosshair->height/2;

        if((drawy + crosshair->height) > ((viewwindowy + viewheight)>>hires) )
                return;

        if(crosshairpal == notargetcolour)
          V_DrawPatchTL(drawx, drawy, 0, crosshair, crosshairpal);
        else
          V_DrawPatchTranslated(drawx, drawy, 0, crosshair, crosshairpal, 0);
}

void HU_CrossHairInit()
{
        crosshairs[0] = W_CacheLumpName("CROSS1", PU_STATIC);
        crosshairs[1] = W_CacheLumpName("CROSS2", PU_STATIC);

        notargetcolour = cr_red;
        targetcolour = cr_green;
        friendcolour = cr_blue;
        crosshairpal = notargetcolour;
        crosshair = crosshairnum ? crosshairs[crosshairnum-1] : NULL;
}

void HU_CrossHairTick()
{
                // fast as possible: don't bother with this crap if
                // the crosshair isn't going to be displayed anyway
        if(!crosshairnum) return;

        crosshairpal = notargetcolour;
        P_AimLineAttack(players[displayplayer].mo,
                players[displayplayer].mo->angle, 16*64*FRACUNIT, 0);
        if(linetarget)
        {
                crosshairpal = targetcolour;
                if(linetarget->flags & MF_FRIEND)
                        crosshairpal = friendcolour;
        }        
}

        // console command
void HU_CrossHairConsole()
{
        int a;

        a=atoi(c_argv[0]);

        crosshair = a ? crosshairs[a-1] : NULL;
        crosshairnum = a;
}

/*****************
          WARNINGS
 *****************/
// several different things that appear, quake-style, to warn you of
// problems

// Open Socket Warning
//
// Problem with network leads or something like that

// VPO Warning indicator
//
// most ports nowadays have removed the visplane overflow problem.
// however, many developers still make wads for plain vanilla doom.
// this should give them a warning for when they have 'a few
// planes too many'

patch_t *vpo;
patch_t *socket;

void HU_WarningsInit()
{
        vpo = W_CacheLumpName("VPO", PU_STATIC);
        socket = W_CacheLumpName("OPENSOCK", PU_STATIC);
}

extern int num_visplanes;
int show_vpo;

void HU_WarningsDrawer()
{
                // the number of visplanes drawn is less in boom.
                // i lower the threshold to 85
        if(show_vpo && num_visplanes > 85)
                V_DrawPatch(250, 10, 0, vpo);
        if(opensocket)
                V_DrawPatch(20, 20, 0, socket);
//        C_WriteText("%i",num_visplanes);
}

/*******************
          WIDGETS
  ******************/
// the main text widgets. does not include the normal messages
// 'picked up a clip' etc

textwidget_t *widgets[MAXWIDGETS];
int num_widgets = 0;

void HU_AddWidget(textwidget_t *widget)
{
        widgets[num_widgets] = widget;
        num_widgets++;
}

void HU_WidgetsDraw()
{
        int i;

        for(i=0; i<num_widgets; i++)
        {
            if(widgets[i]->message &&
                (!widgets[i]->cleartic || gametic<widgets[i]->cleartic) )
               V_WriteText(widgets[i]->message, widgets[i]->x,
                           widgets[i]->y);
        }
}

void HU_WidgetsTick()
{
        int i;

        for(i=0; i<num_widgets; i++)
        {
            if(widgets[i]->handler)
                widgets[i]->handler();
        }
}

        // erase all the widget text
void HU_WidgetsErase()
{
        int i, y;

        for(i=0; i<num_widgets; i++)
        {
           for(y=widgets[i]->y; y<widgets[i]->y+8; y++)
                R_VideoErase(y*SCREENWIDTH, SCREENWIDTH);
        }
}

/*** THE WIDGETS ***/

void HU_LevelTimeHandler();
void HU_CentreMessageHandler();
void HU_LevelNameHandler();
void HU_ChatHandler();

        //// centre of screen 'quake-style' message ////

textwidget_t hu_centremessage = {0, 0, NULL, HU_CentreMessageHandler};
int centremessage_timer = 1500;         // 1.5 seconds

void HU_CentreMessageHandler()
{
        return;         // do nothing
}

void HU_CentreMessageClear()
{
        hu_centremessage.message = NULL;
}

void HU_centremsg(char *s)
{
        static char centremsg[256];
        strcpy(centremsg, s);

        hu_centremessage.message = centremsg;
        hu_centremessage.x = (SCREENWIDTH-V_StringWidth(s)) / 2;
        hu_centremessage.y = (SCREENHEIGHT - 8 -
                ((scaledviewheight==SCREENHEIGHT) ? 0 : (ST_HEIGHT-8)) ) / 2;
        hu_centremessage.cleartic = gametic + (centremessage_timer*35)/1000;
}

        //// level time elapsed so far (automap) ////

textwidget_t hu_leveltime = {SCREENWIDTH-60, SCREENHEIGHT-ST_HEIGHT-8, NULL,
        HU_LevelTimeHandler};

void HU_LevelTimeHandler()
{
        static char timestr[100];
        int seconds;

        if(!automapactive)
        {
                hu_leveltime.message = NULL;
                return;
        }

        seconds = (gametic - levelstarttic) / 35;
        timestr[0] = 0;

        sprintf(timestr, "%02i:%02i:%02i", seconds/3600, (seconds%3600)/60,
                                seconds%60);

        hu_leveltime.message = timestr;        
}

                //// Level Name in Automap ////

textwidget_t hu_levelname = {0, SCREENHEIGHT-ST_HEIGHT-8, NULL,
        HU_LevelNameHandler};

void HU_LevelNameHandler()
{
        hu_levelname.message = automapactive ? levelname : NULL;
}

                ///// Chat message typein ////////

textwidget_t hu_chat = {0, SCREENHEIGHT-ST_HEIGHT-16, NULL,
        HU_ChatHandler};
char chatinput[100] = "";
boolean chat_active = false;

void HU_ChatHandler()
{
        static char chatstring[256];

        if(!chat_active)
        {
                hu_chat.message = NULL;
                return;
        }

        sprintf(chatstring, FC_GRAY "say: " FC_RED "%s", chatinput);

        hu_chat.message = chatstring;
}

boolean HU_ChatRespond(event_t *ev)
{
        char ch;
        static boolean shiftdown;

        if(ev->data1 == KEYD_RSHIFT) shiftdown = ev->type == ev_keydown;

        if(ev->type != ev_keydown) return false;

        if(!chat_active)
        {
                if(ev->data1 == key_chat && netgame) 
                {       
                        chat_active = true;     // activate chat
                        chatinput[0] = 0;       // empty input string
                        return true;
                }
                return false;
        }

        if(ev->data1 == KEYD_ESCAPE)    // kill chat
        {
                chat_active = false;
                return true;
        }

        if(ev->data1 == KEYD_BACKSPACE && chatinput[0])
        {
                chatinput[strlen(chatinput)-1] = 0;      // remove last char
                return true;
        }

        if(ev->data1 == KEYD_ENTER)
        {
                char tempstr[100];
                sprintf(tempstr, "say \"%s\"", chatinput);
                C_RunTextCmd(tempstr);
                chat_active = false;
                return true;
        }

        ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?

        if(ch>31 && ch<127)
        {
                sprintf(chatinput, "%s%c", chatinput, ch);
                C_InitTab();
                return true;
        }
        return false;
}

// Widgets Init

void HU_WidgetsInit()
{
        HU_AddWidget(&hu_centremessage);
        HU_AddWidget(&hu_levelname);
        HU_AddWidget(&hu_leveltime);
        HU_AddWidget(&hu_chat);
}

/* Tables ********************/

const char* shiftxform;

const char english_shiftxform[] =
{
  0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '<', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  ')', // shift-0
  '!', // shift-1
  '@', // shift-2
  '#', // shift-3
  '$', // shift-4
  '%', // shift-5
  '^', // shift-6
  '&', // shift-7
  '*', // shift-8
  '(', // shift-9
  ':',
  ':', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '[', // shift-[
  '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
  ']', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};
