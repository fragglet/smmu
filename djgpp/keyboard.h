// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Include file for modified allegro keyboard.c
//
//--------------------------------------------------------------------------

#ifndef BOOMKEY_H
#define BOOMKEY_H

extern void (*boom_keyboard_lowlevel_callback)(int);
extern int boom_install_keyboard();
extern void boom_clear_keybuf();
extern int boom_keypressed();
extern int boom_readkey();
extern void boom_simulate_keypress(int key);
extern int boom_install_keyboard();
extern void boom_set_leds(int leds);

extern unsigned char boom_key_ascii_table[128];
extern unsigned char boom_key_capslock_table[128];
extern unsigned char boom_key_shift_table[128];

#endif /* BOOMKEY_H */
