/* multiwin.cpp */

/* $Id: idetabs.cpp,v 1.1 2011/07/09 08:16:21 kpettit1 Exp $ */

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
#ifndef TAB_HEIGHT
#define	TAB_HEIGHT 24
#endif

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
: Fl_Group(x, y, w, h, title)
{
	m_prevInRect = FALSE;
	m_pushInRect = FALSE;
	m_hasCloseButton = FALSE;
	box(FL_THIN_UP_BOX);
	push_ = 0;

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

#if 0
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
#endif

	int H2 = H < 0 ? -H : H;

	// Draw the background rectangle
	fl_clip(x(), y(), w(), h()+2);
	draw_box(box(), x(), y(), w(), h()+2, fl_rgb_color(228,226,213));
	fl_pop_clip();

	// Draw the close box
	if (m_hasCloseButton)
	{
		if (H >= 0) 
			m_closeRect = VT_Rect(x() + w() - H2, 0, H2, H2);
		else 
			m_closeRect = VT_Rect(x() + w() - H2, y() + h() - H2, H2, H2);
		draw_box(box(), x() + w() - H2, 0, H2, H2, selection_color());
	}

	// Draw the bottom border line for the "TAB"
	if (H >= 0)
		draw_box(box(), x()+1, H2-3, w(), H2, selection_color());
	else
		draw_box(box(), x()+1, h()-H2-4, w(), 5, selection_color());
	fl_color(fl_rgb_color(127,157,184));
	if (H<0)
		fl_line(x()+1, h()-H2, w(), h()-H2);
	else
		fl_line(x()+1, H-4, w()-H-1, H-4);

	// Draw the 'X' in the close box
	if (m_hasCloseButton)
	{
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
	}
	// Draw the tabs
	if (damage() & (FL_DAMAGE_SCROLL|FL_DAMAGE_ALL)) 
	{
		int p[128]; int wp[128];
		int selected = tab_positions(p,wp);
		int i;
		Fl_Widget*const* a = array();
		for (i=children()-1; i > selected; i--)
		  draw_tab(x()+H2+p[i], x()+H2/2+3+p[i+1], wp[i], H, a[i], RIGHT);
		for (i=selected - 1; i>=0; i--)
		  draw_tab(x()+H2+p[i], x()+H2/2+3+p[i+1], wp[i], H, a[i], LEFT);
		if (v) {
		  i = selected;
		  draw_tab(x()+H2+p[i], x()+H2/2+3+p[i+1], wp[i], H, a[i], SELECTED);
		}
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
    int H = tab_height();
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
  int ret;

  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (o->y() < y()+H) H = o->y()-y();
    if (o->y()+o->h() > H2) H2 = o->y()+o->h();
  }
  H2 = y()+h()-H2;
  if (H2 > H) 
  {
	  ret = (H2 <= 0) ? 0 : -H2;
	  if (ret < -24)
		  ret = -24;
  }
  else
  {
	  ret = (H <= 0) ? 0 : H;
	  if (ret > 24)
		  ret = 24;
  }
  return ret;
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
  int i;
  char prev_draw_shortcut = fl_draw_shortcut;
  fl_draw_shortcut = 1;

  Fl_Boxtype bt = (o==push() &&!sel) ? fl_down(box()) : box();

  // compute offsets to make selected tab look bigger
  int yofs =  sel ? BORDER + 1 : BORDER;

  if (H >= 0) {
    if (sel) fl_clip(x1, y(), x2 - x1, H + dh - dy);
    else fl_clip(x1, y(), x2 - x1, H);

    H += dh;

	Fl_Color c = sel ? fl_lighter(selection_color()) : selection_color();

    // Save the previous label color
    Fl_Color oc = o->labelcolor();

    fl_pop_clip();
	fl_clip(x1-H-10, y()-2, W+H+5, H+2);

	// Draw TAB left curved portion
	fl_color(c);
	int	y2 = y() + BORDER;
	fl_line(x1, y2, x2, y2);
	fl_line(x1-2, y2+1, x2, y2+1);
	fl_line(x1-4, y2+2, x2, y2+2);
	for (i=3; i < H-BORDER-5; i++)
	{
		if (!sel)
		{
			c = fl_color_average(c, FL_BLACK, .985f);
			fl_color(c);
		}
		fl_line(x1-(2+i), y2+i, x2, y2+i);
	}

	// Draw the label using the current color...
	if (sel) o->labelfont(o->labelfont() + 1);
	o->labeltype(FL_NORMAL_LABEL);
    o->labelcolor(sel ? labelcolor() : o->labelcolor());    
    o->draw_label(x1, y() + yofs-3, W, H - yofs, FL_ALIGN_LEFT);
	if (sel) o->labelfont(o->labelfont() - 1);

    // Restore the original label color...
    o->labelcolor(oc);

	// Draw TAB Border
//	fl_color(FL_BLACK);
	if (sel)
		fl_color(fl_rgb_color(127,157,184));
	else
		fl_color(fl_rgb_color(172,168,153));
	i+=4;
	fl_line(x1-(3+i-5), y2+i-5, x1-5,y2+2);
	fl_line(x1-4,y2+1, x1-3,y2+1);
	fl_line(x1-2,y2, x1-1,y2);
	fl_line(x1,y2-1, x2,y2-1);
	fl_line(x2,y2-1, x2,y2+i-5);
	if (!sel)
	{
		fl_color(fl_rgb_color(127,157,184));
		fl_line(x1-(3+i-2), y2+i-5, x2,y2+i-5);
	}

	fl_pop_clip();
  } else {
    H = -H;
    H += dh;

    // Save the previous label color
	Fl_Color c = sel ? fl_lighter(selection_color()) : selection_color();
    Fl_Color oc = o->labelcolor();
	fl_clip(x(), y(), w()+5, h());

	// Draw TAB left curved portion
	fl_color(c);
	int	y2 = y() + h() -6;
	fl_line(x1, y2, x2, y2);
	fl_line(x1-2, y2-1, x2, y2-1);
	fl_line(x1-4, y2-2, x2, y2-2);
	for (i=3; i < H-BORDER-3; i++)
	{
		if (!sel)
		{
			c = fl_color_average(c, FL_BLACK, .985f);
			fl_color(c);
		}
		fl_line(x1-(2+i), y2-i, x2, y2-i);
	}

	// Draw the label using the current color...
	if (sel) o->labelfont(o->labelfont() + 1);
	o->labeltype(FL_NORMAL_LABEL);
    o->labelcolor(sel ? labelcolor() : o->labelcolor());    
    o->draw_label(x1, y() +h()-H+ yofs-3, W, H - yofs, FL_ALIGN_LEFT);
	if (sel) o->labelfont(o->labelfont() - 1);

    // Restore the original label color...
    o->labelcolor(oc);

	// Draw TAB Border
	if (sel)
		fl_color(fl_rgb_color(127,157,184));
	else
		fl_color(fl_rgb_color(172,168,153));
	i+=4;
	fl_line(x1-(3+i-5), y2-i+5, x1-5,y2-2);
	fl_line(x1-4,y2-1, x1-3,y2-1);
	fl_line(x1-2,y2, x1-1,y2);
	fl_line(x1,y2+1, x2,y2+1);
	fl_line(x2,y2+1, x2,y2-i+5);
	if (!sel)
	{
		fl_color(fl_rgb_color(127,157,184));
		fl_line(x1-(3+i-2), y2-i+5, x2,y2-i+5);
	}

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
				return orig_handle(e);
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
		break;
		w = value();
		ret =  orig_handle(e);
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

	return orig_handle(e);
}

// this is used by fluid to pick tabs:
Fl_Widget *Fl_Ide_Tabs::which(int event_x, int event_y) {
  int H = tab_height();
  if (H < 0) {
    if (event_y > y()+h() || event_y < y()+h()+H) return 0;
  } else {
    if (event_y > y()+H || event_y < y()) return 0;
  }
  if (H<0) H=-H;
  if (event_x < x()) return 0;
  int p[128], wp[128];
  tab_positions(p, wp);
  for (int i=0; i<children(); i++) {
    if (event_x < x()+H/2+p[i+1]) return child(i);
  }
  return 0;
}

void Fl_Ide_Tabs::redraw_tabs()
{
  int H = tab_height();
  if (H >= 0) {
    H += Fl::box_dy(box());
    damage(FL_DAMAGE_SCROLL, x(), y(), w(), H);
  } else {
    H = Fl::box_dy(box()) - H;
    damage(FL_DAMAGE_SCROLL, x(), y() + h() - H, w(), H);
  }
}

int Fl_Ide_Tabs::orig_handle(int event) {

  Fl_Widget *o;
  int i;

  switch (event) {

  case FL_PUSH: {
    int H = tab_height();
    if (H >= 0) {
      if (Fl::event_y() > y()+H) return Fl_Group::handle(event);
    } else {
      if (Fl::event_y() < y()+h()+H) return Fl_Group::handle(event);
    }}
  case FL_DRAG:
  case FL_RELEASE:
    o = which(Fl::event_x(), Fl::event_y());
    if (event == FL_RELEASE) {
      push(0);
      if (o && Fl::visible_focus() && Fl::focus()!=this) { 
        Fl::focus(this);
        redraw_tabs();
      }
      if (o && value(o)) {
        set_changed();
	do_callback();
      }
      Fl_Tooltip::current(o);
    } else {
      push(o);
    }
    return 1;
  case FL_MOVE: {
    int ret = Fl_Group::handle(event);
    Fl_Widget *o = Fl_Tooltip::current(), *n = o;
    int H = tab_height();
    if ( (H>=0) && (Fl::event_y()>y()+H) )
      return ret;
    else if ( (H<0) && (Fl::event_y() < y()+h()+H) )
      return ret;
    else { 
      n = which(Fl::event_x(), Fl::event_y());
      if (!n) n = this;
    }
    if (n!=o)
      Fl_Tooltip::enter(n);
    return ret; }
  case FL_FOCUS:
  case FL_UNFOCUS:
    if (!Fl::visible_focus()) return Fl_Group::handle(event);
    if (Fl::event() == FL_RELEASE ||
	Fl::event() == FL_SHORTCUT ||
	Fl::event() == FL_KEYBOARD ||
	Fl::event() == FL_FOCUS ||
	Fl::event() == FL_UNFOCUS) {
      redraw_tabs();
      if (Fl::event() == FL_FOCUS || Fl::event() == FL_UNFOCUS) return 0;
      else return 1;
    } else return Fl_Group::handle(event);
  case FL_KEYBOARD:
    switch (Fl::event_key()) {
      case FL_Left:
        if (child(0)->visible()) return 0;
	for (i = 1; i < children(); i ++)
	  if (child(i)->visible()) break;
	value(child(i - 1));
	set_changed();
	do_callback();
        return 1;
      case FL_Right:
        if (child(children() - 1)->visible()) return 0;
	for (i = 0; i < children(); i ++)
	  if (child(i)->visible()) break;
	value(child(i + 1));
	set_changed();
	do_callback();
        return 1;
      case FL_Down:
        redraw();
        return Fl_Group::handle(FL_FOCUS);
      default:
        break;
    }
    return Fl_Group::handle(event);
  case FL_SHORTCUT:
    for (i = 0; i < children(); ++i) {
      Fl_Widget *c = child(i);
      if (c->test_shortcut(c->label())) {
        char sc = !c->visible();
        value(c);
        if (sc) set_changed();
        do_callback();
        return 1;
      }
    }
    return Fl_Group::handle(event);
  case FL_SHOW:
    value(); // update visibilities and fall through
  default:
    return Fl_Group::handle(event);

  }
}

int Fl_Ide_Tabs::push(Fl_Widget *o) {
  if (push_ == o) return 0;
  if (push_ && !push_->visible() || o && !o->visible())
  {
    if (push_)
		push_->hide();
	if (o)
		o->show();
    redraw_tabs();
  }
  push_ = o;
  return 1;
}

// The value() is the first visible child (or the last child if none
// are visible) and this also hides any other children.
// This allows the tabs to be deleted, moved to other groups, and
// show()/hide() called without it screwing up.
Fl_Widget* Fl_Ide_Tabs::value() {
  Fl_Widget* v = 0;
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (v) o->hide();
    else if (o->visible()) v = o;
    else if (!i) {o->show(); v = o;}
  }
  return v;
}

// Setting the value hides all other children, and makes this one
// visible, iff it is really a child:
int Fl_Ide_Tabs::value(Fl_Widget *newvalue) {
  Fl_Widget*const* a = array();
  int ret = 0;
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (o == newvalue) {
      if (!o->visible()) ret = 1;
      o->show();
    } else {
      o->hide();
    }
  }
  return ret;
}

