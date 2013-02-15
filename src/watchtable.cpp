/* watchtable.cpp */

/* $Id: watchtable.cpp,v 1.15 2013/02/11 08:37:17 kpettit1 Exp $ */

/*
 * Copyright 2004 Ken Pettit and Stephen Hurd 
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
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Input.H>  

#if defined(WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>

#include "VirtualT.h"
#include "memory.h"
#include "cpu.h"
#include "watchtable.h"

int str_to_i(const char *pStr);

/*
============================================================================
Start of implementation for Watch Table Widget
============================================================================
*/

static void cb_watch_item(Fl_Widget* pW, void *pOpaque);

static Fl_Menu_Item gWatchTypeMenu[] = {
	{ " U8 ",		0,	cb_watch_item, (void *) WATCH_TYPE_UCHAR },
	{ " S8 ",		0,	cb_watch_item, (void *) WATCH_TYPE_SCHAR },
	{ " Hex8 ",		0,	cb_watch_item, (void *) WATCH_TYPE_HEXCHAR },
	{ " U16 ",		0,	cb_watch_item, (void *) WATCH_TYPE_USHORT },
	{ " S16 ",		0,	cb_watch_item, (void *) WATCH_TYPE_SSHORT },
	{ " Hex16 ",	0,	cb_watch_item, (void *) WATCH_TYPE_HEXSHORT },
	{ " String ",	0,	cb_watch_item, (void *) WATCH_TYPE_STRING },
	{ 0 }
};

static Fl_Menu_Item gWatchRegionMenu[] = {
	{ " RAM ",		0,	cb_watch_item, (void *) REGION_RAM },
	{ " ROM ",		0,	cb_watch_item, (void *) REGION_ROM },
	{ " OptROM ",	0,	cb_watch_item, (void *) REGION_OPTROM },
	{ " RAM1 ",		0,	cb_watch_item, (void *) REGION_RAM1 },
	{ " RAM2 ",		0,	cb_watch_item, (void *) REGION_RAM2 },
	{ " RAM3 ",		0,	cb_watch_item, (void *) REGION_RAM3 },
	{ " FLASH ",	0,  cb_watch_item, (void *) REGION_REX_FLASH },
	{ 0 }
};

/*
============================================================================
Callback for the Watch Widget popup menu
============================================================================
*/
static void cb_watch_popup(Fl_Widget* pW, void* pOpaque)
{
}

/*
============================================================================
Callback for the Watch Widget popup input
============================================================================
*/
static void cb_popup_input(Fl_Widget* pW, void* pOpaque)
{
	// Get pointer to the parent
	VT_Watch_Table* pTable = (VT_Watch_Table *) pOpaque;
	VT_TableInput* pInput = (VT_TableInput *) pW;

	pTable->PopupInputCallback(pInput->m_reason);
}

/*
============================================================================
Callback for the Watch Widget scrollbar
============================================================================
*/
static void cb_watch_scroll(Fl_Widget* pW, void* pOpaque)
{
	// Get pointer to the parent
	VT_Watch_Table* pTable = (VT_Watch_Table *) pOpaque;

	printf("Scroll\n");
	pTable->ScrollAction();
}

/*
============================================================================
Class constructor for the Watch Table widget.
============================================================================
*/
VT_Watch_Table::VT_Watch_Table(int x, int y, int w, int h) :
	Fl_Widget(x, y, w, h, "")
{
	int		remaining, fontW, c;

	// Initialize class variables
	m_FontSize = 14;
	m_SelLine = -1;
	m_DblclkX = m_DblclkY = -1;
	m_PopupSelSave = -1;
	m_PopupInputField = 0;
	m_pEventHandler = NULL;
	m_DoingWidgetResize = FALSE;
	m_colors.watch_background = FL_BLACK;
	m_colors.watch_lines = fl_lighter(FL_BLACK);
	m_colors.watch_lines = fl_color_average(FL_BLACK, FL_WHITE, .85f);
	m_colors.name = FL_YELLOW;
	m_colors.region = FL_WHITE;
	m_colors.address = (Fl_Color)166;//fl_lighter((Fl_Color) 166);
	m_colors.type = FL_WHITE;
	m_colors.value = FL_WHITE;
	m_colors.select = FL_WHITE;
	m_colors.select_background = FL_BLUE;

	// Create a tile for the top items
	m_pColHeaderTile = new Fl_Tile(x, y, w-15, VT_WATCH_TABLE_HDR_HEIGHT, "");
	
	// Set width of each table header box
	fl_font(FL_COURIER, m_FontSize);
	fontW = (int) fl_width("W");
	m_ColWidth[4] = 5 * fontW;
	m_ColWidth[3] = 5 * fontW;
	m_ColWidth[2] = 8 * fontW;
	remaining = w-15 - (8+5+5) * fontW;
	m_ColWidth[1] = remaining >> 1;
	remaining -= m_ColWidth[1];
	m_ColWidth[0] = remaining;

	// Set the Start col for each field
	m_ColStart[0] = x;
	for (c = 1; c < 5; c++)
		m_ColStart[c+1] = m_ColStart[c] + m_ColWidth[c];

	// Create 5 boxes in the tile
	m_pColHeaders[0] = new VT_TableHeaderBox(FL_UP_BOX, m_ColStart[0], y, m_ColWidth[0], VT_WATCH_TABLE_HDR_HEIGHT, "Watch Name");
	m_pColHeaders[1] = new VT_TableHeaderBox(FL_UP_BOX, m_ColStart[1], y, m_ColWidth[1], VT_WATCH_TABLE_HDR_HEIGHT, "Value");
	m_pColHeaders[2] = new VT_TableHeaderBox(FL_UP_BOX, m_ColStart[2], y, m_ColWidth[2], VT_WATCH_TABLE_HDR_HEIGHT, "Address");
	m_pColHeaders[3] = new VT_TableHeaderBox(FL_UP_BOX, m_ColStart[3], y, m_ColWidth[3], VT_WATCH_TABLE_HDR_HEIGHT, "Type");
	m_pColHeaders[4] = new VT_TableHeaderBox(FL_UP_BOX, m_ColStart[4], y, m_ColWidth[4], VT_WATCH_TABLE_HDR_HEIGHT, "Rgn");

	// Setup alignment, etc.
	for (c = 0; c < VT_WATCH_TABLE_COL_COUNT; c++)
	{
		m_pColHeaders[c]->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		m_pColHeaders[c]->user_data(this);
		
		// Get the width percentage for each column
		m_WidthPercentage[c] = (double) m_ColWidth[c] / (double) (w-15);
	}

	// End the resizing tile
	m_pColHeaderTile->end();

	// Create a window for the variable space
	m_pVarWindow = new Fl_Box(FL_NO_BOX, x, y+VT_WATCH_TABLE_HDR_HEIGHT, w-15, h-VT_WATCH_TABLE_HDR_HEIGHT, "");
	m_pVarWindow->hide();

	// Create a scrollbar
	m_pScroll = new Fl_Scrollbar(x+w-15, y, 15, h,"");
	m_pScroll->step(1);
	m_pScroll->maximum(50);
	m_pScroll->slider_size(.1);
	m_pScroll->value(1, 4, 1, 50);
	m_pScroll->callback(cb_watch_scroll, this);

	// Create a popup menu
	m_pPopupMenu = new Fl_Menu_Button(0,0,100,400,"Popup");
	m_pPopupMenu->type(Fl_Menu_Button::POPUP123);
	m_pPopupMenu->menu(gWatchTypeMenu);
	m_pPopupMenu->selection_color(fl_rgb_color(80,80,255));
	m_pPopupMenu->color(fl_rgb_color(240,239,228));
	m_pPopupMenu->hide();
	m_pPopupMenu->user_data(this);
	m_pPopupMenu->callback(cb_watch_popup);

	m_pPopupBox = new Fl_Box(FL_DOWN_FRAME, 0, 0, 5, 5, "");
	m_pPopupBox->hide();

	// Create a popup edit box
	m_pPopupInput = new VT_TableInput(0, 0, 5, 5, "");
	m_pPopupInput->box(FL_FLAT_BOX);
	m_pPopupInput->color(FL_BLACK);
	m_pPopupInput->textcolor(FL_WHITE);
	m_pPopupInput->cursor_color(FL_WHITE);
	m_pPopupInput->selection_color(FL_BLUE);
	m_pPopupInput->textfont(FL_COURIER);
	m_pPopupInput->labelfont(FL_COURIER);
	m_pPopupInput->labelsize(m_FontSize);
	m_pPopupInput->callback(cb_popup_input, this);
	m_pPopupInput->hide();
}

/*
============================================================================
Class destructor for the Watch Table widget.
============================================================================
*/
VT_Watch_Table::~VT_Watch_Table()
{
	ResetContent();
}

/*
============================================================================
Registers an event handler with the watch table
============================================================================
*/
void VT_Watch_Table::EventHandler(watch_event_handler_t pHandler)
{
	m_pEventHandler = pHandler;
}

/*
============================================================================
Deletes all watches from the array, etc.
============================================================================
*/
void VT_Watch_Table::ResetContent(void)
{
	int				c, count;
	CWatchDef*		pVar;

	count = m_WatchVars.GetSize();
	for (c = 0; c < count; c++)
	{
		pVar = (CWatchDef *) m_WatchVars[c];
		delete pVar;
	}
	m_WatchVars.RemoveAll();
}

/*
============================================================================
Sets the scrollbar settings based on current geometry and item count.
============================================================================
*/
void VT_Watch_Table::SetScrollSizes(void)
{
	int		size, height, count, max;

	// Select 12 point Courier font
	fl_font(FL_COURIER, m_FontSize);

	// Get character width & height
	m_Height = fl_height();
	height = m_pVarWindow->h();
	size = (int) (height / m_Height);
	count = m_WatchVars.GetSize();

	// Set maximum value of scroll 
	if (m_pScroll->value()+size-2 > count )
	{
		int newValue = count-size+2;
		if (newValue < 1)
			newValue = 1;
		m_pScroll->value(newValue, 1, 0, count+2);
	}
	max = count + 2 - size;
	if (max < 1)
		max = 1;
	m_pScroll->maximum(max);
	m_pScroll->minimum(1);
	m_pScroll->step((double) size/(double)(count+1));
	m_pScroll->slider_size((double) size/(double)(count+1));
	m_pScroll->linesize(1);

	redraw();
}

/*
============================================================================
Callback for the scrollbar activity
============================================================================
*/
void VT_Watch_Table::ScrollAction(void)
{
	redraw();
}

/*
============================================================================
Set the specified variable to the value identified by the string given
============================================================================
*/
void VT_Watch_Table::SetVarValue(CWatchDef* pVar, const char *psVal)
{
	short			sval16;
	unsigned short	val16;
	unsigned char	val8;
	const char		*ptr;
	int				regionAddr, i;

	// Get the address within the region
	regionAddr = pVar->m_iAddr;
	if (pVar->m_Region == REGION_RAM || pVar->m_Region == REGION_RAM1 ||
		pVar->m_Region == REGION_RAM2 || pVar->m_Region == REGION_RAM3)
			regionAddr -= ROMSIZE;

	// Set based on type
	switch (pVar->m_Type)
	{
	case WATCH_TYPE_UCHAR:
	case WATCH_TYPE_SCHAR:
	case WATCH_TYPE_HEXCHAR:
	default:
		val8 = str_to_i(psVal);
		set_memory8_ext(pVar->m_Region, regionAddr, 1, &val8);
		break;

	case WATCH_TYPE_SSHORT:
		sval16 = str_to_i(psVal);
		val8 = sval16 & 0xFF;
		set_memory8_ext(pVar->m_Region, regionAddr, 1, &val8);
		val8 = (sval16 >> 8) & 0xFF;
		set_memory8_ext(pVar->m_Region, regionAddr+1, 1, &val8);
		break;

	case WATCH_TYPE_USHORT:
	case WATCH_TYPE_HEXSHORT:
		val16 = str_to_i(psVal);
		val8 = val16 & 0xFF;
		set_memory8_ext(pVar->m_Region, regionAddr, 1, &val8);
		val8 = (val16 >> 8) & 0xFF;
		set_memory8_ext(pVar->m_Region, regionAddr+1, 1, &val8);
		break;

	case WATCH_TYPE_STRING:
		ptr = psVal;
		i = 0;
		// Write bytes until we reach the NULL
		while (*ptr != '\0')
		{
			// Get next value to write
			val8 = *ptr++;
			set_memory8_ext(pVar->m_Region, regionAddr + i++, 1, &val8);
		}

		// Now write the NULL
		val8 = *ptr++;
		set_memory8_ext(pVar->m_Region, regionAddr + i, 1, &val8);
		break;
	}

	// Send an event that a value changed
	if (m_pEventHandler != NULL)
		m_pEventHandler(WATCH_EVENT_VAL_CHANGED, pVar);
}

/*
============================================================================
Get the value of the specified variable
============================================================================
*/
void VT_Watch_Table::UpdateVarValue(CWatchDef* pVar)
{
	int				type = pVar->m_Type;
	unsigned char	c;
	unsigned short	s;
	int				regionAddr;

	// Calculate the region address
	regionAddr = pVar->m_iAddr;
	if (pVar->m_Region == REGION_RAM || pVar->m_Region == REGION_RAM1 ||
		pVar->m_Region == REGION_RAM2 || pVar->m_Region == REGION_RAM3)
			regionAddr -= ROMSIZE;

	// Get the value based on type
	if (type < WATCH_TYPE_USHORT)
	{
		// Get the memory using extended region access
		get_memory8_ext(pVar->m_Region, regionAddr, 1, &c);
		pVar->m_Value = c;
	}
	else if (type != WATCH_TYPE_STRING)
	{
		// Get the memory using extended region access
		get_memory8_ext(pVar->m_Region, regionAddr, 1, &c);
		s = c;
		get_memory8_ext(pVar->m_Region, regionAddr+1, 1, &c);
		s |= ((unsigned short) c) << 8;
		pVar->m_Value = s;
	}
	else
	{
		int x = 0;
		char	str[2];
		
		str[1] = '\0';
		pVar->m_StrData = "";
		get_memory8_ext(pVar->m_Region, regionAddr, 1, (unsigned char *) str);
		while (str[0] != '\0')
		{
			pVar->m_StrData += (char *) str;
			get_memory8_ext(pVar->m_Region, regionAddr + ++x, 1, (unsigned char *) str);
		}
	}
}

/*
============================================================================
Reads the variable's current value and formats it according to the specified
type parameter.
============================================================================
*/
void VT_Watch_Table::FormatVarValue(CWatchDef* pVar, char* str, int len)
{
	short	shortval;

	// Format the value based on type
	switch (pVar->m_Type)
	{
	case WATCH_TYPE_UCHAR:
	default:
		if (pVar->m_Value >= ' ' && pVar->m_Value <= '~')
			sprintf(str, "%d '%c'", (unsigned char) pVar->m_Value, pVar->m_Value);
		else
			sprintf(str, "%d", (unsigned char) pVar->m_Value);
		break;
	case WATCH_TYPE_USHORT:
		sprintf(str, "%d", (unsigned short) pVar->m_Value);
		break;		
	case WATCH_TYPE_HEXCHAR:
		sprintf(str, "%02XH '%c'", (unsigned char) pVar->m_Value, (char) pVar->m_Value);
		break;
	case WATCH_TYPE_SCHAR:
		if (pVar->m_Value > 127)
			sprintf(str, "%d", pVar->m_Value-256);
		else
			sprintf(str, "%d", pVar->m_Value);
		break;		
	case WATCH_TYPE_SSHORT:
		shortval = (short) pVar->m_Value;
		sprintf(str, "%d", shortval);
		break;		
	case WATCH_TYPE_HEXSHORT:
		sprintf(str, "%04XH", (unsigned short) pVar->m_Value);
		break;

	case WATCH_TYPE_STRING:
		strcpy(str, "\"");
		strncat(str, (const char *) pVar->m_StrData, len-3);
		strcat(str, "\"");
		break;
	}
}

/*
============================================================================
Draw the specified watch variable on the specified line
============================================================================
*/
void VT_Watch_Table::DrawWatch(CWatchDef* pVar, int line)
{
	int			wx, wy, ww, wh;
	int			xOffset, yOffset;
	int			fontHeight, lines, topLine;
	char		str[256];

	// Get our dimensions
	wx = m_pVarWindow->x();
	wy = m_pVarWindow->y();
	ww = m_pVarWindow->w();
	wh = m_pVarWindow->h();
	fontHeight = fl_height();
	lines = wh / fontHeight;

	// Test if line is in the scroll region
	topLine = m_pScroll->value() - 1;
	if (line < topLine || line-(topLine-1) > lines)
		return;

	if (line < 0 || line > m_WatchVars.GetSize())
		return;

	// Offset text within cell
	xOffset = 5;
	yOffset = -5;
	line -= topLine;

	// Draw the variable name
	if (line+topLine == m_SelLine)
		fl_color(m_colors.select);
	else
		fl_color(m_colors.name);
	fl_draw((const char *) pVar->m_Name, m_ColStart[0]+xOffset, wy + (line+1) * fontHeight+yOffset);

	// Get the current value
	UpdateVarValue(pVar);
	if (line+topLine != m_SelLine)
		fl_color(m_colors.value);

	// Format the value based on type
	FormatVarValue(pVar, str, sizeof(str));

	fl_push_clip(m_ColStart[1]+xOffset, wy + line * fontHeight + 1, m_ColWidth[1]-2-xOffset, fontHeight);
	fl_draw(str, m_ColStart[1]+xOffset, wy + (line+1) * fontHeight+yOffset);
	fl_pop_clip();

	// Draw the address
	if (line+topLine != m_SelLine)
		fl_color(m_colors.address);
	if (pVar->m_iAddr > 0xFFFF)
		sprintf(str, "%06XH", pVar->m_iAddr);
	else
		sprintf(str, "%04XH", pVar->m_iAddr);
	fl_draw(str, m_ColStart[2]+xOffset, wy + (line+1) * fontHeight+yOffset);

	// Draw the type
	if (line+topLine != m_SelLine)
		fl_color(m_colors.type);
	switch (pVar->m_Type)
	{
	case WATCH_TYPE_UCHAR:    strcpy(str, "U8");  break;
	case WATCH_TYPE_SCHAR:    strcpy(str, "S8");  break;
	case WATCH_TYPE_HEXCHAR:  strcpy(str, "H8");  break;
	case WATCH_TYPE_USHORT:   strcpy(str, "U16");  break;
	case WATCH_TYPE_SSHORT:   strcpy(str, "S16");  break;
	case WATCH_TYPE_HEXSHORT: strcpy(str, "H16");  break;
	case WATCH_TYPE_STRING:   strcpy(str, "Str");  break;
	}
	fl_draw(str, m_ColStart[3]+xOffset, wy + (line+1) * fontHeight+yOffset);

	// Draw the region
	if (line+topLine != m_SelLine)
		fl_color(m_colors.region);
	switch (pVar->m_Region)
	{
	case REGION_RAM:    strcpy(str, "RAM");  break;
	case REGION_ROM:    strcpy(str, "ROM");  break;
	case REGION_OPTROM:	strcpy(str, "Opt"); break;
	case REGION_RAM1:	strcpy(str, "RAM1"); break;
	case REGION_RAM2:	strcpy(str, "RAM2"); break;
	case REGION_RAM3:	strcpy(str, "RAM3"); break;
	case REGION_REX_FLASH: strcpy(str, "FLASH"); break;
	default:			strcpy(str, "RAM");  break;
	}
	fl_draw(str, m_ColStart[4]+xOffset, wy + (line+1) * fontHeight+yOffset);
}

/*
============================================================================
Routine to update watch table values real-time
============================================================================
*/
void VT_Watch_Table::UpdateTable(void)
{
	int			c, count, fontHeight;
	int			wx, wy, ww, wh;
	int			lines, topItem, xOffset, yOffset;
	char		str[256];
	CWatchDef*	pVar;

	// Do some prep work
	window()->make_current();
	fl_font(FL_COURIER, m_FontSize);
	fontHeight = fl_height();
	topItem = m_pScroll->value();

	// Get dimensions of the variable space
	wx = m_pVarWindow->x();
	wy = m_pVarWindow->y();
	ww = m_pVarWindow->w();
	wh = m_pVarWindow->h();
	lines = wh / fontHeight;
	count = m_WatchVars.GetSize();
	xOffset = 5;
	yOffset = -5;

	// Draw the background
	fl_push_clip(wx, wy, ww, wh);
	fl_color(m_colors.watch_background);

	// Now loop for all visible lines
	for (c = 0; c < lines; c++)
	{
		// Test if no more vars to draw
		if (c + topItem-1 >= count)
			break;

		// Get next variable and draw it
		pVar = (CWatchDef *) m_WatchVars[c+topItem-1];

		// Test if this value is being edited
		if (pVar->m_ValEdit)
			continue;

		// Test for a change in the value
		if (pVar->m_Type == WATCH_TYPE_STRING)
		{
			MString temp = pVar->m_StrData;
			UpdateVarValue(pVar);
			if (temp == pVar->m_StrData)
				continue;
		}
		else
		{
			int temp = pVar->m_Value;
			UpdateVarValue(pVar);
			if (temp == pVar->m_Value)
				continue;
		}

		// Erase the old value
		if (c == m_SelLine - topItem + 1)
			fl_color(m_colors.select_background);
		else
			fl_color(m_colors.watch_background);
		fl_rectf(m_ColStart[1], m_pVarWindow->y()+c*fontHeight, m_ColWidth[1], fontHeight-1);

		// Redraw the new value
		FormatVarValue(pVar, str, sizeof(str));
		fl_color(m_colors.value);
		fl_push_clip(m_ColStart[1]+xOffset, wy + c * fontHeight + 1, m_ColWidth[1]-2-xOffset, fontHeight);
		fl_draw(str, m_ColStart[1]+xOffset, wy + (c+1) * fontHeight+yOffset);
		fl_pop_clip();
	}

	// Pop the clip region
	fl_pop_clip();
}

/*
============================================================================
Our Watch Table drawing routine
============================================================================
*/
void VT_Watch_Table::draw(void)
{
	int			c, count, fontHeight;
	int			wx, wy, ww, wh;
	int			lines, topItem;
	CWatchDef*	pVar;

	window()->make_current();

	// Do our custom draw stuff
	fl_font(FL_COURIER, m_FontSize);
	fontHeight = fl_height();
	topItem = m_pScroll->value();

	// Make our window the drawing window
	wx = m_pVarWindow->x();
	wy = m_pVarWindow->y();
	ww = m_pVarWindow->w();
	wh = m_pVarWindow->h();
	lines = wh / fontHeight;

	// Draw the background
	fl_push_clip(wx, wy, ww, wh);
	fl_color(m_colors.watch_background);
	fl_rectf(wx, wy, ww, wh);

	// Draw lines between the watch items
	fl_color(m_colors.watch_lines);
	for (c = 1; c < lines; c++)
		fl_line(wx, wy+c * fontHeight, wx+ww, wy + c * fontHeight);

	// Draw the items
	count = m_WatchVars.GetSize();
	for (c = 0; c < lines; c++)
	{
		// Test if this item is selected
		if (c + topItem - 1 == m_SelLine)
		{
			// Draw the selection rectangle
			fl_color(m_colors.select_background);
			fl_rectf(m_pVarWindow->x(), m_pVarWindow->y()+c*fontHeight, w(), fontHeight-1);
		}

		// Test if no more vars to draw
		if (c + topItem-1 == count)
			break;

		// Get next variable and draw it
		pVar = (CWatchDef *) m_WatchVars[c+topItem-1];
		DrawWatch(pVar, c + topItem - 1);
	}

	// Test if we need to draw the NewWatchName
	if (c + topItem-1 == count)
	{
		fl_color(m_colors.name);
		fl_draw((const char *) m_NewWatchName, m_ColStart[0]+5, wy + (c + topItem) * fontHeight-5);
	}

	// Pop the clip region
	fl_pop_clip();
}

/*
============================================================================
Handle events that occur within the variable window.
============================================================================
*/
int VT_Watch_Table::handle_variable_events(int event)
{
	int		line_click, fontHeight;
	int		xp, yp, topItem;
	int		button, x, key;

	// Get some common variables
	fl_font(FL_COURIER, m_FontSize);
	fontHeight = fl_height();
	m_Height = fontHeight;
	topItem = m_pScroll->value();

	// Get X,Y position of button press
	xp = Fl::event_x();
	yp = Fl::event_y();

	switch (event)
	{
	case FL_KEYDOWN:
		// Get the key that was pressed
		key = Fl::event_key();

		// Process ESC key
		if (key == FL_Escape)
		{
			m_SelLine = -1;
			redraw();
		}

		else if (key == FL_Delete)
		{
			// If there's a selection
			if (m_SelLine != -1)
			{
				if (m_SelLine < m_WatchVars.GetSize())
				{
					DeleteWatch(m_SelLine);
					if (m_SelLine > m_WatchVars.GetSize())
						m_SelLine--;
					redraw();
				}
			}
		}

		else if (key == FL_Down)
		{
			// If there's a selection...
			if (m_SelLine != -1)
			{
				// Increment to next line
				m_SelLine++;

				// Constrain the selection
				if (m_SelLine >= m_WatchVars.GetSize())
					m_SelLine = m_WatchVars.GetSize();

				// Scroll if at bottom
				int size = m_pVarWindow->h() / m_Height;
				if (m_SelLine - topItem+1 == size)
				{
					m_pScroll->value(m_SelLine-2, 1, 0, 0);
					SetScrollSizes();
				}

				redraw();
			}
		}

		else if (key == FL_Up)
		{
			// If there's a selection...
			if (m_SelLine != -1)
			{
				// Decrement to previous line
				m_SelLine--;

				// Constrain the selection 
				if (m_SelLine < 0)
					m_SelLine = 0;

				// Scroll if at top of window
				if (m_SelLine == topItem-2)
				{
					m_pScroll->value(m_SelLine+1, 1, 0, m_WatchVars.GetSize()+2);
					SetScrollSizes();
				}

				redraw();
			}
		}
		else if (key == FL_Enter)
		{
			// If there's a selection...
			if (m_SelLine != -1)
			{
				if (m_SelLine < m_WatchVars.GetSize())
					HandleDoubleClick(1, m_SelLine);
				else
					HandleDoubleClick(0, m_SelLine);
			}
		}
		else if (key >= FL_F + 1 && key <= FL_F + 12)
		{
			if (m_pEventHandler != NULL)
				m_pEventHandler(key, NULL);
		}

		return 1;

	case FL_PUSH:
		take_focus();

		line_click = (yp-VT_WATCH_TABLE_HDR_HEIGHT) / fontHeight;
		if (line_click < 0)
			line_click = 0;

		// Get the mouse button that was pressed
		button = Fl::event_button();

		// Test for right mouse click
		if (button == FL_RIGHT_MOUSE)
		{
			// TODO:  add right mouse click processing!!!!!

			return 1;
		}

		// Test for double click action
		if (xp == m_DblclkX && yp == m_DblclkY)
		{
			// Calculate which column
			for (x = VT_WATCH_TABLE_COL_COUNT - 1; x >= 0; x--)
			{
				// Test if in this col
				if (xp >= m_ColStart[x])
				{
					if (line_click + topItem - 1 < m_WatchVars.GetSize())
						HandleDoubleClick(x, line_click);
					m_DblclkX = m_DblclkY = -1;
					return 1;
				}
			}

			// Cancel the double click
			m_DblclkX = m_DblclkY = -1;
			return 1;
		}
		
		// Save the coords to test for double click next time
		m_DblclkX = xp;
		m_DblclkY = yp;

		// Test for a click in col 0 or 2 of the empty line
		if (line_click + topItem - 1 == m_WatchVars.GetSize())
		{
			if (xp < m_ColStart[1])
			{
				m_SelLine = line_click+topItem-1;
				HandleDoubleClick(0, m_SelLine);
				return 1;
			} else if (xp >= m_ColStart[2] && xp < m_ColStart[3])
			{
				// Allow single click edit if a name already defined
				if (m_NewWatchName.GetLength() != 0)
				{
					// Set the selection line and do double click processing
					m_SelLine = line_click+topItem-1;
					HandleDoubleClick(2, m_SelLine);
					return 1;
				}
			}
		}

		// Constrain line_click based on number of watch items
		if (line_click + topItem - 1 > m_WatchVars.GetSize())
		{
			line_click = m_WatchVars.GetSize() - topItem + 1;
		}

		// Setup the window for drawing
		window()->make_current();
		fl_push_clip(m_pVarWindow->x(), m_pVarWindow->y(), m_pVarWindow->w(), m_pVarWindow->h());

		// Erase any previous selection
		if (m_SelLine != -1)
		{
			// Clear this line
			fl_color(m_colors.watch_background);
			fl_rectf(m_pVarWindow->x(), m_pVarWindow->y()+(m_SelLine-(topItem-1))*fontHeight, w(), fontHeight);
			if (m_SelLine - (topItem-1) != 0)
			{
				fl_color(m_colors.watch_lines);
				int temp = m_SelLine-(topItem-1);
				fl_line(m_pVarWindow->x(), m_pVarWindow->y()+ temp * fontHeight, 
					m_pVarWindow->x() + m_pVarWindow->w(), m_pVarWindow->y() + temp * fontHeight);
			}

			// Redraw the previously selected item
			if (m_SelLine < m_WatchVars.GetSize())
			{
				CWatchDef* pVar = (CWatchDef *) m_WatchVars[m_SelLine];
				int oldSel = m_SelLine;
				m_SelLine = line_click + topItem - 1;
				DrawWatch(pVar, oldSel);
			}
		}

		// Draw the selection rectangle
		fl_color(m_colors.select_background);
		fl_rectf(m_pVarWindow->x(), m_pVarWindow->y()+line_click*fontHeight+1, w(), fontHeight-1);

		// Redraw this item
		m_SelLine = line_click + topItem - 1;
		if (m_SelLine < m_WatchVars.GetSize())
		{
			CWatchDef* pVar = (CWatchDef *) m_WatchVars[line_click + topItem - 1];
			DrawWatch(pVar, m_SelLine);
		}
		fl_pop_clip();

		return 1;

	case FL_RELEASE:
		return 1;

	case FL_FOCUS:
		// Indicate that we accept the focus
		return 1;

	case FL_UNFOCUS:
		// Clear the selection if any
		if (m_SelLine != -1)
		{
			// Save the selection line
			int oldSel = m_SelLine;
			m_SelLine = -1;

			// Erase the selection line
			window()->make_current();
			fl_color(m_colors.watch_background);
			fl_push_clip(m_pVarWindow->x(), m_pVarWindow->y(), m_pVarWindow->w(), m_pVarWindow->h());
			fl_rectf(m_pVarWindow->x(), m_pVarWindow->y()+(oldSel-(topItem-1))*fontHeight, w(), fontHeight);
			if (oldSel - (topItem-1) != 0)
			{
				fl_color(m_colors.watch_lines);
				line_click = oldSel-(topItem-1);
				fl_line(m_pVarWindow->x(), m_pVarWindow->y()+ line_click * fontHeight, 
					m_pVarWindow->x() + m_pVarWindow->w(), m_pVarWindow->y() + line_click * fontHeight);
			}
			fl_pop_clip();
			
			// Redraw the selection item, if any
			if (oldSel < m_WatchVars.GetSize())
			{
				CWatchDef* pVar = (CWatchDef *) m_WatchVars[oldSel];
				DrawWatch(pVar, oldSel);
			}
		}
		return 1;

	case FL_ENTER:
	case FL_LEAVE:
		// Indicate that we accept ENTER / LEAVE
		return 1;

	case FL_MOVE:
	case FL_DRAG:
		if (xp != m_DblclkX || yp != m_DblclkY)
		{
			m_DblclkX = -1;
			m_DblclkY = -1;
		}
		return 1;

	default:
		break;
	}

	return 0;
}

/*
============================================================================
Resize on of the column header boxes
============================================================================
*/
void VT_Watch_Table::HeaderBoxWidth(int col, int w)
{
	if (col < VT_WATCH_TABLE_COL_COUNT)
	{
		// Set the new width
		m_ColWidth[col] = w;

		// If this isn't the 1st col, then set our X location too
		if (col != 0)
			m_ColStart[col] = m_ColStart[col-1] + m_ColWidth[col-1];

		// Now resize all the controls
		m_DoingWidgetResize = TRUE;
		for (col = 0; col < VT_WATCH_TABLE_COL_COUNT; col++)
		{
			if (col < 2)
			{
				int remaining = m_pColHeaderTile->w();
				for (int i = 2; i < VT_WATCH_TABLE_COL_COUNT; i++)
					remaining -= m_ColWidth[i];

				m_WidthPercentage[col] = (double) m_ColWidth[col] / (double) remaining;
			}

			m_pColHeaders[col]->resize(m_ColStart[col], m_pColHeaders[col]->y(), 
				m_ColWidth[col], VT_WATCH_TABLE_HDR_HEIGHT);
		}
		m_DoingWidgetResize = FALSE;
	}
}

/*
============================================================================
Someone resized us...make sure everything gets resized properly.
============================================================================
*/
void VT_Watch_Table::resize(int x, int y, int w, int h)
{
	int		remaining;
	int		c, tx, tw;

	// Disable updating percentages within column resizing
	m_DoingWidgetResize = TRUE;

	// Do default widget resize stuff
	Fl_Widget::resize(x, y, w, h);

	// We will keep cols 2-4 the same size and only resize 0 & 1 (Name, value)
	tx = m_pColHeaderTile->x();
	tw = remaining = m_pColHeaderTile->w();

	// Slide the last 3 columns to right-align with right of tile
	m_ColStart[VT_WATCH_TABLE_COL_COUNT - 1] = tx+tw -
		m_ColWidth[VT_WATCH_TABLE_COL_COUNT - 1];
	remaining -= m_ColWidth[VT_WATCH_TABLE_COL_COUNT - 1];

	for (c = VT_WATCH_TABLE_COL_COUNT-2; c >= 2; c--)
	{
		m_ColStart[c] = m_ColStart[c+1] - m_ColWidth[c];
		remaining -= m_ColWidth[c];
	}

	// Now adjust cols 0 & 1 based on their percentage and remaining pixels
	m_ColWidth[1] = (int) (m_WidthPercentage[1] * (double) remaining);
	m_ColStart[1] = m_ColStart[2] - m_ColWidth[1];
	m_ColWidth[0] = m_ColStart[1] - m_ColStart[0];

	// Now do physical resizing of the boxes
	for (c = 0; c < VT_WATCH_TABLE_COL_COUNT; c++)
	{
		m_pColHeaders[c]->resize(m_ColStart[c], y, m_ColWidth[c], VT_WATCH_TABLE_HDR_HEIGHT);
	}

	// Resize the variable window and scroll bar
	m_pVarWindow->resize(m_pVarWindow->x(), y+VT_WATCH_TABLE_HDR_HEIGHT, 
		w-15, h-VT_WATCH_TABLE_HDR_HEIGHT);
	m_pScroll->resize(x+w-15, y, 15, h);

	SetScrollSizes();

	redraw();
}

/*
============================================================================
Our Watch Table Widget event handler.
============================================================================
*/
int VT_Watch_Table::handle(int event)
{
	int		ret;

	printf("Event %d\n", event);
	// Test for events inside the watch window
	if (Fl::event_inside(m_pVarWindow))
	{
		ret = handle_variable_events(event);
		if (ret)
			return ret;
	}

	switch (event)
	{
	case FL_HIDE:
		return 1;
	case FL_UNFOCUS:
	case FL_KEYDOWN:
		ret = handle_variable_events(event);
		if (ret)
			return ret;
		break;
	}

	// Not handled above.  Do the default 
	return Fl_Widget::handle(event);
}

/*
============================================================================
Sets the color of the header boxes.
============================================================================
*/
void VT_Watch_Table::header_color(Fl_Color c)
{
	int x;

	for (x = 0; x < VT_WATCH_TABLE_COL_COUNT; x++)
		m_pColHeaders[x]->color(c);
}

/*
============================================================================
Adds a new watch to the widget.
============================================================================
*/
int VT_Watch_Table::AddWatch(MString& name, MString& addr, int type, 
							  int region, int need_redraw)
{
	int			index;
	CWatchDef*	pWatch;
	
	// Create a new watch definition
	pWatch = new CWatchDef;
	if (pWatch != NULL)
	{
		// Populate the watch with data
		pWatch->m_Name = name;
		pWatch->m_Addr = addr;
		pWatch->m_iAddr = str_to_i((const char *) addr);
		pWatch->m_Type = type;
		pWatch->m_Region = region;

		// Add the watch to the list
		index = m_WatchVars.Add(pWatch);

		// Now draw the new item if needed
		SetScrollSizes();
		if (need_redraw)
			redraw();

		return TRUE;
	}

	return FALSE;
}

/*
============================================================================
Deletes the specified index watch
============================================================================
*/
void VT_Watch_Table::DeleteWatch(int index)
{
	// Get a pointer to the watch var
	CWatchDef* pVar = (CWatchDef *) m_WatchVars[index];

	// Remove it from the list
	m_WatchVars.RemoveAt(index, 1);

	// Now delete the item
	delete pVar;
}

/*
============================================================================
Test the watch string for parse errors.  When called, we expect the pointer
to be pointing at a comma vs a semicolon or NULL.
============================================================================
*/
int VT_Watch_Table::WatchStringParseError(const char*& ptr)
{
	// Test if we encountered a comma as expected
	if (*ptr != ',')
	{
		// Skip to next entry (next semicolon or NULL)
		while (*ptr != ';' && *ptr != '\0')
			ptr++;

		// If we found a semicolon, then skip past it
		if (*ptr == ';') 
			ptr++;

		// Indicate we had a parse error
		return TRUE;
	}

	// Skip past the ','
	ptr++;

	// No parse error
	return FALSE;
}

/*
============================================================================
Sets up the watch variables from the string from user preferences.
============================================================================
*/
void VT_Watch_Table::WatchVarsString(const char *pVarString)
{
	const char*		ptr;
	const char*		regionPtr;
	MString			name, addr, typeStr;
	int				region, type;

	// Start at beginning of string
	ptr = pVarString;
	while (*ptr != '\0')
	{
		// Parse name
		name = addr = typeStr = "";
		while (*ptr != ',' && *ptr != ';' && *ptr != '\0')
		{
			// Skip escape character
			if (*ptr == '\\')
				ptr++;
			name += *ptr++;
		}

		// Test for format error in string
		if (WatchStringParseError(ptr))
			continue;

		// Parse address
		while (*ptr != ',' && *ptr != ';' && *ptr != '\0')
			addr += *ptr++;

		// Test for format error in string
		if (WatchStringParseError(ptr))
			continue;

		// Parse the type
		while (*ptr != ',' && *ptr != ';' && *ptr != '\0')
			typeStr += *ptr++;

		// Test for format error in string
		if (WatchStringParseError(ptr))
			continue;

		// Parse the region enum
		regionPtr = ptr;
		while (*ptr >= '0' && *ptr <= '9')
			ptr++;

		// Test for error during parse
		if (*ptr != ';' && *ptr != '\0')
		{
			WatchStringParseError(ptr);
			continue;
		}

		// Skip past the semicolon
		ptr++;

		// Convert the enum ascii to in int
		region = atoi(regionPtr);

		// Convert the type string to an int
		if (typeStr == "U8")
			type = WATCH_TYPE_UCHAR;
		else if (typeStr == "S8")
			type = WATCH_TYPE_SCHAR;
		else if (typeStr == "H8")
			type = WATCH_TYPE_HEXCHAR;
		else if (typeStr == "U16")
			type = WATCH_TYPE_USHORT;
		else if (typeStr == "S16")
			type = WATCH_TYPE_SSHORT;
		else if (typeStr == "H16")
			type = WATCH_TYPE_HEXSHORT;
		else if (typeStr == "String")
			type = WATCH_TYPE_STRING;
		else
			type = WATCH_TYPE_UCHAR;

		// Add this watch variable, but don't redraw yet
		AddWatch(name, addr, type, region, FALSE);
	}

	// Now redraw
	redraw();
}

/*
============================================================================
Returns the watch vars string for saving our variables to the preferences file.
============================================================================
*/
MString VT_Watch_Table::WatchVarsString(void)
{
	int			c, count, x, len;
	CWatchDef*	pVar;
	MString		ret, fmt, name;
	const char	*psType;
	
	// Loop for all watches
	count = m_WatchVars.GetSize();
	for (c = 0; c < count; c++)
	{
		// Get next watch
		pVar = (CWatchDef *) m_WatchVars[c];

		// Format the type in case the enums change at some point
		switch (pVar->m_Type)
		{
		case WATCH_TYPE_UCHAR:		psType = "U8";		break;
		case WATCH_TYPE_SCHAR:		psType = "S8";		break;
		case WATCH_TYPE_HEXCHAR:	psType = "H8";		break;
		case WATCH_TYPE_USHORT:		psType = "U16";		break;
		case WATCH_TYPE_SSHORT:		psType = "S16";		break;
		case WATCH_TYPE_HEXSHORT:	psType = "H16";		break;
		case WATCH_TYPE_STRING:		psType = "String";	break;
		default:					psType = "U8";		break;
		}

		// Add escape chars to name if it contains comma, semicolon or '\'
		name = "";
		len = pVar->m_Name.GetLength();
		for (x = 0; x < len; x++)
		{
			if (pVar->m_Name[x] == ',' || pVar->m_Name[x] == ';' ||
				pVar->m_Name[x] == '\\')
			{
				name += '\\';
				name += pVar->m_Name[x];
			}
			else
				name += pVar->m_Name[x];
		}

		// Format a substring for this watch
		fmt.Format("%s,%s,%s,%d;", (const char *) name, 
			(const char *) pVar->m_Addr, psType, pVar->m_Region);

		ret += fmt;
	}

	// Return the formatted string
	return ret;
}

/*
============================================================================
Adds a new watch to the widget.
============================================================================
*/
void VT_Watch_Table::HandleDoubleClick(int field, int line)
{
	int			show_popup = FALSE;
	CWatchDef*	pVar;
	char		str[16];

	// For field 0, popup the input box
	if (field < 3)
	{
		// Save the current selection line, clear it and redraw
		m_PopupSelSave = m_SelLine;
		m_SelLine = -1;
		redraw();

		// Resize the popup input field to fit in our cell
		m_pPopupInput->resize(m_ColStart[field]+4, m_pVarWindow->y()+
			(m_PopupSelSave-(m_pScroll->value()-1))*(int)m_Height-1, 
			m_ColWidth[field]-3, m_Height);
		if (field == 0)
			m_pPopupInput->textcolor(m_colors.name);
		else if (field == 1)
			m_pPopupInput->textcolor(m_colors.value);
		else if (field == 2)
			m_pPopupInput->textcolor(m_colors.address);
		m_pPopupInput->selection_color(m_colors.select_background);
		m_pPopupInput->color(m_colors.watch_background);
		m_PopupInputField = field;

		// Test if we are editing a new entry or an existing one
		if (m_PopupSelSave >= m_WatchVars.GetSize())
		{
			// New entry.  Start with a blank page
			if (field == 0)
				m_pPopupInput->value((const char *) m_NewWatchName);
			else
				m_pPopupInput->value("");
		}
		else
		{
			// Get the variable being edited and prepare the input field
			pVar = (CWatchDef *) m_WatchVars[m_PopupSelSave];
			if (field == 0)
				m_pPopupInput->value((const char *) pVar->m_Name);
			else if (field == 1)
			{
				// Format value based on type
				if (pVar->m_Type == WATCH_TYPE_STRING)
				{
					// Populate input with the string data
					m_pPopupInput->value((const char *) pVar->m_StrData);
				}
				else
				{
					// Format in either HEX or decimal format
					if (pVar->m_Type == WATCH_TYPE_HEXCHAR)
						sprintf(str, "%02XH", pVar->m_Value);
					else if (pVar->m_Type == WATCH_TYPE_HEXSHORT)
						sprintf(str, "%04XH", pVar->m_Value);
					else if (pVar->m_Type == WATCH_TYPE_SCHAR)
						sprintf(str, "%d", pVar->m_Value > 127 ? pVar->m_Value - 256 : pVar->m_Value);
					else if (pVar->m_Type == WATCH_TYPE_SSHORT)
						sprintf(str, "%d", pVar->m_Value > 32767 ? pVar->m_Value - 65536 : pVar->m_Value);
					else
						sprintf(str, "%d", pVar->m_Value);
					m_pPopupInput->value(str);
				}

				// Mark the value as being edited
				pVar->m_ValEdit = TRUE;
			} else if (field == 2)
			{
				m_pPopupInput->value((const char *) pVar->m_Addr);
			}
		}

		// Show the popup input
		m_pPopupInput->show();
		m_pPopupInput->take_focus();
		m_pPopupInput->position(0);
		m_pPopupInput->mark(999999);
	}

	// For field 4, popup the region menu
	else if (field == 4)
	{
		m_pPopupMenu->menu(gWatchRegionMenu);
		show_popup = TRUE;
	}

	// For field 3, popup the type menu
	else if (field == 3)
	{
		m_pPopupMenu->menu(gWatchTypeMenu);
		show_popup = TRUE;
	}

	// Did anyone request the popup menu?
	if (show_popup)
	{
		// Save the selected item, clear it and redraw
		m_PopupSelSave = m_SelLine;
		m_SelLine = -1;
		redraw();

		// Get the screen coordinates
		int screenX, screenY, screenW, screenH;
		Fl::screen_xywh(screenX, screenY, screenW, screenH);

		// Find the screen coordinate for the Y value of the
		// selection to test if the popup will fit below the item or not
		int my = m_pVarWindow->y()+(m_PopupSelSave-m_pScroll->value()+1)*(int)m_Height;
		Fl_Group *p = parent();
		while (p != NULL)
		{
			// Translate Y based on parent's Y and get his parent
			my += p->y();
			p = p->parent();
		}

		// Resize the popup box frame and show it to indicate we are 
		// making a selection
		m_pPopupBox->resize(m_ColStart[field], m_pVarWindow->y()+
			(m_PopupSelSave - (m_pScroll->value()-1))*(int)m_Height, 
			m_ColWidth[field]-3, m_Height);
		m_pPopupBox->box(FL_EMBOSSED_FRAME);
		m_pPopupBox->show();

		// Count the number of items in the menu
		int itemCount;
		const Fl_Menu_Item* pM = m_pPopupMenu->menu();
		for (itemCount = 0; pM[itemCount].text != NULL; itemCount++)
			;

		// Now popup the menu
		Fl_Widget *mb = m_pPopupMenu;
		const Fl_Menu_Item* m;
		Fl::watch_widget_pointer(mb);
		if (my + itemCount * 19 >= screenH)
			m = m_pPopupMenu->menu()->popup(m_ColStart[field]+2, m_pVarWindow->y()+
				(m_PopupSelSave-m_pScroll->value())*(int)m_Height - itemCount*19, NULL, 0, m_pPopupMenu);
		else
			m = m_pPopupMenu->menu()->popup(m_ColStart[field]+2, m_pVarWindow->y()+
				(m_PopupSelSave-m_pScroll->value()+2)*(int)m_Height+3, NULL, 0, m_pPopupMenu);

		// Hide the popup box showing we are selecting a new item
		m_pPopupBox->hide();

		// Restore the selection and redraw
		m_SelLine = m_PopupSelSave;

		// Call the callback item for the selected menu or the cancel callback
		m_pPopupMenu->picked(m);
		if (m == NULL)
			cb_watch_popup(NULL, NULL);
		Fl::release_widget_pointer(mb);

		// Redraw the window
		m_SelLine = -1;
		redraw();
	}
}

/*
============================================================================
Adds a new watch to the widget.
============================================================================
*/
static void cb_watch_item(Fl_Widget* pW, void *pOpaque)
{
	Fl_Menu_Button*	pPopup = (Fl_Menu_Button *) pW;
	VT_Watch_Table* pWatch;

	// Restore the selection
	pWatch = (VT_Watch_Table *) pPopup->user_data();

	// Determine which menu is active
	if (pPopup->menu() == gWatchTypeMenu)
		pWatch->SetSelectedType((int) pOpaque);
	else if (pPopup->menu() == gWatchRegionMenu)
		pWatch->SetSelectedRegion((int) pOpaque);
}

/*
============================================================================
Adds a new watch to the widget.
============================================================================
*/
void VT_Watch_Table::SetSelectedType(int type)
{
	CWatchDef*	pVar;

	if (m_SelLine < m_WatchVars.GetSize())
	{
		// Get the pointe to the variable
		pVar = (CWatchDef *) m_WatchVars[m_SelLine];

		// Update the type and redraw
		pVar->m_Type = type;
		UpdateVarValue(pVar);
		redraw();
	}
}

/*
============================================================================
Adds a new watch to the widget.
============================================================================
*/
void VT_Watch_Table::SetSelectedRegion(int region)
{
	CWatchDef*	pVar;

	if (m_SelLine < m_WatchVars.GetSize())
	{
		// Get the pointer to the variable
		pVar = (CWatchDef *) m_WatchVars[m_SelLine];

		// Update the type and redraw
		pVar->m_Region = region;
		UpdateVarValue(pVar);
		redraw();
	}
}

/*
============================================================================
Manages resizing of any of the header boxes.
============================================================================
*/
void VT_Watch_Table::HeaderResize(VT_TableHeaderBox* pBox, int x,
	int y, int cw, int ch)
{
	int			c;

	// Determine which column is being resized
	for (c = 0; c < VT_WATCH_TABLE_COL_COUNT; c++)
	{
		// Test if it's this header
		if (pBox == m_pColHeaders[c])
			break;
	}

	// If not found, then there's a bug somewhere.  Just return
	if (c == VT_WATCH_TABLE_COL_COUNT)
		return;


	// Get the width percentage for each column
	if (!m_DoingWidgetResize)
	{
		// Save the new start col and width of this column
		m_ColStart[c] = x;
		m_ColWidth[c] = cw;

		// For 1st two fields, calculate percentage width of remaining
		// space after last cols.
		if (c < 2)
		{
			int remaining = m_pColHeaderTile->w();
			for (int i = 2; i < VT_WATCH_TABLE_COL_COUNT; i++)
				remaining -= m_ColWidth[i];

			m_WidthPercentage[c] = (double) m_ColWidth[c] / (double) remaining;
		}
	}

	// Redraw everything
	redraw();
}

/*
============================================================================
Manages resizing of any of the header boxes.
============================================================================
*/
void VT_Watch_Table::PopupInputCallback(int reason)
{
	CWatchDef*		pVar;

	// Only process callback for reasons we understand
	if (reason == 0)
		return;

	// Hide the popup
	m_pPopupInput->hide();
	window()->take_focus();

	// Test if escape pressed
	if (reason == FL_Escape)
	{
		// Show the cursor again
		fl_cursor(FL_CURSOR_DEFAULT);

		// Restore the old selection line and redraw
		m_SelLine = m_PopupSelSave;

		// Test if we need to cancel value editing
		if (m_SelLine < m_WatchVars.GetSize())
		{
			pVar = (CWatchDef *) m_WatchVars[m_SelLine];
			pVar->m_ValEdit  = FALSE;
		}
		redraw();
		return;
	}

	// Upate the variable or do processing to create a new one
	if (m_PopupSelSave >= m_WatchVars.GetSize())
	{
		// Processing a new watch.  Test if we are in field 0 or 2
		if (m_PopupInputField == 0)
		{
			// If reason was UNFOCUS, then cancel the input
			if (reason == FL_UNFOCUS)
			{
				// Test if we need to cancel value editing
				if (m_PopupSelSave < m_WatchVars.GetSize())
				{
					pVar = (CWatchDef *) m_WatchVars[m_PopupSelSave];
					pVar->m_ValEdit  = FALSE;
				}

				// Clear the new watch name, if any
				if (Fl::focus() == this)
					m_SelLine = m_PopupSelSave;
				redraw();
				return;
			}

			// Okay, we got a callback during entry of the name.  Test if a 
			// name was actually entered
			if (strlen(m_pPopupInput->value()) > 0)
			{
				// Get the name and save it until we have the address too
				m_NewWatchName = m_pPopupInput->value();

				// Now move the input field to the address cell
				m_pPopupInput->resize(m_ColStart[2]+4, m_pVarWindow->y()+
					(m_PopupSelSave-(m_pScroll->value()-1))*(int)m_Height-1, 
					m_ColWidth[2]-3, m_Height-1);
				m_pPopupInput->value("");
				m_pPopupInput->textcolor(m_colors.address);

				// Now redraw to show the new watch name
				redraw();

				// Now show the input in field 2 (address)
				m_pPopupInput->show();
				m_pPopupInput->take_focus();
				m_PopupInputField = 2;
				fl_cursor(FL_CURSOR_INSERT);
				return;
			}
			else
			{
				// Just cancel the input
				m_SelLine = m_PopupSelSave;
				redraw();

				// Show the cursor again
				fl_cursor(FL_CURSOR_DEFAULT);

				return;
			}
		}
		else if (m_PopupInputField == 2)
		{
			// Okay, check if an address was actually entered
			if (strlen(m_pPopupInput->value()) > 0)
			{
				MString	addr = m_pPopupInput->value();
				int iaddr = str_to_i((const char *) addr);
				int region;
				if (iaddr < ROMSIZE)
					region = REGION_ROM;
				else
					region = REGION_RAM;
				AddWatch(m_NewWatchName, addr, WATCH_TYPE_UCHAR, region);
				m_NewWatchName = "";

				// Set this line as the selectionline
				redraw();
				m_SelLine = m_PopupSelSave+1;
				
				// Allow entry of another variable
				int lines = m_pVarWindow->h() / m_Height;
				if (m_SelLine >= m_pScroll->value() - 1 + lines)
				{
					// TODO:  Deal with scrolling
					m_SelLine = -1;
				}
				else
					HandleDoubleClick(0, m_SelLine);

				return;
			}
			else 
			{
				// Just cancel the input
				m_SelLine = m_PopupSelSave;
				redraw();

				// Show the cursor again
				fl_cursor(FL_CURSOR_DEFAULT);
				return;
			}
		}
	}
	else
	{
		// Validate the input has data
		if (strlen(m_pPopupInput->value()) == 0)
		{
			// Not allowed!  Must enter something
			m_pPopupInput->show();
			m_pPopupInput->take_focus();
			return;
		}

		// We are editing an existing entry.  Get the watch variable
		pVar = (CWatchDef *) m_WatchVars[m_PopupSelSave];

		// Get edited data from the input
		if (m_PopupInputField == 0)
			// Updating the name
			pVar->m_Name = m_pPopupInput->value();
		else if (m_PopupInputField == 1)
		{
			// Updating the value.  
			SetVarValue(pVar, m_pPopupInput->value());
			pVar->m_ValEdit = FALSE;
		}
		else if (m_PopupInputField == 2)
		{
			// Updating the address
			pVar->m_Addr = m_pPopupInput->value();
			pVar->m_iAddr = str_to_i((const char *) pVar->m_Addr);
			
			// Update the variable value
			UpdateVarValue(pVar);
		}

		// Restore the selection line
		m_SelLine = m_PopupSelSave;
		redraw();
	}

	// Show the cursor again
	fl_cursor(FL_CURSOR_DEFAULT);
}

/*
============================================================================
Constructor for Watch Table Header box.
============================================================================
*/
VT_TableHeaderBox::VT_TableHeaderBox(int boxType, int x, int y, 
	int w, int h, const char *title) : Fl_Box((Fl_Boxtype) boxType, x, y, w, h, title)
{
}

/*
============================================================================
Resize routine for Watch Table Header box
============================================================================
*/
void VT_TableHeaderBox::resize(int x, int y, int w, int h)
{
	// First do default Fl_Box resizing
	Fl_Box::resize(x, y, w, h);

	// Now do our custom stuff
	VT_Watch_Table* pTable = (VT_Watch_Table *) user_data();
	pTable->HeaderResize(this, x, y, w, h);
}

/*
============================================================================
Event handler for Table Header box
============================================================================
*/
int VT_TableHeaderBox::handle(int event)
{
	// Get pointer to the parent
	VT_Watch_Table* pTable = (VT_Watch_Table *) user_data();

	switch (event)
	{
	case FL_ENTER:
	case FL_MOVE:
		pTable->m_DoingWidgetResize = FALSE;
		break;
	}

	return Fl_Box::handle(event);
}

/*
============================================================================
Class constructor for Table Input
============================================================================
*/
VT_TableInput::VT_TableInput(int x, int y, int w, int h, const char *title) :
	Fl_Input(x, y, w, h, title)
{
}

/*
============================================================================
Event handler for Table Input
============================================================================
*/
int VT_TableInput::handle(int event)
{
	int		key;

	printf("Event %d\n", event);

	// Clear the keypress
	m_reason = 0;

	switch (event)
	{
	case FL_FOCUS:
		// On focus, always select the text
		position(0);
		mark(9999);
		break;

	case FL_UNFOCUS:
		m_reason = FL_UNFOCUS;
		do_callback();
		return 1;

	case FL_KEYDOWN:
		// Handle TAB and Enter keys
		// Get the Key that was pressed
		key = Fl::event_key();

		if (key == FL_Enter || key == FL_Tab || key == FL_Escape)
		{
			// Do normal input handling first
			Fl_Input::handle(event);

			// Set the user data to indicate which key caused the callback
			m_reason = key;
			do_callback();
			return 1;
		}
		break;

	case FL_KEYUP:
		return 1;

	default:
		break;
	}

	// Now do the default thing that Fl_Input would do
	return Fl_Input::handle(event);
}

