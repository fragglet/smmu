
#ifndef __C_HANDLE_H__
#define __C_HANDLE_H__

#include "c_runcmd.h"

void C_VidModeList();
void C_VidMode();

void C_CheatGod();
void C_CheatNuke();
void C_CheatNoClip();
void C_HomDetect();

void C_Map();
void C_Consolemode();

void C_Addfile();

void C_Say();
void C__Chat();
void C_Playercolour();
void C_Playername();
void C_Kill();

void C_Centremsg();
void C_Playermsg();
void C_Spawn();
void C_LineTrigger();

void C_Cmdlist();
void C_Echo();
void C_Delay();

void C_Alias();
alias_t *C_NewAlias(unsigned char *aliasname, unsigned char *command);
void C_RemoveAlias(unsigned char *aliasname);

void C_Players();
void C_Disconnect();

void C_PlayDemo();
void C_TimeDemo();
void C_StopDemo();

void C_NullModem();
void C_Answer();
void C_Dial();

void C_ExitLevel();

void C_Psprites();
void C_AnimShot();
void C_Pause();

void C_ScreenSize();

void C_Skill();

void C_ViewDir();
void C_GUITest();
void C_Error();
void C_Flood();

void C_QuitGame();
void C_Kick();

#endif
