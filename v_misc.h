#ifndef __V_MISC_H__
#define __V_MISC_H__

void V_InitMisc();

/******************** setting modes *******************/

void V_Mode();
void V_ModeList();
void V_ResetMode();
extern int v_mode;

/************************ font ************************/

#define V_FONTSTART    '!'     // the first font character
#define V_FONTEND      (0x7f) // jff 2/16/98 '_' the last font characters
// Calculate # of glyphs in font.
#define V_FONTSIZE     (V_FONTEND - V_FONTSTART + 1) 

        // font colours
#define FC_BRICK        "\x80"
#define FC_TAN          "\x81"
#define FC_GRAY         "\x82"
#define FC_GREEN        "\x83"
#define FC_BROWN        "\x84"
#define FC_GOLD         "\x85"
#define FC_RED          "\x86"
#define FC_BLUE         "\x87"
#define FC_ORANGE       "\x88"
#define FC_YELLOW       "\x89"

void V_WriteText(unsigned char *s, int x, int y);
void V_LoadFont();
int V_StringWidth(unsigned char *s);

/******************** box drawing *********************/
void V_DrawBox(int, int, int, int);



/******************* 'loading' pic ********************/
void V_DrawLoading();
void V_SetLoading(int total, char *mess);
void V_LoadingIncrease();
void V_LoadingSetTo(int amount);


/******************** fps ticker **********************/
void V_FPSDrawer();
void V_FPSTicker();
extern int v_ticker;


#endif
