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

// video modes

typedef struct
{
        int hires;
        int pageflip;
        int vesa;
        char *description;
} videomode_t;

videomode_t videomodes[]=
{
        {0,0,0,"320x200 vga"},
        {0,1,0,"320x200 vga (pageflipped)"},
        {0,0,1,"320x200 vesa"},
        {1,0,0,"640x400 autodetect"},
        {1,1,0,"640x400 autodetect (pageflipped)"},
        {1,0,1,"640x400 vesa"},
        {1,1,1,"640x400 vesa (pageflipped)"},
        {-1,-1,-1}
};

void C_VidModeList()
{
        videomode_t* videomode=videomodes;
        while(videomode->hires!=-1)
        {
                C_Printf("%i: %s\n",(int)(videomode-videomodes),
                                        videomode->description);
                videomode++;
        }
}

int NumModes()
{
        int count=0;

        while(videomodes[count].hires!=-1)
                 count++;

        return count;
}

void C_VidMode()
{
        int modenum = atoi(c_argv[0]);

        if(modenum>=NumModes() || modenum<0)
        {
                C_Printf("invalid mode\n");
                return;
        }

        hires = videomodes[modenum].hires;
        page_flip = videomodes[modenum].pageflip;
        vesamode = videomodes[modenum].vesa;
        I_ResetScreen();
}

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

/* gamemode */

void C_Consolemode()
{
        C_SetConsole();
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

void C_PlayDemo()
{
        if(W_CheckNumForName(c_argv[0])==-1)
        {
                C_Printf("%s not found\n",c_argv[0]);
                return;
        }
        G_DeferedPlayDemo(c_argv[0]);
	singledemo = true;            // quit after one demo
}

void C_TimeDemo()
{
        G_TimeDemo(c_argv[0]);
}

void C_Addfile()
{
        D_AddNewFile(c_argv[0]);
}

/* multiplayer */
void C__Chat()
{
        S_StartSound(0, gamemode == commercial ? sfx_radio : sfx_tink);
        dprintf(FC_GRAY"%s:"FC_RED" %s", players[cmdsrc].name, c_args);
}

void C_Playername()
{
        int playernum;

        playernum=cmdsrc;

        strncpy(players[playernum].name, c_argv[0], 18);
        if(playernum==consoleplayer)
        {
                free(default_name);
                default_name=strdup(c_argv[0]);
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
        P_DamageMobj(mobj, mobj, mobj,
        2*(players[playernum].health+players[playernum].armorpoints) );
        mobj->momx = mobj->momy = mobj->momz = 0;
}

void C_Disconnect()
{
        D_QuitNetGame();
        C_SetConsole();
}

        // player info
void C_Players()
{
        int i;

        for(i=0;i<MAXPLAYERS;i++)
           if(playeringame[i])
               C_Printf("%i: %s\n",i, players[i].name);
}

/* Console stuff */

void C_Cmdlist()
{
        command_t *current=commands;
        int count=0;
        char tempstr[100]="";

        while(current->type!=ct_end)
        {
                if(*current->name!='_')
                {
                        sprintf(tempstr,"%s%s ",tempstr,current->name);
                        count++;
                        if(count>=3)
                        {
                                C_Puts(tempstr);
                                count=0;
                                tempstr[0]=0;
                        }
                }
                current++;
        }
        C_Puts(tempstr);
}

void C_Echo()
{
        C_Puts(c_args);
}

/* Handle aliases */

void C_RemoveAlias(char *aliasname)
{
      alias_t *alias;

      alias = C_GetAlias(aliasname);
      if(!alias)
      {
            C_Printf("unknown alias \"%s\"\n", aliasname);
            return;
      }
      free(alias->name); free(alias->command);
      while(alias->name)
      {
            memcpy(alias, alias+1, sizeof(alias_t));
            alias++;
      }
}

        // create a new alias, or use one that already exists

alias_t *C_NewAlias(unsigned char *aliasname, unsigned char *command)
{
        alias_t *alias;

        alias=aliases;

        while(alias->name)
        {
                if(!strcmp(alias->name, c_argv[0]))
                {
                        free(alias->name);
                        free(alias->command);
                        break;
                }
                alias++;
        }

        alias->name = strdup(aliasname);
        alias->command = strdup(command);

        return alias;
}


void C_Alias()
{
        alias_t *alias;
        char *temp;

        if(!c_argc)
        {
                // list em
                C_Printf(FC_GRAY"alias list:" FC_RED "\n\n");
                alias=aliases;
                while(alias->name)
                {
                        C_Printf("\"%s\": \"%s\"\n", alias->name,
					alias->command);
                        alias++;
                }
                if(alias == aliases) C_Printf("(empty)\n");
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

void C_TestUpdate()
{
        int i;

        for(i=0; i<100; i++)
        {
                C_Printf("%i\n", i);
                C_Update();
        }
}

/* serial comms */

extern connect_t connectmode;
extern char phonenum[50];

void C_NullModem()
{
        connectmode = CONNECT;
        ser_Start();
}

void C_Dial()
{
        connectmode = DIAL;
        strcpy(phonenum, c_argv[0]);
        ser_Start();
}

void C_Answer()
{
        connectmode = ANSWER;
        ser_Start();
}

void C_ExitLevel()
{
        G_ExitLevel();
}

void C_Psprites()
{
        default_psprites = atoi(c_argv[0]);
        if(gamestate==GS_LEVEL) showpsprites=default_psprites;
}

void C_AnimShot()
{
        if(!c_argc)
        {
                C_Printf(
			"animated screenshot.\n"
                	"usage: animshot frames\n");
                return;
        }
        animscreenshot = atoi(c_argv[0]);
        consoleactive = 0;
}


/********** cheats ************/

         // sf: moved cheats here from m_cheat.c
void C_CheatGod()
{
             
  int value=0;          // sf: choose to set to 0 or 1 
  if(c_argc)
    sscanf(c_argv[0], "%i", &value);
  else
    value = !(players[consoleplayer].cheats & CF_GODMODE);

  players[consoleplayer].cheats &= ~CF_GODMODE;
  players[consoleplayer].cheats |= value ? CF_GODMODE : 0;

  if (players[consoleplayer].cheats & CF_GODMODE)
    {
      if (players[consoleplayer].mo)
        players[consoleplayer].mo->health = god_health;  // Ty 03/09/98 - deh
          
      players[consoleplayer].health = god_health;
      dprintf(s_STSTR_DQDON); // Ty 03/27/98 - externalized
    }
  else 
      dprintf(s_STSTR_DQDOFF); // Ty 03/27/98 - externalized
}

  // no clipping mode cheat

void C_CheatNoClip()
{
  int value=0;
  if(c_argc)
    sscanf(c_argv[0], "%i", &value);
  else
    value = !(players[consoleplayer].cheats & CF_NOCLIP);

  players[consoleplayer].cheats &= ~CF_NOCLIP;
  players[consoleplayer].cheats |= value ? CF_NOCLIP : 0;

    dprintf( players[consoleplayer].cheats & CF_NOCLIP ?
    s_STSTR_NCON : s_STSTR_NCOFF); // Ty 03/27/98 - externalized
}

void C_CheatNuke()
{
  // jff 02/01/98 'em' cheat - kill all monsters
  // partially taken from Chi's .46 port
  //
  // killough 2/7/98: cleaned up code and changed to use dprintf;
  // fixed lost soul bug (LSs left behind when PEs are killed)

  int killcount=0;
  thinker_t *currentthinker=&thinkercap;
  extern void A_PainDie(mobj_t *);
  // killough 7/20/98: kill friendly monsters only if no others to kill
  int mask = MF_FRIEND;

  if(debugfile) fprintf(debugfile,"do massacre\n");

  do
    while ((currentthinker=currentthinker->next)!=&thinkercap)
      if (currentthinker->function == P_MobjThinker &&
	  !(((mobj_t *) currentthinker)->flags & mask) && // killough 7/20/98
	  (((mobj_t *) currentthinker)->flags & MF_COUNTKILL ||
	   ((mobj_t *) currentthinker)->type == MT_SKULL))
	{ // killough 3/6/98: kill even if PE is dead
	  if (((mobj_t *) currentthinker)->health > 0)
	    {
	      killcount++;
	      P_DamageMobj((mobj_t *) currentthinker, NULL, NULL, 10000);
	    }
	  if (((mobj_t *) currentthinker)->type == MT_PAIN)
	    {
	      A_PainDie((mobj_t *) currentthinker);    // killough 2/8/98
	      P_SetMobjState((mobj_t *) currentthinker, S_PAIN_DIE6);
	    }
	}
  while (!killcount && mask ? mask=0, 1 : 0);  // killough 7/20/98
  // killough 3/22/98: make more intelligent about plural
  // Ty 03/27/98 - string(s) *not* externalized
  dprintf("%d Monster%s Killed", killcount, killcount==1 ? "" : "s");
  if(debugfile) fprintf(debugfile,"done massacre\n");
}

        // hom detection
void C_HomDetect()
{
  extern int autodetect_hom;           // Ty 03/27/98 - *not* externalized

  int value=0;            // sf:
  if(c_argc)
    sscanf(c_argv[0], "%i", &value);
  else
    value = !autodetect_hom;

  autodetect_hom = !!value;

  dprintf( autodetect_hom ? "HOM Detection On" : "HOM Detection Off");
}

extern boolean sendpause;

void C_Pause()
{
        sendpause = true;
}

extern int screenSize;

void C_ScreenSize()
{
        screenSize = atoi(c_argv[0]);
        screenblocks = screenSize + 3;

        if(gamestate == GS_LEVEL) // not in intercam
                R_SetViewSize (screenblocks);
}

void C_StopDemo()
{
        G_StopDemo();
}

void C_Say()
{
        C_SendCmd(CN_BROADCAST, C_GetCmdForName("_cm")->netcmd, c_args);
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

void C_ViewDir()
{
        viewdir = atoi(c_argv[0]);

        viewangleoffset = viewdir == 1 ? ANG45 :
                          viewdir == 2 ? -ANG45 : 0;
}

void C_GUITest()
{
        char tempstr[100] = "/doom2/";

        C_Printf("debug option\n"); return;

        gui_fg_color = 0; gui_bg_color = 4;
        gui_mg_color = 0;

        file_select("Select a WAD", tempstr, "wad");
}

void C_Error()
{
        I_Error(c_args);
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

void C_QuitGame()
{
        exit(0);
}

void C_Kick()
{
        if(!c_argc)
        {
                C_Printf("usage: kick <playernum>\n"
                         " use playerinfo to find playernum\n");
                return;
        }
        D_KickPlayer(atoi(c_argv[0]));
}
