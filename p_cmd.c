/******************************* console **********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
//      Game console variables 
//

/* includes ************************/

#include <stdio.h>

#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "doomstat.h"

#include "d_main.h"
#include "f_wipe.h"
#include "g_game.h"
#include "m_random.h"
#include "p_info.h"
#include "p_mobj.h"
#include "p_inter.h"
#include "p_spec.h"
#include "r_draw.h"

/***************************************************************************
                'defines': string values for variables
***************************************************************************/

char *yesno[]={"no","yes"};
char *onoff[]={"off","on"};

char *colournames[]= {"green","indigo","brown","red","pink","tomato","cream",
                      "white","dirt","blue","vomit", "black"};
char *textcolours[]={
        FC_BRICK  "brick" FC_RED,
        FC_TAN    "tan"   FC_RED,
        FC_GRAY   "gray"  FC_RED,
        FC_GREEN  "green" FC_RED,
        FC_BROWN  "brown" FC_RED,
        FC_GOLD   "gold"  FC_RED,
        FC_RED    "red"   FC_RED,
        FC_BLUE   "blue"  FC_RED,
        FC_ORANGE "orange"FC_RED,
        FC_YELLOW "yellow"FC_RED
};
char *skills[]=
{"im too young to die", "hey not too rough", "hurt me plenty",
 "ultra violence", "nightmare"};
char *bfgtypestr[3]= {"bfg9000", "classic", "bfg11k"};
char *dmstr[] = {"co-op", "dm", "altdeath", "trideath"};

// version hack

char *verdate_hack = (char*)version_date;
char *vername_hack = (char*)version_name;
extern char *cmdoptions;

/*************************************************************************
        Constants
 *************************************************************************/

CONST_STRING(info_creator);
CONSOLE_CONST(creator, info_creator);

CONST_INT(VERSION);
CONSOLE_CONST(version, VERSION);

CONST_STRING(verdate_hack);
CONSOLE_CONST(ver_date, verdate_hack);

CONST_STRING(vername_hack);
CONSOLE_CONST(ver_name, vername_hack);

/*************************************************************************
                Game variables
 *************************************************************************/

        // player colour

VARIABLE_INT(default_colour, NULL, 0, TRANSLATIONCOLOURS-1, colournames);
CONSOLE_NETVAR(colour, default_colour, cf_handlerset, netcmd_colour)
{
        int playernum, colour;

        playernum = cmdsrc;
        colour = atoi(c_argv[0]) % TRANSLATIONCOLOURS;

        players[playernum].colormap = colour;
        if(gamestate == GS_LEVEL)
                players[playernum].mo->colour = colour;

        if(playernum == consoleplayer) default_colour = colour; // typed
}

        // deathmatch

VARIABLE_INT(deathmatch, NULL,                  0, 3, dmstr);
CONSOLE_NETVAR(deathmatch, deathmatch, cf_server, netcmd_deathmatch) {}

        // skill level

VARIABLE_INT(gameskill, &defaultskill,          0, 4, skills);
CONSOLE_NETVAR(skill, gameskill, cf_server, netcmd_skill)
{
     startskill = gameskill = atoi(c_argv[0]);
     if(cmdsrc == consoleplayer)
          defaultskill = gameskill + 1;
}

        // allow mlook

VARIABLE_BOOLEAN(allowmlook,NULL,                   onoff);
CONSOLE_NETVAR(allowmlook, allowmlook, cf_server, netcmd_allowmlook) {}

        // bfg type

VARIABLE_INT(bfgtype, &default_bfgtype,         0, 2, bfgtypestr);
CONSOLE_NETVAR(bfgtype, bfgtype, cf_server, netcmd_bfgtype) {}

        // autoaiming 

VARIABLE_BOOLEAN(autoaim, &default_autoaim,         onoff);
CONSOLE_NETVAR(autoaim, autoaim, cf_server, netcmd_autoaim) {}

        // weapons recoil 

VARIABLE_BOOLEAN(weapon_recoil, &default_weapon_recoil, onoff);
CONSOLE_NETVAR(recoil, weapon_recoil, cf_server, netcmd_recoil) {}

        // allow pushers

VARIABLE_BOOLEAN(allow_pushers, &default_allow_pushers, onoff);
CONSOLE_NETVAR(pushers, allow_pushers, cf_server, netcmd_pushers) {}

        // varying friction

VARIABLE_BOOLEAN(variable_friction, &default_variable_friction, onoff);
CONSOLE_NETVAR(varfriction, variable_friction, cf_server, netcmd_varfriction) {}

        // weapon changing speed

VARIABLE_INT(weapon_speed, &default_weapon_speed, 1, 200, NULL);
CONSOLE_NETVAR(weapspeed, weapon_speed, cf_server, netcmd_weapspeed) {}

        // allow mlook with bfg

VARIABLE_INT(bfglook,   NULL,                   0, 2, NULL);
CONSOLE_NETVAR(bfglook, bfglook, cf_server, netcmd_bfglook) {}

  /************** monster variables ***********/

        // fast monsters

VARIABLE_BOOLEAN(fastparm, NULL,                    onoff);
CONSOLE_NETVAR(fast, fastparm, cf_server, netcmd_fast) {}

        // no monsters

VARIABLE_BOOLEAN(nomonsters, NULL,                  onoff);
CONSOLE_NETVAR(nomonsters, nomonsters, cf_server, netcmd_nomonsters) { }

        // respawning monsters

VARIABLE_BOOLEAN(respawnparm, NULL,                 onoff);
CONSOLE_NETVAR(respawn, respawnparm, cf_server, netcmd_respawn) {}

        // monsters remember

VARIABLE_BOOLEAN(monsters_remember, &default_monsters_remember, onoff);
CONSOLE_NETVAR(mon_remember, monsters_remember, cf_server, netcmd_monremember) {}

        // infighting among monsters

VARIABLE_BOOLEAN(monster_infighting, &default_monster_infighting, onoff);
CONSOLE_NETVAR(mon_infight, monster_infighting, cf_server, netcmd_moninfight) {}

        // monsters backing out

VARIABLE_BOOLEAN(monster_backing, &default_monster_backing, onoff);
CONSOLE_NETVAR(mon_backing, monster_backing, cf_server, netcmd_monbacking) {}

        // monsters avoid hazards

VARIABLE_BOOLEAN(monster_avoid_hazards, &default_monster_avoid_hazards, onoff);
CONSOLE_NETVAR(mon_avoid, monster_avoid_hazards, cf_server, netcmd_monavoid) {}

        // monsters affected by friction

VARIABLE_BOOLEAN(monster_friction, &default_monster_friction, onoff);
CONSOLE_NETVAR(mon_friction, monster_friction, cf_server, netcmd_monfriction) {}

        // monsters climb tall steps

VARIABLE_BOOLEAN(monkeys, &default_monkeys,         onoff);
CONSOLE_NETVAR(mon_climb, monkeys, cf_server, netcmd_monclimb) {}

        // help dying friends

VARIABLE_BOOLEAN(help_friends, &default_help_friends, onoff);
CONSOLE_NETVAR(mon_helpfriends, help_friends, cf_server, netcmd_monhelpfriends) {}

        // distance friends keep from player

VARIABLE_INT(distfriend, &default_distfriend,   0, 1024, NULL);
CONSOLE_NETVAR(mon_distfriend, distfriend, cf_server, netcmd_mondistfriend) {}

    /************ console control **********/
        // these should be in c_??.c

        // command aliases

CONSOLE_COMMAND(alias, 0)
{
        alias_t *alias;
        char *temp;

        if(!c_argc)
        {
                // list em
                C_Printf(FC_GRAY"alias list:" FC_RED "\n\n");
                alias = aliases;
                while(alias->name)
                {
                        C_Printf("\"%s\": \"%s\"\n", alias->name,
					alias->command);
                        alias++;
                }
                if(alias==aliases) C_Printf("(empty)\n");
                return;
        }

        if(c_argc == 1)  // only one, remove alias
        {
                C_RemoveAlias(c_argv[0]);
                return;
        }

       // find it or make a new one

        temp = c_args + strlen(c_argv[0]);
        while(*temp == ' ') temp++;

        C_NewAlias(c_argv[0], temp);
}

        // %opt for aliases
CONST_STRING(cmdoptions);
CONSOLE_CONST(opt, cmdoptions);

        // command list
CONSOLE_COMMAND(cmdlist, 0)
{
        int numonline = 0;
        command_t *current;
        int i;
        int charnum;

                // list each command from the hash chains

                //  5/8/99 change: use hash table and 
                //  alphabetical order by first letter
        for(charnum=33; charnum < 'z'; charnum++)
          for(i=0; i<CMDCHAINS; i++)
            for(current = cmdroots[i]; current; current = current->next)
            {
              if(current->name[0]==charnum && !(current->flags & cf_hidden))
              {
                 C_Printf("%s ", current->name);
                 numonline++;
                 if(numonline >= 3)
                 {
                   numonline = 0;
                   C_Printf("\n");
                 }
               }
            }
        C_Printf("\n");
}

        // console height
VARIABLE_INT(c_height,  NULL,                   20, 200, NULL);
CONSOLE_VARIABLE(c_height, c_height, 0) {}

        // console speed
VARIABLE_INT(c_speed,   NULL,                   1, 200, NULL);
CONSOLE_VARIABLE(c_speed, c_speed, 0) {}

        // echo string to console
CONSOLE_COMMAND(echo, 0)
{
        C_Puts(c_args);
}

        // delay in console
CONSOLE_COMMAND(delay, 0)
{
        int tics;

        if(c_argc) tics = atoi(c_argv[0]);
        else tics = 1;

        C_BufferDelay(cmdtype, tics);
}

        // flood the console with crap
CONSOLE_COMMAND(flood, 0)
{
        int a;

        for(a=0; a<300; a++)
                C_Printf("%c\n", a%64 + 32);
}

void P_Chase_AddCommands();

void P_AddCommands()
{
        C_AddCommand(creator);
        C_AddCommand(version);
        C_AddCommand(ver_date);
        C_AddCommand(ver_name);
        C_AddCommand(colour);
        C_AddCommand(deathmatch);
        C_AddCommand(skill);
        C_AddCommand(allowmlook);
        C_AddCommand(bfgtype);
        C_AddCommand(autoaim);
        C_AddCommand(recoil);
        C_AddCommand(pushers);
        C_AddCommand(varfriction);
        C_AddCommand(weapspeed);
        C_AddCommand(bfglook);

        C_AddCommand(fast);
        C_AddCommand(nomonsters);
        C_AddCommand(respawn);
        C_AddCommand(mon_remember);
        C_AddCommand(mon_infight);
        C_AddCommand(mon_backing);
        C_AddCommand(mon_avoid);
        C_AddCommand(mon_friction);
        C_AddCommand(mon_climb);
        C_AddCommand(mon_helpfriends);
        C_AddCommand(mon_distfriend);

        C_AddCommand(alias);
        C_AddCommand(opt);
        C_AddCommand(cmdlist);
        C_AddCommand(c_height);
        C_AddCommand(c_speed);
        C_AddCommand(echo);
        C_AddCommand(delay);
        C_AddCommand(flood);

        P_Chase_AddCommands();
}
