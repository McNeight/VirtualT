/* vtobj.h */

/* $Id: vtobj.h,v 1.1 2007/03/31 22:09:19 kpettit1 Exp $ */

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


#ifndef	VTOBJ_H
#define	VTOBJ_H

#include <map>
#include "MString.h"

class VTObject;

struct VTClass
{
	const char	*m_ClassName;
	int			m_ObjectSize;
	VTObject*	(*m_pCreateObject)(); 
	VTClass		*m_pBaseClass;

	VTObject*	CreateObject();
	int		IsDerivedFrom(const VTClass* pBaseClass) const;
	VTClass		*m_pNextClass;
};


#define VT_CLASS(ClassName) ((VTClass*)(&ClassName::class##ClassName))

// not serializable, but dynamically constructable
#define DECLARE_DYNCREATE(ClassName) \
public: \
	static const VTClass class##ClassName; \
	virtual VTClass* GetClass() const; \
	static VTObject* CreateObject();

#define IMPLEMENT_DYNCREATE(ClassName, BaseClassName) \
	VTObject* ClassName::CreateObject() \
		{ return new ClassName; } \
	const VTClass ClassName::class##ClassName = { \
		#ClassName, sizeof(class ClassName), ClassName::CreateObject, \
			VT_CLASS(BaseClassName), 0 }; \
	VTClass* ClassName::GetClass() const \
		{ return VT_CLASS(ClassName); } 


class VTObject
{
protected:
	VTObject();

private:
	VTObject			(const VTObject& objectSrc);              // no implementation
	void				operator=(const VTObject& objectSrc);       // no implementation

public:
	virtual				~VTObject(void);
	virtual VTClass		*GetClass()const;
	static const		VTClass classVTObject;

};


class VTObArray	: public VTObject
{
	DECLARE_DYNCREATE(VTObArray)


public:
	VTObArray();
	~VTObArray();
	void		SetSize(int nNewSize, int nGrowBy = 0);
	void		InsertAt(int nIndex, VTObject* newElement);
	void		RemoveAt(int nIndex, int nCount);
	int			Add(VTObject* pObj);
	VTObject*	GetAt(int nIndex) const;
	int			GetSize() { return m_Count; };
	void		RemoveAll() { m_Count = 0; };

	// overloaded operator helpers
	VTObject*	operator[](int nIndex) const;

protected:
	int			m_GrowBy;
	int			m_Size;
	int			m_Count;
	void *		m_pData;
};


//typedef std::map <string, VTObject *> STR_TO_OB_MAP;
//typedef std::map<string, VTObject *>::iterator POSITION;

typedef struct _StrToObStruct {
	char *					pKey;
	VTObject*				pObj;
	struct _StrToObStruct*	pNext;
	unsigned char			hash;
} StrToObStruct;

typedef StrToObStruct* POSITION;

class VTMapStringToOb : public VTObject
{

	DECLARE_DYNCREATE(VTMapStringToOb)

protected:
	StrToObStruct*			m_HashPtrs[256];
	int						m_Count;

	VTObject*				m_Null;

	unsigned char			HashKey(const char *key);

public:
	VTMapStringToOb();
	~VTMapStringToOb();
	int						Lookup(const char *key, VTObject*& rValue);
	VTObject*&				operator[](const char *key);
	void					RemoveAll();
	void					RemoveAt(MString& rKey);
	int						Size() { return m_Count; };
	void					GetNextAssoc(POSITION& rNextPosition,
								MString& rKey, VTObject*& rValue) const;
	POSITION				GetStartPosition(void);
};

class VT_Rect {
public:
	VT_Rect()	{ m_X = 0; m_Y = 0; m_W = 0; m_H = 0; };
	VT_Rect(int x, int y, int w, int h)
				{ m_X = x; m_Y = y; m_W = w; m_H = h; };

	const VT_Rect& operator=(const VT_Rect& srcRect)
	{ if (this == &srcRect) return *this; m_X = srcRect.m_X; 
		m_Y = srcRect.m_Y; m_W = srcRect.m_W; m_H = srcRect.m_H; 
		return *this; };
	int		x()	{ return m_X; };
	int		y() { return m_Y; };
	int		w() { return m_W; };
	int		h()	{ return m_H; };

	void	x(int x) { m_X = x; };
	void	y(int y) { m_Y = y; };
	void	w(int w) { m_W = w; };
	void	h(int h) { m_H = h; };

	int		x1() { return m_X + m_W; };
	int		y1() { return m_Y + m_H; };

	int		PointInRect(int x, int y) 
			{ 
				if ((x >= m_X) && (x <= m_X + m_W) &&
				  (y >= m_Y) && (y <= m_Y + m_H)) return 1; 
				else return 0;
			}

private:
	int		m_X;
	int		m_Y;
	int		m_W;
	int		m_H;

};

#endif

