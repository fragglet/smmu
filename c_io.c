// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Console I/O
//
// Basic routines: outputting text to the console, main console functions:
//                 drawer, responder, ticker, init
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"

#include "d_event.h"
#include "d_main.h"
#include "doomdef.h"
#include "g_game.h"
#include "g_bind.h"
#include "hu_stuff.h"
#include "hu_over.h"
#include "i_system.h"
#include "v_video.h"
#include "doomstat.h"
#include "v_mode.h"
#include "w_wad.h"
#include "s_sound.h"


#define MESSAGES 384
// keep the last 32 typed commands
#define HISTORY 32

#define C_SCREENHEIGHT (SCREENHEIGHT<<hires)
#define C_SCREENWIDTH (SCREENWIDTH<<hires)

extern const char* shiftxform;
void Egg();

// the messages (what you see in the console window)
static unsigned char messages[MESSAGES][LINELENGTH];
static int message_pos=0;      // position in the history (last line in window)
static int message_last=0;     // the last message

// the command history(what you type in)
static unsigned char history[HISTORY][LINELENGTH];
static int history_last=0;
static int history_current=0;

char* inputprompt = FC_GRAY "$" FC_RED;
int c_height=100;     // the height of the console
int c_speed=10;       // pixels/tic it moves
int current_target = 0;
int current_height = 0;
boolean c_showprompt;
static char *backdrop;
static char inputtext[INPUTLENGTH];
static char *input_point;      // left-most point you see of the command line

// for scrolling command line
static int pgup_down=0, pgdn_down=0;
int console_enabled = true;

/////////////////////////////////////////////////////////////////////////
//
// Main Console functions
//
// ticker, responder, drawer, init etc.
//

static void C_InitBackdrop()
{
  patch_t *patch;
  char *lumpname;
  byte *oldscreen;
  

  if(W_CheckNumForName("CONSOLE") >= 0)
    lumpname = "CONSOLE";
  else
    {
      // replace this with the new SMMU graphic soon i hope..
      switch(gamemode)
	{
	  case commercial: case retail: lumpname = "INTERPIC";break;
	  case registered: lumpname = "PFUB2"; break;
	  default: lumpname = "TITLEPIC"; break;
	}
    }
  
  if(backdrop)
    Z_Free(backdrop);
  backdrop = Z_Malloc(C_SCREENHEIGHT*C_SCREENWIDTH, PU_STATIC, 0);

  // hack to write to backdrop
  oldscreen = screens[1]; screens[1] = backdrop; 

  patch = W_CacheLumpName(lumpname, PU_CACHE);
  V_DrawPatch(0, 0, 1, patch);
  
  screens[1] = oldscreen;
}

// draw the backdrop to the screen

void C_DrawBackdrop()
{
  static int oldscreenheight = -1;
  // Check for change in screen res

  if(oldscreenheight != C_SCREENHEIGHT)
    {
      C_InitBackdrop();       // re-init to the new screen size
      oldscreenheight = C_SCREENHEIGHT;
    }

  memcpy(screens[0],
	 backdrop + (C_SCREENHEIGHT-(current_height<<hires))*C_SCREENWIDTH,
	 (current_height<<hires)*C_SCREENWIDTH);
}

// input_point is the leftmost point of the inputtext which
// we see. This function is called every time the inputtext
// changes to decide where input_point should be.

static void C_UpdateInputPoint()
{
  for(input_point=inputtext;
      V_StringWidth(input_point) > SCREENWIDTH-20; input_point++);
}

// initialise the console

void C_Init()
{ 
  // sf: stupid american spellings =)
  C_NewAlias("color", "colour %opt");
  C_NewAlias("centermsg", "centremsg %opt");

  C_AddCommands();
  
  input_point = inputtext;
  //  C_UpdateInputPoint();

  G_InitKeyBindings();
}

// called every tic

void C_Ticker()
{
  c_showprompt = true;
  
  if(gamestate != GS_CONSOLE)
    {
      // specific to half-screen version only
      
      if(current_height != current_target)
	redrawsbar = true;
      
      // move the console toward its target
      if(abs(current_height-current_target)>=c_speed)
	current_height +=
	  current_target<current_height ? -c_speed : c_speed;
      else
	current_height = current_target;
    }
  else
    {
      // console gamestate: no moving consoles!
      current_target = current_height;
    }
  
  if(consoleactive)  // no scrolling thru messages when fullscreen
    {
      // scroll based on keys down
      message_pos += pgdn_down - pgup_down;
      
      // check we're in the area of valid messages        
      if(message_pos < 0) message_pos = 0;
      if(message_pos > message_last) message_pos = message_last;
    }

  if(!default_name)
    {
      C_Printf("1 default_name == NULL!\n");
      default_name = "def";
    }
  
  C_RunBuffer(c_typed);   // run the delayed typed commands
  C_RunBuffer(c_menu);

  if(!default_name)
    {
      C_Printf("default_name == NULL!\n");
      default_name = "abc";
    }
}

static void C_AddToHistory(char *s)
{
  char *t;
  
  // display the command in console
  C_Printf("%s%s\n", inputprompt, s);
  
  t = s;                  // check for nothing typed
  while(*t==' ') t++;     // or just spaces
  if(!*t) return; 
  
  // add it to the history
  // 6/8/99 maximum linelength to prevent segfaults
  // -3 for safety
  strncpy(history[history_last], s, LINELENGTH-3);
  history_last++;

  // scroll the history if neccesary
  while(history_last >= HISTORY)
    {
      int i;
      for(i=0; i<HISTORY; i++)
	strcpy(history[i], history[i+1]);
      history_last--;
    }
  history_current = history_last;
  history[history_last][0] = 0;
}

// respond to keyboard input/events

int C_Responder(event_t* ev)
{
  static int shiftdown;
  char ch;
  
  if(ev->data1 == KEYD_RSHIFT)
    {
      shiftdown = ev->type==ev_keydown;
      return consoleactive;   // eat if console active
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

  // only interested in keypresses
  if(ev->type != ev_keydown)
    return false;
  
  //------------------------------------------------------------------------
  // Check for special keypresses
  
  // detect activating of console etc.
  
  // activate console?
  if(ev->data1 == KEYD_CONSOLE && console_enabled)
    {
      // set console
      current_target = current_target == c_height ? 0 : c_height;
      return true;
    }

  if(!consoleactive) return false;

  // not til its stopped moving
  if(current_target < current_height) return false;

  //------------------------------------------------------------------------
  // Console active commands
  //
  // keypresses only dealt with if console active
  //
  
  // tab-completion
  if(ev->data1 == KEYD_TAB)
    {
      // set inputtext to next or previous in
      // tab-completion list depending on whether
      // shift is being held down
      strcpy(inputtext, shiftdown ? C_NextTab(inputtext) :
	     C_PrevTab(inputtext));
      
      C_UpdateInputPoint(); // reset scrolling
      return true;
    }
  
  // run command

  if(ev->data1 == KEYD_ENTER)
    {
      C_AddToHistory(inputtext);      // add to history
      
      if(!strcmp(inputtext, "r0x0rz delux0rz")) Egg(); //shh!
      
      // run the command
      cmdtype = c_typed;
      C_RunTextCmd(inputtext);
      
      C_InitTab();            // reset tab completion
      
      inputtext[0] = 0;       // clear inputtext now
      C_UpdateInputPoint();   // reset scrolling

      return true;
    }

  //------------------------------------------------------------------------
  //
  // Command history
  //  

  // previous command
  
  if(ev->data1 == KEYD_UPARROW)
    {
      history_current =
	history_current <= 0 ? 0 : history_current-1;
      
      // read history from inputtext
      strcpy(inputtext, history[history_current]);
      
      C_InitTab();            // reset tab completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
    }
  
  // next command
  
  if(ev->data1 == KEYD_DOWNARROW)
    {
      history_current = history_current >= history_last ?
	history_last : history_current+1;

      // the last history is an empty string
      strcpy(inputtext, (history_current == history_last) ?
	     "" : (char*)history[history_current]);
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
    }

  if(ev->data1 == KEYD_END)
    {
      message_pos = message_last;
      return true;
    }
  
  //------------------------------------------------------------------------
  // Normal Text Input
  
  // backspace
  
  if(ev->data1 == KEYD_BACKSPACE)
    {
      if(strlen(inputtext) > 0)
	inputtext[strlen(inputtext)-1] = '\0';
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
    }

  // none of these, probably just a normal character

  ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?

  // only care about valid characters
  // dont allow too many characters on one command line
  
  if(isprint(ch) && strlen(inputtext) < INPUTLENGTH-3)
    {
      sprintf(inputtext, "%s%c", inputtext, ch);
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
    }
  
  return false;   // dont care about this event
}


// draw the console

void C_Drawer()
{
  int y;
  int count;
  
  if(!consoleactive) return;   // dont draw if not active

  // fullscreen console for fullscreen mode
  if(gamestate == GS_CONSOLE)
    current_height = SCREENHEIGHT;

  // draw backdrop

  C_DrawBackdrop();
  
  //------------------------------------------------------------------------
  // draw text messages
  
  // offset starting point up by 8 if we are showing input prompt
  
  y = current_height - ((c_showprompt && message_pos==message_last) ? 8 : 0);

  // start at our position in the message history
  count = message_pos;
        
  while(1)
    {
      // move up one line on the screen
      // back one line in the history
      y -= 8;
      
      if(--count < 0) break;    // end of message history?
      if(y < 0) break;        // past top of screen?
      
      // draw this line
      V_WriteText(messages[count], 0, y);
    }

  //------------------------------------------------------------------------
  // Draw input line
  
  // input line on screen, not scrolled back in history?
  
  if(current_height > 8 && c_showprompt && message_pos == message_last)
    {
      unsigned char tempstr[LINELENGTH];
      
      // if we are scrolled back, dont draw the input line
      if(message_pos == message_last)
	sprintf(tempstr, "%s%s_", inputprompt, input_point);
      
      V_WriteText(tempstr, 0, current_height-8);
    }
}

// updates the screen without actually waiting for d_display
// useful for functions that get input without using the gameloop
// eg. serial code

void C_Update()
{
  C_Drawer();
  V_FinishUpdate ();
}

//-------------------------------------------------------------------------
//
// I/O Functions
//

// scroll console up

static void C_ScrollUp()
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

// add a character to the console

static void C_AddChar(unsigned char c)
{
  char *end;

  // in text mode, just write to text display
  
  if(!in_graphics_mode)
    {
      if(isprint(c) || c== '\n' || c=='\t')
	putchar(c);
      return;
    }
  
  if( c=='\t' || isprint(c) || c>=128)  // >=128 for colours
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
  if(c == '\b') // backspace
    {
      if(strlen(messages[message_last]))
	messages[message_last][strlen(messages[message_last]) - 1] = '\0';
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

// write some text 'printf' style to the console
// the main function for I/O

void C_Printf(unsigned char *s, ...)
{
  va_list args;
  unsigned char tempstr[10240];   // 10k should be enough i hope
  
  // difficult to remove limit
  va_start(args, s);
  vsprintf(tempstr, s, args);
  va_end(args);

  for(s = tempstr; *s; s++)
    C_AddChar(*s);
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

void C_Seperator()
{
  C_Printf("{|||||||||||||||||||||||||||||}\n");
}

///////////////////////////////////////////////////////////////////
//
// Console activation
//

// put smmu into console mode

void C_SetConsole()
{
  gamestate = GS_CONSOLE;         
  gameaction = ga_nothing;
  current_height = SCREENHEIGHT;
  current_target = SCREENHEIGHT;
  
  //  C_Update();
  S_StopMusic();                  // stop music if any
  S_StopSounds();                 // and sounds
  G_StopDemo();                   // stop demo playing
}

// make the console go up

void C_Popup()
{
  current_target = 0;
}

// make the console disappear

void C_InstaPopup()
{
  current_target = current_height = 0;
}

#define E extern
#define U unsigned
#define C char
#define I int
#define V void
#define F for
#define Z FC_BROWN
#define C_W C_SCREENWIDTH
#define C_H C_SCREENHEIGHT
#define s0 screens[0]
#define WT HU_WriteText
#define bd backdrop

V Egg(V){C *os;I x,y;E U C egg[];F(x=0;x<C_W;x++)F(y=0;y<C_H;y
++){U C *s=egg+((y%44)*42)+(x%42);if(*s!=247)bd[y*C_W+x]=*s;}
os=s0;s0=bd;WT(Z"my hair looks much too\n dark in this pic.\n"
"oh well, have fun!\n      -- fraggle",160,168);s0=os;}
