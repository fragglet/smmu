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

void MN_PopupDrawer();
boolean MN_PopupResponder(event_t *ev);
void MN_Alert(char *message, ...);
void MN_Question(char *message, char *command);

extern char popup_message[128];
extern boolean popup_message_active;


#endif /** __MN_MISC_H__ **/
