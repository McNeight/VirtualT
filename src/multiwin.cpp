/* multiwin.cpp */

/* $Id: multiwin.cpp,v 1.3 2008/02/01 06:18:04 kpettit1 Exp $ */

/*
 * Copyright 2006 Ken Pettit
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
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Window.H>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "multiwin.h"
#include "multiwin_icons.h"

int Fl_Multi_Window::m_WindowCount = 0;
int	Fl_Multi_Window::m_MinimizeRegions[200] = { -1 };

Fl_Multi_Window* gpDestroyPtr = 0;

IMPLEMENT_DYNCREATE(Fl_Multi_Window, VTObject)

void multi_window_idle_proc(void* w)
{
	if (gpDestroyPtr != 0)
	{
		gpDestroyPtr->do_callback();
		gpDestroyPtr = 0;
	}

}

void multi_window_def_callback(Fl_Widget *w, void *)
{
	w->hide();
	delete (Fl_Multi_Window *) w;
}

Fl_Multi_Window::Fl_Multi_Window(int x, int y, int w, int h, const char* title)
: Fl_Double_Window(x, y, w, h, title)
{
	int		c;

	m_pClientArea = new Fl_Window(MW_BORDER_WIDTH, MW_TITLE_HEIGHT, w - MW_BORDER_WIDTH*2,
		h - MW_BORDER_WIDTH - MW_TITLE_HEIGHT);
//	resizable(m_pClientArea);
	m_pLabel = new char[strlen(title) + 1];
	strcpy(m_pLabel, title);
	label(m_pLabel);

	// Set default Icon
	m_pIcon = new Fl_Pixmap(gMultiWinDoc_xpm);
	
	// Set default callback
	callback(multi_window_def_callback);

	m_InDragArea = 0;
	m_InResizeArea = 0;
	m_InIconArea = 0;
	m_IconArea[0] = VT_Rect(w-25, 6, 19, 20);
	m_IconArea[1] = VT_Rect(w-45, 6, 19, 20);
	m_IconArea[2] = VT_Rect(w-65, 6, 19, 20);

	m_NormalIcon[0] = &gCloseIcon;
	m_NormalIcon[1] = &gMaximizeIcon;
	m_NormalIcon[2] = &gMinimizeIcon;

	m_PushedIcon[0] = &gCloseIconSelected;
	m_PushedIcon[1] = &gMaximizeIconSelected;
	m_PushedIcon[2] = &gMinimizeIconSelected;

	m_InactiveIcon[0] = &gCloseIconInactive;
	m_InactiveIcon[1] = &gMaximizeIconInactive;
	m_InactiveIcon[2] = &gMinimizeIconInactive;

	m_CaptureMode = 0;
	m_NoResize = 0;
	m_Minimized = 0;
	m_Maximized = 0;
	m_MinimizeLoc = -1;

	if (m_WindowCount++ == 0)
	{
		Fl::add_idle(multi_window_idle_proc, parent());
	}

	if (m_MinimizeRegions[0] == -1)
	{
		for (c = 0; c < 200; c++)
			m_MinimizeRegions[c] = 0;
	}
}

Fl_Multi_Window::~Fl_Multi_Window()
{
	if (m_MinimizeLoc != -1)
		m_MinimizeRegions[m_MinimizeLoc] = 0;
	delete m_pLabel;
	delete m_pIcon;
	if (--m_WindowCount == 0)
		Fl::remove_idle(multi_window_idle_proc, parent());
}

/*
===============================================================
Routine to draw the border and title bar of the window
===============================================================
*/
void Fl_Multi_Window::draw(void)
{
	int	baseColor;
	int	hilight;
	int	c;

	if (parent()->child(parent()->children()-1) == this)
	{
		baseColor = FL_BLUE;
		hilight = 4;
	}
	else
	{
		baseColor = FL_BLUE+5;
		hilight = 2;
	}

	// Draw the window borders
	fl_color(baseColor);
	fl_rectf(0, 0, w(), MW_TITLE_HEIGHT);
	fl_rectf(0, MW_TITLE_HEIGHT, MW_BORDER_WIDTH, h()-MW_TITLE_HEIGHT);
	fl_rectf(w()-MW_BORDER_WIDTH, MW_TITLE_HEIGHT, MW_BORDER_WIDTH, h()-MW_TITLE_HEIGHT);
	fl_rectf(0, h()-MW_BORDER_WIDTH, w(), MW_BORDER_WIDTH);

	// Draw hilight bars base on activation state of the window
	fl_color(baseColor+hilight);
	fl_rectf(1, 1, w()-2, 2); 
	fl_color(baseColor+hilight-1);
	fl_rectf(1, 3, w()-2, 2);
	fl_line(1, 1, 1, h()-2);
	fl_line(w()-2, 1, w()-2, h()-2);
	fl_line(1, h()-2, w()-2, h()-2);
	if (baseColor == FL_BLUE)
	{
		for (c = 0; c < (m_NoResize ? 1 : 3); c++)
		{
			// Draw icons in upper right of window
			if ((m_InIconArea == c+1) && (m_LastInIconArea))
				m_PushedIcon[c]->draw(m_IconArea[c].x(), m_IconArea[c].y());
			else
				m_NormalIcon[c]->draw(m_IconArea[c].x(), m_IconArea[c].y());
		}
	}
	else
	{
		// Draw icons in upper right of window
		for (c = 0; c < (m_NoResize ? 1 : 3); c++)
			m_InactiveIcon[c]->draw(m_IconArea[c].x(), m_IconArea[c].y());
	}

	// Draw the icon
	m_pIcon->draw(7,8);

	// Draw the window title
	fl_font(FL_HELVETICA_BOLD, 14);
	if (baseColor == FL_BLUE)
	{
		fl_color(FL_BLACK);
		fl_draw(label(), 26, 23);
	}
	fl_color(FL_WHITE);
	fl_draw(label(), 25, 22);
}

/*
===============================================================
Routine to handle events such as window move/resize, icon 
processing, etc.
===============================================================
*/
int Fl_Multi_Window::handle(int event)
{
	int		mx, my;
	int		ix, iy;
	int		newx, newy;
	int		absx, absy;
	int		c;

	Fl_Group*	p;
	Fl_Widget*	prev;

	switch (event)
	{
	case FL_ENTER:
		return 1;

	case FL_DRAG:
		// Get window relative mouse coordinates
		mx = Fl::event_x();
		my = Fl::event_y();

		// Check if we are in capture mode
		if (m_CaptureMode)
		{
			// Check if moving the window
			if (m_InDragArea)
			{
				// Get absolute (screen) mouse coordinates
				Fl::get_mouse(mx, my);

				// Calculate new window position
				newx = m_WinAnchorX + mx - m_MouseAnchorX;
				newy = m_WinAnchorY + my - m_MouseAnchorY;

				// Restrict position
				if (newx + w() < 100)
					newx = 100 - w();
				if (newx > parent()->w()-MW_TITLE_HEIGHT)
					newx = parent()->w()-MW_TITLE_HEIGHT;
				if (newy < 0)
					newy = 0;
				if (newy > parent()->h()-MW_TITLE_HEIGHT)
					newy = parent()->h()-MW_TITLE_HEIGHT;

				// Move the window
				position(newx, newy);

				// If window is minimized, save location of the window
				if (m_Minimized)
				{
					m_MinimizeRect = VT_Rect(x(), y(), w(), h());
					if (m_MinimizeLoc != -1)
					{
						m_MinimizeRegions[m_MinimizeLoc] = 0;
						m_MinimizeLoc = -1;
					}
				}
			}
			else if (m_InIconArea)
			{
				ix = m_IconArea[m_InIconArea-1].x();
				iy = m_IconArea[m_InIconArea-1].y();

				// Dragging while in Capture to Icon area
				if  ((mx >= ix) && (mx <= m_IconArea[m_InIconArea-1].x1()) &&
					 (my >= iy) && (my <= m_IconArea[m_InIconArea-1].y1()) )
				{
					// Check if we were already in icon area before and skip drawing if so
					if (!m_LastInIconArea)
					{
						// Select window for drawing
						make_current();

						// Draw the pushed icon
						m_PushedIcon[m_InIconArea-1]->draw(m_IconArea[m_InIconArea-1].x(),
							m_IconArea[m_InIconArea-1].y());
						m_LastInIconArea = 1;
					}
				}
				else
					// Test if we were already out of Icon area and skip drawing if so
					if (m_LastInIconArea)
					{
						// Select window for drawing
						make_current();

						// Draw the normal ("unpushed") icon
						m_NormalIcon[m_InIconArea-1]->draw(m_IconArea[m_InIconArea-1].x(),
							m_IconArea[m_InIconArea-1].y());
						m_LastInIconArea = 0;
					}
			}
			else if (m_InResizeArea)
			{
				// Resizing the window
				Fl::get_mouse(mx, my);
				newx = m_WinAnchorX + mx - m_MouseAnchorX;
				newy = m_WinAnchorY + my - m_MouseAnchorY;

				// Set minimum window size
				if (newx < 100)
					newx = 100;
				if (newy < 100)
					newy = 100;

				// Resize window and it's client
				resize(x(), y(), newx, newy);
				m_pClientArea->resize(MW_BORDER_WIDTH, MW_TITLE_HEIGHT, newx - MW_BORDER_WIDTH*2,
					newy - MW_BORDER_WIDTH - MW_TITLE_HEIGHT);

				// Move location of Icon Areas base on new window size
				m_IconArea[0].x(w()-25);
				m_IconArea[1].x(w()-45);
				m_IconArea[2].x(w()-65);
			}
		}
		return 1;

	case FL_RELEASE:
		if (m_CaptureMode)
		{
			// Test if we are processing an Icon selection
			if (m_InIconArea)
			{
				// Test if button release while still in icon
				make_current();
				m_NormalIcon[m_InIconArea-1]->draw(m_IconArea[m_InIconArea-1].x(),
					m_IconArea[m_InIconArea-1].y());

				// Test if mouse released while in icon area and do some action
				if (m_LastInIconArea)
				{
					// Check which Icon was selected
					if (m_InIconArea == 1)
					{
						Fl::release();
						CloseIconSelected();
					}
					else if (m_InIconArea == 2)
						MaximizeIconSelected();
					else if (m_InIconArea == 3)
						MinimizeIconSelected();
				}

				// Clear flags indicating active Icon manipulation
				m_LastInIconArea = 0;
				m_InIconArea = 0;
			}

			// Release the mouse and all capture variables
			Fl::release();
			m_CaptureMode = 0;
			m_InDragArea = 0;
			m_InResizeArea = 0;
		}
		break;

	case FL_PUSH:
		// Get the mouse event coordinates
		mx = Fl::event_x();
		my = Fl::event_y();

		// Take action depending on which mouse button selected
	    switch (Fl::event_button())
		{
		case FL_LEFT_MOUSE:
			// Check if we are the first window
			p = parent();
			prev = p->child(p->children()-1);
			if (prev != this)
			{
				// Insert ourselves as the first window
				p->remove(this);
				p->insert(*this, p->children());

				// Show window to bring to top. redraw and take focus
				show();
				redraw();
				take_focus();

				// Redraw previously selected window to change border color
				prev->redraw();
			}

			// Test if in any of the icon areas
			for (c = 0; c < (m_NoResize ? 1 : 3); c++)
			{
				// Get coordinates for this icon
				ix = m_IconArea[c].x();
				iy = m_IconArea[c].y();

				// Dragging while in Capture to Icon area
				if  ((mx >= ix) && (mx <= m_IconArea[c].x1()) &&
					 (my >= iy) && (my <= m_IconArea[c].y1()) )
				{
					// Setup capture mode for this icon
					m_InIconArea = c+1;
					Fl::grab(this);
					m_CaptureMode = 1;
					m_LastInIconArea = 1;

					// Draw the "Pushed" version of this icon
					make_current();
					if (m_PushedIcon[c] != 0)
						m_PushedIcon[c]->draw(m_IconArea[c].x(), m_IconArea[c].y());
					return 1;
				}
			}

			// Check if in title bar area
			if (my < MW_TITLE_HEIGHT)
			{
				// Configure capture mode
				Fl::get_mouse(absx, absy);
				Fl::grab(this);
				m_InDragArea = 1;
				m_CaptureMode = 1;

				// Get anchor point for mouse and window
				m_MouseAnchorX = absx;
				m_MouseAnchorY = absy;
				m_WinAnchorX = x();
				m_WinAnchorY = y();
			}

			// Check if in resize area
			if ((mx > w()-12-MW_BORDER_WIDTH) && (my > h()-12-MW_BORDER_WIDTH) &&
				!m_NoResize)
			{
				// Configure capture mode
				Fl::get_mouse(absx, absy);
				Fl::grab(this);
				m_InResizeArea = 1;
				m_CaptureMode = 1;

				// Get anchor point for mouse and window size
				m_MouseAnchorX = absx;
				m_MouseAnchorY = absy;
				m_WinAnchorX = w();
				m_WinAnchorY = h();
			}

			// Do other mouse button push stuff
			Fl_Double_Window::handle(event);
			return 1;
		}
		break;

	default:
		break;
	}

	return Fl_Double_Window::handle(event);
}

/*
===============================================================
Routine to handle selection of the close window icon
===============================================================
*/
void Fl_Multi_Window::CloseIconSelected()
{
	Fl_Window* p;

	// Check if it is okay to close
	if (!OkToClose())
		return;

	hide();
	p = (Fl_Window*) parent();
	p->remove(this);
	if (p->children() != 0)
		p->child(p->children()-1)->redraw();
	gpDestroyPtr = this;
}

int Fl_Multi_Window::OkToClose(void)
{
	return TRUE;
}
/*
===============================================================
Routine to handle selection of the restore icon
===============================================================
*/
void Fl_Multi_Window::MaximizeIconSelected()
{
	int		newx, newy;

	// Check if window minimized
	if (m_Maximized)
	{
		m_NormalIcon[1] = &gMaximizeIcon;
		m_PushedIcon[1] = &gMaximizeIconSelected;
		m_InactiveIcon[1] = &gMaximizeIconInactive;
		m_Maximized = 0;

		// Restore window to original location
		resize(m_RestoreRect.x(), m_RestoreRect.y(), m_RestoreRect.w(), m_RestoreRect.h());
		m_pClientArea->resize(MW_BORDER_WIDTH, MW_TITLE_HEIGHT, m_RestoreRect.w() - MW_BORDER_WIDTH*2,
			m_RestoreRect.h() - MW_BORDER_WIDTH - MW_TITLE_HEIGHT);

		// Move location of Icon Areas base on new window size
		m_IconArea[0].x(w()-25);
		m_IconArea[1].x(w()-45);
		m_IconArea[2].x(w()-65);

		make_current();
		redraw();
	}
	else
	{
		m_NormalIcon[1] = &gRestoreIcon;
		m_PushedIcon[1] = &gRestoreIconSelected;
		m_InactiveIcon[1] = &gRestoreIconInactive;
		m_Maximized = 1;

		// If the window isn't minimized, save the current position for restoring purposes
		if (!m_Minimized)
			m_RestoreRect = VT_Rect(x(), y(), w(), h());

		// Move window so it occupies the enntire client area of the parent
		// such that the side and bottom borders are hidden
		newx = parent()->w()+MW_BORDER_WIDTH*2;
		newy = parent()->h()+MW_BORDER_WIDTH;

		resize(-MW_BORDER_WIDTH, 0, newx, newy);
		m_pClientArea->resize(MW_BORDER_WIDTH, MW_TITLE_HEIGHT, newx - MW_BORDER_WIDTH*2,
			newy - MW_BORDER_WIDTH - MW_TITLE_HEIGHT);

		// Move location of Icon Areas base on new window size
		m_IconArea[0].x(w()-25);
		m_IconArea[1].x(w()-45);
		m_IconArea[2].x(w()-65);



		make_current();
		redraw();
	}
}

/*
===============================================================
Routine to handle selection of the minimize icon
===============================================================
*/
void Fl_Multi_Window::MinimizeIconSelected()
{
	// Check if window minimized
	if (m_Minimized)
	{
		m_NormalIcon[2] = &gMinimizeIcon;
		m_PushedIcon[2] = &gMinimizeIconSelected;
		m_InactiveIcon[2] = &gMinimizeIconInactive;

		// Restore window to original location
		resize(m_RestoreRect.x(), m_RestoreRect.y(), m_RestoreRect.w(), m_RestoreRect.h());

		// Move location of Icon Areas base on new window size
		m_IconArea[0].x(w()-25);
		m_IconArea[1].x(w()-45);
		m_IconArea[2].x(w()-65);

		parent()->insert(*this, parent()->children()-1);
		m_Minimized = 0;
		m_MinimizeRegions[m_MinimizeLoc] = 0;
		m_MinimizeLoc = -1;
		make_current();
		redraw();
	}
	else
	{
		// Change icon to be displayed 
		m_NormalIcon[2] = &gRestoreIcon;
		m_PushedIcon[2] = &gRestoreIconSelected;
		m_InactiveIcon[2] = &gRestoreIconInactive;

		// If the window isn't maximized, save the current position for restoring purposes
		if (!m_Maximized)
			m_RestoreRect = VT_Rect(x(), y(), w(), h());

		// Determine if this window has a specific minimize location
		if (m_MinimizeRect.w() != 0)
		{
			// Determine if specified coords are still within the view
			if ((m_MinimizeRect.x1() <= parent()->w()) && (m_MinimizeRect.y1() <= parent()->h()))
			{
				// Move to specified locaiton
				resize(m_MinimizeRect.x(), m_MinimizeRect.y(), MW_MINIMIZE_WIDTH, MW_TITLE_HEIGHT);

				// Move location of Icon Areas base on new window size
				m_IconArea[0].x(w()-25);
				m_IconArea[1].x(w()-45);
				m_IconArea[2].x(w()-65);
			}
			else
				m_MinimizeRect = VT_Rect(0, 0, 0, 0);
		}

		// Check if minimize operation to the next available location
		if (m_MinimizeRect.w() == 0)
		{
			// Check if window already has a minimize location assigned
			if (m_MinimizeLoc == -1)
			{
				// Assign a new minimize location
				int c;
				for (c = 0; c < 200; c++)
				{
					if (m_MinimizeRegions[c] == 0)
					{
						m_MinimizeLoc = c;
						m_MinimizeRegions[c] = 1;
						break;
					}
				}
			}

			// Minimize window to the assigned location
			int winPerLine = parent()->w() / MW_MINIMIZE_WIDTH;
			int locX = m_MinimizeLoc % winPerLine;
			int locY = m_MinimizeLoc / winPerLine;
			resize(locX * MW_MINIMIZE_WIDTH, parent()->h()-(locY+1)*MW_TITLE_HEIGHT,
				MW_MINIMIZE_WIDTH, MW_TITLE_HEIGHT);

			// Move location of Icon Areas base on new window size
			m_IconArea[0].x(w()-25);
			m_IconArea[1].x(w()-45);
			m_IconArea[2].x(w()-65);

			parent()->insert(*this, 0);
			parent()->child(parent()->children()-1)->redraw();
		}

		// Insert this window at the bottom
		m_Minimized = 1;
		make_current();
		redraw();
	}
}
