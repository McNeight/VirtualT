//
// "$Id: My_Text_Editor.cpp,v 1.3 2013/02/08 00:05:27 kpettit1 Exp $"
//
// Copyright 2001-2003 by Bill Spitzak and others.
// Original code Copyright Mark Edel.  Permission to distribute under
// the LGPL for the FLTK library granted by Mark Edel.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@fltk.org".
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <FL/Fl.H>
#include "My_Text_Editor.h"
#include <FL/fl_ask.H>


My_Text_Editor::My_Text_Editor(int X, int Y, int W, int H,  const char* l)
    : My_Text_Display(X, Y, W, H, l) {
  mCursorOn = 1;
  insert_mode_ = 1;
  key_bindings = 0;
  autobrace = 0;
  auto_brace_mode = 0;
  smart_indent = 1;
  m_pStatusBar = NULL;

  // handle the default key bindings
  add_default_key_bindings(&key_bindings);

  // handle everything else
  default_key_function(kf_default);
}

My_Text_Editor::Key_Binding* My_Text_Editor::global_key_bindings = 0;

static int ctrl_a(int, My_Text_Editor* e);

// These are the default key bindings every widget should start with
static struct {
  int key;
  int state;
  My_Text_Editor::Key_Func func;
} default_key_bindings[] = {
  { FL_Escape,    My_Text_Editor_ANY_STATE, My_Text_Editor::kf_ignore     },
  { FL_Enter,     My_Text_Editor_ANY_STATE, My_Text_Editor::kf_enter      },
  { FL_KP_Enter,  My_Text_Editor_ANY_STATE, My_Text_Editor::kf_enter      },
  { FL_BackSpace, My_Text_Editor_ANY_STATE, My_Text_Editor::kf_backspace  },
  { FL_Insert,    My_Text_Editor_ANY_STATE, My_Text_Editor::kf_insert     },
  { FL_Delete,    My_Text_Editor_ANY_STATE, My_Text_Editor::kf_delete     },
  { FL_Home,      0,                        My_Text_Editor::kf_move       },
  { FL_End,       0,                        My_Text_Editor::kf_move       },
  { FL_Left,      0,                        My_Text_Editor::kf_move       },
  { FL_Up,        0,                        My_Text_Editor::kf_move       },
  { FL_Right,     0,                        My_Text_Editor::kf_move       },
  { FL_Down,      0,                        My_Text_Editor::kf_move       },
  { FL_Page_Up,   0,                        My_Text_Editor::kf_move       },
  { FL_Page_Down, 0,                        My_Text_Editor::kf_move       },
  { FL_Home,      FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_End,       FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_Left,      FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_Up,        FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_Right,     FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_Down,      FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_Page_Up,   FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_Page_Down, FL_SHIFT,                 My_Text_Editor::kf_shift_move },
  { FL_Home,      FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_End,       FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_Left,      FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_Up,        FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_Right,     FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_Down,      FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_Page_Up,   FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_Page_Down, FL_CTRL,                  My_Text_Editor::kf_ctrl_move  },
  { FL_Home,      FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
  { FL_End,       FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
  { FL_Left,      FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
  { FL_Up,        FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
  { FL_Right,     FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
  { FL_Down,      FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
  { FL_Page_Up,   FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
  { FL_Page_Down, FL_CTRL|FL_SHIFT,         My_Text_Editor::kf_c_s_move   },
//{ FL_Clear,	  0,                        My_Text_Editor::delete_to_eol },
  { 'z',          FL_CTRL,                  My_Text_Editor::kf_undo	  },
  { '/',          FL_CTRL,                  My_Text_Editor::kf_undo	  },
  { 'x',          FL_CTRL,                  My_Text_Editor::kf_cut        },
  { FL_Delete,    FL_SHIFT,                 My_Text_Editor::kf_cut        },
  { 'c',          FL_CTRL,                  My_Text_Editor::kf_copy       },
  { FL_Insert,    FL_CTRL,                  My_Text_Editor::kf_copy       },
  { 'v',          FL_CTRL,                  My_Text_Editor::kf_paste      },
  { FL_Insert,    FL_SHIFT,                 My_Text_Editor::kf_paste      },
  { 'a',          FL_CTRL,                  ctrl_a                        },

#ifdef __APPLE__
  // Define CMD+key accelerators...
  { 'z',          FL_COMMAND,               My_Text_Editor::kf_undo       },
  { 'x',          FL_COMMAND,               My_Text_Editor::kf_cut        },
  { 'c',          FL_COMMAND,               My_Text_Editor::kf_copy       },
  { 'v',          FL_COMMAND,               My_Text_Editor::kf_paste      },
  { 'a',          FL_COMMAND,               ctrl_a                        },
#endif // __APPLE__

  { 0,            0,                        0                             }
};

void My_Text_Editor::add_default_key_bindings(Key_Binding** list) {
  for (int i = 0; default_key_bindings[i].key; i++) {
    add_key_binding(default_key_bindings[i].key,
                    default_key_bindings[i].state,
                    default_key_bindings[i].func,
                    list);
  }
}

My_Text_Editor::Key_Func
My_Text_Editor::bound_key_function(int key, int state, Key_Binding* list) {
  Key_Binding* cur;
  for (cur = list; cur; cur = cur->next)
    if (cur->key == key)
      if (cur->state == My_Text_Editor_ANY_STATE || cur->state == state)
        break;
  if (!cur) return 0;
  return cur->function;
}

void
My_Text_Editor::remove_all_key_bindings(Key_Binding** list) {
  Key_Binding *cur, *next;
  for (cur = *list; cur; cur = next) {
    next = cur->next;
    delete cur;
  }
  *list = 0;
}

void
My_Text_Editor::remove_key_binding(int key, int state, Key_Binding** list) {
  Key_Binding *cur, *last = 0;
  for (cur = *list; cur; last = cur, cur = cur->next)
    if (cur->key == key && cur->state == state) break;
  if (!cur) return;
  if (last) last->next = cur->next;
  else *list = cur->next;
  delete cur;
}

void
My_Text_Editor::add_key_binding(int key, int state, Key_Func function,
                                Key_Binding** list) {
  Key_Binding* kb = new Key_Binding;
  kb->key = key;
  kb->state = state;
  kb->function = function;
  kb->next = *list;
  *list = kb;
}

////////////////////////////////////////////////////////////////

#define NORMAL_INPUT_MOVE 0

static void kill_selection(My_Text_Editor* e) {
  if (e->buffer()->selected()) {
    e->insert_position(e->buffer()->primary_selection()->start());
    e->buffer()->remove_selection();
  }
}

int My_Text_Editor::kf_default(int c, My_Text_Editor* e) {
  if (!c || (!isprint(c) && c != '\t')) return 0;
  char s[2] = "\0";
  s[0] = (char)c;
  kill_selection(e);
  if (e->insert_mode()) e->insert(s);
  else e->overstrike(s);
  e->show_insert_position();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();
  else e->set_changed();
  return 1;
}

int My_Text_Editor::kf_ignore(int, My_Text_Editor*) {
  return 0; // don't handle
}

int My_Text_Editor::kf_backspace(int, My_Text_Editor* e) {
  if (!e->buffer()->selected() && e->move_left())
    e->buffer()->select(e->insert_position(), e->insert_position()+1);
  kill_selection(e);
  e->show_insert_position();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback(); else e->set_changed();
  return 1;
}

int My_Text_Editor::kf_enter(int, My_Text_Editor* e) {
  kill_selection(e);
  e->insert("\n");
  e->show_insert_position();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback(); else e->set_changed();
  return 1;
}

extern void fl_text_drag_me(int pos, My_Text_Display* d);

int My_Text_Editor::kf_move(int c, My_Text_Editor* e) {
  int i;
  int selected = e->buffer()->selected();
  if (!selected)
    e->dragPos = e->insert_position();
  e->buffer()->unselect();
  switch (c) {
  case FL_Home:
      e->insert_position(e->buffer()->line_start(e->insert_position()));
      break;
    case FL_End:
      e->insert_position(e->buffer()->line_end(e->insert_position()));
      break;
    case FL_Left:
      e->move_left();
      break;
    case FL_Right:
      e->move_right();
      break;
    case FL_Up:
      e->move_up();
      break;
    case FL_Down:
      e->move_down();
      break;
    case FL_Page_Up:
      for (i = 0; i < e->mNVisibleLines - 1; i++) e->move_up();
      break;
    case FL_Page_Down:
      for (i = 0; i < e->mNVisibleLines - 1; i++) e->move_down();
      break;
  }
  e->show_insert_position();
  return 1;
}

int My_Text_Editor::kf_shift_move(int c, My_Text_Editor* e) {
  kf_move(c, e);
  fl_text_drag_me(e->insert_position(), e);
  return 1;
}

int My_Text_Editor::kf_ctrl_move(int c, My_Text_Editor* e) {
  if (!e->buffer()->selected())
    e->dragPos = e->insert_position();
  if (c != FL_Up && c != FL_Down) {
    e->buffer()->unselect();
    e->show_insert_position();
  }
  switch (c) {
    case FL_Home:
      e->insert_position(0);
      e->scroll(0, 0);
      break;
    case FL_End:
      e->insert_position(e->buffer()->length());
      e->scroll(e->count_lines(0, e->buffer()->length(), 1), 0);
      break;
    case FL_Left:
      e->previous_word();
      break;
    case FL_Right:
      e->next_word();
      break;
    case FL_Up:
      e->scroll(e->mTopLineNum-1, e->mHorizOffset);
      break;
    case FL_Down:
      e->scroll(e->mTopLineNum+1, e->mHorizOffset);
      break;
    case FL_Page_Up:
      e->insert_position(e->mLineStarts[0]);
      break;
    case FL_Page_Down:
      e->insert_position(e->mLineStarts[e->mNVisibleLines-2]);
      break;
  }
  return 1;
}

int My_Text_Editor::kf_c_s_move(int c, My_Text_Editor* e) {
  kf_ctrl_move(c, e);
  fl_text_drag_me(e->insert_position(), e);
  return 1;
}

static int ctrl_a(int, My_Text_Editor* e) {
  // make 2+ ^A's in a row toggle select-all:
  int i = e->buffer()->line_start(e->insert_position());
  if (i != e->insert_position())
    return My_Text_Editor::kf_move(FL_Home, e);
  else {
    if (e->buffer()->selected())
      e->buffer()->unselect();
    else
      My_Text_Editor::kf_select_all(0, e);
  }
  return 1;
}

int My_Text_Editor::kf_home(int, My_Text_Editor* e) {
    return kf_move(FL_Home, e);
}

int My_Text_Editor::kf_end(int, My_Text_Editor* e) {
  return kf_move(FL_End, e);
}

int My_Text_Editor::kf_left(int, My_Text_Editor* e) {
  return kf_move(FL_Left, e);
}

int My_Text_Editor::kf_up(int, My_Text_Editor* e) {
  return kf_move(FL_Up, e);
}

int My_Text_Editor::kf_right(int, My_Text_Editor* e) {
  return kf_move(FL_Right, e);
}

int My_Text_Editor::kf_down(int, My_Text_Editor* e) {
  return kf_move(FL_Down, e);
}

int My_Text_Editor::kf_page_up(int, My_Text_Editor* e) {
  return kf_move(FL_Page_Up, e);
}

int My_Text_Editor::kf_page_down(int, My_Text_Editor* e) {
  return kf_move(FL_Page_Down, e);
}


int My_Text_Editor::kf_insert(int, My_Text_Editor* e) {
  e->insert_mode(e->insert_mode() ? 0 : 1);
  return 1;
}

int My_Text_Editor::kf_delete(int, My_Text_Editor* e) {
  if (!e->buffer()->selected())
    e->buffer()->select(e->insert_position(), e->insert_position()+1);
  kill_selection(e);
  e->show_insert_position();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback(); else e->set_changed();
  return 1;
}

int My_Text_Editor::kf_copy(int, My_Text_Editor* e) {
  if (!e->buffer()->selected()) return 1;
  const char *copy = e->buffer()->selection_text();
  if (*copy) Fl::copy(copy, strlen(copy), 1);
  free((void*)copy);
  e->show_insert_position();
  return 1;
}

int My_Text_Editor::kf_cut(int c, My_Text_Editor* e) {
  kf_copy(c, e);
  kill_selection(e);
  if (e->when()&FL_WHEN_CHANGED) e->do_callback(); else e->set_changed();
  return 1;
}

int My_Text_Editor::kf_paste(int, My_Text_Editor* e) {
  kill_selection(e);
  Fl::paste(*e, 1);
  e->show_insert_position();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback(); else e->set_changed();
  return 1;
}

int My_Text_Editor::kf_select_all(int, My_Text_Editor* e) {
  e->buffer()->select(0, e->buffer()->length());
  return 1;
}

int My_Text_Editor::kf_undo(int , My_Text_Editor* e) {
  e->buffer()->unselect();
  int crsr;
  int ret = e->buffer()->undo(&crsr);
  if (ret)
  {
	  e->insert_position(crsr);
	  e->show_insert_position();
	  if (e->when()&FL_WHEN_CHANGED) e->do_callback(); else e->set_changed();
  }
  return ret;
}

int My_Text_Editor::handle_key() {
  // Call FLTK's rules to try to turn this into a printing character.
  // This uses the right-hand ctrl key as a "compose prefix" and returns
  // the changes that should be made to the text, as a number of
  // bytes to delete and a string to insert:
  int del;
  if (Fl::compose(del)) {
    if (del) buffer()->select(insert_position()-del, insert_position());
    kill_selection(this);
	if (Fl::event_length()) {
	  if (insert_mode()) 
	  {
		  insert(Fl::event_text());
		  if(strcmp(Fl::event_text(),"{")==0 && auto_brace_mode) 
			  autobrace = true;
		  else 
			  autobrace = false;
	  }
	  else overstrike(Fl::event_text());
	}
	show_insert_position();
    if (when()&FL_WHEN_CHANGED) do_callback();
    else set_changed();
    return 1;
  }

  int key = Fl::event_key(), state = Fl::event_state(), c = Fl::event_text()[0];
  state &= FL_SHIFT|FL_CTRL|FL_ALT|FL_META; // only care about these states
  
  if(Fl::event_key()==FL_Enter && smart_indent) 
  {
	  int max = insert_position() - buffer()->line_start(insert_position());
	char *line = new char[strlen(buffer()->line_text(insert_position()))+1];
	strcpy(line, buffer()->line_text(insert_position()));
	for(unsigned int i = 0; i < strlen(line); i++) {
		if(line[i]!=' ' && line[i]!='\t' || i >= (unsigned int) max) line[i]='\0';
	}
	kf_enter(c,this);
  	insert(line); 
  	
  	if(autobrace) {
  		int inspos = insert_position();
  		kf_enter(c,this);
  		insert(line); 
  		insert("}");
  		insert_position(inspos);
  		insert("\t");
  	}
	delete line;
  	autobrace = false;
	return 1;
  }
  

  else if(Fl::event_key()==FL_Home) {
	int pos = 0;
	char *line = new char[strlen(buffer()->line_text(insert_position()))+1];
	strcpy(line, buffer()->line_text(insert_position()));
	for(unsigned int i = 0; i < strlen(line); i++) {
		if(line[i]!=' ' && line[i]!='\t') { pos = i; break; }
	}
	delete line;
	int old = insert_position();
	insert_position(buffer()->line_start(insert_position()) + pos);
	if(state == FL_SHIFT) 
		buffer()->select(insert_position(),old);
	if(insert_position() != old) 
		return 1;
  }

  Key_Func f;
  f = bound_key_function(key, state, global_key_bindings);
  if (!f) f = bound_key_function(key, state, key_bindings);
  if (f) return f(key, this);
  if (default_key_function_ && !state) return default_key_function_(c, this);
  return 0;
}

void My_Text_Editor::maybe_do_callback() {
//  printf("My_Text_Editor::maybe_do_callback()\n");
//  printf("changed()=%d, when()=%x\n", changed(), when());
  if (changed() || (when()&FL_WHEN_NOT_CHANGED)) {
    clear_changed(); do_callback();}
}

int My_Text_Editor::handle(int event) 
{
	int		ret;

  if (!buffer()) return 0;

  if (event == FL_PUSH && Fl::event_button() == 2) {
    dragType = -1;
    Fl::paste(*this, 0);
    Fl::focus(this);
    if (when()&FL_WHEN_CHANGED) do_callback(); else set_changed();
	UpdateStatusBar();
    return 1;
  }

  switch (event) {
    case FL_FOCUS:
      show_cursor(mCursorOn); // redraws the cursor
      if (buffer()->primary_selection()->start() !=
          buffer()->primary_selection()->end()) redraw(); // Redraw selections...
 	  UpdateStatusBar();
      Fl::focus(this);
      return 1;

    case FL_UNFOCUS:
      show_cursor(mCursorOn); // redraws the cursor
      if (buffer()->primary_selection()->start() !=
          buffer()->primary_selection()->end()) redraw(); // Redraw selections...
    case FL_HIDE:
      if (when() & FL_WHEN_RELEASE) 
		  maybe_do_callback();
//	  ret = My_Text_Display::handle(event);
//      return ret;
	  break;

    case FL_KEYBOARD:
      ret = handle_key();
	  
	  UpdateStatusBar();
	  return ret;

    case FL_PASTE:
      if (!Fl::event_text()) {
        fl_beep();
		return 1;
      }
      buffer()->remove_selection();
      if (insert_mode()) insert(Fl::event_text());
      else overstrike(Fl::event_text());
      show_insert_position();
      if (when()&FL_WHEN_CHANGED) do_callback(); else set_changed();
	  UpdateStatusBar();
      return 1;

    case FL_ENTER:
// MRS: WIN32 only?  Need to test!
//    case FL_MOVE:
      show_cursor(mCursorOn);
	  UpdateStatusBar();
	  break;
//	  My_Text_Display::handle(event);
//      return 1;
  }

  ret = My_Text_Display::handle(event);
  UpdateStatusBar();
  return ret;
}

void My_Text_Editor::UpdateStatusBar(void)
{
 	int zeile = count_lines(0,insert_position(),0) + 1;
    int spalte = current_column();
  	int ins_mode = insert_mode();
	int	oldzeile = 0;
	int	oldspalte = 0;
	int oldins_mode = 0;

	if (m_pStatusBar != NULL)
	{
		oldzeile = m_pStatusBar->line;
		oldspalte = m_pStatusBar->col;
		oldins_mode = m_pStatusBar->ins_mode;
	}

    if (zeile != oldzeile || spalte != oldspalte || ins_mode != oldins_mode)
	{
		if (m_pStatusBar != NULL)
		{
			// Update row
			sprintf(m_pStatusBar->linestr, "Ln %d", zeile);
			m_pStatusBar->m_pLineBox->label(m_pStatusBar->linestr);
			m_pStatusBar->line = zeile;

			// Update col
			sprintf(m_pStatusBar->colstr, "Col %d", spalte+1);
			m_pStatusBar->m_pColBox->label(m_pStatusBar->colstr);
			m_pStatusBar->col = spalte;

			// Update ins mode
			if (ins_mode)
			{
				m_pStatusBar->m_pInsBox->label("INS");
			}
			else
			{
				m_pStatusBar->m_pInsBox->label("OVR");
			}
			m_pStatusBar->ins_mode = ins_mode;
		}
	}
}

//
// End of "$Id: My_Text_Editor.cpp,v 1.3 2013/02/08 00:05:27 kpettit1 Exp $".
//
