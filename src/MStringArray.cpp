/*  MStringArray - Dynamic Array of MString objects
    Copyright (C) 2001-2005 Jesse L. Lovelace (jesse at aslogicsys dot com)

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

//#include <fstream>
//#include <iostream>
#include "MStringArray.h"
#include "MString.h"


#ifndef NULL
#define NULL 0
#endif
//using namespace std;

class MArrayNode {
public:
	MArrayNode(MString string, MArrayNode* link = 0) {MLink_Forward = link; MInfo = string;}
	MArrayNode *MLink_Forward;
	MString MInfo;
};

static void deallocate(MArrayNode* p)
{
	MArrayNode* tmp;
	while (p) {
		tmp = p;
		p = p->MLink_Forward;
		delete tmp;
	}
}

static MArrayNode* findTail(MArrayNode * startPointer) {
	MArrayNode * tmp=startPointer;
	while (tmp) {
		if (!tmp->MLink_Forward)
			return tmp;
		tmp = tmp->MLink_Forward;
	}
	return startPointer;
}

static MArrayNode* copy(MArrayNode* p, MArrayNode*& tailNode)
{
	MArrayNode* head = 0;
	MArrayNode* tail = 0;


	while(p) {
		if (!head)
			head = tail = new MArrayNode(p->MInfo);
		else {
			tail->MLink_Forward = new MArrayNode(p->MInfo);
			tail = tail->MLink_Forward;
		}
		p = p->MLink_Forward;
	}
	tailNode = tail;
	return head;
}

MArrayNode* MStringArray::GetPointerAt(int nIndex) {

	MArrayNode* tmp = headMNode;
	
	for (int i = 0; (i < nIndex); i++) {

		if (0 == tmp)
			return 0;

		tmp = tmp->MLink_Forward;
	}
	return tmp;
}


MStringArray::~MStringArray() {
	deallocate(headMNode);
	headMNode = tailMNode = NULL;
}

MStringArray::MStringArray() {

	headMNode = tailMNode = NULL;
}

MStringArray::MStringArray(const MStringArray& arraySrc) {

	headMNode = copy(arraySrc.headMNode, tailMNode);
}

MStringArray::MStringArray(const MString& stringSrc) {

	headMNode = tailMNode = new MArrayNode(stringSrc);
}

const MStringArray& MStringArray::operator = (const MStringArray& arraySrc) {

	if (this != &arraySrc) {
		deallocate(headMNode);
		headMNode = tailMNode = NULL;
		headMNode = copy(arraySrc.headMNode, tailMNode);
	}
	return *this;
}


int MStringArray::GetSize() const {

	int count = 0;

	if (headMNode==NULL)
		return count;

	MArrayNode *tmp = headMNode;

	while(tmp) {
		count++;
		tmp = tmp->MLink_Forward;
	}
	return count;

}

int MStringArray::GetUpperBound() const {

	return (GetSize() - 1);
}

void MStringArray::SetSize(int nNewSize, int nGrowBy) {

	if (nNewSize < 0)
		return;
	//UNFINISHED FUNCTION
	//nGrowBy unreferenced formal param

}


void MStringArray::FreeExtra() {

	//UNFINISHED FUNCTION

}

void MStringArray::RemoveAll() {

	deallocate(headMNode);
	headMNode = tailMNode = NULL;

}

void MStringArray::test() {

	MArrayNode *tmp = headMNode;

//	cout << "Head = " << headMNode << endl;
	while (tmp)
	{ 
//		cout << "Index(" << count++ << ") " << tmp->MInfo << " " << tmp->MLink_Forward << endl;
		tmp = tmp->MLink_Forward;
	}
//	cout << "Tail = " << tailMNode << endl;
//	cout << "GetSize = " << GetSize() << endl;
}
	


MString MStringArray::GetAt(int nIndex) const {
	
	MString tmpStr;
	if ((nIndex < 0) || (nIndex > GetUpperBound()))
		return tmpStr;   //return's null MString if the nIndex is to big or small.

	MArrayNode* tmp = headMNode;

	for (int i = 0; i < nIndex; i++)
		tmp=tmp->MLink_Forward;

	return tmp->MInfo;
}

void MStringArray::SetAt(int nIndex, const MString &newElement) {

	if ((nIndex < 0) || (nIndex > GetUpperBound()))
		return;   //return's null MString if the nIndex is to big or small.

	MArrayNode* tmp = headMNode;

	for (int i = 0; i < nIndex; i++)
		tmp=tmp->MLink_Forward;

	tmp->MInfo = newElement;
}

void MStringArray::SetAt(int nIndex, char newString[]) {

	if ((nIndex < 0) || (nIndex > GetUpperBound()))
		return;   //return's null MString if the nIndex is to big or small.

	MArrayNode* tmp = headMNode;
	
	for (int i = 0; i < nIndex; i++)
		tmp=tmp->MLink_Forward;

	tmp->MInfo = newString;
}


MString& MStringArray::ElementAt(int nIndex) {
	
	MString* tmpStr = NULL;
	if ((nIndex < 0) || (nIndex > GetUpperBound()))
		return *tmpStr;   //return's null MString if the nIndex is to big or small.

	MArrayNode* tmp = headMNode;

	for (int i = 0; i < nIndex; i++)
		tmp=tmp->MLink_Forward;

	return tmp->MInfo;
}

// const MString* MStringArray::GetData() const;
// MString* MStringArray::GetData();


void MStringArray::SetAtGrow(int nIndex, const MString &newElement) {

	//Not written yet
}

void MStringArray::SetAtGrow(int nIndex, char newString[]) {
	//Not written yet

}

int MStringArray::Add(const MString &newElement) {

	InsertAt(GetSize(), newElement);

	return GetUpperBound();
}

int MStringArray::Add(char newString[]) { //works

	InsertAt(GetSize(), newString);

	return GetUpperBound();
}


int MStringArray::Append(const MStringArray &src) { //works

	InsertAt(GetSize(), src);

	return GetSize();
}

void MStringArray::Copy(const MStringArray &src) {

	headMNode = copy(src.headMNode, tailMNode);

}
void MStringArray::InsertAt(int nIndex, const MString &newElement, int nCount) { //works
	//MFC Uses pointer to MString, 
	
	int length = GetSize();

	if ((nCount == 0) || (nIndex < 0))
		return;

	if (length == 0) {  //if size is zero
		headMNode = tailMNode = new MArrayNode(newElement);
		return;
	}


	if (nIndex == length) {//if just adding to end, use tailMNode
		for (int a = 0; a < nCount; a++) {
			tailMNode->MLink_Forward = new MArrayNode(newElement);
			tailMNode = tailMNode->MLink_Forward;
		}
		return;
	}

	if (nIndex == 0) {
		headMNode = new MArrayNode(newElement, headMNode);
		MArrayNode* tmp = headMNode;

		for (int i = 1; i < nCount; i++) {
			tmp->MLink_Forward = new MArrayNode(newElement, tmp->MLink_Forward);
			tmp = tmp->MLink_Forward;
		}
		return;

	}

	
	if (nIndex > (length - 1)) {//if space between new element and old ones,
								 // pad with null MStrings
		
		for (int adds = nIndex; nIndex > GetSize(); adds++) {
			MString tmpStr;
			Add(tmpStr);
		}
	
		for (int other = 0; other < nCount; other++) {
			Add(newElement);
		}
		return;
	}

	// if inserting in the middle somewhere
	MArrayNode * tmp = headMNode;
	for (int k = 0; k < (nIndex - 1); k++) { //go to index you want to insert infront of.
		tmp = tmp->MLink_Forward;
	}
	for (int w = 0; w < nCount; w++) {
		tmp->MLink_Forward = new MArrayNode(newElement, tmp->MLink_Forward);
		tmp = tmp->MLink_Forward;
	}
	return;

}
		
			
void MStringArray::InsertAt(int nStartIndex, MStringArray pNewArray){ //works

	int size = pNewArray.GetSize();

	for (int i = 0; i < size; i++)
		InsertAt(nStartIndex++, pNewArray.GetAt(i)); //sloppy jesse, don't use GetAt
}


void MStringArray::InsertAt(int nIndex, char newString[], int nCount) {
	

	MString tmp(newString);
	InsertAt(nIndex, tmp, nCount);

	return;

}

void MStringArray::RemoveAt(int nIndex, int nCount) {

	int length = GetSize();

	if ((length < 1) || (nIndex > length) || (nCount < 1))
		return;

	if (nCount > (length - nIndex))
		nCount = (length - nIndex);

	if (0 == nIndex) {
		MArrayNode* first = headMNode;
		MArrayNode* last = GetPointerAt((nIndex + nCount)-1);
	
		if (last->MLink_Forward) {
			headMNode = last->MLink_Forward;
			last->MLink_Forward = NULL;
			deallocate(first);
		}
		else {
			headMNode = NULL;
			deallocate(first);
		}

	}	
	else {
		MArrayNode* toHead = GetPointerAt(nIndex - 1);
		MArrayNode* first = GetPointerAt(nIndex);

		MArrayNode* last = GetPointerAt((nCount + nIndex) - 1);
	
		if (last->MLink_Forward) {
			if (last->MLink_Forward->MLink_Forward) {
				toHead->MLink_Forward = last->MLink_Forward;
				last->MLink_Forward = NULL;
				deallocate(first);
			}
			else {
				toHead->MLink_Forward=last->MLink_Forward;
				last->MLink_Forward = NULL;
				deallocate(first);
			}
		}
		else {
			toHead->MLink_Forward = NULL;
			deallocate(first);
		}
	}
	tailMNode = findTail(headMNode);

}

MString& MStringArray::operator[](int nIndex) {
	
	MString* tmpStr = NULL;
	if ((nIndex < 0) || (nIndex > GetUpperBound()))
		return *tmpStr;   //return's null MString if the nIndex is to big or small.

	MArrayNode* tmp = headMNode;

	for (int i = 0; i < nIndex; i++)
		tmp=tmp->MLink_Forward;

	return tmp->MInfo;
}

MString MStringArray::operator[](int nIndex) const {
	
	MString tmpStr;
	if ((nIndex < 0) || (nIndex > GetUpperBound()))
		return tmpStr;   //return's null MString if the nIndex is to big or small.

	MArrayNode* tmp = headMNode;

	for (int i = 0; i < nIndex; i++)
		tmp=tmp->MLink_Forward;

	return tmp->MInfo;
}

	//New functions:

void MStringArray::Split(MString stringToSplit, char ch, int nStartIndex, int insertAt){

	MStringArray tmpArray;

	MString tmpString;

	int place = nStartIndex,
		nextFind = stringToSplit.Find(ch, place);

	while (nextFind >= 0) {

		tmpString = stringToSplit.Mid(place, nextFind - place);

		tmpString.Trim(ch);

//		cout << tmpString << endl;
//		cout << "place = " << place << endl;

		tmpArray.Add(tmpString);

		tmpString.Empty();

		place = nextFind + 1;
		nextFind = stringToSplit.Find(ch, place);

//		cout << "place = " << place << endl;
	}
	tmpString = stringToSplit.Right(stringToSplit.GetLength() - place); 
	//get the last word and add
	// it
	tmpString.Trim(ch);
	if (!tmpString.IsEmpty()) {
		tmpArray.Add(tmpString);
		tmpString.Empty();
	}

	InsertAt(insertAt, tmpArray);
}
