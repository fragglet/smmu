/****************************** level info *********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Level info.
//
// Under smmu, level info is stored in the level marker: ie. "mapxx"
// or "exmx" lump. This contains new info such as: the level name, music
// lump to be played, par time etc.
//

/* includes ************************/

#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "doomdef.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "w_wad.h"
#include "p_setup.h"
#include "p_info.h"
#include "p_mobj.h"
#include "t_script.h"
#include "z_zone.h"

char *info_interpic;
char *info_levelname;
int info_partime;
char *info_music;
char *info_skyname;
char *info_creator="unknown";
char *info_levelpic;
char *info_nextlevel;
char *info_intertext;
char *info_backdrop;
char *info_weapons;

static int reading_script = -1;        // the current script being read into

static void P_RemoveEqualses(char *line);
static void P_RemoveComments(char *line);

void P_ParseInfoCmd(char *line);
void P_ParseLevelVar(char *cmd);
void P_ParseInfoCmd(char *line);
void P_ParseLevelCmd(char *line);
void P_ClearLevelVars();
void P_ParseInterText(char *line);
void P_InitWeapons();

enum
{
        RT_LEVELINFO,
        RT_CONSOLECMD,
        RT_OTHER,
        RT_LEVELCMD,
        RT_INTERTEXT
} readtype;

void P_LoadLevelInfo(int lumpnum)
{
        char *lump;
        char *rover;
        char *startofline;

        readtype = RT_OTHER;
        P_ClearLevelVars();

        lump = W_CacheLumpNum(lumpnum,PU_STATIC);

        rover = startofline = lump;

        while(rover < lump+lumpinfo[lumpnum]->size)
        {
            if(*rover == '\n') // end of line
            {
                *rover = 0;               // make it an end of string (0)
                P_ParseInfoCmd(startofline);
                startofline = rover+1; // next line
                *rover = '\n';            // back to end of line
            }
            rover++;
        }
        free(lump);

        P_InitWeapons();
}

void P_ParseInfoCmd(char *line)
{
        P_CleanLine(line);
        while(*line == ' ') line++;
        if(!*line) return;

        if((line[0] == '/' && line[1] == '/') ||            // comment
            line[0] == '#' || line[0] == ';') return;

        if(*line == '[')                // a new section seperator
        {
                line++;
                if(!strncmp(line, "level info", 10))
                        readtype = RT_LEVELINFO;
                if(!strncmp(line, "console commands", 16))
                        readtype = RT_CONSOLECMD;
                if(!strncmp(line, "level commands", 14))
                        readtype = RT_LEVELCMD;
                if(!strncmp(line, "intertext", 9))
                        readtype = RT_INTERTEXT;
                return;
        }

        switch(readtype)
        {
              case RT_LEVELINFO:
              P_ParseLevelVar(line);
              break;

              case RT_CONSOLECMD:
              cmdtype = c_script; // consider it as a script
              C_RunTextCmd(line);
              break;

              case RT_LEVELCMD:
              P_ParseLevelCmd(line);
              break;

              case RT_INTERTEXT:
              P_ParseInterText(line);
              break;

              case RT_OTHER:
              break;
        }
}

//
//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//

enum
{
        IVT_STRING,
        IVT_INT,
        IVT_END
};

typedef struct
{
        int type;
        char *name;
        void *variable;
} levelvar_t;

levelvar_t levelvars[]=
{
        {IVT_STRING,    "levelpic",     &info_levelpic},
        {IVT_STRING,    "levelname",    &info_levelname},
        {IVT_INT,       "partime",      &info_partime},
        {IVT_STRING,    "music",        &info_music},
        {IVT_STRING,    "skyname",      &info_skyname},
        {IVT_STRING,    "creator",      &info_creator},
        {IVT_STRING,    "interpic",     &info_interpic},
        {IVT_STRING,    "nextlevel",    &info_nextlevel},
        {IVT_INT,       "gravity",      &gravity},
        {IVT_STRING,    "inter-backdrop",&info_backdrop},
        {IVT_STRING,    "defaultweapons",&info_weapons},
        {IVT_END,       0,              0}
};

void P_ParseLevelVar(char *cmd)
{
        char varname[50];
        char *equals;
        levelvar_t* current;

        if(!*cmd) return;

        P_RemoveEqualses(cmd);

        // right, first find the variable name

        sscanf(cmd, "%s", varname);

                // find what it equals
        equals = cmd+strlen(varname);
        while(*equals == ' ') equals++; // cut off the leading spaces

        current = levelvars;

        while(current->type != IVT_END)
        {
                if(!strcmp(current->name, varname))
                {
                  switch(current->type)
                  {
                     case IVT_STRING:
                     *(char**)current->variable         // +5 for safety
                        = Z_Malloc(strlen(equals)+5, PU_LEVEL, NULL);
                     strcpy(*(char**)current->variable, equals);
                     break;

                     case IVT_INT:
                     *(int*)current->variable = atoi(equals);
                     break;
                  }
                }
                current++;
        }
}

// clear all the level variables so that none are left over from a
// previous level

void P_ClearLevelVars()
{
        info_levelname = info_skyname = info_levelpic = "";
        info_music = gamemode == commercial ? "runnin" : "e1m1";
        info_creator = "unknown";
        info_interpic = "INTERPIC";
        info_partime = -1;

        if(gamemode == commercial && isExMy(levelmapname))
        {
                static char nextlevel[10];
                info_nextlevel = nextlevel;

                        // set the next episode
                strcpy(nextlevel, levelmapname);
                nextlevel[3] ++;
                if(nextlevel[3] > '9')  // next episode
                {
                        nextlevel[3] = '1';
                        nextlevel[1] ++;
                }

                info_music = levelmapname;
        }
        else 
                info_nextlevel = "";

        info_weapons = "";
        gravity = FRACUNIT;     // default gravity
        info_intertext = info_backdrop = NULL;

        T_ClearScripts();
        reading_script = -1; // reset script
}

//
// Level Commands: level commands are console commands called when a
// player crosses a linedef of a particular type. The particular console
// commands executed depend on the sector tag number which is used to
// link it to a level command
//
//  Take the form:
//
//      startscript <n>
//              [command]
//              [command]; [command] etc..
//      endscript

/*                      // old script code
        int cmdnum=-1;

        P_RemoveEqualses(line);

        sscanf(line, "%i", &cmdnum);
        if(cmdnum < 0) return;

        info_levelcmd[cmdnum] = Z_Malloc(strlen(line)+5, PU_LEVEL, NULL);
        strcpy(info_levelcmd[cmdnum], line+intstrlen(cmdnum));
*/

int intstrlen(int value);

void P_ParseLevelCmd(char *line)
{
        if(!strncmp(line, "endscript", 9))
        {
                reading_script = -1; // end of script
        }

        if(reading_script != -1)
        {
                int allocsize;

                allocsize =             // 10 for comfort
                       strlen(line) + strlen(scripts[reading_script]) + 10;

                        // realloc the script bigger
                scripts[reading_script] =
                realloc(scripts[reading_script], allocsize);

                      // add the new line to the current data using sprintf
                sprintf(scripts[reading_script], "%s%s\n",
                        scripts[reading_script], line);
        }
        else
        if(!strncmp(line, "startscript", 11))   // start of script
        {
                        // find script number
                reading_script = atoi(line + 11);

                        // alloc a small bit of memory
                scripts[reading_script] = malloc(5);
                        // make an empty string
                *scripts[reading_script] = 0;
        }
}

int intstrlen(int value)
{
        char tempstr[50];

        sprintf(tempstr, "%i", value);

        return strlen(tempstr);
}

void P_CleanLine(char *line)
{
        char *temp;

        for(temp=line; *temp; temp++)
                *temp = *temp<32 ? ' ' : tolower(*temp);

        temp = line+strlen(line)-1;

        while(*temp == ' ')
        {
                *temp = 0;
                temp--;
        }
}

static void P_RemoveComments(char *line)
{
        char *temp = line;

        while(*temp)
        {
                if(*temp=='/' && *(temp+1)=='/')
                {
                        *temp = 0; return;
                }
                temp++;
        }
}

static void P_RemoveEqualses(char *line)
{
        char *temp;

        temp = line;

        while(*temp)
        {
                if(*temp == '=')
                {
                    *temp = ' ';
                }
                temp++;
        }
}

        // dumbass fixed-length intertext size
#define INTERTEXTSIZE 1024

void P_ParseInterText(char *line)
{
        while(*line==' ') line++;
        if(!*line) return;

        if(!info_intertext)
        {
                info_intertext = Z_Malloc(INTERTEXTSIZE, PU_LEVEL, 0);
                *info_intertext = 0; // first char as the end of the string
        }
        sprintf(info_intertext, "%s%s\n", info_intertext, line);
}

boolean default_weaponowned[NUMWEAPONS];

void P_InitWeapons()
{
        char *s;

        memset(default_weaponowned, 0, sizeof(default_weaponowned));

        s = info_weapons;

        while(*s)
        {
           switch(*s)
           {
              case '3': default_weaponowned[wp_shotgun] = true; break;
              case '4': default_weaponowned[wp_chaingun] = true; break;
              case '5': default_weaponowned[wp_missile] = true; break;
              case '6': default_weaponowned[wp_plasma] = true; break;
              case '7': default_weaponowned[wp_bfg] = true; break;
              case '8': default_weaponowned[wp_supershotgun] = true; break;
              default: break;
           }
           s++;
        }
}

