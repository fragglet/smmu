#ifndef __P_INFO_H__
#define __P_INFO_H__

#include "c_io.h"

void P_LoadLevelInfo(int lumpnum);

void P_CleanLine(char *line);

extern char *info_interpic;
extern char *info_levelname;
extern char *info_levelpic;
extern char *info_music;
extern int info_partime;
extern char *info_levelcmd[128];
extern char *info_skyname;
extern char *info_creator;
extern char *info_nextlevel;
extern char *info_intertext;
extern char *info_backdrop;
extern int info_scripts;        // whether the current level has scripts

extern boolean default_weaponowned[NUMWEAPONS];


#endif
