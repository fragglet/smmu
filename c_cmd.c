/************************** console variables **************************/

        // basic console commands and variables for controlling
        // the console itself.

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

char *quotes[] =
{
"* DooMWiz whips out DCK and fixes the problem.\n<DvlPup> Whipps out WHAT?!?",
"<citrus-> well how great, ionstorm.com crashed netscape... they really dont want me, the bastards",
"<\\dev\\null> i'm a black hole",
"<DVS01> i'd /finger lewinsky",
"<Ansi[c]> Kong didn't even have a penis",
"<Satori> like uh... grabbing her saggy titties and shaking thim at you while flapping her legs so the stench of her demonic femininity overpowers you (complete with a Hexen-like fog cloud, of course)??",
"<OJ> I didnt do it\n<Ito> YOU FUCKING DID IT OJ!",
"<Kirk> very funny scotty, now beam down my clothes!",
"* Mordeth powers up his full ethernet bastard capabilities",
"<Endura> ASCII porn is the best shit\n<Endura> almost as good as braille porn",
"<Spike> dont u DARE ping me",
"<prower> wtf is prowerland?",
"*** unnukeabl has quit IRC (Connection timed out)",
"<[Vicious]> Radio turns on when I get up, can't live without it....unless you've got Spike to wake u up first",
"<ricrob> hector's in a beer bottle in my freezer",
"<mystican> this channel is pro-nato rather than pro-ruski\n<mewse> but i'm communist!\n<mewse> i'm a pinko actually\n<Afterglow> i'm a lesbian",
"* DVS01 saw breasts today\n<DVS01> KFC",
"<Rethcir> \"And now, 26 more hours of 'The best of #doomroom'\"",
"<cph> Error 1337: j00 4r3 n07 31337 3n0ugh7 70 pr0gr4m",
"<BrV-Zokum> ms monoploy: this is where u will go today",
"<mystican> the sun shines out of your ass?\n<DVS01> myst: yes!\n<DVS01> (__0__) -- where da sun shines",
"<Cyb> Thundercats HO!",
"<DVS01> 486 is too slow to suck ass\n<DVS01> 486 nibbles ass",
"<prowlin> \"mystican saw Bessie and was amazed.  He slowly unbuttoned his pants, his cock already hardened to its full three inches.\"",
"<kern_proc> has it occured to anyone else that \"aol for dummies\" is an extremely redundant name for a book",
"<ninjaworf> today is a good day to hide",
"<Rethcir> Quake's bad guys' names were too hard to pronounce, that's why it wasn't as good as doom",
"<endura> I was drawn to the Imac, I had this incredible huge urge to go pop the back of it off then fill it full of water and put goldfish in them",
"<Linguica> There is a http://hector.ml.org\n<Hector> where?",
"<prower> <Lyfe> Alright, I quit Crypt TC!<FlyatWork> Oh no you don't",
"<Endura> JFL go put ice down your pants\n<Endura> it helps in more ways than one\n<JFL> <<Endura>> i can't im naked",
"<prower> \"dont eat pork, not even with a fork CAN'T TOUCH THIS\"",
"<DVS01> hmm, myst you gave me an idea.. New OS: FradBSD",
"<Mud_Shark> the page is fucked up\n* Prower points all fingers at Covaro \n<Mud_Shark> Covaro fucked up doomworld!\n<Arno> now we see how rumors start\n<Mud_Shark> Arno fucked up doomworld!",
"<aurikan> SMMU is like the coolest name for a port",
"<Linguica> We should have a freaking Doom pilgrimage to ION Storm... it's like that Muslim big black box",
"* Arno^QS tries to imagine a cyberdemon in a skimpy bra and some lace panties",
"I haven't pinged Romero in a while -- maybe I should. -- Lee Killough",
"<[GOD]> we have parties there\n<[GOD]> but dont tell jesus",
"<Endura> my sister's moustache is bigger than mine",
"<Cybrdemon> rofdl? Rolling on the FLoor Downloading?",
"<EyeArsy> there, my nick is IRC",
"* citrus owns an ass \n<citrus> it's attatched to my lower spine",
"<Kayin`> Its Mystican, like Mohican!\n<Kayin`> He's the Last of the Mysticans!",
"<prower> i never see \"h3y 9uyZ h00k m3 uP w17h f0n7 jU4r3Z\" in the warez channels",
"<{BFGspaz}> i like prower\n<SSGbitch> i love prower man\n<prower> awww, come on bots... group hug!",
"* DVS01 pats _prower_ on the head (not of his cock)",
"<DVS01> uhh, what th fuck happnd to my  ky?",
"<DVS01> <mystican> hrrrrr     <--- hell really really really really revealed?",
"<_GoodKo_> I am BahdKo's evil twin\n<_BahdKo_> What do you do, play quake?",
"<DooMWiz> Aur: No. Nark is the best idler.\n<Nark> how about I put my idle foot up your idle ass",
"<Endura> man I hate when my dad comes in my room and farts\n<Endura> then leaves",
"<Ricrob> I'm such a frad at times",
"<DVS01> Bill Gate's wife understanded what \"microsoft\" really mean, when they were on honeymoon.... :)",
"<Fradman> Frad Flinstone",
"<fake^myst> i am not myst!!",
"<prower> [19:48] <_Prower_> Hi! I dig men! I like to stick foreign objects up my ass!\n<prower> i'm still killing my dad for that thing",
"<ola_mail> I think they should have merged Village People and NiN instead.",
"<prower> CHASECAM ROCKS!!\n<prower> now i can see myself suck in the 3rd person!",
"<ricrob> I touched Half-Life in the mall yesterday and almost came",
"<LeRomero> je suis le grand fromage",
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
extern void Cheat_AddCommands();    // m_cheat.c
extern void G_AddCommands();        // g_cmd.c
extern void HU_AddCommands();       // hu_stuff.c
extern void I_AddCommands();        // i_system.c
extern void net_AddCommands();      // d_net.c
extern void P_AddCommands();        // p_cmd.c
extern void R_AddCommands();        // r_main.c
extern void S_AddCommands();        // s_sound.c
extern void ST_AddCommands();       // st_stuff.c
extern void T_AddCommands();        // t_script.c
extern void V_AddCommands();        // v_misc.c
extern void M_AddCommands();        // m_menu.c

void C_AddCommands()
{
        C_AddCommand(version);
        C_AddCommand(ver_date);
        C_AddCommand(ver_name);

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
        M_AddCommands();
}

