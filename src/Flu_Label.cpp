// $Id: Flu_Label.cpp,v 1.3 2003/08/27 03:58:26 jbryan Exp $

/***************************************************************
 *                FLU - FLTK Utility Widgets 
 *  Copyright (C) 2002 Ohio Supercomputer Center, Ohio State University
 *
 * This file and its content is protected by a software license.
 * You should have received a copy of this license with this file.
 * If not, please contact the Ohio Supercomputer Center immediately:
 * Attn: Jason Bryan Re: FLU 1224 Kinnear Rd, Columbus, Ohio 43212
 * 
 ***************************************************************/



#include "FLU/Flu_Label.h"

Flu_Label :: Flu_Label( int x, int y, int w, int h, const char* l )
  : Fl_Box( x, y, w, h, 0 )
{
  align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
  _label = NULL;
  label( l );
  //box( FL_FLAT_BOX );
  box( FL_NO_BOX );
}

Flu_Label :: ~Flu_Label()
{
  if( _label )
    delete[] _label;
}

void Flu_Label :: label( const char* l )
{
  if( _label )
    delete[] _label;
  if( l == NULL )
    {
      _label = new char[1];
      _label[0] = '\0';
    }
  else
    {
      _label = new char[strlen(l)+1];
      strcpy( _label, l );
    }
  Fl_Box::label( _label );
  redraw();
}
