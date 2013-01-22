//
// "$Id: My_Text_Editor.h,v 1.1 2011/07/09 08:16:21 kpettit1 Exp $"
//
// Header file for My_Text_Editor class.
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


#ifndef My_Text_Editor_H
#define My_Text_Editor_H

#include <FL/Fl_Box.H>
#include "My_Text_Display.h"

typedef struct _StatusBar
{
	int			line;
	int			col;
	int			ins_mode;
	Fl_Box*		m_pLineBox;
	Fl_Box*		m_pColBox;
	Fl_Box*		m_pInsBox;
	char		linestr[20];
	char		colstr[20];
} StatusBar_t;

// key will match in any state
#define My_Text_Editor_ANY_STATE  (-1L)

class FL_EXPORT My_Text_Editor : public My_Text_Display {
  public:
    typedef int (*Key_Func)(int key, My_Text_Editor* editor);

    struct Key_Binding {
      int          key;
      int          state;
      Key_Func     function;
      Key_Binding* next;
    };

	int		autobrace;
	int		smart_indent;
	int		auto_brace_mode;

    My_Text_Editor(int X, int Y, int W, int H, const char* l = 0);
    ~My_Text_Editor() { remove_all_key_bindings(); }
    virtual int handle(int e);
    void insert_mode(int b) { insert_mode_ = b; }
    int insert_mode() { return insert_mode_; }

	void add_status_bar(StatusBar_t* pStatusBar) { m_pStatusBar = pStatusBar; };
    void add_key_binding(int key, int state, Key_Func f, Key_Binding** list);
    void add_key_binding(int key, int state, Key_Func f)
      { add_key_binding(key, state, f, &key_bindings); }
    void remove_key_binding(int key, int state, Key_Binding** list);
    void remove_key_binding(int key, int state)
      { remove_key_binding(key, state, &key_bindings); }
    void remove_all_key_bindings(Key_Binding** list);
    void remove_all_key_bindings() { remove_all_key_bindings(&key_bindings); }
    void add_default_key_bindings(Key_Binding** list);
    Key_Func bound_key_function(int key, int state, Key_Binding* list);
    Key_Func bound_key_function(int key, int state)
      { return bound_key_function(key, state, key_bindings); }
    void default_key_function(Key_Func f) { default_key_function_ = f; }
	void UpdateStatusBar(void);

    // functions for the built in default bindings
    static int kf_default(int c, My_Text_Editor* e);
    static int kf_ignore(int c, My_Text_Editor* e);
    static int kf_backspace(int c, My_Text_Editor* e);
    static int kf_enter(int c, My_Text_Editor* e);
    static int kf_move(int c, My_Text_Editor* e);
    static int kf_shift_move(int c, My_Text_Editor* e);
    static int kf_ctrl_move(int c, My_Text_Editor* e);
    static int kf_c_s_move(int c, My_Text_Editor* e);
    static int kf_home(int, My_Text_Editor* e);
    static int kf_end(int c, My_Text_Editor* e);
    static int kf_left(int c, My_Text_Editor* e);
    static int kf_up(int c, My_Text_Editor* e);
    static int kf_right(int c, My_Text_Editor* e);
    static int kf_down(int c, My_Text_Editor* e);
    static int kf_page_up(int c, My_Text_Editor* e);
    static int kf_page_down(int c, My_Text_Editor* e);
    static int kf_insert(int c, My_Text_Editor* e);
    static int kf_delete(int c, My_Text_Editor* e);
    static int kf_copy(int c, My_Text_Editor* e);
    static int kf_cut(int c, My_Text_Editor* e);
    static int kf_paste(int c, My_Text_Editor* e);
    static int kf_select_all(int c, My_Text_Editor* e);
    static int kf_undo(int c, My_Text_Editor* e);

  protected:
    int handle_key();
    void maybe_do_callback();

	StatusBar_t*	m_pStatusBar;
    int insert_mode_;
    Key_Binding* key_bindings;
    static Key_Binding* global_key_bindings;
    Key_Func default_key_function_;
};

#endif

//
// End of "$Id: My_Text_Editor.h,v 1.1 2011/07/09 08:16:21 kpettit1 Exp $".
//

