/***************************** FraggleScript ******************************/
                // Copyright(C) 1999 Simon Howard 'Fraggle' //
//
// Preprocessor.
//
// The preprocessor must be called when the script is first loaded.
// It performs 2 functions:
//      1: blank out comments (which could be misinterpreted)
//      2: makes a list of all the sections held within {} braces
//

/* includes ************************/

#include <stdio.h>
#include <string.h>
#include "c_io.h"
#include "z_zone.h"

#include "t_main.h"
#include "t_parse.h"
#include "t_spec.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"

/*********** {} sections *************/

// during preprocessing all of the {} sections
// are found. these are stored in a hash table
// according to their offset in the script. 
// functions here deal with creating new section_t's
// and finding them from a given offset.

#define section_hash(b)           \
        ( (int)((b)-current_script->data) % SECTIONSLOTS)

void clear_sections()
{
        int i;

        for(i=0; i<SECTIONSLOTS; i++)
                current_script->sections[i] = NULL;

        // clear the variables while we're here

        for(i=0; i<VARIABLESLOTS; i++)
                current_script->variables[i] = NULL;
}

section_t *new_section(char *brace)
{
        int n;
        section_t *newsec;

                // create section
                // make level so its cleared at start of new level
        newsec = Z_Malloc(sizeof(section_t), PU_LEVEL, 0);
        newsec->start = brace;

        // hook it into the hashchain

        n = section_hash(brace);
        newsec->next = current_script->sections[n];
        current_script->sections[n] = newsec;

        return newsec;
}

        // find a section_t from the location of the starting { brace
section_t *find_section_start(char *brace)
{
        int n = section_hash(brace);
        section_t *current;

        current = current_script->sections[n];

        // use the hash table: check the appropriate hash chain

        while(current)
        {
                if(current->start == brace)
                        return current;
                current = current->next;
        }

        return NULL;    // not found
}

        // find a section_t from the location of the ending } brace
section_t *find_section_end(char *brace)
{
        int n;

        // hash table is no use, they are hashed according to
        // the offset of the starting brace

        // we have to go through every entry to find from the
        // ending brace

        for(n=0; n<SECTIONSLOTS; n++)      // check all sections in all chains
        {
          section_t *current = current_script->sections[n];

          while(current)
          {
             if(current->end == brace)
                 return current;        // found it
             current = current->next;
          }
        }

        return NULL;    // not found
}

/********** labels ****************/

// labels are also found during the
// preprocessing. these are of the form
//
//      label_name:
//
// and are used for goto statements.

                // from parse.c
#define isop(c)   !( ( (c)<='Z' && (c)>='A') || ( (c)<='z' && (c)>='a') || \
                     ( (c)<='9' && (c)>='0') || ( (c)=='_') )

        // create a new label. pass the location inside the script
svariable_t *new_label(char *labelptr)
{
        svariable_t *newlabel;   // labels are stored as variables
        char labelname[128];
        char *temp, *temp2;

                // copy the label name from the script up to ':'
        for(temp=labelptr, temp2 = labelname; *temp!=':'; temp++, temp2++)
                *temp2 = *temp;
        *temp2 = NULL;  // end string in null

        newlabel = new_variable(current_script, labelname, svt_label);

        // put neccesary data in the label

        newlabel->value.labelptr = labelptr;

        return newlabel;
}

/*********** main loop **************/

char *process_find_char(char *data, char find)
{
        while(*data)
        {
// DEBUG                C_Printf("%c\n",*data); 
                if(*data==find) return data;
                if(*data=='\"')       // found a quote: ignore stuff in it
                {
                        data++;
                        while(*data && *data != '\"')
                        {
                                        // escape sequence ?
                                if(*data=='\\') data++;
                                data++;
                        }
                              // error: end of script in a constant
                        if(!*data) return NULL;
                }
                        // comments: blank out
                if(*data=='/' && *(data+1)=='*')        // /* -- */ comment
                {
                        while(*data && (*data != '*' || *(data+1) != '/') )
                        {
                                *data=' '; data++;
                        }
                        if(*data)
                          *data = *(data+1) = ' ';   // blank the last bit
                        else
                        {
                          rover = data;
                          // script terminated in comment
                          script_error("script terminated inside comment\n");
                        }
                }
                if(*data=='/' && *(data+1)=='/')        // // -- comment
                        while(*data != '\n')
                        {
                                *data=' '; data++;       // blank out
                        }
                if(*data==':'  // ':' -- a label
                   && current_script->scriptnum != -1)   // not levelscript
                {
                        char *labelptr = data-1;

                        while(!isop(*labelptr)) labelptr--;
                        new_label(labelptr+1);
                }
                
                if(*data=='{')  // { -- } sections: add 'em
                {
                        section_t *newsec = new_section(data);

                        newsec->type = st_empty;
                                // find the ending } and save
                        newsec->end = process_find_char(data+1, '}');
                        if(!newsec->end)
                        {                // brace not found
                                rover = data;
                                script_error("section error: no ending brace\n");
                                return NULL;
                        }
                                // continue from the end of the section
                        data = newsec->end;
                }
                data++;
        }
        return NULL;
}

void preprocess(script_t *script)
{
        current_script = script;
        script->len = strlen(script->data);

        clear_sections();

        process_find_char(script->data, NULL);  // fill in everything

}

