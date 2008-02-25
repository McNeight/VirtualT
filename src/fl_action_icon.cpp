/* fl_action_icon.cpp */

/* $Id: fl_action_icon.cpp,v 1.1 2008/01/26 14:42:51 kpettit1 Exp $ */

/*
 * Copyright 2008 Ken Pettit
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>
#include "fl_action_icon.h"

#include <string.h>
#include <stdio.h>

Fl_Menu_Item	gEmptyMenu[] = {
	{ "Empty", 0, 0, 0, 0 },
	{ 0 }
};

/*
=======================================================
Fl_Action_Icon:	This is the class constructor
=======================================================
*/
Fl_Action_Icon::Fl_Action_Icon(int x, int y, int w, int h, const char *title) :
  Fl_Box(FL_NO_BOX, x, y, w, h, "")
{
	m_pImage = NULL;
	m_pMenu = gEmptyMenu;
	m_pPopup = new Fl_Menu_Button(0, 0, 100, 400, title);
	m_pPopup->type(Fl_Menu_Button::POPUP3);
	m_pPopup->hide();
}

/*
==========================================================
SetUsageRange:	This function sets the range of the usage
				box.  This range is used to perform
				scaling during usage updates.
==========================================================
*/
void Fl_Action_Icon::set_image(Fl_Image* image)
{
	m_pImage = image->copy();
	Fl_Box::image(m_pImage);
}

/*
==========================================================
Handles the events sent to the action_icon
==========================================================
*/
int Fl_Action_Icon::handle(int event)
{
	int		button, xp, yp;

	switch (event)
	{
	case FL_PUSH:
		button = Fl::event_button();
		switch (button)
		{
		case FL_LEFT_MOUSE:
			// Create a pop-up menu
			xp = Fl::event_x();
			yp = Fl::event_y();

			m_pPopup->menu(m_pMenu);	
			m_pPopup->popup();
		}
		break;

	}
	return Fl_Box::handle(event);
}

/*
==========================================================
Set the popup menu.
==========================================================
*/
void Fl_Action_Icon::menu(Fl_Menu_Item* pMenu)
{
	m_pMenu = pMenu;
}


/*
==========================================================
Draw the action icon.
==========================================================
*/
void Fl_Action_Icon::draw(void)
{
	Fl_Box::draw();

	// Draw the label
	window()->make_current();
	fl_color(FL_BLACK);
	fl_draw(label(),x() +  m_pImage->w() + 5, y() + h() -5 );
}

