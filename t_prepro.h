// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//

#ifndef __PREPRO_H__
#define __PREPRO_H__

typedef struct section_s section_t;
typedef struct label_s label_t;
#define SECTIONSLOTS 17
#define LABELSLOTS 17

#include "t_parse.h"

void preprocess(script_t *script);

/***** {} sections **********/

section_t *find_section_start(char *brace);
section_t *find_section_end(char *brace);

struct section_s
{
  char *start;    // offset of starting brace {
    char *end;      // offset of ending brace   }
  int type;       // section type: for() loop, while() loop etc
  
  union
  {
    struct
    {
      char *loopstart;  // positioned before the while()
    } data_loop;
  } data; // data for section
  
  section_t *next;        // for hashing
};

enum    // section types
{
  st_empty,       // none: empty {} braces
  st_if,          // if() statement             
  st_loop,        // loop
};

/****** goto labels ***********/

label_t *labelforname(char *labelname);

#endif

