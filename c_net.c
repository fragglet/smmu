/******************************* console **********************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Console Network support 
//
// Network commands can be sent across netgames using 'C_SendCmd'. The
// command is transferred byte by byte, 1 per tic cmd, using the 
// chatchar variable (previously used for chat messages)

/* includes ************************/
#include <stdio.h>
#include <stdarg.h>
#include "c_io.h"
#include "c_runcmd.h"
#include "c_cmdlst.h"
#include "c_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"

int incomingdest[MAXPLAYERS];
char incomingmsg[MAXPLAYERS][256];
int cmdsrc = 0;           // the source of a network console command

command_t *c_netcmds[NUMNETCMDS];

char *default_name = "player";
int default_colour;

// basic chat char stuff: taken from hu_stuff.c and renamed

#define QUEUESIZE   1024

static char chatchars[QUEUESIZE];
static int  head = 0;
static int  tail = 0;

//
// C_queueChatChar() (used to be HU_*)
//
// Add an incoming character to the circular chat queue
//
// Passed the character to queue, returns nothing
//
void C_queueChatChar(unsigned char c)
{
  if (((head + 1) & (QUEUESIZE-1)) == tail)
    C_Printf("command unsent\n");
  else
    {
      chatchars[head++] = c;
      head &= QUEUESIZE-1;
    }
}

//
// C_dequeueChatChar() (used to be HU_*)
//
// Remove the earliest added character from the circular chat queue
//
// Passed nothing, returns the character dequeued
//

unsigned char C_dequeueChatChar(void)
{
  char c;

  if (head != tail)
    {
      c = chatchars[tail++];
      tail &= QUEUESIZE-1;
    }
  else
    c = 0;
  return c;
}

void C_SendCmd(int dest, int cmdnum, char *s,...)
{
        va_list args;
        char tempstr[500];
        va_start(args, s);

        vsprintf(tempstr,s, args);
        s = tempstr;

        if(!netgame || demoplayback)
        {
                cmdsrc = consoleplayer;
                cmdtype = c_netcmd;
                C_RunCommand(c_netcmds[cmdnum], s);
                return;
        }

        C_queueChatChar(0); // flush out previous commands
        C_queueChatChar(dest+1); // the chat message destination
        C_queueChatChar(cmdnum);        // command num

        while(*s)
        {
                C_queueChatChar(*s);
                s++;
        }
        C_queueChatChar(0);
}

void C_NetInit()
{
        int i;
        command_t *current;

        for(i=0; i<MAXPLAYERS; i++)
        {
                incomingdest[i] = -1;
                *incomingmsg[i] = 0;
        }

        players[consoleplayer].colormap = default_colour;
        strcpy(players[consoleplayer].name, default_name);

        current = commands;             // load in the netcmds
        while(current->type != ct_end)
        {
                c_netcmds[current->netcmd] = current;
                current++;
        }
}

void C_DealWithChar(unsigned char c, int source);

void C_NetTicker()
{
      int i;

      if(netgame && !demoplayback)      // only deal with chat chars in
                                        // netgames

        // check for incoming chat chars
      for(i=0; i<MAXPLAYERS; i++)
      {
          if(!playeringame[i]) continue;
#ifdef CONSHUGE
          if(gamestate == GS_CONSOLE)  // use the whole ticcmd in console mode
          {
                int a;
                for(a=0; a<sizeof(ticcmd_t); a++)
                    C_DealWithChar( ((unsigned char*)&players[i].cmd)[a], i);
          }
          else
#endif
                C_DealWithChar(players[i].cmd.chatchar,i);
      }

        // run buffered commands essential for netgame sync
      C_RunBuffer(c_netcmd);
      C_RunBuffer(c_script);
}

void C_DealWithChar(unsigned char c, int source)
{
    int netcmdnum;

    if(c)
    {
          if(incomingdest[source] == -1)  // first char: the destination
          {
                incomingdest[source] = c-1;
          }
          else                  // append to string
          {
                sprintf(incomingmsg[source], "%s%c", incomingmsg[source], c);
          }
    }
    else
    {
          if(incomingdest[source] != -1)        // end of message
          {
              if((incomingdest[source] == consoleplayer)
               || incomingdest[source] == CN_BROADCAST)
              {
                  cmdsrc = source;
                  cmdtype = c_netcmd;
                              // the first byte is the command num
                  netcmdnum = incomingmsg[source][0];

                  if(netcmdnum >= NUMNETCMDS || netcmdnum == 0)
                        C_Printf("unknown netcmd: %i", netcmdnum);
                  else
                        C_RunCommand(c_netcmds[netcmdnum],
                               incomingmsg[source] + 1);
              }
              *incomingmsg[source] = 0;
              incomingdest[source] = -1;
          }
    }
}

char *G_GetNameForMap(int episode, int map);

void C_SendNetData()
{
    char tempstr[50];
    command_t *command;

    C_SetConsole();

    C_Printf(consoleplayer ?
      FC_GRAY"Please Wait"FC_RED" Receiving game data..\n" :
      FC_GRAY"Please Wait"FC_RED" Sending game data..\n");

    command = commands;

    while(command->type != ct_end)
    {
      if(command->flags & cf_netvar && command->type==ct_variable)
      {
             // only send non-server commands or all if this is the server
        if(!(command->flags & cf_server) || consoleplayer == 0)
        {
          C_UpdateVar(command);
        }
      }
      command++;
    }

    if(consoleplayer == 0)      // if server, send command to warp to map
    {
         sprintf(tempstr, "map %s", G_GetNameForMap(startepisode, startmap));
         C_RunTextCmd(tempstr);
    }
    C_Printf("finished C_SendNetData\n"); C_Update();
}


//
//  Variables needed for sync:
//
// done:
// classic_bfg, startskill, nomonsters, respawnparm
// player_bobbing, allowmlook, weapon_recoil
// deathmatch, 
// monsters_remember, monster_infighting, monkeys,
// monster_backing, monster_avoid_hazards, monster_friction
// variable_friction, allow_pushers
// distfriend, help_friends,
//
// todo:
// dogs, dog_jumping
//

int allowmlook=1;

//
//      Update a network variable
//

void C_UpdateVar(command_t *command)
{
        char tempstr[100];

        sprintf(tempstr,"\"%s\"", C_VariableValue(command) );

        C_SendCmd(CN_BROADCAST, command->netcmd, tempstr);
}
