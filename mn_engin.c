// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
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
#include "g_game.h"
#include "hu_over.h"
#include "i_video.h"
#include "mn_engin.h"
#include "mn_menus.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "w_wad.h"
#include "v_video.h"


#define SKULL_HEIGHT 19
#define BLINK_TIME 8
#define MENU_HISTORY 128
        // gap from variable description to value
#define GAP 20
#define background_flat "FLOOR4_8"

// colours
#define unselect_colour                 CR_RED
#define select_colour                   CR_GRAY
#define var_colour                      CR_GREEN

boolean menuactive = false;             // menu active?
menu_t *current_menu;   // the current menu_t being displayed
menu_t *menu_history[MENU_HISTORY];   // previously selected menus
int menu_history_num;                 // location in history

boolean inhelpscreens; // indicates we are in or just left a help screen

        // menu keys
int     key_menu_right;
int     key_menu_left;
int     key_menu_up;
int     key_menu_down;
int     key_menu_backspace;
int     key_menu_escape;
int     key_menu_enter;

// menu error message
char menu_error_message[128];
int menu_error_time = 0;

int hide_menu = 0;      // hide the menu for a duration of time

int menutime = 0;

patch_t *skulls[2];

enum
{
  slider_left,
  slider_right,
  slider_mid,
  slider_slider,
  num_slider_gfx
};
patch_t *slider_gfx[num_slider_gfx];

extern menu_t menu_main;
extern menu_t menu_newgame;

        // input for typing in new value
static command_t *input_command = NULL;       // NULL if not typing in
static char input_buffer[128] = "";

/******** functions **********/

        // init menu
void MN_Init()
{
  skulls[0] = W_CacheLumpName("M_SKULL1", PU_STATIC);
  skulls[1] = W_CacheLumpName("M_SKULL2", PU_STATIC);
  
  // load slider gfx
  
  slider_gfx[slider_left]   = W_CacheLumpName("M_SLIDEL", PU_STATIC);
  slider_gfx[slider_right]  = W_CacheLumpName("M_SLIDER", PU_STATIC);
  slider_gfx[slider_mid]    = W_CacheLumpName("M_SLIDEM", PU_STATIC);
  slider_gfx[slider_slider] = W_CacheLumpName("M_SLIDEO", PU_STATIC);
  
  MN_InitMenus();
}

        // ticker
void MN_Ticker()
{
  if(menu_error_time)
    menu_error_time--;
  if(hide_menu)                   // count down hide_menu
    hide_menu--;
  menutime++;
}

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
	  item->type = it_info;   // turn into normal unselectable text
	  item->var = NULL;
	  return;
        }
      item->var = cmd->variable;
    }
}

        // width of slider, in mid-patches
#define SLIDE_PATCHES 9

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
static int MN_DrawMenuItem(menuitem_t *item, int x, int y, int colour,
                                int leftaligned)
{
  if(item->type == it_gap) return 8;    // skip drawing if a gap

  item->x = x; item->y = y;       // save x,y to item
  
  if(item->patch)
    {
      // draw alternate patch
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
  
  if(item->type == it_title)
    {
      // centre the text
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
  
  switch(item->type)      
    {
    case it_title:              // just description drawn
    case it_info:
    case it_runcmd:
      {
        break;
      }
    
    case it_toggle:
    case it_variable:
      {
        char varvalue[128];             // temp buffer 

	MN_GetItemVariable(item);
	
	// adjust colour for different coloured variables
	if(colour == unselect_colour) colour = var_colour;
	
	// create variable description:
	// Use console variable descriptions.
	
	// display input buffer if inputting new var value
	if(input_command && item->var == input_command->variable)
          sprintf(varvalue, "%s_", input_buffer);
	else
          strcpy(varvalue, C_VariableStringValue(item->var));
	
        // draw it
        MN_WriteTextColoured
	  (
	   varvalue,
           colour,
           x + GAP + (leftaligned ? MN_StringWidth(item->description) : 0),
	   y
	   );
	break;
      }
    
    case it_gap:    // just a gap, draw nothing
      {
	break;
      }
    
    case it_slider:               // slider
      { 
	MN_GetItemVariable(item);
	
	// draw slider
	// only ints
	if(item->var && item->var->type == vt_int)
	  {
	    int range = item->var->max - item->var->min;
	    int posn = *(int *)item->var->variable - item->var->min;
	    
	    MN_DrawSlider(x + GAP, y, (posn*100) / range);
	  }
	
	break;
      }

    case it_automap:
      {
#define BLOCK_SIZE 9
	char block[BLOCK_SIZE*BLOCK_SIZE];
	int bx, by;
	int colour;
	
	MN_GetItemVariable(item);

	if(!item->var || item->var->type != vt_int) break;
	
	// find colour of this variable from console variable
	colour = *(int *)item->var->variable;

	// create block

	// border
	memset(block, 0, BLOCK_SIZE*BLOCK_SIZE);

	// middle
	for(bx=1; bx<BLOCK_SIZE-1; bx++)
	  for(by=1; by<BLOCK_SIZE-1; by++)
	    block[by*BLOCK_SIZE+bx] = colour;

	// draw it

	V_DrawBlock(x+GAP, y-1, 0, BLOCK_SIZE, BLOCK_SIZE, block);
      }
    
    default:
      {
        break;
      }
    }
  
  return 8;
}

        // draw a menu
static void MN_DrawMenu(menu_t *menu)
{
  int y;
  int itemnum;
  
  if(menu->flags & mf_background) // draw background
    MN_DrawBackground(background_flat, screens[0]);
  
  if(menu->drawer) menu->drawer();
  
  y = menu->y;
  
  for(itemnum = 0; menu->menuitems[itemnum].type != it_end; itemnum++)
    {
      int item_height;
      // choose item colour based on selected item
      int item_colour =
	menu->selected == itemnum && !(menu->flags & mf_skullmenu) ?
	select_colour : unselect_colour;
      
      // if skull menu, left aligned
      item_height =
	MN_DrawMenuItem
	(
	 &menu->menuitems[itemnum],
	 menu->x,
	 y,
	 item_colour,
	 menu->flags & mf_skullmenu || menu->flags & mf_leftaligned
	 );
      
      // if selected item, draw skull next to it
      if(menu->flags & mf_skullmenu && menu->selected == itemnum)
        V_DrawPatch
	  (
	   menu->x - 30,                         // 30 left
	   y+(item_height-SKULL_HEIGHT)/2,       // midpoint
	   0, skulls[(menutime/BLINK_TIME) % 2]
	   );
      
      y += item_height;            // go down by item height
    }
}

        // drawer
void MN_Drawer()
{
  char *helpmsg = "";
  
  // redraw needed if menu hidden
  if(hide_menu) redrawsbar = redrawborder = true;
  
  if(!menuactive || hide_menu) return;
  
      // display popup messages
  if(popup_message_active)
    {
      MN_PopupDrawer();
      return;
    }

  MN_DrawMenu(current_menu);
  
  // choose help message to print
  
  if(menu_error_time)             // error message takes priority
    helpmsg = menu_error_message;
  else
    {
      // write some help about the item
      menuitem_t *menuitem =
	&current_menu->menuitems[current_menu->selected];
      
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
    }
  MN_WriteTextColoured(helpmsg, CR_GOLD, 10, 192);
}

        // whether a menu item is a 'gap' item
#define is_a_gap(it) ((it)->type == it_gap || (it)->type == it_info ||  \
                      (it)->type == it_title)

extern menu_t menu_sound;

boolean MN_TempResponder(unsigned char key)
{
  if(key == key_quit)
    {
      C_RunTextCmd("mn_quit");
      return true;
    }
  if(key == key_soundvolume)
    {
      MN_StartMenu(&menu_sound);
      return true;
    }
  if(key == key_endgame)
    {    
      C_RunTextCmd("mn_endgame");
      return true;
    }
  if(key == key_hud)
    {
      HU_OverlayStyle();
      return true;
    }
  return false;
}
                

        // responder
boolean MN_Responder (event_t *ev)
{
  char tempstr[128];
  char ch;

  if(ev->type != ev_keydown)
    return false;   // no use for it

  // are we displaying a pop-up message?

  if(popup_message_active)
    return MN_PopupResponder(ev);

  // are we inputting a new value into a variable?
  
  if(input_command)
    {
      char ch = ev->data1;
      
      if(ev->data1 == KEYD_ESCAPE)        // cancel input
	input_command = NULL;
      
      if(ev->data1 == KEYD_ENTER && input_buffer[0])
	{
	  char *temp;
	  // place " marks round the string for the console
	  // ugh.
	  temp = strdup(input_buffer);
	  sprintf(input_buffer, "\"%s\"", temp);
	  free(temp);
	  
	  // set the command
	  cmdtype = c_typed;
	  C_RunCommand(input_command, input_buffer);
	  input_command = false;
	}
      
      // check for backspace
      if(ev->data1 == KEYD_BACKSPACE && input_buffer[0])
	input_buffer[strlen(input_buffer)-1] = 0;
      
      // probably just a normal character
      
      // only care about valid characters
      // dont allow too many characters on one command line
      if(ch > 31 && ch < 127 && strlen(input_buffer) < 126)
	sprintf(input_buffer, "%s%c", input_buffer, ch);
      
      return true;
    } 

  if ((devparm && ev->data1 == key_help)
      || ev->data1 == key_screenshot)
    {
      G_ScreenShot ();
      return true;
    }                             
  
  if(ev->data1 == key_escape)
    {
      // toggle menu
      
      // start up main menu or kill menu
      if(menuactive) MN_ClearMenus();
      else MN_StartControlPanel();
      
      S_StartSound(NULL, menuactive ? sfx_swtchn : sfx_swtchx);
    }

  if(MN_TempResponder(ev->data1)) return true;
  
  // not interested in keys if not in menu
  if(!menuactive) return false;

  if(ev->data1 == key_menu_up)
    {
      // skip gaps
      do
	{
	  if(--current_menu->selected < 0)
            {
              int i;
	      // jump to end of menu
              for(i=0; current_menu->menuitems[i].type != it_end; i++);
              current_menu->selected = i-1;
            }
	}
      while(is_a_gap(&current_menu->menuitems[current_menu->selected]));
      
      S_StartSound(NULL,sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_down)
    {
      do
	{
	  ++current_menu->selected;
	  if(current_menu->menuitems[current_menu->selected].type == it_end)
	    {
	      current_menu->selected = 0;     // jump back to start
	    }
	}
      while(is_a_gap(&current_menu->menuitems[current_menu->selected]));
      
      S_StartSound(NULL,sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_enter)
    {
      menuitem_t *menuitem = &current_menu->menuitems[current_menu->selected];
      
      switch(menuitem->type)
	{
	case it_runcmd:
	  {
	    S_StartSound(NULL,sfx_pistol);  // make sound
	    cmdtype = c_menu;
	    C_RunTextCmd(current_menu->menuitems[current_menu->selected].data);
	    break;
	  }
	
	case it_toggle:
	  {
	    // on-off values only only toggled on enter
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
      menuitem_t *menuitem =
	&current_menu->menuitems[current_menu->selected];
      
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
      menuitem_t *menuitem =
	&current_menu->menuitems[current_menu->selected];
      
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
  if(ch >= 'a' && ch <= 'z')
    {
      
      // sf: experimented with various algorithms for this
      //     this one seems to work as it should

      int n = current_menu->selected;

      do
	{
	  n++;
	  if(current_menu->menuitems[n].type == it_end) n = 0; // loop round

	  // ignore unselectables
	  if(!is_a_gap(&current_menu->menuitems[n])) 
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

void MN_StartMenu(menu_t *menu)
{
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
  
  menu_error_time = 0;      // clear error message
  redrawsbar = redrawborder = true;  // need redraw
}

void MN_PrevMenu()
{
   // go back to a previous menu
  if(--menu_history_num < 0) MN_ClearMenus();
  else
    current_menu = menu_history[menu_history_num];
      
  menu_error_time = 0;          // clear errors
  redrawsbar = redrawborder = true;  // need redraw
  S_StartSound(NULL, sfx_swtchx);
}

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

void MN_ErrorMsg(char *s, ...)
{
  va_list args;
  
  va_start(args, s);
  vsprintf(menu_error_message, s, args);
  va_end(args);
  
  menu_error_time = 70;
}

extern void MN_AddMenus();              // mn_menus.c

void MN_AddCommands()
{
  C_AddCommand(mn_clearmenus);
  C_AddCommand(mn_prevmenu);

  MN_AddMenus();               // add commands to call the menus
}

/////////////////////////////
//
// MN_DrawBackground tiles a 64x64 patch over the entire screen, providing the
// background for the Help and Setup screens.
//
// killough 11/98: rewritten to support hires

char *R_DistortedFlat(int);

void MN_DrawBackground(char* patchname, byte *back_dest)
{
  int x,y;
  byte *back_src, *src;
  
  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
  
  src = back_src = 
    W_CacheLumpNum(firstflat+R_FlatNumForName(patchname),PU_CACHE);
  
  if (hires)       // killough 11/98: hires support
#if 0              // this tiles it in hires:
    for (y = 0 ; y < SCREENHEIGHT*2 ; src = ((++y & 63)<<6) + back_src)
      for (x = 0 ; x < SCREENWIDTH*2/64 ; x++)
	{
	  memcpy (back_dest,back_src+((y & 63)<<6),64);
	  back_dest += 64;
	}
#endif
  
  // while this pixel-doubles it
  for (y = 0 ; y < SCREENHEIGHT ; src = ((++y & 63)<<6) + back_src,
	 back_dest += SCREENWIDTH*2)
    for (x = 0 ; x < SCREENWIDTH/64 ; x++)
      {
	int i = 63;
	do
	  back_dest[i*2] = back_dest[i*2+SCREENWIDTH*2] =
	    back_dest[i*2+1] = back_dest[i*2+SCREENWIDTH*2+1] = src[i];
	while (--i>=0);
	back_dest += 128;
      }
  else
    for (y = 0 ; y < SCREENHEIGHT ; src = ((++y & 63)<<6) + back_src)
      for (x = 0 ; x < SCREENWIDTH/64 ; x++)
	{
	  memcpy (back_dest,back_src+((y & 63)<<6),64);
	  back_dest += 64;
	}
}

        // sf:
void MN_DrawDistortedBackground(char* patchname, byte *back_dest)
{
  int x,y;
  byte *back_src, *src;

  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

  src = back_src = R_DistortedFlat(R_FlatNumForName(patchname));

  if (hires)       // killough 11/98: hires support
#if 0              // this tiles it in hires:
    for (y = 0 ; y < SCREENHEIGHT*2 ; src = ((++y & 63)<<6) + back_src)
      for (x = 0 ; x < SCREENWIDTH*2/64 ; x++)
	{
	  memcpy (back_dest,back_src+((y & 63)<<6),64);
	  back_dest += 64;
	}
#endif

              // while this pixel-doubles it
      for (y = 0 ; y < SCREENHEIGHT ; src = ((++y & 63)<<6) + back_src,
	     back_dest += SCREENWIDTH*2)
	for (x = 0 ; x < SCREENWIDTH/64 ; x++)
	  {
	    int i = 63;
	    do
	      back_dest[i*2] = back_dest[i*2+SCREENWIDTH*2] =
		back_dest[i*2+1] = back_dest[i*2+SCREENWIDTH*2+1] = src[i];
	    while (--i>=0);
	    back_dest += 128;
	  }
  else
    for (y = 0 ; y < SCREENHEIGHT ; src = ((++y & 63)<<6) + back_src)
      for (x = 0 ; x < SCREENWIDTH/64 ; x++)
	{
	  memcpy (back_dest,back_src+((y & 63)<<6),64);
	  back_dest += 64;
	}
}

void MN_DrawCredits(void)     // killough 10/98: credit screen
{
  inhelpscreens = true;

        // sf: altered for SMMU

  MN_DrawDistortedBackground(gamemode==commercial ? "SLIME05" : "LAVA1",
                                screens[0]);

        // sf: SMMU credits
  V_WriteText(
	      FC_GRAY "SMMU:" FC_RED " \"Smack my marine up\"\n"
	      "\n"
	      "Port by Simon Howard 'Fraggle'\n"
	      "\n"
	      "Based on the MBF port by Lee Killough\n"
	      "\n"
	      FC_GRAY "Programming:" FC_RED " Simon Howard\n"
	      FC_GRAY "Graphics:" FC_RED " Bob Satori\n"
	      FC_GRAY "Level editing/start map:" FC_RED " Derek MacDonald\n"
	      "\n"
	      "\n"
	      "Copyright(C) 1999 Simon Howard\n"
	      FC_GRAY"         http://fraggle.tsx.org/",
	      10, 60);


}

        // activate main menu
void MN_StartControlPanel()
{
  MN_StartMenu(&menu_main);
  
  S_StartSound(NULL,sfx_swtchn);
}


// copy of V_* functions
// these do not leave a 1 pixel-gap between chars, I think it looks
// better for the menu

extern patch_t* v_font[V_FONTSIZE];

void MN_WriteText(unsigned char *s, int x, int y)
{
  int   w;
  unsigned char* ch;
  char *colour = cr_red;
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
           colour = colrngs[c - 128];
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

