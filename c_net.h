#ifndef __C_NET_H__
#define __C_NET_H__

#include "c_runcmd.h"

void C_SendCmd(int dest, int, char *s,...);
void C_queueChatChar(unsigned char c);
unsigned char C_dequeueChatChar(void);
void C_NetTicker();
void C_NetInit();
void C_SendNetData();
void C_UpdateVar(command_t *command);

extern char* default_name;
extern int default_colour;
extern int allowmlook;
extern int cmdsrc;           // the source of a network console command

#define CN_BROADCAST 128

        // net command numbers
enum
{
        cmd_null,       // 0 is empty
        cmd_colour,
        cmd_deathmatch,
        cmd_exitlevel,
        cmd_fast,
        cmd_kill,
        cmd_name,
        cmd_nomonsters,
        cmd_nuke,
        cmd_respawn,
        cmd_skill,
        cmd_skin,
        cmd_allowmlook,
        cmd_autoaim,
        cmd_bfglook,
        cmd_bfgtype,
        cmd_recoil,
        cmd_pushers,
        cmd_varfriction,
        cmd_chat,
        cmd_monremember,
        cmd_moninfight,
        cmd_monbacking,
        cmd_monavoid,
        cmd_monfriction,
        cmd_monclimb,
        cmd_monhelpfriends,
        cmd_mondistfriend,
        cmd_map,
        NUMNETCMDS
};

        // use the entire ticcmd for transferring console commands when
        // in console mode ?
// #define CONSHUGE

#endif
