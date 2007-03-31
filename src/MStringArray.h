/*  MStringArray - Dynamic Array of MString objects
    Copyright (C) 2001-2005 Jesse L. Lovelace

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MSTRINGARRAY_H
#define MSTRINGARRAY_H

#include "MString.h"

const char MStringArray_VERSION[8] = "BETA 1";

class MArrayNode;


class MStringArray {
public:
	MStringArray();
	MStringArray(const MStringArray& arraySrc);
	MStringArray(const MString& stringSrc);
	~MStringArray();

	const MStringArray& operator =(const MStringArray& arraySrc);
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

	void FreeExtra();
	void RemoveAll();
	MString GetAt(int nIndex) const; 
	void SetAt(int nIndex, const MString &newElement);
	void SetAt(int nIndex, char newString[]);
	MString& ElementAt(int nIndex);

//	const MString* GetData() const;
//	MString* GetData();


	void SetAtGrow(int nIndex, const MString &newElement);
	void SetAtGrow(int nIndex, char newString[]);
	int Add(const MString &newElement);
	int Add(char newString[]);
	MString Pop();
	int Append(const MStringArray &src);
	void Copy(const MStringArray &src);

	void InsertAt(int nIndex, const MString &newElement, int nCount = 1);
	void InsertAt(int nStartIndex, MStringArray pNewArray);
	void InsertAt(int nIndex, char newString[], int nCount = 1);

	void RemoveAt(int nIndex, int nCount = 1);

	MString& operator[](int nIndex);
	MString operator[](int nIndex) const;

	//New functions:

	void Split(MString stringToSplit, char ch = ' ',  int nStartIndex = 0, int insertAt = 0);

	void test();

private:
	MArrayNode* GetPointerAt(int nIndex);

	MArrayNode *headMNode;
	MArrayNode *tailMNode;

};

#endif
