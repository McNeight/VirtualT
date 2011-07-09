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
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tooltip.H>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "VirtualT.h"
#include "m100emu.h"
#include "multiwin_icons.h"
#include "idetabs.h"

#define BORDER 3
#define EXTRASPACE 16
enum {LEFT, RIGHT, SELECTED};

IMPLEMENT_DYNCREATE(Fl_Ide_Tabs, VTObject)

void cb_idetabs(Fl_Widget* w, void *args)
{
	Fl_Ide_Tabs*	pTabs;

	// Test for closure event
	if (args == (void *) FL_IDE_TABS_CLOSE)
	{
		// Find the widget being closed and remove it from the group
		pTabs = (Fl_Ide_Tabs *) w->parent();
		if (pTabs->children() == 1)
			pTabs->hide();
		
		pTabs->remove(w);
		Fl::delete_widget(w);
		if (pTabs->children() != 0)
		{
			pTabs->parent()->redraw();
			pTabs->redraw();
			int count = pTabs->children();
			for (int c = 0; c < count; c++)
				pTabs->resizable(pTabs->child(c));
		}
	}
}

Fl_Ide_Tabs::Fl_Ide_Tabs(int x, int y, int w, int h, const char* title)
: Fl_Tabs(x, y, w, h, title)
{
	m_prevInRect = FALSE;
	m_pushInRect = FALSE;

	callback(cb_idetabs);
}

Fl_Ide_Tabs::~Fl_Ide_Tabs()
{
}

/*
===============================================================
Redraw routine
===============================================================
*/
void Fl_Ide_Tabs::draw() {
  Fl_Widget *v = value();
  int H = tab_height();

  if (damage() & FL_DAMAGE_ALL) { // redraw the entire thing:
    Fl_Color c = v ? v->color() : color();

    draw_box(box(), x(), y()+(H>=0?H:0), w(), h()-(H>=0?H:-H), c);

    if (selection_color() != c) {
      // Draw the top 5 lines of the tab pane in the selection color so
      // that the user knows which tab is selected...
      if (H >= 0) fl_push_clip(x(), y() + H, w(), 5);
      else fl_push_clip(x(), y() + h() - H - 4, w(), 5);

      draw_box(box(), x(), y()+(H>=0?H:0), w(), h()-(H>=0?H:-H),
               selection_color());

      fl_pop_clip();

   }
    if (v) draw_child(*v);
  } else { // redraw the child
    if (v) update_child(*v);
  }

  	  // Draw the close box
      if (H >= 0) 
		  m_closeRect = VT_Rect(x() + w() - H, 0, H, H);
      else 
		  m_closeRect = VT_Rect(x() + w() - H, y() + h() - H, H, H);

	  draw_box(box(), x() + w() - H, 0, H, H,
			   selection_color());
	  fl_color(FL_BLACK);

	  int rx1 = m_closeRect.x() + 8;
	  int ry1 = m_closeRect.y();
	  int rx2 = m_closeRect.x1() - 8;
	  int ry2 = m_closeRect.y1();
	  fl_line(rx1, ry1 + 7, rx2, ry2 - 9);
	  fl_line(rx1, ry1 + 8, rx2, ry2 - 8);
	  fl_line(rx1, ry1 + 9, rx2, ry2 - 7);
	  fl_line(rx1, ry2 - 7, rx2, ry1 + 9);
	  fl_line(rx1, ry2 - 8, rx2, ry1 + 8);
	  fl_line(rx1, ry2 - 9, rx2, ry1 + 7);

  if (damage() & (FL_DAMAGE_SCROLL|FL_DAMAGE_ALL)) {
    int p[128]; int wp[128];
    int selected = tab_positions(p,wp);
    int i;
    Fl_Widget*const* a = array();
    for (i=0; i<selected; i++)
      draw_tab(x()+p[i], x()+p[i+1], wp[i], H, a[i], LEFT);
    for (i=children()-1; i > selected; i--)
      draw_tab(x()+p[i], x()+p[i+1], wp[i], H, a[i], RIGHT);
    if (v) {
      i = selected;
      draw_tab(x()+p[i], x()+p[i+1], wp[i], H, a[i], SELECTED);
    }

	// Draw the close box
  }
}

/*
===============================================================
Calculates the position of all tabs based on the children in
the tab group.
===============================================================
*/
int Fl_Ide_Tabs::tab_positions(int* p, int* wp) {
	int selected = 0;
	Fl_Widget*const* a = array();
	int i;
	char prev_draw_shortcut = fl_draw_shortcut;

	fl_draw_shortcut = 1;
	p[0] = Fl::box_dx(box());
	for (i=0; i<children(); i++) 
	{
		Fl_Widget* o = *a++;
		if (o->visible()) 
			selected = i;

	    int wt = 0; int ht = 0;
		const char *pLabel = o->label();
		o->measure_label(wt,ht);
		if (pLabel[strlen(pLabel)-1] != '*')
		{
			int ah = 0; int aw = 0;
			fl_font(o->labelfont(), o->labelsize());
			fl_measure("*", aw, ah);
			wt += aw;
		}

		wp[i]  = wt+EXTRASPACE;
		p[i+1] = p[i]+wp[i]+BORDER;
	}
	fl_draw_shortcut = prev_draw_shortcut;

	int r = w() - m_closeRect.w();
	if (p[i] <= r) return selected;
	// uh oh, they are too big:
	// pack them against right edge:
	p[i] = r;
	for (i = children(); i--;) 
	{
		int l = r-wp[i];
		if (p[i+1] < l) 
			l = p[i+1];
		if (p[i] <= l) 
			break;
		p[i] = l;
		r -= EXTRASPACE;
	}
	// pack them against left edge and truncate width if they still don't fit:
	for (i = 0; i<children(); i++) 
	{
		if (p[i] >= i*EXTRASPACE) 
			break;
		p[i] = i*EXTRASPACE;
		int W = w()-1-m_closeRect.w()-EXTRASPACE*(children()-i) - p[i];
		if (wp[i] > W) 
			wp[i] = W;
	}
	// adjust edges according to visiblity:
	for (i = children(); i > selected; i--) 
	{
		p[i] = p[i-1]+wp[i-1];
	}
	return selected;
}

/*
===============================================================
return space needed for tabs.  Negative to put them on the bottom:
===============================================================
*/
int Fl_Ide_Tabs::tab_height() {
  int H = h();
  int H2 = y();
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (o->y() < y()+H) H = o->y()-y();
    if (o->y()+o->h() > H2) H2 = o->y()+o->h();
  }
  H2 = y()+h()-H2;
  if (H2 > H) return (H2 <= 0) ? 0 : -H2;
  else return (H <= 0) ? 0 : H;
}

/*
===============================================================
Draws a single tab
===============================================================
*/
void Fl_Ide_Tabs::draw_tab(int x1, int x2, int W, int H, Fl_Widget* o, int what) {
  int sel = (what == SELECTED);
  int dh = Fl::box_dh(box());
  int dy = Fl::box_dy(box());
  char prev_draw_shortcut = fl_draw_shortcut;
  fl_draw_shortcut = 1;

  Fl_Boxtype bt = (o==push() &&!sel) ? fl_down(box()) : box();

  // compute offsets to make selected tab look bigger
  int yofs = sel ? 0 : BORDER;

  if ((x2 < x1+W) && what == RIGHT) x1 = x2 - W;

  if (H >= 0) {
    if (sel) fl_clip(x1, y(), x2 - x1, H + dh - dy);
    else fl_clip(x1, y(), x2 - x1, H);

    H += dh;

//    Fl_Color c = sel ? selection_color() : o->selection_color();
	Fl_Color c = selection_color();

    draw_box(bt, x1, y() + yofs, W, H + 10 - yofs, c);

    // Save the previous label color
    Fl_Color oc = o->labelcolor();

    // Draw the label using the current color...
	if (sel) o->labelfont(o->labelfont() + 1);
	o->labeltype(FL_NORMAL_LABEL);
    o->labelcolor(sel ? labelcolor() : o->labelcolor());    
    o->draw_label(x1, y() + yofs, W, H - yofs, FL_ALIGN_CENTER);
	if (sel) o->labelfont(o->labelfont() - 1);

    // Restore the original label color...
    o->labelcolor(oc);

    if (Fl::focus() == this && o->visible())
      draw_focus(box(), x1, y(), W, H);

    fl_pop_clip();
  } else {
    H = -H;

    if (sel) fl_clip(x1, y() + h() - H - dy, x2 - x1, H + dy);
    else fl_clip(x1, y() + h() - H, x2 - x1, H);

    H += dh;

///    Fl_Color c = sel ? selection_color() : o->selection_color();
	Fl_Color c = selection_color();

    draw_box(bt, x1, y() + h() - H - 10, W, H + 10 - yofs, c);

    // Save the previous label color
    Fl_Color oc = o->color();

    // Draw the label using the current color...
	if (sel) o->labelfont(o->labelfont() + 1);
    o->labelcolor(sel ? labelcolor() : o->labelcolor());
    o->draw_label(x1, y() + h() - H, W, H - yofs, FL_ALIGN_CENTER);
	if (sel) o->labelfont(o->labelfont() - 1);

    // Restore the original label color...
    o->labelcolor(oc);

    if (Fl::focus() == this && o->visible())
      draw_focus(box(), x1, y() + h() - H, W, H);

    fl_pop_clip();
  }
  fl_draw_shortcut = prev_draw_shortcut;
}

/*
===============================================================
Handle events
===============================================================
*/
int Fl_Ide_Tabs::handle(int e)
{
	int		drawbox = FALSE;
	int		ret;
	Fl_Widget* w;

	switch (e)
	{
	case FL_MOVE:
		// If the mouse was pressed in the close rect, just exit
		if (m_pushInRect)
			return 1;


		// Test if mouse is in our close rect
		if (m_closeRect.PointInRect(Fl::event_x(), Fl::event_y()))
		{
			if (m_prevInRect)
				return 0;

			window()->make_current();
			// Draw the activity box
			fl_color(FL_BLACK);
			drawbox = TRUE;
			m_prevInRect = TRUE;
		}
		else
		{
			if (!m_prevInRect)
				return Fl_Tabs::handle(e);
			window()->make_current();
			fl_color(selection_color());
			drawbox = TRUE;
			m_prevInRect = FALSE;
		}

		if (drawbox)
		{
			int rx1 = m_closeRect.x() + x();
			int ry1 = m_closeRect.y() + y();
			window()->make_current();
			fl_rect(rx1 + 4, ry1 + 4, m_closeRect.w() - 8, m_closeRect.h() - 8);
		}
		break;

	case FL_LEAVE:
		if (m_prevInRect && !m_pushInRect)
		{
			fl_color(selection_color());
			m_prevInRect = FALSE;
			int rx1 = m_closeRect.x() + x();
			int ry1 = m_closeRect.y() + y();
			fl_rect(rx1 + 4, ry1 + 4, m_closeRect.w() - 8, m_closeRect.h() - 8);
		}
		break;

	case FL_PUSH:
		// If the mouse is in the close rect, indicate a push in rect event
		if (m_prevInRect)
		{
			m_pushInRect = TRUE;
			return 1;
		}

		w = value();
		ret =  Fl_Tabs::handle(e);
		if (w != value())
		{
			if (w != NULL)	
				w->hide();
			if (value() != NULL)
				value()->show();
		}

		return ret;

	case FL_RELEASE:
		// If the mouse was previously pused in the close box, then test for closure
		if (m_pushInRect)
		{
			window()->make_current();

			// Test if mouse still in rect
			if (m_closeRect.PointInRect(Fl::event_x(), Fl::event_y()))
			{
				value()->do_callback(value(), FL_IDE_TABS_CLOSE);
			}
			m_pushInRect = FALSE;
		}

		break;
	}

	return Fl_Tabs::handle(e);
}

