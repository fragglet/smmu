#ifndef __P_SKIN_H__
#define __P_SKIN_H__

typedef struct skin_s skin_t;

#include "info.h"
#include "sounds.h"
#include "st_stuff.h"
#include "r_defs.h"

enum
{
   sk_plpain,
   sk_pdiehi,
   sk_oof,
   sk_slop,
   sk_punch,
   sk_radio,
   sk_pldeth,
   NUMSKINSOUNDS
};

struct skin_s
{
        char      *spritename;   // 4 chars
        char      *skinname;     // name of the skin: eg 'marine'
        spritenum_t sprite;     // set by initskins
        char      *sounds[NUMSKINSOUNDS];
        char      *facename;         // statusbar face
        patch_t   **faces;
};

#define MAXSKINS 256

extern skin_t marine;
extern skin_t **skins;
extern char **spritelist;       // new spritelist, same format as sprnames
        // in info.c, but includes skins sprites.

void P_InitSkins();

void P_ListSkins();
void P_ChangeSkin();
void P_ParseSkin(int lumpnum);
void P_SetSkin(skin_t *skin, int playernum);

#endif
