#ifndef __P_CHASE_H__
#define __P_CHASE_H__

#include "p_mobj.h"

typedef struct
{
        long x;
        long y;
        long z;
        angle_t angle;
        int updownangle;
} camera_t;


extern int chasex;
extern int chasey;
extern int chasez;
extern int chaseangle;
extern int chasecam_active;

extern int walkcam_active;

void P_ChaseSetupFrame();
void P_ChaseTicker();
void P_ChaseStart();
void P_ChaseEnd();
void P_ResetChasecam();
void P_ToggleChasecam();

void P_WalkTicker();
void P_ToggleWalk();
void P_ResetWalkcam();

#endif
