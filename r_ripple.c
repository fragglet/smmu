/********************* Flat ripple distortion *****************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //

// Takes a normal, flat texture and distorts it like the water distortion
// in Quake.

#include "doomdef.h"
#include "doomstat.h"
#include "tables.h"

#include "r_defs.h"
#include "r_data.h"
#include "w_wad.h"
#include "v_video.h"
#include "z_zone.h"

        // swirl factors determine the number of waves per
        // flat width
        // 1 cycle per 64 units
#define swirlfactor (8192/64)
        // 1 cycle per 32 units (2 in 64)
#define swirlfactor2 (8192/32)

char *normalflat;
char distortedflat[4096];
int swirly_water;       // hack
int xoffset[64];
int yoffset[64];
int swirltic = -1;

// DEBUG: draw lines on the flat to see the distortion
void R_DrawLines()
{
        int x,y;

        for(x=0; x<64; x++)
          for(y=0; y<64; y++)
            if((!x%8) || (!y%8)) normalflat[(y<<6)+x]=0;
}

#define AMP 3
#define AMP2 2

char *R_DistortedFlat(int flatnum)
{
        int x, y, i, newy;
        long sinvalue, sinvalue2;

        normalflat = W_CacheLumpNum(firstflat + flatnum, PU_STATIC);

//        R_DrawLines();

                // if the offsets have been made for this tic already,
                // dont make them again
        if(swirltic != gametic)
            for(i=0; i<64; i++)
            {
                // one is x, one is y
                // they must be slightly different to add an element
                // of unpredictablity to the effect

                // gametic*n determines the speed
                // 2 waves are used and added together: sinvalue
                // and sinvalue2 are the offsets from each individual
                // wave. The two waves have different swirlfactors to
                // ensure they never get in phase and cancel each other
                // out. It looks better if the distortion keeps moving.

                // 128 is added to the offsets to ensure that no negative
                // values turn up

                sinvalue = (i * swirlfactor + gametic*110 + 900) & 8191;
                sinvalue2 = (i * swirlfactor2 + gametic*90 + 300) & 8191;
                xoffset[i] = 128 + ((finesine[sinvalue]*AMP) >> FRACBITS)
                       + ((finesine[sinvalue2]*AMP2) >> FRACBITS);

                sinvalue = (i * swirlfactor + gametic*130 + 700) & 8191;
                sinvalue2 = (i * swirlfactor2 + gametic*65 + 1200) & 8191;
                yoffset[i] = 128 + ((finesine[sinvalue]*AMP) >> FRACBITS)
                       + ((finesine[sinvalue2]*AMP2) >> FRACBITS);

                        // do not rebuild the offsets until next tic
                swirltic = gametic;
            }


                // create the new flat using the offsets

                // newy is neccesary because the x offset at the
                // offset y co-ordinate is not the same as the
                // x offset at the original y co-ordinate
                // if plain x,y are used, 'holes' appear in the
                // texture


        for(x=0; x<64; x++)
          for(y=0; y<64; y++)
            distortedflat[ ((newy = (y+yoffset[x]) & 63) << 6)
                          + ((x+xoffset[newy]) & 63) ] =
                          normalflat[(y << 6) + x];

                // free the original
        Z_ChangeTag(normalflat, PU_CACHE);

        return distortedflat;
}

void R_HackDrawer()
{
        int x,y;
        static int lumpnum = -1;
        unsigned char *source;
        unsigned char *dest;

        source = R_DistortedFlat(lumpnum == -1 ? 
                lumpnum = R_FlatNumForName("NUKAGE1") : lumpnum);

        for(x=93<<hires; x<230<<hires; x++)
        {
                for(y=70<<hires; y<157<<hires; y++)
                {
                        dest = screens[0] + (SCREENWIDTH<<hires)*y + x;
                        *dest = main_tranmap[ ((*dest)<<8) + 
                                 ( source[((y&63) << 6) + (x&63)] ) ];
                }
        }
}

