// Emacs style mode select -*- C++ -*-
//--------------------------------------------------------------------------
//
// smmuserv
//
// Server listing program for smmu networking
// This runs on someones box somewhere
// New servers register with smmuserv and clients can find servers
// by querying it
//
// By Simon Howard
//
//-------------------------------------------------------------------------

#include <stdio.h>

typedef struct
{
  int socknum;
  unsigned long remote_ip;
} connection_t;

int listen_socket;

void main()
{
  
}
