// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
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
// Extern definitions for netmodule_t's
//
//---------------------------------------------------------------------------

#ifndef NET_MODULES
#define NET_MODULES

#include "config.h"
#include "sv_serv.h"

// loopback

extern netmodule_t loopback_client; 
extern netmodule_t loopback_server; 

// tcp/ip (udp)

#ifdef TCPIP

extern boolean tcpip_support;

extern void UDP_InitLibrary();
extern int UDP_Resolve(char *location);

extern netmodule_t udp;

#endif

#ifdef DJGPP

// external driver

extern netmodule_t external;

// serial/modem

extern netmodule_t modem;
extern netmodule_t serial;

extern void Ser_Dial();
extern void Ser_WaitForCall();

#endif /* DJGPP */

#endif /* NET_MODULES */

//-------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2001-01-13 22:39:36  fraggle
// TCPIP #define moved to config.h, autoconfed
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//-------------------------------------------------------------------------
