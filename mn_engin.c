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
// New menu
//
// The menu engine. All the menus themselves are in mn_menus.c
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "g_bind.h"
#include "g_game.h"
#include "hu_over.h"
#include "mn_engin.h"
#include "mn_menus.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "w_wad.h"
#include "v_video.h"

// useful macro: finds if a particular item can be selected on the menu
#define selectable(it)   (                  \
            (it)->type == it_runcmd   ||    \
	    (it)->type == it_variable ||    \
	    (it)->type == it_toggle   ||    \
	    (it)->type == it_slider   ||    \
	    (it)->type == it_automap  ||    \
	    (it)->type == it_binding        \
					    )

#define selected_item(menu) ( &(menu)->menuitems[(menu)->selected] )
  
boolean inhelpscreens; // indicates we are in or just left a help screen

// menu error message
char menu_error_message[128];
//int menu_error_time = 0;

// input for typing in new value
static command_t *input_command = NULL;       // NULL if not typing in
static char input_buffer[128] = "";

/////////////////////////////////////////////////////////////////////////////
// 
// MENU DRAWING
//
/////////////////////////////////////////////////////////////////////////////

        // gap from variable description to value
#define GAP 20
#define SKULL_HEIGHT 19
#define BLINK_TIME 8

// colours
#define unselect_colour    CR_RED
#define select_colour      CR_GRAY
#define var_colour         CR_GREEN
#define disabled_colour    CR_TRANS

enum
{
  slider_left,
  slider_right,
  slider_mid,
  slider_slider,
  num_slider_gfx
};
static patch_t *slider_gfx[num_slider_gfx];
static menu_t *drawing_menu;
static patch_t *skulls[2];

// width of slider, in mid-patches
#define SLIDE_PATCHES 9

// draw a 'slider' (for sound volume, etc)

static void MN_DrawSlider(int x, int y, int pct)
{
  int i;
  int draw_x = x;
  int slider_width = 0;       // find slider width in pixels
  
  V_DrawPatch(draw_x, y, 0, slider_gfx[slider_left]);
  draw_x += slider_gfx[slider_left]->width;
  
  for(i=0; i<SLIDE_PATCHES; i++)
    {
      V_DrawPatch(draw_x, y, 0, slider_gfx[slider_mid]);
      draw_x += slider_gfx[slider_mid]->width - 1;
    }
  
  V_DrawPatch(draw_x, y, 0, slider_gfx[slider_right]);
  
  // find position to draw
  
  slider_width = (slider_gfx[slider_mid]->width - 1) * SLIDE_PATCHES;
  draw_x = slider_gfx[slider_left]->width +
    (pct * (slider_width - slider_gfx[slider_slider]->width)) / 100;
  
  V_DrawPatch(x + draw_x, y, 0, slider_gfx[slider_slider]);
}

// draw a menu item. returns the height in pixels

static int MN_DrawMenuItem(menuitem_t *item, int x, int y, int colour)
{
  boolean leftaligned =
    (drawing_menu->flags & mf_skullmenu) ||
    (drawing_menu->flags & mf_leftaligned);

  if(item->type == it_gap)
    return 8;    // skip drawing if a gap
  
  item->x = x; item->y = y;       // save x,y to item
 
  // draw an alternate patch instead of text?
 
  if(item->patch)
    {
      patch_t *patch;
      int lumpnum;
      
      lumpnum = W_CheckNumForName(item->patch);
      
      // default to text-based message if patch missing
      if(lumpnum >= 0)
	{
	  int height;
	  
	  patch = W_CacheLumpNum(lumpnum, PU_CACHE);
	  height = patch->height;
	  
	  // check for left-aligned
	  if(!leftaligned) x-= patch->width;
	  
	  // adjust x if a centered title
	  if(item->type == it_title)
	    x = (SCREENWIDTH-patch->width)/2;
	  
	  V_DrawPatchTranslated(x, y, 0, patch, colrngs[colour], 0);
	  
	  return height + 1;   // 1 pixel gap
	}
    }

  // draw description text
  
  if(item->type == it_title)
    {
      // if it_title, we draw the description centered

      MN_WriteTextColoured
	(
         item->description,
         colour,
         (SCREENWIDTH-MN_StringWidth(item->description))/2,
	 y
	 );
    }
  else
    {
      
      // write description
      MN_WriteTextColoured
	(
         item->description,
         colour,
         x - (leftaligned ? 0 : MN_StringWidth(item->description)),
	 y
	 );
    }

  // draw other data: variable data etc.
  
  switch(item->type)      
    {
      case it_title:              // just description drawn
      case it_info:
      case it_runcmd:
      case it_gap:                // just a gap, draw nothing
      case it_disabled: 
	{
	  break;
	}

      case it_binding:            // key binding
	{
	  char *boundkeys = G_BoundKeys(item->data);

	  if(drawing_menu->flags & mf_background)
	    {
	      // include gap on fullscreen menus
	      x += GAP;
	      // adjust colour for different coloured variables
	      if(colour == unselect_colour) colour = var_colour;
	    }
	  
	  // write variable value text
	  MN_WriteTextColoured
	    (
	     boundkeys,
	     colour,
	     x + (leftaligned ? MN_StringWidth(item->description): 0),
	     y
	     );

	  break;
	}
      
      // it_toggle, it_variable and it_constant are drawn the same
      // the only difference is the way they are changed
      
      case it_constant:
      case it_toggle:
      case it_variable:
	{
	  char varvalue[128];             // temp buffer 
	  
	  // create variable description:
	  // Use console variable descriptions.
	  
	  // display input buffer if inputting new var value
	  if(input_command && item->var == input_command->variable)
	    sprintf(varvalue, "%s_", input_buffer);
	  else
	    strcpy(varvalue, C_VariableStringValue(item->var));
	  
	  if(drawing_menu->flags & mf_background)
	    {
	      // include gap on fullscreen menus
	      x += GAP;
	      // adjust colour for different coloured variables
	      if(colour == unselect_colour) colour = var_colour;
	    }
	  
	  // write variable value text
	  MN_WriteTextColoured
	    (
	     varvalue,
	     colour,
	     x + (leftaligned ? MN_StringWidth(item->description): 0),
	     y
	     );

	  break;
	}
      
      // slider
      
      case it_slider:
	{ 
	  // draw slider
	  // only int variables
	  
	  if(item->var && item->var->type == vt_int)
	    {
	      int range = item->var->max - item->var->min;
	      int posn = *(int *)item->var->variable - item->var->min;
	      
	      MN_DrawSlider(x + GAP, y, (posn*100) / range);
	    }
	  
	  break;
	}
      
      // automap colour block
      
      case it_automap:
	{
	  int bx, by;
	  int colour;
	  char block[BLOCK_SIZE*BLOCK_SIZE];

	  // only with variables, only int variables
	  if(!item->var || item->var->type != vt_int)
	    break;
	  
	  // find colour of this variable from console variable
	  colour = *(int *)item->var->variable;
	  
	  // create block
	  // border
	  memset(block, 0, BLOCK_SIZE*BLOCK_SIZE);
	  
	  if(colour)
	    {	
	      // middle
	      for(bx=1; bx<BLOCK_SIZE-1; bx++)
		for(by=1; by<BLOCK_SIZE-1; by++)
		  block[by*BLOCK_SIZE+bx] = colour;
	    }
	  // draw it
	  
	  V_DrawBlock(x+GAP, y-1, 0, BLOCK_SIZE, BLOCK_SIZE, block);
	  
	  if(!colour)
	    {
	      // draw patch w/cross
	      V_DrawPatch(x+GAP+1, y, 0, W_CacheLumpName("M_PALNO", PU_CACHE));
	    }
	}    
      
      default:
	{
	  break;
	}
    }
  
  return 8;   // text height
}

///////////////////////////////////////////////////////////////////////
//
// MAIN FUNCTION
//
// Draw a menu
//

void MN_DrawMenu(menu_t *menu)
{
  int y;
  int i;

  drawing_menu = menu;    // needed by MN_DrawMenuItem
  y = menu->y;
  
  // draw background

  if(menu->flags & mf_background)
    V_DrawBackground(background_flat, screens[0]);
  
  // menu-specific drawer function

  if(menu->drawer) menu->drawer();

  // draw items in menu

  for(i = 0; menu->menuitems[i].type != it_end; i++)
    {
      int item_height;
      int item_colour;

      // choose item colour:
      // if item is the selected item on the menu, use select_colour
      // if item is disabled use disabled_colour
      // otherwise use unselect_colour

      if(menu->selected == i && !(menu->flags & mf_skullmenu))
	item_colour = select_colour;
      else if(menu->menuitems[i].type == it_disabled)
	item_colour = disabled_colour;
      else
	item_colour = unselect_colour;
      
      // draw item

      item_height =
	MN_DrawMenuItem
	(
	 &menu->menuitems[i],
	 menu->x,
	 y,
	 item_colour
	 );
      
      // if selected item, draw skull next to it

      if(menu->flags & mf_skullmenu && menu->selected == i)
        V_DrawPatch
	  (
	   menu->x - 30,                                // 30 left
	   y + (item_height - SKULL_HEIGHT) / 2,        // midpoint
	   0,
	   skulls[(menutime / BLINK_TIME) % 2]
	   );

      // move down by item height
      y += item_height; 
    }

  /////////////////////////////////////////////////////////
  //
  // Item help
  //

  // no help msg in skull menu

  if(menu->flags & mf_skullmenu)
    return; 

  // choose help message to print
  
  if(menu_error_message[0])             // error message takes priority
    {
      // make it flash
      //      if((menu_error_time / 8) % 2)
      MN_WriteTextColoured
	(
	 menu_error_message,
	 CR_TAN,
	 10,
	 SCREENHEIGHT - V_StringHeight(menu_error_message)
	 );
    }
  else
    {
      char *helpmsg = "";

      // write some help about the item
      menuitem_t *menuitem = selected_item(menu);
      
      if(menuitem->type == it_variable)       // variable
	helpmsg = "press enter to change";
      
      if(menuitem->type == it_toggle)         // togglable variable
	{
	  // enter to change boolean variables
	  // left/right otherwise

	  if(menuitem->var->type == vt_int &&
	     menuitem->var->max - menuitem->var->min == 1)
	    helpmsg = "press enter to change";
	  else
	    helpmsg = "use left/right to change value";
	}

      MN_WriteTextColoured
	(
	 helpmsg,
	 CR_GOLD,
	 10,
	 SCREENHEIGHT - V_StringHeight(helpmsg)
	 );
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Menu Module Functions
//
// Drawer, ticker etc.
//
/////////////////////////////////////////////////////////////////////////

#define MENU_HISTORY 128

boolean menuactive = false;             // menu active?
menu_t *current_menu;   // the current menu_t being displayed
static menu_t *menu_history[MENU_HISTORY];   // previously selected menus
static int menu_history_num;                 // location in history
int hide_menu = 0;      // hide the menu for a duration of time
int menutime = 0;

// menu keys
int key_menu_right     = KEYD_RIGHTARROW;
int key_menu_left      = KEYD_LEFTARROW;
int key_menu_up        = KEYD_UPARROW;
int key_menu_down      = KEYD_DOWNARROW;
int key_menu_backspace = KEYD_BACKSPACE;
int key_menu_escape    = KEYD_ESCAPE;
int key_menu_enter     = KEYD_ENTER;

// menu widget for alternate drawer + responder
menuwidget_t *current_menuwidget = NULL; 

static void MN_LoadGraphics()
{
  skulls[0] = W_CacheLumpName("M_SKULL1", PU_STATIC);
  skulls[1] = W_CacheLumpName("M_SKULL2", PU_STATIC);
  
  // load slider gfx
  
  slider_gfx[slider_left]   = W_CacheLumpName("M_SLIDEL", PU_STATIC);
  slider_gfx[slider_right]  = W_CacheLumpName("M_SLIDER", PU_STATIC);
  slider_gfx[slider_mid]    = W_CacheLumpName("M_SLIDEM", PU_STATIC);
  slider_gfx[slider_slider] = W_CacheLumpName("M_SLIDEO", PU_STATIC);
}

static void MN_UnloadGraphics()
{
  Z_Free(skulls[0]);
  Z_Free(skulls[1]);

  Z_Free(slider_gfx[slider_left]);
  Z_Free(slider_gfx[slider_right]);
  Z_Free(slider_gfx[slider_mid]);
  Z_Free(slider_gfx[slider_slider]);
}

void MN_ReloadGraphics()
{
  MN_UnloadGraphics();
  MN_LoadGraphics();
}

// init menu

void MN_Init()
{
  MN_LoadGraphics(); // load skull etc.
  MN_InitMenus();   // create menu commands in mn_menus.c
}

//////////////////////////////////
// ticker

void MN_Ticker()
{
//  if(menu_error_time)
  //    menu_error_time--;
  if(hide_menu)                   // count down hide_menu
    hide_menu--;
  menutime++;
}

////////////////////////////////
// drawer

void MN_Drawer()
{ 
  // redraw needed if menu hidden
  if(hide_menu) redrawsbar = redrawborder = true;

  // activate menu if displaying widget
  if(current_menuwidget) menuactive = true; 

  if(!menuactive || hide_menu) return;

  if(current_menuwidget)
    {
      // alternate drawer
      if(current_menuwidget->drawer)
	current_menuwidget->drawer();
      return;
    }
 
  MN_DrawMenu(current_menu);  
}

// whether a menu item is a 'gap' item
// ie. one that cannot be selected

extern menu_t menu_sound;

/////////////////////////////////
// Responder

extern char *shiftxform; // hu_stuff.c

boolean MN_Responder (event_t *ev)
{
  char tempstr[128];
  unsigned char ch;
  static boolean shiftdown;

  // are we displaying a widget?
  
  if(current_menuwidget)
    return
      current_menuwidget->responder ?
      current_menuwidget->responder(ev) : false;

  // check for shift key
  
  if((ev->type == ev_keydown || ev->type == ev_keyup)
     && ev->data1 == KEYD_RSHIFT)
    {
      shiftdown = ev->type == ev_keydown;
      return false;
    }

  // we only care about key presses

  if(ev->type != ev_keydown)
    return false;

  ch = shiftdown ? shiftxform[ev->data1] : ev->data1;
  
  // are we inputting a new value into a variable?
  
  if(input_command)
    {
      variable_t *var = input_command->variable;
      
      if(ev->data1 == KEYD_ESCAPE)        // cancel input
	input_command = NULL;
      
      if(ev->data1 == KEYD_ENTER)
	{
	  if(input_buffer[0])
	    {
	      char *temp;
	      
	      // place " marks round the new value
	      temp = strdup(input_buffer);
	      sprintf(input_buffer, "\"%s\"", temp);
	      free(temp);

	      // set the command
	      cmdtype = c_menu;
	      C_RunCommand(input_command, input_buffer);
	      input_command = NULL;
	    }

	  return true; // eat key
	}

      // check for backspace
      if(ev->data1 == KEYD_BACKSPACE && input_buffer[0])
	{
	  input_buffer[strlen(input_buffer)-1] = '\0';
	  return true; // eatkey
	}
      // probably just a normal character

      // only care about valid characters
      // dont allow too many characters on one command line
      
      if(isprint(ch) && ch <= 0x80 &&
	 strlen(input_buffer) <=
	 (var->type == vt_string ? var->max :
	  var->type == vt_int ? 10 : 20))
	 {
	   input_buffer[strlen(input_buffer) + 1] = 0;
	   input_buffer[strlen(input_buffer)] = ch;
	 }
	   
      return true;
    } 

  if(ev->data1 == key_escape)
    {
      // toggle menu
      
      // start up main menu or kill menu
      if(menuactive) MN_ClearMenus();
      else MN_StartControlPanel();
      
      S_StartSound(NULL, menuactive ? sfx_swtchn : sfx_swtchx);

      return true;
    }

  // not interested in keys if not in menu
  if(!menuactive)
    return false;

  if(ev->data1 == key_menu_up)
    {
      // skip gaps
      do
	{
	  if(--current_menu->selected < 0)
            {
	      // jump to end of menu
              int i;
              for(i=0; current_menu->menuitems[i].type != it_end; i++);
              current_menu->selected = i-1;
            }
	}
      while(!selectable(selected_item(current_menu)));
      
      S_StartSound(NULL, sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_down)
    {
      do
	{
	  ++current_menu->selected;
	  if(selected_item(current_menu)->type == it_end)
	    {
	      current_menu->selected = 0;     // jump back to start
	    }
	}
      while(!selectable(selected_item(current_menu)));
      
      S_StartSound(NULL,sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_enter)
    {
      menuitem_t *menuitem = selected_item(current_menu);
      
      switch(menuitem->type)
	{
	  case it_runcmd:
	    {
	      S_StartSound(NULL,sfx_pistol);  // make sound
	      cmdtype = c_menu;
	      C_RunTextCmd(menuitem->data);
	      break;
	    }
	  
	  case it_toggle:
	    {
	      // boolean values only toggled on enter
	      if(menuitem->var->type != vt_int ||
		 menuitem->var->max-menuitem->var->min > 1) break;
	      
	      // toggle value now
	      sprintf(tempstr, "%s /", menuitem->data);
	      cmdtype = c_menu;
	      C_RunTextCmd(tempstr);
	      
	      S_StartSound(NULL,sfx_pistol);  // make sound
	      break;
	    }
	  
	  case it_variable:
	    {
	      menuitem_t *menuitem =
		&current_menu->menuitems[current_menu->selected];
	      
	      // get input for new value
	      input_command = C_GetCmdForName(menuitem->data);
	      input_buffer[0] = 0;             // clear input buffer
	      break;
	    }
	  
	  case it_automap:
	    {
	      MN_SelectColour(menuitem->data);
	      
	      return true;
	    }
	  
	  case it_binding:
	    {
	      G_EditBinding(menuitem->data);
	      
	      return true;
	    }
	
	  default: break;
	}
      return true;
    }
  
  if(ev->data1 == key_menu_backspace)
    {
      MN_PrevMenu();
      return true;          // eatkey
    }
  
  // decrease value of variable
  if(ev->data1 == key_menu_left)
    {
      menuitem_t *menuitem = selected_item(current_menu);
      
      switch(menuitem->type)
	{
	case it_slider:
	case it_toggle:
	  {
	    // no on-off int values
	    if(menuitem->var->type == vt_int &&
	       menuitem->var->max-menuitem->var->min == 1) break;
	    
	    // change variable
	    sprintf(tempstr, "%s -", menuitem->data);
	    cmdtype = c_menu;
	    C_RunTextCmd(tempstr);
	  }
	default:
	  {
	    break;
	  }
	}
      return true;
    }
  
  // increase value of variable
  if(ev->data1 == key_menu_right)
    {
      menuitem_t *menuitem = selected_item(current_menu);
      
      switch(menuitem->type)
	{
	case it_slider:
	case it_toggle:
	  {
	    // no on-off int values
	    if(menuitem->var->type == vt_int &&
	       menuitem->var->max-menuitem->var->min == 1) break;
	    
	    // change variable
	    sprintf(tempstr, "%s +", menuitem->data);
	    cmdtype = c_menu;
	    C_RunTextCmd(tempstr);
	  }
	
	default:
	  {
	    break;
	  }
	}
      return true;
    }

  // search for matching item in menu

  ch = tolower(ev->data1);
  if(isalnum(ch))
    {
      // sf: experimented with various algorithms for this
      //     this one seems to work as it should

      int n = current_menu->selected;

      do
	{
	  n++;
	  if(current_menu->menuitems[n].type == it_end) n = 0; // loop round

	  // ignore unselectables
	  if(selectable(&current_menu->menuitems[n])) 
	    if(tolower(current_menu->menuitems[n].description[0]) == ch)
	      {
		// found a matching item!
		current_menu->selected = n;
		return true; // eat key
	      }
      	} while(n != current_menu->selected);
    }
  
  return false;
}

///////////////////////////////////////////////////////////////////////////
//
// Other Menu Functions
//

// ?
void MN_ResetMenu()
{
}

// make menu 'clunk' sound on opening

void MN_ActivateMenu()
{
  if(!menuactive)  // activate menu if not already
    {
      menuactive = true;
      S_StartSound(NULL, sfx_swtchn);
    }
}

////////////////////////////////////////////////////////////////////////////
//
// Start Menu
//

// ensure we have a valid item selected on a menu
static void MN_SelectValidItem(menu_t *menu)
{
  int start = menu->selected;
  
  while(!selectable(selected_item(menu)))
    {
      ++menu->selected;
      if(selected_item(menu)->type == it_end)
	{
	  menu->selected = 0;     // jump back to start
	}
      if(menu->selected == start)
	{
	  // no selectable items
	  return;  
	}
    }
}

/////////////////////////////
//
// Menu caching
//

static void MN_GetItemVariable(menuitem_t *item)
{
  // get variable if neccesary

  if(!item->var)
    {
      command_t *cmd;
      // use data for variable name
      if(!(cmd = C_GetCmdForName(item->data)))
        {
          C_Printf("variable not found: %s\n", item->data);
	  item->type = it_disabled;   // disable item
	  // possibly change selected item on menu
	  MN_SelectValidItem(drawing_menu);
	  return;
        }
      item->var = cmd->variable;
    }
}

// Called on menu activation.
// Get all variables for menu items
// cache any other stuff
static void MN_CacheMenu(menu_t *menu)
{
  int i;

  for(i=0; menu->menuitems[i].type != it_end; i++)
    {
      if(menu->menuitems[i].type == it_variable ||
	 menu->menuitems[i].type == it_toggle ||
	 menu->menuitems[i].type == it_slider ||
	 menu->menuitems[i].type == it_automap ||
	 menu->menuitems[i].type == it_constant)
	MN_GetItemVariable(&menu->menuitems[i]);
    }
}

// start a menu:

void MN_StartMenu(menu_t *menu)
{
  MN_CacheMenu(menu);

  // ensure we have a selectable item selected on new menu
  MN_SelectValidItem(menu);

  // no selectable items? then dont start menu
  if(!selectable(selected_item(menu)))
    return;
  
  if(!menuactive)
    {
      MN_ActivateMenu();
      current_menu = menu;
      menu_history_num = 0;  // reset history
    }
  else
    {
      menu_history[menu_history_num++] = current_menu;
      current_menu = menu;
    }
  
  menu_error_message[0] = '\0';      // clear error message
  redrawsbar = redrawborder = true;  // need redraw
}

// go back to a previous menu

void MN_PrevMenu()
{
  if(--menu_history_num < 0) MN_ClearMenus();
  else
    current_menu = menu_history[menu_history_num];
      
  menu_error_message[0] = '\0';          // clear errors
  redrawsbar = redrawborder = true;  // need redraw
  S_StartSound(NULL, sfx_swtchx);
}

// turn off menus
void MN_ClearMenus()
{
  menuactive = false;
  redrawsbar = redrawborder = true;  // need redraw
}

CONSOLE_COMMAND(mn_clearmenus, 0)
{
  MN_ClearMenus();
}

CONSOLE_COMMAND(mn_prevmenu, 0)
{
  MN_PrevMenu();
}

        // ??
void MN_ForcedLoadGame(const char *msg)
{
}

// display error msg in popup display at bottom of screen

void MN_ErrorMsg(char *s, ...)
{
  va_list args;
  
  va_start(args, s);
  vsprintf(menu_error_message, s, args);
  va_end(args);
}

// activate main menu

extern menu_t menu_main;

void MN_StartControlPanel()
{
  MN_StartMenu(&menu_main);
  
  S_StartSound(NULL,sfx_swtchn);
}

///////////////////////////////////////////////////////////////////////////
//
// Menu Font Drawing
//
// copy of V_* functions
// these do not leave a 1 pixel-gap between chars, I think it looks
// better for the menu

extern patch_t* v_font[V_FONTSIZE];

void MN_WriteText(unsigned char *s, int x, int y)
{
  int   w;
  unsigned char* ch;
  char *colour = cr_red;
  boolean translucent = false;
  unsigned int c;
  int   cx;
  int   cy;
  patch_t *patch;

  ch = s;
  cx = x;
  cy = y;
  
  while(1)
    {
      c = *ch++;
      if (!c)
	break;
      if (c >= 128)     // new colour
	{
	  if(c == *(unsigned char *)FC_TRANS)
	    translucent = !translucent;
	  else
	    {
	      int colnum = c - 128;
	      
	      if(colnum < 0 || colnum >= CR_LIMIT)
		I_Error("MN_WriteText: invalid colour %i\n", colnum);
	      else
		colour = colrngs[colnum];
	    }
	  continue;
	}
      if (c == '\t')
        {
          cx = (cx/40)+1;
          cx = cx*40;
        }
      if (c == '\n')
	{
	  cx = x;
          cy += 8;
	  continue;
	}
  
      c = toupper(c) - V_FONTSTART;
      if (c < 0 || c>= V_FONTSIZE)
	{
	  cx += 4;
	  continue;
	}

      patch = v_font[c];
      if(!patch) continue;

      w = SHORT (patch->width);
      if (cx+w > SCREENWIDTH)
	break;

      if(translucent)
	V_DrawPatchTL(cx, cy, 0, patch, colour);
      else
	V_DrawPatchTranslated(cx, cy, 0, patch, colour, 0);

      cx+=w-1;
    }
}

        // write text in a particular colour

void MN_WriteTextColoured(unsigned char *s, int colour, int x, int y)
{
   static char *tempstr = NULL;
   static int allocedsize=-1;

        // if string bigger than allocated, realloc bigger
   if(!tempstr || strlen(s) > allocedsize)
     {
       if(tempstr)       // already alloced?
	 tempstr = realloc(tempstr, strlen(s) + 5);
       else
	 tempstr = malloc(strlen(s) + 5);
       
       allocedsize = strlen(s);  // save for next time
     }

   tempstr[0] = 128 + colour;
   strcpy(&tempstr[1], s);

   MN_WriteText(tempstr, x, y);
}


int MN_StringWidth(unsigned char *s)
{
  int length = 0;
  unsigned char c;
  
  for(; *s; s++)
    {
      c = *s;
      if(c >= 128)         // colour
	continue;
      c = toupper(c) - V_FONTSTART;
      length += c >= V_FONTSIZE ? 4 : SHORT(v_font[c]->width)-1;
    }
  return length;
}

/////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

extern void MN_AddMenus();               // mn_menus.c
extern void MN_Misc_AddCommands();       // mn_misc.c
extern void MN_File_AddCommands();       // mn_files.c
extern void MN_Net_AddCommands();        // mn_net.c

void MN_AddCommands()
{
  C_AddCommand(mn_clearmenus);
  C_AddCommand(mn_prevmenu);

  MN_AddMenus();               // add commands to call the menus
  MN_Misc_AddCommands();
  MN_File_AddCommands();
  MN_Net_AddCommands();
}

//-------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:09  fraggle
// Initial revision
//
//
//-------------------------------------------------------------------------
