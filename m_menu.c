/******************************** new menu *********************************/
                  // copyright(c) 1999 Simon Howard //

#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_over.h"
#include "i_video.h"
#include "m_menu.h"
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
#define WriteText V_WriteText
#define StringWidth V_StringWidth

// colours
#define unselect_colour                 CR_RED
#define select_colour                   CR_GRAY
#define var_colour                      CR_GREEN

boolean menuactive = false;             // menu active?
menu_t *current_menu;   // the current menu_t being displayed
menu_t *history[MENU_HISTORY];   // previously selected menus
int history_num;                 // location in history

boolean inhelpscreens; // indicates we are in or just left a help screen

        // menu keys
extern int key_quit;
extern int key_escape;
extern int key_menu_up;
extern int key_menu_down;
extern int key_menu_left;
extern int key_menu_right;
extern int key_menu_enter;
extern int key_menu_backspace;

// Blocky mode, has default, 0 = high, 1 = normal
//int     detailLevel;    obsolete -- killough

// menu error message
char menu_error_message[128];
int menu_error_time = 0;

int screenSize;      // screen size
boolean menu_skull;     // eyes on or off
patch_t *skulls[2];
int menutime = 0;

int hide_menu = 0;      // hide the menu for a duration of time

        // menus: all in this file (not really extern)
extern menu_t menu_newgame;
extern menu_t menu_main;
extern menu_t menu_episode;

char *m_phonenum;           // phone number to dial

/******** functions **********/

        // init menu
void M_Init()
{
        skulls[0] = W_CacheLumpName("M_SKULL1", PU_STATIC);
        skulls[1] = W_CacheLumpName("M_SKULL2", PU_STATIC);

        m_phonenum = Z_Strdup("0891 50 50 50", PU_STATIC, 0);
}

        // ticker
void M_Ticker()
{
        if(menu_error_time)
                menu_error_time--;
        if(hide_menu)                   // count down hide_menu
                hide_menu--;
        menutime++;
}

        // draw a menu item. returns the height in pixels
static int M_DrawMenuItem(menuitem_t *item, int x, int y, int colour,
                                int leftaligned)
{
        char itemname[128];     // temporary store for item name
                                // (allows for adding of colour-change chars)
        char varvalue[128];        // similar store for variable value

        sprintf(itemname, "%c%s", 128+colour, item->name);

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

        switch(item->type)
        {
                case it_title:
                        // centre the text
                      WriteText
                        (
                            itemname,
                            (SCREENWIDTH-StringWidth(item->name))/2,
                            y
                        );
                      break;

                case it_info:
                case it_runcmd:
                      WriteText
                        (
                            itemname,
                            x - (leftaligned ? 0 : StringWidth(itemname)),
                            y
                        );
                      break;

                case it_toggle:
                case it_variable:
                      WriteText
                        (
                          itemname,
                          leftaligned ? x : x-StringWidth(itemname),
                          y
                        );
                      // get variable if neccesary
                      if(!item->var)
                      {
                        command_t *cmd;
                                // use data for variable name
                        if(!(cmd = C_GetCmdForName(item->data)))
                          I_Error("unknown console command '%s'\n", item->data);
                        item->var = cmd->variable;
                      }

                      sprintf
                       (
                         varvalue, "%c%s",
                         128 + (colour==unselect_colour ? var_colour : colour),
                         C_VariableStringValue(item->var)
                       );
                      WriteText
                       (
                         varvalue,
                         x + GAP + (leftaligned ? StringWidth(itemname) : 0),
                         y
                       );
                      break;

                case it_gap:    // just a gap, draw nothing
                      break;

                default:
        }

        return 8;
}

        // draw a menu
void M_DrawMenu(menu_t *menu)
{
        int y;
        int itemnum;

        if(menu->flags & mf_background) // draw background
                M_DrawBackground(background_flat, screens[0]);

        if(menu->drawer) menu->drawer();

        y = menu->y;

        for(itemnum = 0; menu->menuitems[itemnum].type != it_end; itemnum++)
        {
           int item_height;
           int item_colour =
               menu->selected == itemnum && !(menu->flags & mf_skullmenu) ?
                                select_colour : unselect_colour;

                // if skull menu, left aligned
           item_height =
             M_DrawMenuItem
                (
                   &menu->menuitems[itemnum],
                   menu->x,
                   y,
                   item_colour,
                   menu->flags & mf_skullmenu || menu->flags & mf_leftaligned
                );

           if(menu->flags & mf_skullmenu && menu->selected == itemnum)
                V_DrawPatch(menu->x - 30, y+(item_height-SKULL_HEIGHT)/2,
                        0, skulls[(menutime/BLINK_TIME) % 2]);

           y += item_height;
        }
}

        // drawer
void M_Drawer()
{
        char *helpmsg = "";

                // redraw needed if menu hidden
        if(hide_menu) redrawsbar = redrawborder = true;

        if(!menuactive || hide_menu) return;

        M_DrawMenu(current_menu);

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
        WriteText(helpmsg, 10, 192);
}

        // whether a menu item is a 'gap' item
#define is_a_gap(it) ((it)->type == it_gap || (it)->type == it_info ||  \
                      (it)->type == it_title)
                

        // responder
boolean M_Responder (event_t *ev)
{
        char tempstr[128];

        if(ev->type != ev_keydown)
                return false;   // no use for it

        if(ev->data1 == key_quit)       // quit
                exit(0);

        if(ev->data1 == key_escape)
        {
                // toggle menu

                        // start up main menu or kill menu
                if(menuactive) M_ClearMenus();
                else M_StartMenu(&menu_main);

                S_StartSound(NULL, menuactive ? sfx_swtchn : sfx_swtchx);
        }

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
               C_RunTextCmd(tempstr);

               S_StartSound(NULL,sfx_pistol);  // make sound
               break;
             }

             case it_variable:
             {
                // get input for new value
               break;
             }

             default: break;
          }
          return true;
        }

        if(ev->data1 == key_menu_backspace)
        {
                // go back to a previous menu
          if(--history_num < 0) M_ClearMenus();
          else
             current_menu = history[history_num];              

          menu_error_time = 0;          // clear errors
          redrawsbar = redrawborder = true;  // need redraw
          S_StartSound(NULL, sfx_swtchx);
          return true;          // eatkey
        }

                // decrease value of variable
        if(ev->data1 == key_menu_left)
        {
          menuitem_t *menuitem =
                &current_menu->menuitems[current_menu->selected];

          switch(menuitem->type)
          {
                case it_toggle:
                {
                                // no on-off int values
                        if(menuitem->var->type == vt_int &&
                           menuitem->var->max-menuitem->var->min == 1) break;

                                // change variable
                        sprintf(tempstr, "%s -", menuitem->data);
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
                case it_toggle:
                {
                                // no on-off int values
                        if(menuitem->var->type == vt_int &&
                           menuitem->var->max-menuitem->var->min == 1) break;

                          // change variable
                        sprintf(tempstr, "%s +", menuitem->data);
                        C_RunTextCmd(tempstr);
                }

                default:
                {
                        break;
                }
          }
          return true;
        }

        return false;
}

void M_ResetMenu()
{
}

void M_StartMenu(menu_t *menu)
{
     if(!menuactive)
     {
             menuactive = true;
             current_menu = menu;
             history_num = 0;  // reset history
     }
     else
     {
             history[history_num++] = current_menu;
             current_menu = menu;
     }

     menu_error_time = 0;      // clear error message
     redrawsbar = redrawborder = true;  // need redraw
}

void M_ClearMenus()
{
     menuactive = false;
     redrawsbar = redrawborder = true;  // need redraw
}

        // ??
void M_ForcedLoadGame(const char *msg)
{
}

void M_ErrorMsg(char *s, ...)
{
        va_list args;

        va_start(args, s);
        vsprintf(menu_error_message, s, args);
        va_end(args);

        menu_error_time = 70;
}

/***************************** THE MENUS **********************************/

/***********************
              MAIN MENU
 ***********************/

void M_MainMenuDrawer();

menu_t menu_main =
{
  {
                // 'doom' title drawn by the drawer

    {it_runcmd, "new game",             "m_newgame",            "M_NGAME"},
    {it_runcmd, "options",              "m_options",            "M_OPTION"},
    {it_runcmd, "load game",            "m_loadgame",           "M_LOADG"},
    {it_runcmd, "save game",            "m_savegame",           "M_SAVEG"},
    {it_runcmd, "features",             "m_features",           "M_FEAT"},
    {it_runcmd, "quit",                 "quit",                 "M_QUITG"},
    {it_end},
  },
  100, 65,                // x, y offsets
  0,                     // start with 'new game' selected
  mf_skullmenu,          // a skull menu
  M_MainMenuDrawer
};

void M_MainMenuDrawer()
{
          // hack for m_doom compatibility
   V_DrawPatch(94, 2, 0, W_CacheLumpName("M_DOOM", PU_CACHE));
}

        // activate main menu
void M_StartControlPanel()
{
        M_StartMenu(&menu_main);

        S_StartSound(NULL,sfx_swtchn);
}

CONSOLE_COMMAND(m_newgame, cf_notnet)
{
    if(gamemode == commercial)
    {
        // dont use new game menu if not needed
        if(!modifiedgame && gamemode==commercial && W_CheckNumForName("START") >= 0)
        {
            G_DeferedInitNew(defaultskill, "START");    // start on the start
                                                        // map
            M_ClearMenus ();
        }
        else
            M_StartMenu(&menu_newgame);
    }
    else
    {
        if(modifiedgame) M_StartMenu(&menu_newgame);
        else M_StartMenu(&menu_episode);
    }
}

/************************
        SELECT EPISODE
 ************************/

int start_episode;

menu_t menu_episode =
{
   {
       {it_title, "which episode?",             NULL,           "M_EPISOD"},
       {it_gap},
       {it_runcmd, "knee deep in the dead",     "m_episode 1",  "M_EPI1"},
       {it_runcmd, "the shores of hell",        "m_episode 2",  "M_EPI2"},
       {it_runcmd, "inferno!",                  "m_episode 3",  "M_EPI3"},
       {it_runcmd, "thy flesh consumed",        "m_episode 4",  "M_EPI4"},
       {it_end},
   },
   40, 30,              // x, y offsets
   2,                   // select episode 1
   mf_skullmenu,        // skull menu
};

        // console command to select episode
CONSOLE_COMMAND(m_episode, cf_notnet)
{
   if(!c_argc)
   {
        C_Printf("usage: episode <epinum>\n");
        return;
   }
   start_episode = atoi(c_argv[0]);
   M_StartMenu(&menu_newgame);
}


/************************
        NEW GAME MENU
 ************************/

menu_t menu_newgame =
{
  {
    {it_title,  "new game",             NULL,                   "M_NEWG"},
    {it_gap},
    {it_info,   "choose skill level",   NULL,                   "M_SKILL"},
    {it_gap},
    {it_runcmd, "i'm too young to die", "newgame 0",            "M_JKILL"},
    {it_runcmd, "hey, not too rough",   "newgame 1",            "M_ROUGH"},
    {it_runcmd, "hurt me plenty.",      "newgame 2",            "M_HURT"},
    {it_runcmd, "ultra-violence",       "newgame 3",            "M_ULTRA"},
    {it_runcmd, "nightmare!",           "newgame 4",            "M_NMARE"},
    {it_end},
  },
  40, 15,               // x,y offsets
  6,                    // starting item: hurt me plenty
  mf_skullmenu,         // is a skull menu
};

CONSOLE_COMMAND(newgame, cf_notnet)
{
  int skill = gameskill;

        // skill level is argv 0
  if(c_argc) skill = atoi(c_argv[0]);

  if(gamemode == commercial || modifiedgame)
           // start on newest level from wad
      G_DeferedInitNew(skill, startlevel);
  else
                // start on first level of selected episode
      G_DeferedInitNewNum(skill, start_episode, 1);

  M_ClearMenus();
}

/***********************
        FEATURES MENU
 ***********************/

menu_t menu_features =
{
  {
    {it_title,  FC_GOLD "features",     NULL,                   "M_FEAT"},
    {it_gap},
    {it_gap},
    {it_runcmd, "multiplayer",          "m_multi",              "M_MULTI"},
    {it_gap},
    {it_runcmd, "load wad",             "m_loadwad",            "M_WAD"},
    {it_gap},
    {it_runcmd, "demos",                "m_demos",              "M_DEMOS"},
    {it_gap},
    {it_runcmd, "about",                "m_about",              "M_ABOUT"},
    {it_end},
  },
  100, 15,                              // x,y
  3,                                    // start item
  mf_leftaligned | mf_skullmenu         // skull menu
};

CONSOLE_COMMAND(m_features, 0)
{
  M_StartMenu(&menu_features);
}

/************************
        MULTIPLAYER
 ************************/

menu_t menu_multiplayer =
{
  {
    {it_title,  FC_GOLD "multiplayer",  NULL,                   "M_MULTI"},
    {it_gap},
    {it_gap},
    {it_info,   FC_GOLD "connect:"},
    {it_runcmd, "serial/modem",         "m_serial"},
    {it_runcmd, "tcp/ip",               "m_tcpip"},
    {it_gap},
    {it_runcmd, "disconnect",           "disconnect"},
    {it_gap},
    {it_info,   FC_GOLD "setup"},
    {it_runcmd, "chat macros",          "m_chatmacros"},
    {it_runcmd, "player setup",         "m_player"},
    {it_end},
  },
  100, 15,                                      // x,y offsets
  4,                                            // starting item
  mf_background|mf_leftaligned,                 // fullscreen
};

CONSOLE_COMMAND(m_multi, 0)
{
    M_StartMenu(&menu_multiplayer);
}

/************************
        TCP/IP MENU
 ************************/

menu_t menu_tcpip =
{
  {
    {it_title,  FC_GOLD "TCP/IP",            NULL,           "M_TCPIP"},
    {it_gap},
    {it_info,   "not implemented yet."},
    {it_runcmd, "",                          "echo monkey boy"},
    {it_end},
  },
  180,15,                       // x,y offset
  3,
  mf_background,                // full-screen
};

CONSOLE_COMMAND(m_tcpip, 0)
{
     M_StartMenu(&menu_tcpip);
}

/**************************
        SERIAL/MODEM MENU
 **************************/

menu_t menu_serial =
{
  {
    {it_title,  FC_GOLD "Serial/modem",          NULL,           "M_SERIAL"},
    {it_gap},
    {it_info,           FC_GOLD "settings"},
    {it_toggle,         "com port to use",      "com"},
    {it_variable,       "phone number",         "m_phonenum"},
    {it_gap},
    {it_info,           FC_GOLD "connect:"},
    {it_runcmd,         "null modem link",      "nullmodem"},
    {it_runcmd,         "dial",                 "dial %m_phonenum"},
    {it_runcmd,         "wait for call",        "answer"},
    {it_end},
  },
  180,15,                       // x,y offset
  3,
  mf_background,                // fullscreen
};

CONSOLE_COMMAND(m_serial, 0)
{
     M_StartMenu(&menu_serial);
}

VARIABLE_STRING(m_phonenum,     NULL,           126);
CONSOLE_VARIABLE(m_phonenum,    m_phonenum,     0) {}

/************************
        CHAT MACROS MENU
 ************************/

menu_t menu_chatmacros =
{
  {
    {it_title,  FC_GOLD "chat macros",           NULL,           "M_CHATM"},
    {it_gap},
    {it_variable,       "0",            "chatmacro0"},
    {it_variable,       "1",            "chatmacro1"},
    {it_variable,       "2",            "chatmacro2"},
    {it_variable,       "3",            "chatmacro3"},
    {it_variable,       "4",            "chatmacro4"},
    {it_variable,       "5",            "chatmacro5"},
    {it_variable,       "6",            "chatmacro6"},
    {it_variable,       "7",            "chatmacro7"},
    {it_variable,       "8",            "chatmacro8"},
    {it_variable,       "9",            "chatmacro9"},
    {it_end}
  },
  20,5,                                 // x, y offset
  2,                                    // chatmacro0 at start
  mf_background,                        // full-screen

};

CONSOLE_COMMAND(m_chatmacros, 0)
{
   M_StartMenu(&menu_chatmacros);
}

/************************
        PLAYER SETUP
 ************************/

void M_PlayerDrawer();

menu_t menu_player =
{
  {
    {it_title,  FC_GOLD "player setup",           NULL,           "M_PLAYER"},
    {it_gap},
    {it_variable,       "player name",          "name"},
    {it_toggle,         "player colour",        "colour"},
    {it_toggle,         "player skin",          "skin"},
    {it_gap},
    {it_toggle,       "handedness",           "lefthanded"},
    {it_end}
  },
  150,5,                                // x, y offset
  2,                                    // chatmacro0 at start
  mf_background,                        // full-screen
  M_PlayerDrawer
};

#define SPRITEBOX_X 200
#define SPRITEBOX_Y 80

void M_PlayerDrawer()
{
     int lump;
     spritedef_t *sprdef;
     spriteframe_t *sprframe;
     patch_t *patch;

     V_DrawBox(SPRITEBOX_X, SPRITEBOX_Y, 80, 80);

     sprdef = &sprites[players[consoleplayer].skin->sprite];

     sprframe = &sprdef->spriteframes[0];
     lump = sprframe->lump[1];

     patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);

     V_DrawPatchTranslated (SPRITEBOX_X + 40, SPRITEBOX_Y + 70, 0, patch,
        players[displayplayer].colormap ?
        (char*)translationtables + 256*(players[displayplayer].colormap-1) :
                cr_red, -1);
}

CONSOLE_COMMAND(m_player, 0)
{
     M_StartMenu(&menu_player);
}

/************************
        OPTIONS MENU
 ************************/

menu_t menu_options =
{
  {
    {it_title,  FC_GOLD "options",              NULL,             "M_OPTTTL"},
    {it_gap},
    {it_info,   FC_GOLD "input"},
    {it_info,   FC_BRICK "key bindings",        "m_keybindings"},
    {it_runcmd, "mouse options",                "m_mouse"},
    {it_gap},
    {it_info,   FC_GOLD "output"},
    {it_runcmd, "video options",                "m_video"},
    {it_runcmd, "sound options",                "m_sound"},
    {it_gap},
    {it_info,   FC_GOLD "game options"},
    {it_info,   FC_BRICK "compatibility",       "m_compat"},
    {it_runcmd, "enemies",                      "m_enemies"},
    {it_runcmd, "weapons",                      "m_weapons"},
    {it_runcmd, "end game",                     "starttitle"},
    {it_gap},
    {it_info,   FC_GOLD "game widgets"},
    {it_runcmd, "hud settings",                 "m_hud"},
    {it_runcmd, "status bar",                   "m_status"},
    {it_info,   FC_BRICK"automap",              "m_automap"},
    {it_end},
  },
  100, 15,                              // x,y offsets
  4,                                    // starting item: first selectable
  mf_background|mf_leftaligned,         // draw background: not a skull menu
};

CONSOLE_COMMAND(m_options, 0)
{
    M_StartMenu(&menu_options);
}

/*********************
        VIDEO MODES
 *********************/

void M_VideoModeDrawer();

menu_t menu_video =
{
  {
      {it_title,        FC_GOLD "video",                NULL, "m_video"},
      {it_gap},
      {it_info,         FC_GOLD "mode"},
      {it_variable,     "video mode",                   "v_mode"},
      {it_gap},
      {it_toggle,       "wait for retrace",             "v_retrace"},
      {it_runcmd,       "test framerate..",             "timedemo demo2"},

      {it_gap},
      {it_info,         FC_GOLD "rendering"},
      {it_toggle,       "screen size",                  "screensize"},
      {it_toggle,       "hom detector flashes",         "r_homflash"},
      {it_toggle,       "translucency",                 "r_trans"},
      {it_variable,     "translucency percentage",      "r_tranpct"},
      {it_toggle,       "stretch sky",                  "r_stretchsky"},

      {it_gap},
      {it_info,         FC_GOLD "misc."},
      {it_toggle,       "\"loading\" disk icon",        "v_diskicon"},
      {it_toggle,       "screenshot format",            "shot_type"},
      {it_toggle,       "text mode startup",            "textmode_startup"},

      {it_end},
  },
  200, 15,              // x,y offset
  3,                    // start on first selectable
  mf_background,        // full-screen menu
  M_VideoModeDrawer
};

void M_VideoModeDrawer()
{
   WriteText
   (
      videomodes[v_mode].description,
      menu_video.menuitems[4].x - StringWidth(videomodes[v_mode].description),
      menu_video.menuitems[4].y
   );
}

CONSOLE_COMMAND(m_video, 0)
{
        M_StartMenu(&menu_video);
}

/***********************
                SOUND
 ***********************/

menu_t menu_sound =
{
   {
        {it_title,      FC_GOLD "sound",                NULL, "m_sound"},
        {it_gap},
        {it_info,       FC_GOLD "volume"},
        {it_toggle,     "sfx volume",                   "sfx_volume"},
        {it_toggle,     "music volume",                 "music_volume"},
        {it_gap},
        {it_info,       FC_GOLD "setup"},
        {it_toggle,     "sound card",                   "snd_card"},
        {it_toggle,     "music card",                   "mus_card"},
        {it_toggle,     "autodetect voices",            "detect_voices"},
        {it_toggle,     "sound channels",               "snd_channels"},
        {it_gap},
        {it_info,       FC_GOLD "misc"},
        {it_toggle,     "precache sounds",              "s_precache"},
        {it_toggle,     "pitched sounds",               "s_pitched"},
        {it_end},
   },
   180, 15,                     // x, y offset
   3,                           // first selectable
   mf_background,               // full-screen menu
};

CONSOLE_COMMAND(m_sound, 0)
{
   M_StartMenu(&menu_sound);
}

/***********************
                MOUSE
 ***********************/

menu_t menu_mouse =
{
  {
        {it_title,      FC_GOLD "mouse",                NULL,   "m_mouse"},
        {it_gap},
        {it_toggle,     "enable mouse",                 "use_mouse"},
        {it_gap},
        {it_info,       FC_GOLD "sensitivity"},
        {it_toggle,     "horizontal",                   "sens_horiz"},
        {it_toggle,     "vertical",                     "sens_vert"},
        {it_gap},
        {it_info,       FC_GOLD "misc."},
        {it_toggle,     "invert mouse",                 "invertmouse"},
        {it_toggle,     "always mouselook",             "alwaysmlook"},
        {it_toggle,     "enable joystick",              "use_joystick"},
        {it_end},
  },
  200, 15,                      // x, y offset
  2,                            // first selectable
  mf_background,                // full-screen menu
};

CONSOLE_COMMAND(m_mouse, 0)
{
   M_StartMenu(&menu_mouse);
}

/************************
          HUD SETTINGS
 ************************/

menu_t menu_hud =
{
   {
        {it_title,      FC_GOLD "hud settings",         NULL,      "m_hud"},
        {it_gap},
        {it_info,       FC_GOLD "hud messages"},
        {it_toggle,     "messages",                     "messages"},
        {it_toggle,     "message colour",               "mess_colour"},
        {it_toggle,     "messages scroll",              "mess_scrollup"},
        {it_toggle,     "message lines",                "mess_lines"},
        {it_variable,   "message time (ms)",            "mess_timer"},
        {it_toggle,     "obituaries",                   "obituaries"},
        {it_toggle,     "obituary colour",              "obcolour"},
        {it_gap},
        {it_info,       FC_GOLD "overlay"},
        {it_gap},
        {it_info,       FC_GOLD "misc."},
        {it_toggle,     "crosshair type",               "crosshair"},
        {it_toggle,     "show frags in DM",             "show_scores"},
        {it_end},
   },
   200, 15,                             // x,y offset
   3,
   mf_background,
};

CONSOLE_COMMAND(m_hud, 0)
{
    M_StartMenu(&menu_hud);
}

/***************************
        STATUS BAR SETTINGS
 ***************************/

menu_t menu_statusbar =
{
   {
        {it_title,      FC_GOLD "status bar",           NULL,           "m_stat"},
        {it_gap},
        {it_info,       FC_GOLD "status bar colours"},
        {it_variable,   "ammo ok percentage",           "ammo_yellow"},
        {it_variable,   "ammo low percentage",          "ammo_red"},
        {it_gap},
        {it_variable,   "armour high percentage",       "armor_green"},
        {it_variable,   "armour ok percentage",         "armor_yellow"},
        {it_variable,   "armour low percentage",        "armor_red"},
        {it_gap},
        {it_variable,   "health high percentage",       "health_green"},
        {it_variable,   "health ok percentage",         "health_yellow"},
        {it_variable,   "health low percentage",        "health_red"},
        {it_end},
   },
   200, 15,
   3,
   mf_background,
};

CONSOLE_COMMAND(m_status, 0)
{
   M_StartMenu(&menu_statusbar);
}

/************************
                WEAPONS
 ************************/

menu_t menu_weapons =
{
   {
      {it_title,      FC_GOLD "weapons",              NULL,        "m_weap"},
      {it_gap},
      {it_info,       FC_GOLD "weapon options"},
      {it_toggle,     "bfg type",                       "bfgtype"},
      {it_toggle,     "bobbing",                        "bobbing"},
      {it_toggle,     "recoil",                         "recoil"},
      {it_info,       FC_BRICK "fist/chainsaw switch"},
      {it_gap},
      {it_info,       FC_GOLD "weapon prefs."},
      {it_variable,   "1st choice",                     "weappref_1"},
      {it_variable,   "2nd choice",                     "weappref_2"},
      {it_variable,   "3rd choice",                     "weappref_3"},
      {it_variable,   "4th choice",                     "weappref_4"},
      {it_variable,   "5th choice",                     "weappref_5"},
      {it_variable,   "6th choice",                     "weappref_6"},
      {it_variable,   "7th choice",                     "weappref_7"},
      {it_variable,   "8th choice",                     "weappref_8"},
      {it_variable,   "9th choice",                     "weappref_9"},
      {it_end},
   },
   150, 15,                             // x,y offset
   3,                                   // starting item
   mf_background,                       // full screen
};

CONSOLE_COMMAND(m_weapons, 0)
{
   M_StartMenu(&menu_weapons);
}

/**************************
                  ENEMIES
 **************************/

menu_t menu_enemies =
{
    {
        {it_title,      FC_GOLD "enemies",              NULL,      "m_enem"},
        {it_gap},
        {it_info,       FC_GOLD "monster options"},
        {it_toggle,     "monsters remember target",     "mon_remember"},
        {it_toggle,     "monster infighting",           "mon_infight"},
        {it_toggle,     "monsters back out",            "mon_backing"},
        {it_toggle,     "monsters avoid hazards",       "mon_avoid"},
        {it_toggle,     "affected by friction",         "mon_friction"},
        {it_toggle,     "climb tall stairs",            "mon_climb"},
        {it_gap},
        {it_info,       FC_GOLD "mbf friend options"},
        {it_variable,   "friend distance",              "mon_distfriend"},
        {it_toggle,     "rescue dying friends",         "mon_helpfriends"},
        {it_end},
    },
    200,15,                             // x,y offset
    3,                                  // starting item
    mf_background                       // full screen
};

CONSOLE_COMMAND(m_enemies, 0)
{
    M_StartMenu(&menu_enemies);
}

void M_AddCommands()
{
   C_AddCommand(m_newgame);
   C_AddCommand(m_episode);

   C_AddCommand(m_features);

   C_AddCommand(m_multi);
     C_AddCommand(m_serial);
       C_AddCommand(m_phonenum);
     C_AddCommand(m_tcpip);
     C_AddCommand(m_chatmacros);
     C_AddCommand(m_player);

   C_AddCommand(m_options);
     C_AddCommand(m_mouse);
     C_AddCommand(m_video);
     C_AddCommand(m_sound);
     C_AddCommand(m_weapons);
     C_AddCommand(m_enemies);
     C_AddCommand(m_hud);
     C_AddCommand(m_status);

   C_AddCommand(newgame);
}


/////////////////////////////
//
// M_DrawBackground tiles a 64x64 patch over the entire screen, providing the
// background for the Help and Setup screens.
//
// killough 11/98: rewritten to support hires

char *R_DistortedFlat(int);

void M_DrawBackground(char* patchname, byte *back_dest)
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
void M_DrawDistortedBackground(char* patchname, byte *back_dest)
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

void M_DrawCredits(void)     // killough 10/98: credit screen
{
  inhelpscreens = true;

        // sf: altered for SMMU

  M_DrawDistortedBackground(gamemode==commercial ? "SLIME05" : "LAVA1",
                                screens[0]);

        // sf: SMMU credits
  WriteText(
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

