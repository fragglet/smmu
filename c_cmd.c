// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Console commands
//
// basic console commands and variables for controlling
// the console itself.
// 
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"

#include "m_random.h"

// version hack

char *verdate_hack = (char*)version_date;
char *vername_hack = (char*)version_name;
extern char *cmdoptions;

               /************* constants *************/

// version
CONST_INT(VERSION);
CONSOLE_CONST(version, VERSION);

// version date
CONST_STRING(verdate_hack);
CONSOLE_CONST(ver_date, verdate_hack);

// version name
CONST_STRING(vername_hack);
CONSOLE_CONST(ver_name, vername_hack);

                /************* aliases ***************/
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

  for(charnum=33; charnum < 'z'; charnum++) // go thru each char in alphabet
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

//VARIABLE_INT(c_height,  NULL,                   20, 200, NULL);
//CONSOLE_VARIABLE(c_height, c_height, 0) {}

CONSOLE_INT(c_height, c_height, NULL, 20, 200, NULL, 0) {}

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
  C_BufferDelay(cmdtype, c_argc ? atoi(c_argv[0]) : 1);
}

// flood the console with crap
// .. such a great and useful command

CONSOLE_COMMAND(flood, 0)
{
  int a;

  for(a=0; a<300; a++)
    C_Printf("%c\n", a%64 + 32);
}


CONSOLE_COMMAND(warranty, 0)
{
  C_SetConsole();
  C_Printf
    (
     FC_GRAY "NO WARRANTY\n\n"
     
     "11. BECAUSE THE PROGRAM IS LICENSED FREE\n"
     "OF CHARGE, THERE IS NO WARRANTY FOR THE\n"
     "PROGRAM, TO THE EXTENT PERMITTED BY\n"
     "APPLICABLE LAW.  EXCEPT WHEN OTHERWISE\n"
     "STATED IN WRITING THE COPYRIGHT HOLDERS\n"
     "AND/OR OTHER PARTIES PROVIDE THE PROGRAM\n"
     "\"AS IS\" WITHOUT WARRANTY OF ANY KIND,\n"
     "EITHER EXPRESSED OR IMPLIED, INCLUDING,\n"
     "BUT NOT LIMITED TO, THE IMPLIED\n"
     "WARRANTIES OF MERCHANTABILITY AND FITNESS\n"
     "FOR A PARTICULAR PURPOSE. THE ENTIRE RISK\n"
     "AS TO THE QUALITY AND PERFORMANCE OF THE\n"
     "PROGRAM IS WITH YOU.  SHOULD THE PROGRAM\n"
     "PROVE DEFECTIVE, YOU ASSUME THE COST OF\n"
     "ALL NECESSARY SERVICING, REPAIR OR\n"
     "CORRECTION.\n"

     "12. IN NO EVENT UNLESS REQUIRED BY\n"
     "APPLICABLE LAW OR AGREED TO IN WRITING\n"
     "WILL ANY COPYRIGHT HOLDER, OR ANY OTHER\n"
     "PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE\n"
     "THE PROGRAM AS PERMITTED ABOVE, BE LIABLE\n"
     "TO YOU FOR DAMAGES, INCLUDING ANY GENERAL,\n"
     "SPECIAL, INCIDENTAL OR CONSEQUENTIAL\n"
     "DAMAGES ARISING OUT OF THE USE OR\n"
     "INABILITY TO USE THE PROGRAM\n"
     "(INCLUDING BUT NOT LIMITED TO LOSS OF\n"
     "DATA OR DATA BEING RENDERED INACCURATE\n"
     "OR LOSSES SUSTAINED BY YOU OR THIRD\n"
     "PARTIES OR A FAILURE OF THE PROGRAM TO\n"
     "OPERATE WITH ANY OTHER PROGRAMS), EVEN\n"
     "IF SUCH HOLDER OR OTHER PARTY HAS BEEN\n"
     "ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.\n"
     );

}


// ... the most important of all console commands
char *quotes[] =
{
"<SocratesX> caco seen Cacodemon\n<Cacodemon> Never seen cacodemon.  (but they do have an account.)",
"<Afterglow> does too dickfuck\n<Cyb> does not fradwhore\n<Afterglow> you are a yankee fag <Cyb> you are a canadian moron",
"<sfraggle> is tom still asleep\n<Evil`Dave> perhaps he was sucked into the computer\n<Evil`Dave> or another dimension where people walk on their hands and hamburgers eat people",
"<sfraggle> :|\n<tom_> :|\n<ali_j> :)",
"<Cyb> my dad has huge underwear\n<Cyb> I could prolly wear it as a shirt",
"<tom_> im just an irc monkey\n<tom_> ooh ooh",
"<Cyb_> emacs?\n<Cyb_> is that like an imac?",
"<lau> oh jon and elton john chatting together...\n<Isle> me?",
"<prowlin> best hip-hop record ever made was fucking EMINEM BABY",
"<prowlin> Afterglow: anal sex at the bottom of lakes dumbass",
"<Afterglow> wtf is compton",
"<tom_> cats r dumb\n<tom_> mewse: mine is memorex",
"<sfraggle> he had this thing about prepubescent girls\n<tom_> i think thats going to be my new excuse for why i never do any work",
"<mancabus> I used to be a  member of doomhq then I quit cause they suck\n<mancabus> actually i was fired",
"<Quasar`> UAC crates are specially designed to be highly resistant to unauthorized entrance, especially by crowbar toting characters from other games",
"<Katarhyne> Wow....\n<Katarhyne> That's hard.",
"<mewse> whered the wh0re go\n<mewse> whats up with that ^wisp^ guy\n<mewse> who am i talking to\n<mewse> wheres my sandwich",
"<Mewse> ok ok cool l33t awesome super duper gravy",
"<Aardappel> yeah ppl will take the quake source, remove mouselook, and replace those wank mdls by goodlooking sprites",
"<endura> dude I just realized whats so kickass\n<endura> my dads out of town the whole weekend\n<endura> I can walk around naked for the next 48 hours!",
"<doomfreke> Mystican: (Greek) He who kicks women",
"<[Cyb]> damnit you don't say old chap?\n<[Cyb]> wtf is the point of being british then",
"<HellKnite> I am dead serious, my sister looks like an imp",
"<tom_> afterglow is cool\n<tom_> no wait i dont mean cool do i i mean fool",
"<tom_> hehe cyb\n<tom_> not all metal is heavy\n<cph> aluminium",
"<tom_> gotta watch that\n<fraggle> because you're obsessed with jeremy spake?\n<tom_> yes.",
"<cph> Package: tom\n<cph> Version: 1.1.1-2\n<Cyb> leave mysts package alone",
"<Jon`> get your lime ass into da channel, straight outta compton, or Ill reach for ma 9 an pop a cap in your ass",
"<tom_> i have a vd",
"<Kayin`> I cant get the new Zdoomgl to work\n<Afterglow> i can't get your mom to work",
"<HellKnigh> I got kicked out of preschool",
"<Evil`Dave> sorry i know some guy with the last name fraggle",
"<Jon`> !aurikanmode\n<MEEPY> bah dah dah doo wah",

// *** old quotes ***

"* DooMWiz whips out DCK and fixes the problem.\n<DvlPup> Whipps out WHAT?!?",
"<citrus-> well how great, ionstorm.com crashed netscape... they really dont want me, the bastards",
"<Spike> dont u DARE ping me",
"<mystican> this channel is pro-nato rather than pro-ruski\n<mewse> but i'm communist!\n<mewse> i'm a pinko actually\n<Afterglow> i'm a lesbian",
"* DVS01 saw breasts today\n<DVS01> KFC",
"<BrV-Zokum> ms monoploy: this is where u will go today",
"<kern_proc> has it occured to anyone else that \"aol for dummies\" is an extremely redundant name for a book",
"<endura> I was drawn to the Imac, I had this incredible huge urge to go pop the back of it off then fill it full of water and put goldfish in them",
"<Linguica> There is a http://hector.ml.org\n<Hector> where?",
"<Endura> JFL go put ice down your pants\n<Endura> it helps in more ways than one\n<JFL> <<Endura>> i can't im naked",
"<prower> \"dont eat pork, not even with a fork CAN'T TOUCH THIS\"",
"<aurikan> SMMU is like the coolest name for a port",
"<Linguica> We should have a freaking Doom pilgrimage to ION Storm... it's like that Muslim big black box",
"I haven't pinged Romero in a while -- maybe I should. -- Lee Killough",
"<Endura> my sister's moustache is bigger than mine",
"<Cybrdemon> rofdl? Rolling on the FLoor Downloading?",
"<prower> i never see \"h3y 9uyZ h00k m3 uP w17h f0n7 jU4r3Z\" in the warez channels",
"<{BFGspaz}> i like prower\n<SSGbitch> i love prower man\n<prower> awww, come on bots... group hug!",
"<_GoodKo_> I am BahdKo's evil twin\n<_BahdKo_> What do you do, play quake?",
"<DooMWiz> Aur: No. Nark is the best idler.\n<Nark> how about I put my idle foot up your idle ass",
"<Endura> man I hate when my dad comes in my room and farts\n<Endura> then leaves",
"<ricrob> I touched Half-Life in the mall yesterday and almost came",
"<fod> i was doin 110 mph up the motorway last night and a Yamaha 1250 sn20x passed me!\n<fod> i wouldnt mind but thats a KEYBOARD!",
"<DGevert> My cats straferun into doors and walls a lot",
"<LlamaServ> pop tarts (n.) See \"Spice Girls\".",
"<NickBaker> How about playing Doom whilst having sex?\n<NickBaker> na, it'd be like \"ooh, yes... DIE IMP! aaah, come on baby... EAT THIS DEMON FUCKER!\" etc",
};

CONSOLE_COMMAND(quote, 0)
{
  int quotenum;

  quotenum = M_Random() % (sizeof quotes / sizeof(char*));
  
  C_Printf("%s\n", quotes[quotenum]);
}

        /******** add commands *******/

// command-adding functions in other modules

extern void    AM_AddCommands();        // am_map.c
extern void Cheat_AddCommands();        // m_cheat.c
extern void     G_AddCommands();        // g_cmd.c
extern void    HU_AddCommands();        // hu_stuff.c
extern void     I_AddCommands();        // i_system.c
extern void   net_AddCommands();        // d_net.c
extern void     P_AddCommands();        // p_cmd.c
extern void     R_AddCommands();        // r_main.c
extern void     S_AddCommands();        // s_sound.c
extern void    ST_AddCommands();        // st_stuff.c
extern void     T_AddCommands();        // t_script.c
extern void     V_AddCommands();        // v_misc.c
extern void    MN_AddCommands();        // mn_menu.c

void C_AddCommands()
{
  C_AddCommand(version);
  C_AddCommand(ver_date);
  C_AddCommand(ver_name);
  C_AddCommand(warranty);
  
  C_AddCommand(c_height);
  C_AddCommand(c_speed);
  C_AddCommand(cmdlist);
  C_AddCommand(delay);
  C_AddCommand(alias);
  C_AddCommand(opt);
  C_AddCommand(echo);
  C_AddCommand(flood);
  C_AddCommand(quote);
  
  // add commands in other modules
  AM_AddCommands();
  Cheat_AddCommands();
  G_AddCommands();
  HU_AddCommands();
  I_AddCommands();
  net_AddCommands();
  P_AddCommands();
  R_AddCommands();
  S_AddCommands();
  ST_AddCommands();
  T_AddCommands();
  V_AddCommands();
  MN_AddCommands();
}

