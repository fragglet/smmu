// Emacs style mode select -*- C++ -*- 
//----------------------------------------------------------------------------
//
// Misc menu stuff
//
// By Simon Howard 
//
//----------------------------------------------------------------------------

#ifndef __MN_MISC_H__
#define __MN_MISC_H__

// pop-up messages

void MN_Alert(char *message, ...);
void MN_Question(char *message, char *command);

// help screens

void MN_StartHelpScreen();

// map colour selection

void MN_SelectColour(char *variable_name);

#endif /** __MN_MISC_H__ **/
