// Emacs style mode select -*- C++ -*-
//--------------------------------------------------------------------------
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
//-------------------------------------------------------------------------
//
// New SWANTBLS (the old one was awful)
// This new one is much nicer and should be more portable
//
// By Simon Howard 'Fraggle'
//
//--------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef unsigned char byte;
int line_num;  // line number we are reading

//////////////////////////////////////////////////////////////////////////

// Useful utilities

//----------------------------------
// string trimming functions
// from the original swantbls source

// trim spaces off left of string
void LeftTrim(char *str)
{
  int k,n;
  
  k = strlen(str);
  if (k)
    {
      n = strspn(str," \t");
      memmove(str,str+n, k-n+1);
    }
}

// trim spaces off right of string

void RightTrim(char *str)
{
  int k,n;
  
  n = k = strlen(str);
  if (k)
    {
      n--;
      while (n>=0 && (str[n]=='\n' || str[n]==' ' || str[n]=='\t')) n--;
      str[n+1]='\0';
    }
}

// trim spaces off both ends of a string

void Trim(char *str)
{
  RightTrim(str);
  LeftTrim(str);
}

//-------------------------------------
// error handling

void error(char *error, ...)
{
  va_list args;
  va_start(args, error);

  vfprintf(stderr, error, args);

  va_end(args);
  
  exit(-1);
}

//--------------------------------------
// portable writing

void write_short(unsigned short s, FILE *fstream)
{
  char c;

  c = (s) & 255;       fwrite(&c, sizeof(c), 1, fstream);
  c = (s >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
}

void write_long(unsigned long l, FILE *fstream)
{
  char c;
  
  c = (l) & 255;       fwrite(&c, sizeof(c), 1, fstream);
  c = (l >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
  c = (l >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
  c = (l >>= 8) & 255; fwrite(&c, sizeof(c), 1, fstream);
}
  
//////////////////////////////////////////////////////////////////////////

// ANIMATED lump

// this should be done with structs, but I tried
// it and the compiler thought the struct was
// 24 bytes long when it was only 23.
// Also it makes for non-portable code, as we end
// up with the endianness problem

// for anim_type:

enum
  {
    at_end         = -1,
    at_flat        = 0,
    at_texture     = 1,
  };

#define animated_filename "animated.lmp"
FILE *animated_file;

// write the end to the animated lump

void Animated_Close()
{  
  byte ending_byte = at_end;
  fwrite(&ending_byte, sizeof(ending_byte), 1, animated_file);
}

// open the animated file

void Animated_Open()
{
  printf("creating " animated_filename "\n");
  animated_file = fopen(animated_filename, "wb");
  if(!animated_file)
    error("error opening " animated_filename "\n");  
  atexit(Animated_Close);
}

// parse lines in text file for flats

void Flats_ParseInput(char *inputline)
{
  byte anim_type = at_flat;
  char first_buffer[128];       // store lump names in seperate buffers
  char last_buffer[128];        // to avoid possible segfaults
  int speed;

  if(!animated_file)
    Animated_Open();
  
  // parse line
  
  sscanf(inputline,
	 "%i %s %s",
	 &speed,
	 last_buffer,
	 first_buffer);

  // limit length of lump names

  first_buffer[8] = '\0';
  last_buffer[8] = '\0';
  
  fwrite(&anim_type, sizeof(anim_type), 1, animated_file);
  fwrite(last_buffer, 9, sizeof(char), animated_file);
  fwrite(first_buffer, 9, sizeof(char), animated_file);
  write_long(speed, animated_file);  
}

// parse line for textures
// similar to flats read

void Textures_ParseInput(char *inputline)
{
  byte anim_type = at_texture;
  char first_buffer[128];       // store lump names in seperate buffers
  char last_buffer[128];        // to avoid possible segfaults
  int speed;

  if(!animated_file)
    Animated_Open();
  
  // parse line
  
  sscanf(inputline,
	 "%i %s %s",
	 &speed,
	 last_buffer,
	 first_buffer);

  // limit length of lump names

  first_buffer[8] = '\0';
  last_buffer[8] = '\0';

  fwrite(&anim_type, sizeof(anim_type), 1, animated_file);

  // why _are_ these 9 bytes instead of 8?
  fwrite(last_buffer, 9, sizeof(char), animated_file);
  fwrite(first_buffer, 9, sizeof(char), animated_file);
  write_long(speed, animated_file);
}

// sf: parser for swirly flats

void Swirly_ParseInput(char *inputline)
{
  byte anim_type = at_flat;
  char texture_name[128];
  int speed = 65536;            // must be >= 65536 for swirly
  
  if(!animated_file)
    Animated_Open();
  
  // get texture name
  
  sscanf(inputline, "%s", texture_name);

  printf("> %s\n", inputline);
  printf("add: %s\n", texture_name);
  
  // limit name length

  texture_name[8] = '\0';

  fwrite(&anim_type, sizeof(anim_type), 1, animated_file);
  fwrite(texture_name, 9, sizeof(char), animated_file);
  fwrite(texture_name, 9, sizeof(char), animated_file);
  write_long(speed, animated_file);  
}

////////////////////////////////////////////////////////////////////////////
//
// SWITCHES lump

typedef struct switch_s switch_t;

// for iwads
enum
  {
    iwads_end,           // end of lump
    iwads_shareware,
    iwads_registered,
    iwads_retail,
    num_iwads,
  };

#define switches_filename "switches.lmp"
FILE *switches_file;

// shutdown: close switches file

void Switches_Close()
{
  char empty_string[9] = "fragl:)";  // leave my mark

  // write 2 empty fields where the switches would be
  fwrite(empty_string, 9, sizeof(char), switches_file);
  fwrite(empty_string, 9, sizeof(char), switches_file);
  write_short(iwads_end, switches_file);
  
  fclose(switches_file);
}

// open switches file

void Switches_Open()
{
  printf("creating " switches_filename "\n");
  switches_file = fopen(switches_filename, "wb");
  if(!switches_file)
    error("error opening " switches_filename "\n");
  atexit(Switches_Close);
}

void Switches_ParseInput(char *inputline)
{
  int iwads;
  char off_text_buffer[128];
  char on_text_buffer[128];

  if(!switches_file)
    Switches_Open();
  
  sscanf(inputline,
	 "%i %s %s",
	 &iwads,
	 off_text_buffer,
	 on_text_buffer);

  // cut off long strings

  off_text_buffer[8] = '\0';
  on_text_buffer[8] = '\0';

  // check for invalid iwads
  
  if(iwads < iwads_shareware || iwads >= num_iwads)
    error("line %i: invalid iwads selected", line_num);
  
  // write fields
  
  fwrite(off_text_buffer, 9, sizeof(char), switches_file);
  fwrite(on_text_buffer, 9, sizeof(char), switches_file);
  write_short(iwads, switches_file);  
}

////////////////////////////////////////////////////////////////////////////
//
// Main
//

#define MAX_READ 128

void main(int argc, char *argv[])
{
  char *file_name;
  FILE *read_file;
  char read_buffer[MAX_READ +1];
  enum
    {
      rt_none,
      rt_switches,
      rt_flats,
      rt_textures,
      rt_swirly,       // [swirly flats]
    } read_type;

  printf
    (
     "-------------------------------------\n"
     "swantbls switch/table generator\n"
     "rewritten by Simon Howard 'Fraggle'\n"
     "http://fraggle.tsx.org/\n"
     "distributed under GNU GPL\n"
     "\n"
     );

  // get filename and open
  
  if(argc < 2)
    {
      error("usage: swantbls swanfile.dat\n");
    }
  else
    file_name = argv[1];

  if(! (read_file = fopen(file_name, "r")) )
    error("couldn't open '%s'\n", file_name);
  else
    printf("reading from '%s'\n", file_name);
  
  // read each line from the switches file now
  
  for(line_num = 1; ! feof(read_file) ; line_num++)
    {
      read_buffer[0] = '\0';      // clear string for next read
      // get the next line
      fgets(read_buffer, MAX_READ, read_file);
      Trim(read_buffer);

      // debug:
      //      puts(read_buffer);
      
      // check for comments
      if(read_buffer[0] == '#') continue;

      // check for empty line
      if(read_buffer[0] == '\0') continue;
      // check for new section
      if(read_buffer[0] == '[')
	{
	  char section_name[32];
	  
	  if(read_buffer[strlen(read_buffer)-1] != ']')
	    {
	      // missed ending ']'
	      error("line %i: missed ending ']'\n", line_num);
	    }

	  // copy out section name
	  strcpy(section_name, read_buffer+1);
	  section_name[strlen(section_name)-1] = '\0'; // remove ']'

	  if(!strcasecmp(section_name, "switches"))
	    read_type = rt_switches;
	  else if(!strcasecmp(section_name, "textures"))
	    read_type = rt_textures;
	  else if(!strcasecmp(section_name, "flats"))
	    read_type = rt_flats;
	  else if(!strcasecmp(section_name, "liquid"))
	    read_type = rt_swirly;
	  else
	    error("line %i: unknown section type '%s'\n",
		  line_num, section_name);

	  printf("parsing %s section\n", section_name);
	  
	  continue;
	}

      // normal read
      // call other function according to read type
      
      switch(read_type)
	{
	  case rt_switches:                   // switches
	    Switches_ParseInput(read_buffer);
	    break;
	    
	  case rt_flats:                      // flats
	    Flats_ParseInput(read_buffer);
	    break;
	    
	  case rt_textures:                   // wall textures
	    Textures_ParseInput(read_buffer);
	    break;

	  case rt_swirly:                     // liquid flats
	    Swirly_ParseInput(read_buffer);
	    break;
	    
	  default:
	    break;
	}
      
    }

  fclose(read_file);
}

//------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2000-04-30 19:12:12  fraggle
// Initial revision
//
//
//------------------------------------------------------------------------
