/***************************** FraggleScript ******************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Functions
//
// functions are stored as variables(see variable.c), the
// value being a pointer to a 'handler' function for the
// function. Arguments are stored in an argc/argv-style list
//
// this module contains all the handler functions for the
// basic FraggleScript Functions.

/* includes ************************/

#include <stdio.h>
#include "c_io.h"
#include "doomstat.h"
#include "doomtype.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "info.h"
#include "m_random.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "p_spec.h"
#include "p_inter.h"
#include "r_main.h"
#include "r_segs.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_script.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);

        // functions. SF_ means Script Function not, well.. heh, me

        /////////// actually running a function /////////////


/*******************
  FUNCTIONS
 *******************/

// the actual handler functions for the
// functions themselves

// arguments are evaluated and passed to the
// handler functions using 't_argc' and 't_argv'
// in a similar way to the way C does with command
// line options.

// values can be returned from the functions using
// the variable 't_return'

void SF_Print()
{
        int i;

        if(!t_argc) return;

        for(i=0; i<t_argc; i++)
        {
                if(t_argv[i].type == svt_string)
                        C_Printf("%s", t_argv[i].value.s);
                else    // assume int
                        C_Printf("%i", (int)t_argv[i].value.i);
        }
}

        // return a random number from 0 to 255
void SF_Rnd()
{
        t_return.type = svt_int;
        t_return.value.i = P_Random(pr_script);
}

        // looping section. using the rover, find the highest level
        // loop we are currently in and return the section_t for it.

section_t *looping_section()
{
        section_t *best = NULL;         // highest level loop we're in
                                        // that has been found so far
        int n;

                // check thru all the hashchains
        for(n=0; n<SECTIONSLOTS; n++)
        {
           section_t *current = current_script->sections[n];

                // check all the sections in this hashchain
           while(current)
           {
                        // a loop?
                if(current->type == st_loop)
                       // check to see if it's a loop that we're inside
                   if(rover >= current->start && rover <= current->end)
                   {
                        // a higher nesting level than the best one so far?
                      if(!best || (current->start > best->start))
                          best = current;     // save it
                   }
                current = current->next;
           }
        }

        return best;    // return the best one found
}

        // "continue;" in FraggleScript is a function
void SF_Continue()
{
        section_t *section;

        if(!(section = looping_section()) )       // no loop found
        {
                script_error("continue() not in loop\n");
                return;
        }

        rover = section->end;      // jump to the closing brace
}

void SF_Break()
{
        section_t *section;

        if(!(section = looping_section()) )
        {
                script_error("break() not in loop\n");
                return;
        }
        rover = section->end+1;   // jump out of the loop
}

void SF_Goto()
{
        if(t_argc < 1)
        {
                script_error("incorrect arguments to goto\n");
                return;
        }

        // check argument is a labelptr

        if(t_argv[0].type != svt_label)
        {
                script_error("goto argument not a label\n");
                return;
        }

        // go there then if everythings fine

        rover = t_argv[0].value.labelptr;
}

void SF_Return()
{
        killscript = true;      // kill the script
}

void SF_Include()
{
        char tempstr[12];

        if(t_argc < 1)
        {
                script_error("incorrect arguments to include()");
                return;
        }

        if(t_argv[0].type == svt_string)
          strncpy(tempstr, t_argv[0].value.s, 8);
        else
          sprintf(tempstr, "%i", (int)t_argv[0].value.i);

        parse_include(tempstr);
}

void SF_Input()
{
/*        static char inputstr[128];

        gets(inputstr);

        t_return.type = svt_string;
        t_return.value.s = inputstr;
*/
        C_Printf("input() function not available in doom\a\n");
}

void SF_Beep()
{
        C_Printf("\a");
}

void SF_Clock()
{
        t_return.type = svt_int;
        t_return.value.i = (gametic*100)/35;
}

    /**************** doom stuff ****************/

void SF_ExitLevel()
{
        G_ExitLevel();
}

        // centremsg
void SF_Tip()
{
        int i;
        char tempstr[128]="";

        if(current_script->trigger->player != &players[displayplayer])
                return;

        for(i=0; i<t_argc; i++)
              if(t_argv[i].type == svt_string)
                     sprintf(tempstr,"%s%s", tempstr, t_argv[i].value.s);
              else    // assume int
                     sprintf(tempstr,"%s%i", tempstr, (int)t_argv[i].value.i);

        HU_centremsg(tempstr);
}

        // tip to a particular player
void SF_PlayerTip()
{
        int i, plnum;
        char tempstr[128]="";
        
        if(!t_argc)
        { script_error("player not specified\n"); return;}

        plnum = intvalue(t_argv[0]);
        if(plnum != displayplayer) return;      // not this player

        for(i=1; i<t_argc; i++)
                if(t_argv[i].type == svt_string)
                        sprintf(tempstr,"%s%s", tempstr, t_argv[i].value.s);
                else    // assume int
                        sprintf(tempstr,"%s%i", tempstr, (int)t_argv[i].value.i);

        HU_centremsg(tempstr);
}

        // message player
void SF_Message()
{
        int i;
        char tempstr[128]="";

        if(current_script->trigger->player != &players[displayplayer])
                return;

        for(i=0; i<t_argc; i++)
                if(t_argv[i].type == svt_string)
                        sprintf(tempstr,"%s%s", tempstr, t_argv[i].value.s);
                else    // assume int
                        sprintf(tempstr,"%s%i", tempstr, (int)t_argv[i].value.i);

        dprintf(tempstr);
}

        // message to a particular player
void SF_PlayerMsg()
{
        int i, plnum;
        char tempstr[128]="";

        if(!t_argc)
        { script_error("player not specified\n"); return;}

        plnum = intvalue(t_argv[0]);
        if(plnum != displayplayer) return;      // not this player

        for(i=1; i<t_argc; i++)
                if(t_argv[i].type == svt_string)
                        sprintf(tempstr,"%s%s", tempstr, t_argv[i].value.s);
                else    // assume int
                        sprintf(tempstr,"%s%i", tempstr, (int)t_argv[i].value.i);

        dprintf(tempstr);
}

void SF_PlayerInGame()
{
        if(!t_argc)
        { script_error("player not specified\n"); return;}

        t_return.type = svt_int;
        t_return.value.i = playeringame[intvalue(t_argv[0])];
}

void SF_PlayerName()
{
        int plnum;

        if(!t_argc)
        {
                player_t *pl;
                pl = current_script->trigger->player;
                if(pl) plnum = pl - players;
                else
                {
                        script_error("script not started by player\n");
                        return;
                }
        }
        else
                plnum = intvalue(t_argv[0]);

        t_return.type = svt_string;
        t_return.value.s = players[plnum].name;
}

        // object being controlled by player
void SF_PlayerObj()
{
        int plnum;

        if(!t_argc)
        {
                player_t *pl;
                pl = current_script->trigger->player;
                if(pl) plnum = pl - players;
                else
                {
                        script_error("script not started by player\n");
                        return;
                }
        }
        else
                plnum = intvalue(t_argv[0]);

        t_return.type = svt_mobj;
        t_return.value.mobj = players[plnum].mo;
}

extern void SF_StartScript();      // in t_script.c
extern void SF_Wait();

/*********** Mobj code ***************/

void SF_Player()
{
        mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) :
                              current_script->trigger;

        t_return.type = svt_int;

        if(mo)
        {
                t_return.value.i = (int)(mo->player - players);
        }
        else
        {
                t_return.value.i = -1;
        }

}

        // spawn an object: type, x, y, [angle]
void SF_Spawn()
{
        int x, y, z, objtype;
        angle_t angle = 0;

        if(t_argc < 3)
        { script_error("insufficient arguments to function\n"); return; }

        objtype = intvalue(t_argv[0]);
        x = intvalue(t_argv[1]) * FRACUNIT;
        y = intvalue(t_argv[2]) * FRACUNIT;
        z = R_PointInSubsector(x, y)->sector->floorheight;
        if(t_argc > 3)
                angle = R_WadToAngle(intvalue(t_argv[3]));

        if(objtype < 0 || objtype >= NUMMOBJTYPES)
        { script_error("unknown object type: %i\n", objtype); return; }

        t_return.type = svt_mobj;
        t_return.value.mobj = P_SpawnMobj(x,y,z, objtype);
}

void SF_RemoveObj()
{
        mobj_t *mo;

        if(!t_argc)
        { script_error("insufficient arguments to function\n"); return; }

        mo = MobjForSvalue(t_argv[0]);
        if(mo)  // nullptr check
          P_RemoveMobj(mo);
}

void SF_KillObj()
{
        mobj_t *mo;

        if(t_argc)
                mo = MobjForSvalue(t_argv[0]);
        else
                mo = current_script->trigger;  // default to trigger object

        if(mo)  // nullptr check
          P_KillMobj(current_script->trigger, mo);         // kill it
}

        // mobj x, y, z
void SF_ObjX()
{
        mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) :
                              current_script->trigger;

        t_return.type = svt_int;
        t_return.value.i = mo ? mo->x / FRACUNIT : 0;   // null ptr check
}

void SF_ObjY()
{
        mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) :
                              current_script->trigger;

        t_return.type = svt_int;
        t_return.value.i = mo ? mo->y / FRACUNIT : 0; // null ptr check
}

void SF_ObjZ()
{
        mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) :
                              current_script->trigger;

        t_return.type = svt_int;
        t_return.value.i = mo ? mo->z / FRACUNIT : 0; // null ptr check
}

        // mobj angle
void SF_ObjAngle()
{
        mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) :
                              current_script->trigger;

        t_return.type = svt_int;
        t_return.value.i = mo ? mo->angle / (ANG180/180) : 0;   // null ptr check
}


        // teleport: object, sector_tag
void SF_Teleport()
{
        line_t line;    // dummy line for teleport function
        mobj_t *mo;

        if(t_argc==0)   // no arguments
        { script_error("insufficient arguments to function\n"); return; }
        else if(t_argc == 1)    // 1 argument: sector tag
        {
            mo = current_script->trigger;   // default to trigger
            line.tag = intvalue(t_argv[0]);
        }
        else    // 2 or more
        {                       // teleport a given object
            mo = MobjForSvalue(t_argv[0]);
            line.tag = intvalue(t_argv[1]);
        }

        if(mo)
          EV_Teleport(&line, 0, mo);
}

void SF_SilentTeleport()
{
        line_t line;    // dummy line for teleport function
        mobj_t *mo;

        if(t_argc==0)   // no arguments
        { script_error("insufficient arguments to function\n"); return; }
        else if(t_argc == 1)    // 1 argument: sector tag
        {
            mo = current_script->trigger;   // default to trigger
            line.tag = intvalue(t_argv[0]);
        }
        else    // 2 or more
        {                       // teleport a given object
            mo = MobjForSvalue(t_argv[0]);
            line.tag = intvalue(t_argv[1]);
        }

        if(mo)
          EV_SilentTeleport(&line, 0, mo);
}

void SF_DamageObj()
{
        mobj_t *mo;
        int damageamount;

        if(t_argc==0)   // no arguments
        { script_error("insufficient arguments to function\n"); return; }
        else if(t_argc == 1)    // 1 argument: damage trigger by amount
        {
            mo = current_script->trigger;   // default to trigger
            damageamount = intvalue(t_argv[0]);
        }
        else    // 2 or more
        {                       // teleport a given object
            mo = MobjForSvalue(t_argv[0]);
            damageamount = intvalue(t_argv[1]);
        }

        if(mo)
          P_DamageMobj(mo, NULL, current_script->trigger, damageamount);
}

        // the tag number of the sector the thing is in
void SF_ObjSector()
{
        mobj_t *mo;
        
                // use trigger object if not specified
        mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

        t_return.type = svt_int;
        t_return.value.i = mo ? mo->subsector->sector->tag : 0; // nullptr check
}

        // the health number of an object
void SF_ObjHealth()
{
        mobj_t *mo;

                // use trigger object if not specified
        mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

        t_return.type = svt_int;
        t_return.value.i = mo ? mo->health : 0;
}

void SF_ObjFlag()
{
        mobj_t *mo;
        int flagnum;

        if(t_argc==0)   // no arguments
         { script_error("no arguments for function\n"); return; }
        else
        if(t_argc == 1)         // use trigger, 1st is flag
        {
                        // use trigger:
                mo = current_script->trigger;
                flagnum = intvalue(t_argv[0]);
        }
        else
        if(t_argc == 2)
        {
                        // specified object
                mo = MobjForSvalue(t_argv[0]);
                flagnum = intvalue(t_argv[1]);
        }
        else                     // >= 3 : SET flags
        {
                mo = MobjForSvalue(t_argv[0]);
                flagnum = intvalue(t_argv[1]);

                if(mo)          // nullptr check
                {
                    long newflag;
                                       // remove old bit
                    mo->flags = mo->flags & ~(1 << flagnum);
        
                                // make the new flag
                    newflag = (!!intvalue(t_argv[2])) << flagnum;
                    mo->flags |= newflag;   // add new flag to mobj flags
                }
        }

        t_return.type = svt_int;
                // nullptr check:
        t_return.value.i = mo ? !!(mo->flags & (1 << flagnum)) : 0;
}

        // apply momentum to a thing
void SF_PushThing()
{
        mobj_t *mo;
        angle_t angle;
        fixed_t force;

        if(t_argc<3)   // missing arguments
         { script_error("insufficient arguments for function\n"); return; }

        mo = MobjForSvalue(t_argv[0]);

        if(!mo) return;

        angle = R_WadToAngle(intvalue(t_argv[1]));
        force = intvalue(t_argv[2]) * FRACUNIT;

        mo->momx += FixedMul(finecosine[angle >> ANGLETOFINESHIFT], force);
        mo->momy += FixedMul(finesine[angle >> ANGLETOFINESHIFT], force);
}

/****************** Trig *********************/

void SF_PointToAngle()
{
        angle_t angle;
        int x1, y1, x2, y2;

        if(t_argc<4)
        { script_error("insufficient arguments to function\n"); return; }

        x1 = intvalue(t_argv[0]) * FRACUNIT;
        y1 = intvalue(t_argv[1]) * FRACUNIT;
        x2 = intvalue(t_argv[2]) * FRACUNIT;
        y2 = intvalue(t_argv[3]) * FRACUNIT;
                            
        angle = R_PointToAngle2(x1, y1, x2, y2);
        t_return.type = svt_int;
        t_return.value.i = angle/(ANG45/45);
}

void SF_PointToDist()
{
        int dist;
        int x1, x2, y1, y2;

        if(t_argc<4)
        { script_error("insufficient arguments to function\n"); return; }

        x1 = intvalue(t_argv[0]) * FRACUNIT;
        y1 = intvalue(t_argv[1]) * FRACUNIT;
        x2 = intvalue(t_argv[2]) * FRACUNIT;
        y2 = intvalue(t_argv[3]) * FRACUNIT;
                            
        dist = R_PointToDist2(x1, y1, x2, y2);
        t_return.type = svt_int;
        t_return.value.i = dist/FRACUNIT;
}

/************* Camera functions ***************/

camera_t script_camera;

        // setcamera(obj, [angle])
void SF_SetCamera()
{
        mobj_t *mo;
        angle_t angle;

        if(t_argc < 1)
        { script_error("insufficient arguments to function\n"); return; }

        mo = MobjForSvalue(t_argv[0]);
        if(!mo) return;         // nullptr check
        angle = t_argc==1 ? mo->angle :
                R_WadToAngle(intvalue(t_argv[1]));

        script_camera.x = mo->x;
        script_camera.y = mo->y;
        script_camera.z = mo->z + 41*FRACUNIT;
        script_camera.angle = angle;
        script_camera.updownangle = 0;  // straight forward

        camera = &script_camera;
}

void SF_ClearCamera()
{
        camera = NULL;          // turn off camera
}

/*********** sounds ******************/

        // start sound from thing
void SF_StartSound()
{
        mobj_t *mo;

        if(t_argc < 2)
        { script_error("insufficient arguments to function\n"); return; }

        if(t_argv[1].type != svt_string)
        { script_error("sound lump argument not a string!\n"); return;}

        mo = MobjForSvalue(t_argv[0]);

        S_StartSoundName(mo, t_argv[1].value.s);
}

        // start sound from sector
void SF_StartSectorSound()
{
        sector_t *sector;
        int tagnum, secnum;

        if(t_argc < 2)
        { script_error("insufficient arguments to function\n"); return; }
        if(t_argv[1].type != svt_string)
        { script_error("sound lump argument not a string!\n"); return;}

        tagnum = intvalue(t_argv[0]);
            // argv is sector tag

        secnum = P_FindSectorFromTag(tagnum, -1);

        if(secnum < 0)
        { script_error("sector not found with tagnum %i\n", tagnum); return;}

        sector = &sectors[secnum];

        S_StartSoundName((mobj_t *)&sector->soundorg, t_argv[1].value.s);
}

/************* Sector functions ***************/

        // floor height of sector
void SF_FloorHeight()
{
        sector_t *sector;
        int tagnum;
        int secnum;

        if(!t_argc)
        { script_error("insufficient arguments to function\n"); return; }

        tagnum = intvalue(t_argv[0]);

            // argv is sector tag
        secnum = P_FindSectorFromTag(tagnum, -1);

        if(secnum < 0)
        { script_error("sector not found with tagnum %i\n", tagnum); return;}
        
        sector = &sectors[secnum];

        if(t_argc > 1)          // > 1: set floorheight
        {
                int i = -1;

                // set all sectors with tag
                while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
                {
                   sectors[i].floorheight = intvalue(t_argv[1]) * FRACUNIT;
                }
        }

                // return floorheight
        t_return.type = svt_int;
        t_return.value.i = sector->floorheight / FRACUNIT;
}

void SF_MoveFloor()
{
        int secnum = -1;
        sector_t *sec;
        floormove_t *floor;
        int tagnum, platspeed = 1, destheight;

        if(t_argc < 2)
        { script_error("insufficient arguments to function\n"); return; }

        tagnum = intvalue(t_argv[0]);
        destheight = intvalue(t_argv[1]) * FRACUNIT;
        platspeed = FLOORSPEED * (t_argc > 2 ? intvalue(t_argv[2]) : 1);

                // move all sectors with tag
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            sec = &sectors[secnum];
              
            // Don't start a second thinker on the same floor
            if (P_SectorActive(floor_special,sec))
              continue;

            floor = Z_Malloc(sizeof(floormove_t), PU_LEVSPEC, 0);
            P_AddThinker(&floor->thinker);
            sec->floordata = floor;
            floor->thinker.function = T_MoveFloor;
            floor->type = -1;   // not done by line
            floor->crush = false;

            floor->direction =
                destheight < sec->floorheight ? plat_down : plat_up;
            floor->sector = sec;
            floor->speed = platspeed;
            floor->floordestheight = destheight;
        }
}

        // ceiling height of sector
void SF_CeilingHeight()
{
        sector_t *sector;
        int secnum;
        int tagnum;

        if(!t_argc)
        { script_error("insufficient arguments to function\n"); return; }

        tagnum = intvalue(t_argv[0]);

            // argv is sector tag
        secnum = P_FindSectorFromTag(tagnum, -1);

        if(secnum < 0)
        { script_error("sector not found with tagnum %i\n", tagnum); return;}
        
        sector = &sectors[secnum];


        if(t_argc > 1)          // > 1: set ceilheight
        {
                int i = -1;

                // set all sectors with tag
                while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
                {
                  sectors[i].ceilingheight = intvalue(t_argv[1]) * FRACUNIT;
                }
        }

                // return floorheight
        t_return.type = svt_int;
        t_return.value.i = sector->ceilingheight / FRACUNIT;
}

void SF_MoveCeiling()
{
        int secnum = -1;
        sector_t *sec;
        ceiling_t *ceiling;
        int tagnum, platspeed = 1, destheight;

        if(t_argc < 2)
        { script_error("insufficient arguments to function\n"); return; }

        tagnum = intvalue(t_argv[0]);
        destheight = intvalue(t_argv[1]) * FRACUNIT;
        platspeed = FLOORSPEED * (t_argc > 2 ? intvalue(t_argv[2]) : 1);

                // move all sectors with tag
        while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
        {
            sec = &sectors[secnum];
              
            // Don't start a second thinker on the same floor
            if (P_SectorActive(ceiling_special,sec))
              continue;

            ceiling = Z_Malloc(sizeof(*ceiling), PU_LEVSPEC, 0);
            P_AddThinker(&ceiling->thinker);
            sec->ceilingdata = ceiling;
            ceiling->thinker.function = T_MoveCeiling;
            ceiling->type = genCeiling;   // not done by line
            ceiling->crush = false;

            ceiling->direction =
                destheight < sec->ceilingheight ? plat_down : plat_up;
            ceiling->sector = sec;
            ceiling->speed = platspeed;
                // just set top and bottomheight the same
            ceiling->topheight = ceiling->bottomheight = destheight;

            ceiling->tag = sec->tag;
            P_AddActiveCeiling(ceiling);
        }
}

void SF_LightLevel()
{
        sector_t *sector;
        int secnum;
        int tagnum;

        if(!t_argc)
        { script_error("insufficient arguments to function\n"); return; }

        tagnum = intvalue(t_argv[0]);

            // argv is sector tag
        secnum = P_FindSectorFromTag(tagnum, -1);

        if(secnum < 0)
        { script_error("sector not found with tagnum %i\n", tagnum); return;}
        
        sector = &sectors[secnum];

        if(t_argc > 1)          // > 1: set ceilheight
        {
                int i = -1;

                // set all sectors with tag
                while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
                {
                  sectors[i].lightlevel = intvalue(t_argv[1]);
                }
        }

                // return lightlevel
        t_return.type = svt_int;
        t_return.value.i = sector->lightlevel;
}

void SF_FadeLight()
{
        int sectag, destlevel, speed = 1;

        if(t_argc < 2)
        { script_error("insufficient arguments to function\n"); return; }
                
        sectag = intvalue(t_argv[0]);
        destlevel = intvalue(t_argv[1]);
        speed = t_argc>2 ? intvalue(t_argv[2]) : 1;

        P_FadeLight(sectag, destlevel, speed);
}

void SF_FloorTexture()
{
        int tagnum, secnum;
        sector_t *sector;
        char floortextname[10];

        if(!t_argc)
        { script_error("insufficient arguments to function\n"); return; }

        tagnum = intvalue(t_argv[0]);

            // argv is sector tag
        secnum = P_FindSectorFromTag(tagnum, -1);

        if(secnum < 0)
        { script_error("sector not found with tagnum %i\n", tagnum); return;}

        sector = &sectors[secnum];

        if(t_argc > 1)
        {
                int i = -1;
                int picnum = R_FlatNumForName(t_argv[1].value.s);

                // set all sectors with tag
                while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
                {
                   sectors[i].floorpic = picnum;
                }
        }

        strncpy(floortextname, lumpinfo[sector->floorpic + firstflat]->name, 8);

        t_return.type = svt_string;
        t_return.value.s = Z_Strdup(floortextname, PU_LEVEL, 0);
}

void SF_CeilingTexture()
{
        int tagnum, secnum;
        sector_t *sector;
        char floortextname[10];

        if(!t_argc)
        { script_error("insufficient arguments to function\n"); return; }

        tagnum = intvalue(t_argv[0]);

            // argv is sector tag
        secnum = P_FindSectorFromTag(tagnum, -1);

        if(secnum < 0)
        { script_error("sector not found with tagnum %i\n", tagnum); return;}

        sector = &sectors[secnum];

        if(t_argc > 1)
        {
                int i = -1;
                int picnum = R_FlatNumForName(t_argv[1].value.s);

                // set all sectors with tag
                while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
                {
                   sectors[i].ceilingpic = picnum;
                }
        }

        strncpy(floortextname, lumpinfo[sector->ceilingpic + firstflat]->name, 8);

        t_return.type = svt_string;
        t_return.value.s = Z_Strdup(floortextname, PU_LEVEL, 0);
}


        /************* init_functions *******************/

void init_functions()
{
                // add all the functions
        add_game_int("consoleplayer", &consoleplayer);
        add_game_int("displayplayer", &displayplayer);
        add_game_int("zoom", &zoom);
        add_game_mobj("trigger", &trigger_obj);

                // important C-emulating stuff
        new_function("break", SF_Break);
        new_function("continue", SF_Continue);
        new_function("return", SF_Return);
        new_function("goto", SF_Goto);
        new_function("include", SF_Include);

                // standard FraggleScript functions
        new_function("print", SF_Print);
        new_function("rnd", SF_Rnd);
        new_function("input", SF_Input);
        new_function("beep", SF_Beep);
        new_function("clock", SF_Clock);
        new_function("wait", SF_Wait);
        new_function("startscript", SF_StartScript);

                // doom stuff
        new_function("exitlevel", SF_ExitLevel);
        new_function("tip", SF_Tip);
        new_function("message", SF_Message);

                // player stuff
        new_function("playermsg", SF_PlayerMsg);
        new_function("playertip", SF_PlayerTip);
        new_function("playeringame", SF_PlayerInGame);
        new_function("playername", SF_PlayerName);
        new_function("playerobj", SF_PlayerObj);

                // mobj stuff
        new_function("spawn", SF_Spawn);
        new_function("kill", SF_KillObj);
        new_function("removeobj", SF_RemoveObj);
        new_function("objx", SF_ObjX);
        new_function("objy", SF_ObjY);
        new_function("objz", SF_ObjZ);
        new_function("teleport", SF_Teleport);
        new_function("silentteleport", SF_SilentTeleport);
        new_function("damageobj", SF_DamageObj);
        new_function("player", SF_Player);
        new_function("objsector", SF_ObjSector);
        new_function("objflag", SF_ObjFlag);
        new_function("pushobj", SF_PushThing);
        new_function("objangle", SF_ObjAngle);

                // sector stuff
        new_function("floorheight", SF_FloorHeight);
        new_function("floortext", SF_FloorTexture);
        new_function("movefloor", SF_MoveFloor);
        new_function("ceilheight", SF_FloorHeight);
        new_function("moveceil", SF_MoveCeiling);
        new_function("ceiltext", SF_FloorTexture);
        new_function("lightlevel", SF_LightLevel);
        new_function("fadelight", SF_FadeLight);

                // cameras!
        new_function("setcamera", SF_SetCamera);
        new_function("clearcamera", SF_ClearCamera);

                // trig functions
        new_function("pointtoangle", SF_PointToAngle);
        new_function("pointtodist", SF_PointToDist);

                // sound functions
        new_function("startsound", SF_StartSound);
        new_function("startsectorsound", SF_StartSectorSound);
}
