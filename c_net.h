#ifndef __C_NET_H__
#define __C_NET_H__

#include "c_runcmd.h"

        // net command numbers
enum
{
        netcmd_null,       // 0 is empty
        netcmd_colour,
        netcmd_deathmatch,
        netcmd_exitlevel,
        netcmd_fast,
        netcmd_kill,
        netcmd_name,
        netcmd_nomonsters,
        netcmd_nuke,
        netcmd_respawn,
        netcmd_skill,
        netcmd_skin,
        netcmd_allowmlook,
        netcmd_autoaim,
        netcmd_bfglook,
        netcmd_bfgtype,
        netcmd_recoil,
        netcmd_pushers,
        netcmd_varfriction,
        netcmd_chat,
        netcmd_monremember,
        netcmd_moninfight,
        netcmd_monbacking,
        netcmd_monavoid,
        netcmd_monfriction,
        netcmd_monclimb,
        netcmd_monhelpfriends,
        netcmd_mondistfriend,
        netcmd_map,
        netcmd_weapspeed,
        NUMNETCMDS,
};

void C_SendCmd(int dest, int, char *s,...);
void C_queueChatChar(unsigned char c);
unsigned char C_dequeueChatChar(void);
void C_NetTicker();
void C_NetInit();
void C_SendNetData();
void C_UpdateVar(command_t *command);

extern command_t *c_netcmds[NUMNETCMDS];
extern char* default_name;
extern int default_colour;
extern int allowmlook;
extern int cmdsrc;           // the source of a network console command

#define CN_BROADCAST 128

        // use the entire ticcmd for transferring console commands when
        // in console mode ?
// #define CONSHUGE

#endif
