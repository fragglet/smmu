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


#ifndef __VARIABLE_H__
#define __VARIABLE_H__

typedef struct svariable_s svariable_t;
#define VARIABLESLOTS 16

#include "m_fixed.h"
#include "t_parse.h"
#include "p_mobj.h"

// hash the variables for speed: this is the hashkey

#define variable_hash(n)                \
              (   ( (n)[0] + (n)[1] +   \
                   ((n)[1] ? (n)[2] +   \
                   ((n)[2] ? (n)[3]  : 0) : 0) ) % VARIABLESLOTS )

     // svariable_t
struct svariable_s
{
  char *name;
  int type;       // vt_string or vt_int: same as in svalue_t
  union
  {
    char *s;
    long i;
    mobj_t *mobj;
    fixed_t fixed;
    
    char **pS;              // pointer to game string
    int *pI;                // pointer to game int
    mobj_t **pMobj;         // pointer to game obj
    fixed_t *pFixed;
    
    void (*handler)();      // for functions
    char *labelptr;         // for labels
  } value;
  svariable_t *next;       // for hashing
};

// variable types

enum
{
  svt_string,
  svt_int,
  svt_mobj,         // a map object
  svt_function,     // functions are stored as variables
  svt_label,        // labels for goto calls are variables
  svt_const,        // const
  svt_fixed,        // fixed_t
  svt_pInt,         // pointer to game int
  svt_pString,      // pointer to game string
  svt_pMobj,        // pointer to game mobj
  svt_pFixed,       // pointer to fixed_t
};

// variables

void T_ClearHubScript();

void init_variables();
svariable_t *new_variable(script_t *script, char *name, int vtype);
svariable_t *find_variable(char *name);
svariable_t *variableforname(script_t *script, char *name);
svalue_t getvariablevalue(svariable_t *v);
void setvariablevalue(svariable_t *v, svalue_t newvalue);
void clear_variables(script_t *script);

svariable_t *add_game_int(char *name, int *var);
svariable_t *add_game_string(char *name, char **var);
svariable_t *add_game_mobj(char *name, mobj_t **mo);

// functions

svalue_t evaluate_function(int start, int stop);   // actually run a function
svariable_t *new_function(char *name, void (*handler)() );

// arguments to handler functions

#define MAXARGS 128
extern int t_argc;
extern svalue_t *t_argv;
extern svalue_t t_return;

#endif

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2000-07-28 21:52:00  fraggle
// floating point math in FraggleScript
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//---------------------------------------------------------------------------

