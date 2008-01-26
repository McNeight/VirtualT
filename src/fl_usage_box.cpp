/* fl_usage_box.cpp */

/* $Id: fl_usage_box.cpp,v 1.2 2008/01/05 15:08:56 kpettit1 Exp $ */

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
#include <FL/fl_draw.H>
#include "fl_usage_box.h"

#include <string.h>
#include <stdio.h>

/*
=======================================================
Fl_Usage_Box:	This is the class construcor
=======================================================
*/
Fl_Usage_Box::Fl_Usage_Box(int x, int y, int w, int h) :
  Fl_Box(x, y, w, h)
{
	int		i;

	// Allocate space for the usage data
	m_usageMapSize = (w-4) * (h-4);		// Leave room for border
	
	m_pUsageMap = new unsigned char[m_usageMapSize];
	for (i = 0; i < m_usageMapSize; i++)
		m_pUsageMap[i] = 0;

	m_usageMin = 0;
	m_usageMax = 99;
	m_maxUsageEntry = 0;
	m_backgroundColor = fl_rgb_color(255, 255, 255);
	m_pixelsPerUsage = m_usageMapSize / 100;
	m_usageScale = 1.0 / 100.0;
}

/*
==========================================================
SetUsageRange:	This function sets the range of the usage
				box.  This range is used to perform
				scaling during usage updates.
==========================================================
*/
void Fl_Usage_Box::SetUsageRange(int min, int max)
{
	m_usageMin = min;
	m_usageMax = max;
	m_pixelsPerUsage = m_usageMapSize / (max - min+1);
	m_usageScale = 1.0 / ((double) (max - min + 1));
}

/*
==========================================================
SetUsageColor:	This function sets the display color for
				the specified usage entry.
==========================================================
*/
void Fl_Usage_Box::SetUsageColor(int index, int color)
{
	if (index > 255)
		return;

	m_usageColors[index] = color;
	if (index > m_maxUsageEntry)
		m_maxUsageEntry = index;
}

/*
==========================================================
ClearUsageMap:	This function clears the usage map for the
				entire usage box.
==========================================================
*/
void Fl_Usage_Box::ClearUsageMap(void)
{
	int 	c;

	for (c = 0; c < m_usageMapSize; c++)
		m_pUsageMap[c] = 0;

	redraw();
}

/*
==========================================================
AddUsageEvent:	This function adds a new usage event to 
				the usage box and forces a redraw.
==========================================================
*/
void Fl_Usage_Box::AddUsageEvent(int start, int end, unsigned char usage)
{
	int		i;
	int		usage_start, usage_end;

	// Ensure start and end are within specified usage range
	if ((start < m_usageMin) || (start > m_usageMax) || (end < m_usageMin)
		|| (end > m_usageMax) || (start > end))
	{
			return;
	}

	// Determine first and last usage entry point to update
	usage_start = (int) ((double) start * m_usageScale * m_usageMapSize) ;
	usage_end = (int) ((double) (end + 1) * m_usageScale * m_usageMapSize) - 1;

	// Fill in usage map entries for this usage
	for (i = usage_start; i <= usage_end; i++)
		m_pUsageMap[i] += usage;

	if (usage > m_maxUsageEntry)
		m_maxUsageEntry = usage;

	redraw();
}

/*
=================================================================
draw_static:	This routine draws the static portions of the LCD,
				such as erasing the background, drawing function
				key labls, etc.
=================================================================
*/
void Fl_Usage_Box::draw()
{
	int c, i;
	int x_pos, y_pos;

	// Draw background
    fl_color(m_backgroundColor);
    fl_rectf(x()+1,y()+1,w()-2,h()-2);

	// Draw frame
	fl_color(FL_BLACK);
	fl_rect(x(),y(), w(), h());

	// Draw usage information
	for (c = 1; c < 256; c++)
	{
		// Test if index greater than max usage entry
		if (c > m_maxUsageEntry)
			break;

		// Selet Color for this usage entry
		fl_color(m_usageColors[c]);

		// "Walk" through the usage map and draw a pixel for each entry with this usage
		for (i = 0; i < m_usageMapSize; i++)
		{
			if (m_pUsageMap[i] == c)
			{
				x_pos = i / (h()-4) + 2;
				y_pos = i % (h()-4) + 2;
				fl_point(x() + x_pos, y() + y_pos);
			}
		}
	}
}

