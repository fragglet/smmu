#ifndef __HU_OVER_H__
#define __HU_OVER_H__

    /*************** heads up font **************/
        // copied from v_video.h
#define HU_FONTSTART    '!'     /* the first font characters */
#define HU_FONTEND      (0x7f) /*jff 2/16/98 '_' the last font characters */
// Calculate # of glyphs in font.
#define HU_FONTSIZE     (HU_FONTEND - HU_FONTSTART + 1) 

void HU_LoadFont();
void HU_WriteText(unsigned char *s, int x, int y);
int HU_StringWidth(unsigned char *s);

    /************** overlay drawing ***************/

typedef struct overlay_s overlay_t;

struct overlay_s
{
        int x, y;
        void (*drawer)(int x, int y);
};

enum
{
        ol_health,
        ol_ammo,
        ol_armor,
        ol_weap,
        ol_key,
        ol_frag,
        ol_status,
        NUMOVERLAY
};

extern overlay_t overlay[NUMOVERLAY];

void HU_OverlayDraw();
void HU_OverlayStyle();
void HU_ToggleHUD();

#endif
