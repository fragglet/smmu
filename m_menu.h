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

#ifndef __M_MENU__
#define __M_MENU__

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

boolean M_Responder (event_t *ev);

// Called by main loop,
// only used for menu (skull cursor) animation.

void M_Ticker (void);

// Called by main loop,
// draws the menus directly into the screen buffer.

void M_Drawer (void);

// Called by D_DoomMain,
// loads the config file.

void M_Init (void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.

void M_StartControlPanel (void);

void M_ForcedLoadGame(const char *msg); // killough 5/15/98: forced loadgames

void M_ResetMenu(void);      // killough 11/98: reset main menu ordering

void M_DrawBackground(char *patch, byte *screen);  // killough 11/98

void M_DrawCredits(void);    // killough 11/98

void M_StartMenu(menu_t *menu);         // sf 10/99

void M_ClearMenus();                    // sf 10/99


//
// menu_t
//

#define MAXMENUITEMS 128

struct menuitem_s
{
        enum                    // item types
        {
           it_gap,              // empty line
           it_runcmd,           // run console command
           it_variable,         // variable
                                // enter pressed to type in new value
           it_toggle,           // togglable variable
                                // can use left/right to change value
           it_title,            // the menu title
           it_info,             // information / section header
           it_end,              // last menuitem in the list
        } type;

        char *name;             // the describing name of this item

        char *data;             // useful data for the item:
                                // console command if console
                                // variable name if variable, etc
        char *patch;            // patch to use or NULL

                /*** internal stuff used by menu code ***/
                // messing with this is a bad idea(prob)
        int x, y;
        variable_t *var;        // ptr to console variable
};

struct menu_s
{
        menuitem_t menuitems[MAXMENUITEMS];

        int x, y;               // x,y offset of menu
        int selected;           // currently selected item
        enum                    // menu flags
        {
            mf_skullmenu =1,    // show skull rather than highlight
            mf_background=2,    // show background
            mf_leftaligned=4,   // left-aligned menu
        } flags;               
        void (*drawer)();       // seperate drawer function 
};

void M_ErrorMsg(char *s, ...);

// menu error message
extern char menu_error_message[128];
extern int menu_error_time;

extern int hide_menu;

#endif
                            
