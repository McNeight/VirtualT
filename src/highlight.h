/*
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <FL/Fl_Text_Buffer.H>
#include "My_Text_Editor.h"

#define TEXTSIZE 12

typedef struct _HighlightCtrl 
{
	My_Text_Buffer		*textbuf;
	My_Text_Buffer		*stylebuf;
	My_Text_Buffer		*op_stylebuf;
	My_Text_Editor		*te;
	int					cppfile;
} HighlightCtrl_t;


//
// 'compare_keywords()' - Compare two keywords...
//
int compare_keywords(const void *a, const void *b);

//
// 'style_parse()' - Parse text and produce style data.
//
//
void style_parse(const char *text, char *style, int length) ;
//
// 'style_init()' - Initialize the style buffer...
//

void style_init(HighlightCtrl_t* pHlCtrl);
//
// 'style_unfinished_cb()' - Update unfinished styles.
//

void style_unfinished_cb(int, void*); 

//
// 'style_update()' - Update the style buffer...
//

void old_style_update(	int        pos,		// I - Position of update
             	int        nInserted,	// I - Number of inserted chars
	     		int        nDeleted,	// I - Number of deleted chars
             	int        /*nRestyled*/,	// I - Number of restyled chars
	     		const char * /*deletedText*/,// I - Text that was deleted
             	void       *cbArg);

void style_update(	int        pos,		// I - Position of update
             	int        nInserted,	// I - Number of inserted chars
	     		int        nDeleted,	// I - Number of deleted chars
             	int        /*nRestyled*/,	// I - Number of restyled chars
	     		const char * /*deletedText*/,// I - Text that was deleted
             	void       *cbArg);

#endif
