/******************************* console **********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Handler functions
//
// All the commands have a handler function which is called with the 
// arguments typed at the console when a command is executed. Some
// variables also have handler functions which call functions in the
// game to implement the effects of the change in the variable.
//

/* includes *********************/

#include <stdio.h>

#include "c_io.h"
#include "c_runcmd.h"
#include "c_cmdlst.h"
#include "c_net.h"

#include "doomdef.h"
#include "doomstat.h"
#include "d_deh.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_video.h"
#include "m_cheat.h"
#include "m_random.h"
#include "p_enemy.h"
#include "p_chase.h"
#include "p_inter.h"
#include "p_map.h"
#include "p_mobj.h"
#include "p_spec.h"
#include "r_Draw.h"
#include "r_main.h"
#include "sounds.h"
#include "s_sound.h"
#include "ser_main.h"
#include "t_script.h"
#include "w_wad.h"

/* functions ********************/

/* Map */

void C_Map()
{
        if(!c_argc)
        {
                C_Printf("usage: map <mapname>\n");
                return;
        }

        G_StopDemo();

        // check for .wad files
        // i'm not particularly a fan of this myself, but..

        if(strlen(c_argv[0]) > 4)
        {
                char *extension;
                extension = c_argv[0] + strlen(c_argv[0]) - 4;
                if(!strcmp(extension, ".wad"))
                {
                        if(D_AddNewFile(c_argv[0]))
                        {
                                G_InitNew(gameskill, startlevel);
                        }
                        return;
                }
        }

        G_InitNew(gameskill, c_argv[0]);
}

void C_Skill()
{
        startskill = gameskill = atoi(c_argv[0]);
        if(cmdsrc == consoleplayer)
                defaultskill = gameskill + 1;
}

/* game/player stuff */

        // cent_re_msg
void C_Centremsg()
{
        S_StartSound(NULL,sfx_tink);
        HU_centremsg(c_args);
}

void C_Playermsg()
{
        S_StartSound(NULL,sfx_tink);
        dprintf(c_args);
}

void C_ResetNet()
{
        ResetNet();
}

void C_Addfile()
{
        D_AddNewFile(c_argv[0]);
}

/* multiplayer */

void C_Playername()
{
        int playernum;

        playernum = cmdsrc;

        strncpy(players[playernum].name, c_argv[0], 18);
        if(playernum == consoleplayer)
        {
                free(default_name);
                default_name = strdup(c_argv[0]);
        }
}

void C_Playercolour()
{
        int playernum, colour;

        playernum = cmdsrc;
        colour = atoi(c_argv[0]) % TRANSLATIONCOLOURS;

        players[playernum].colormap = colour;
        if(gamestate == GS_LEVEL)
                players[playernum].mo->colour = colour;

        if(playernum == consoleplayer) default_colour = colour; // typed
}

void C_Kill()
{
        mobj_t *mobj;
        int playernum;

        playernum = cmdsrc;

        mobj = players[playernum].mo;
        P_DamageMobj(mobj, NULL, mobj,
        2*(players[playernum].health+players[playernum].armorpoints) );
        mobj->momx = mobj->momy = mobj->momz = 0;
}

/* Console stuff */
void C_Echo()
{
        C_Puts(c_args);
}


void C_TestUpdate()
{
        int i;

        for(i=0; i<100; i++)
        {
                C_Printf("%i\n", i);
                C_Update();
        }
}

void C_ExitLevel()
{
        G_ExitLevel();
}


        // spawn an object: x, y, type, [angle]
void C_Spawn()
{
        mapthing_t spawnthing;

        if(c_argc < 3)
        {
                C_Printf("spawn an object: spawn x y type\n");
                return;
        }

        spawnthing.x = atoi(c_argv[0]);
        spawnthing.y = atoi(c_argv[1]);
        spawnthing.type = atoi(c_argv[2]);
        spawnthing.angle = c_argc>3 ? atoi(c_argv[3]) : 0;
                        // always spawn
        spawnthing.options = MTF_EASY | MTF_NORMAL | MTF_HARD;

        P_SpawnMapThing(&spawnthing);
}

void C_GUITest()
{
}

        // delay for a few tics
void C_Delay()
{
        int tics;

        if(c_argc) tics = atoi(c_argv[0]);
        else tics = 1;

        C_BufferDelay(cmdtype, tics);
}

        // flood the console with crap
void C_Flood()
{
        int a;

        for(a=0; a<300; a++)
                C_Printf("%c\n", a%64 + 32);
}

void C_LineTrigger()
{
        line_t junk;

        if(!c_argc)
        {
                C_Printf("usage: linetrigger <line type> [sector tag]\n");
                return;
        }

        junk.special = atoi(c_argv[0]);
        junk.tag = c_argc == 1 ? 0 : atoi(c_argv[1]);

        if (!P_UseSpecialLine(t_trigger, &junk, 0))    // Try using it
            P_CrossSpecialLine(&junk, 0, t_trigger);   // Try crossing it
}

