// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_menu.h,v 1.4 1998/05/16 09:17:18 killough Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//   Menu widget stuff, episode selection and such.
//    
//-----------------------------------------------------------------------------

#ifndef __MN_MENU__
#define __MN_MENU__

#include "c_runcmd.h"
#include "d_event.h"

typedef struct menu_s menu_t;
typedef struct menuitem_s menuitem_t;


//
// MENUS
//
// Called by main loop,
// saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed,
// this can resize the view and change game parameters.
// Does all the real work of the menu interaction.

boolean MN_Responder (event_t *ev);

// Called by main loop,
// only used for menu (skull cursor) animation.

void MN_Ticker (void);

// Called by main loop,
// draws the menus directly into the screen buffer.

void MN_Drawer (void);

// Called by D_DoomMain,
// loads the config file.

void MN_Init (void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.

void MN_StartControlPanel (void);

void MN_ForcedLoadGame(const char *msg); // killough 5/15/98: forced loadgames

void MN_ResetMenu(void);      // killough 11/98: reset main menu ordering

void MN_DrawBackground(char *patch, byte *screen);  // killough 11/98

void MN_DrawCredits(void);    // killough 11/98

void MN_ActivateMenu();
void MN_StartMenu(menu_t *menu);         // sf 10/99
void MN_PrevMenu();
void MN_ClearMenus();                    // sf 10/99

// font functions
void MN_WriteText(unsigned char *s, int x, int y);
void MN_WriteTextColoured(unsigned char *s, int colour, int x, int y);
int MN_StringWidth(unsigned char *s);

//
// menu_t
//

#define MAXMENUITEMS 128

struct menuitem_s
{
  // item types
  enum
  {
    it_gap,              // empty line
    it_runcmd,           // run console command
    it_variable,         // variable
                         // enter pressed to type in new value
    it_toggle,           // togglable variable
                         // can use left/right to change value
    it_title,            // the menu title
    it_info,             // information / section header
    it_slider,           // slider
    it_automap,          // an automap colour
    it_end,              // last menuitem in the list
  } type;
  
  // the describing name of this item
  char *description;

  // useful data for the item:
  // console command if console
  // variable name if variable, etc

  char *data;         

  // patch to use or NULL
  char *patch;

                  /*** internal stuff used by menu code ***/
                  // messing with this is a bad idea(prob)
  int x, y;
  variable_t *var;        // ptr to console variable
};

struct menu_s
{
  menuitem_t menuitems[MAXMENUITEMS];

  // x,y offset of menu
  int x, y;
  
  // currently selected item
  int selected;
  
  // menu flags
  enum
  {
    mf_skullmenu =1,    // show skull rather than highlight
    mf_background=2,    // show background
    mf_leftaligned=4,   // left-aligned menu
  } flags;               
  void (*drawer)();       // seperate drawer function 
};

void MN_ErrorMsg(char *s, ...);

// menu error message
extern char menu_error_message[128];
extern int menu_error_time;

extern int hide_menu;
extern int menutime;

#endif
                            
