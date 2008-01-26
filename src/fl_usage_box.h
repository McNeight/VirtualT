/* fl_usage_box.h */

/* $Id: fl_usage_box.h,v 1.1.1.1 2008/01/05 kpettit1 Exp $ */

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


#ifndef _FL_USAGE_BOX_H_
#define _FL_USAGE_BOX_H_

#include <FL/Fl.H>
#include <FL/Fl_Box.H>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus

class Fl_Usage_Box : public Fl_Box
{
public:
	Fl_Usage_Box(int x, int y, int w, int h);

	void			SetUsageColor(int index, int color);
	void			SetUsageRange(int min, int max);
	void			SetBackgroundColor(int color);
	void			AddUsageEvent(int start, int end, unsigned char usage);
	void			ClearUsageMap(void);

protected:
	virtual void	draw();

	unsigned char*	m_pUsageMap;
	int				m_usageMapSize;
	int				m_usageMin, m_usageMax;

	int				m_usageColors[256];
	int				m_maxUsageEntry;
	int				m_backgroundColor;
	int				m_pixelsPerUsage;
	double			m_usageScale;
};

}
#endif

#endif
