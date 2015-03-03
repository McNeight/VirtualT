/*  MString - Dynamic string data type library
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

#ifndef MSTRING_H
#define MSTRING_H

#ifdef __cplusplus  // we only want this for C++ code

/*  MString by Jesse Lovelace (mentat)
	jesse at aslogicsys dot com

    (MString began as GString but was changed to remain compatible with
	gtk+'s GString data type.)

	http://mstring.sourceforge.net

	MString is a string library that uses dynamically sizable character
	strings.  To do this I used linked lists instead of dynamic arrays for
	various reasons (one being the problems that arrays of dynamic arrays
	cause).

	MString is designed to maintain compatibility with the MFC CString
	library.  MString should be 99% compatible if you exclude the windoze
	specific operations (including buffer access) and the .Format commands.

	For the full documentation see functions.txt

	MString also includes the MStringArray class.  MStringArray follows the
	same scheme as CStringArray from MFC.  CStringArrays are also linked
	lists for now (I may change them to dynamically allocated arrays in the
	future).

	Please submit any suggestions/bugs/comments to jllovela@eos.ncsu.edu

	Enjoy.

	Thanks to:
      Brian Matherly - Project manager and developer.  
      Balint Toth - Fixed insert bug and new features,
      Bruce Riggins - New char * code,
      Dingo - pointer debugging, 
      Rask - math and Trim(char ch), 
      Botch - debugging and advice,
      Antiloop - for phat grooves.

	(Last modified April 10, 2005)
*/

const int MAX_PRECISION = 9; //due to bad double to string code.
const char MString_VERSION[8] = "0.70";
//typedef const char * LPCSTR;

//#include <fstream>

class MNode;

struct MStringData {
	char * pcStr;
	int precision;
	int iBufLen; // length of buffer minus 1 for null terminator
//	int iBufferInUse;
};

class MString {

//Begin overloaded stream functions -----------------------

//	friend std::ostream& operator<< (std::ostream& out, const MString& string); 
	// PRE: out is a valid output stream, string is a valid MString
	// POST: contents of string are sent to stream out.

//	friend std::istream& operator>> (std::istream& in, MString& string); 
	// PRE: in is a valid input stream, string is a valid MString
	// POST: characters in input stream are stored in string.

//	friend std::istream& getline(std::istream& in, MString& string); 
	// PRE: in is valid input stream, sting is a valid MString
	// POST: characters up to (and including) the newline are stored 
	//       in string.
	// EX: getline(in, string);

//End overloaded stream functions -------------------------

public:

//Begin Constructors --------------------------------------

	MString(); 
	// Info: construct empty MString
	
	MString(const MString& stringSrc); 
	// Info: copy constructor
	// PRE: stringSrc is a valid MString object.
	// POST: new object is a deep copy of stringSrc.

	MString(const char ch, int nRepeat = 1);
	// Info: construct with single character repeated nRepeat times
	// PRE: ch is a valid ascii character and non null.
	// POST: new object contains ch repeated nRepeat times.

	MString(const char* string);
	// PRE: string is a valid null-terminated string.
	// POST: new object contains the data from string minues null.

// Added by Bruce Riggins	
	MString(const char * string, int nLength);
	// Info: copy with maximum length
	// PRE: string is a valid null-terminated string, nLength>0
	// POST: new object contains the data from string minues null
// End addition	
	
	MString(const int num);
	// NEW CODE, PLEASE HELP DEBUG (v. 60)
	MString(double num);

	~MString(); 
	// Info: destructor

//End Constructors ----------------------------------------

//Begin Test Functions ------------------------------------

	void testG();
	// Info: uses cout to display the data and pointer of each
	//       Node.

	MString GetGVersion();
	// Info: returns the current version of MString.
	// POST: returns an object of type MString containing version info

	MString ReturnLicense();
	// Info: returns license info
	// POST: returns an object of type MString containing license info


//End Test Functions --------------------------------------

//Begin Precision Functions -------------------------------

	void SetPrecision(int num);
	// PRE: num <= MAX_PRECISION
	// POST: double conversion precision changed to num.

	int GetPrecision() const;
	// POST: returns an int type equal to the current double precision.

//End Precision Functions ---------------------------------

//Begin String as Array Functions -------------------------
	
	int GetLength() const; 
	// Info: returns the length of the string excluding null

	bool IsEmpty() const;  
	// Info: nonzero if string has 0 length
	
	void Empty();  
	// Info: empty's the string and free's memory
	
	char GetAt(int nIndex) const; 
	// Info: returns char at index
	
	char operator [](int nIndex) const; 
	// Info: operator overload of GetAt(int nIndex)

	char& operator [](int nIndex);
	// Info: use like SetAt, myString[0] = 'M';

	void SetAt(int nIndex, char ch); 
	// Info: sets the character at nIndex

//End String as Array Functions ---------------------------

//Begin added operators by Bruce Riggins ---------------
	operator const char * () const;
//End added operators by Bruce Riggins -----------------

//Begin Assignment/Concatination operators ----------------

	const MString& operator =(const MString& stringSrc); //Idea from CString
	const MString& operator =(char ch);
	const MString& operator =(const char* string);
	const MString& operator =(int num); //Original MString
	const MString& operator =(double num); //Original MString
	const MString& operator =(float num); //Original MString

	const MString& operator +=(const MString& string); //Idea from CString
	const MString& operator +=(char ch);
	const MString& operator +=(char* string);
	const MString& operator +=(int num); //Original MString
	const MString& operator +=(double num); //Original MString
	const MString& operator +=(float num); //Original MString

	void Cat(char ch);   //Not implimented yet.
	void Cat(char* string);
	void Cat(MString& string); 

	

	friend MString operator +(const MString& string1, const MString& string2);
	friend MString operator +(const MString& string, char ch);
	friend MString operator +(char ch, const MString& string);
	friend MString operator +(const MString& string1, char* string2);
	friend MString operator +(char* string1, const MString& string2);
	friend MString operator +(int num, const MString& string);
	friend MString operator +(const MString& string, int num);


//End Assignment/Concatination operators ------------------

//Begin Comparison operators ------------------------------

	friend bool operator==(const MString& s1, const MString& s2); //Idea from CString
	friend bool operator==(const MString& s1, const char* s2);
	friend bool operator==(const char* s1, const MString& s2);

	friend bool operator!=(const MString& s1, const MString& s2); //Idea from CString
	friend bool operator!=(const MString& s1, char* s2);
	friend bool operator!=(char* s1, const MString& s2);
	friend bool operator!=(const MString& s1, const char* s2);

	friend bool operator <(const MString& s1, const MString& s2); //Idea from CString
	friend bool operator <(const MString& s1, char* s2);
	friend bool operator <(char* s1, const MString& s2);

	friend bool operator >(const MString& s1, const MString& s2); //Idea from CString
	friend bool operator >(const MString& s1, char* s2);
	friend bool operator >(char* s1, const MString& s2);

	friend bool operator <=(const MString& s1, const MString& s2); //Idea from CString
	friend bool operator <=(const MString& s1, char* s2);
	friend bool operator <=(char* s1, const MString& s2);

	friend bool operator >=(const MString& s1, const MString& s2); //Idea from CString
	friend bool operator >=(const MString& s1, char* s2);
	friend bool operator >=(char* s1, const MString& s2);

	int Compare(const char* string) const; //Idea from CString
	int Compare(const MString& string) const; //MString original
	int CompareNoCase(const char* string) const; //Idea from CString
	int Collate(char* string) const; //Idea from CString
	int CollateNoCase(char* string) const; //Idea from CString

	bool CompareGlob(const MString& glob) const;

//End Comparison operators --------------------------------

//Begin Extraction operators ------------------------------

	MString Mid(int nFirst) const;  //Idea from CString
	MString Mid(int nFirst, int nCount) const; //Idea from CString
	MString Left(int nCount) const; //Idea from CString
	MString Right(int nCount) const; //Idea from CString
	MString SpanIncluding(char* string) const; //Idea from CString
	MString SpanExcluding(char* string) const; //Idea from CString
	MString Filename(void) const;
	char* ToChar(int nStart = 0,int nCount = -1); 
	// PRE: nStart is >0 and < GetLength().
	// POST: returns a pointer to a new null-terminated character array
	//       that is a copy of the infomation in the MString.

	int ToInt(int nStart = 0) const;
	// PRE: nStart is > 0 and < GetLength().  
	// POST: returns an integer created from the numbers starting at nStart
	// Other: ToInt does not check to see if the string of numbers contained 
	//        in the MString object is to long for type int.  ToInt converts
	//        a sequence of ASCII values 48 thru 57 ('0' to '9') non-number
	//        values result in a return from the function.
	// EX: if myString is of type MString containing "12345C1232"
	//     myString.ToInt(0) returns 12345
	//     myString.ToInt(1) returns 2345

	double ToDouble( int nStart = 0 );

//End Extraction operators --------------------------------

//Begin Other Conversions ---------------------------------
	
	void MakeUpper(); //Idea from CString
	void MakeLower(); //Idea from CString
	void MakeReverse(); //Idea from CString

	int Replace(char chOld, char chNew); //Idea from CString
	int Replace(char* stringOld, char* stringNew); //Idea from CString
	int Remove(char ch); //Idea from CString
	int Insert(int nIndex, char ch); //Idea from CString
	int Insert(int nIndex, char* string); //Idea from CString
	int Delete(int nIndex, int nCount = 1); //Idea from CString

	//Research Format
// Additions by Bruce Riggins 11/14/00	
	void Format(const char * sFormat, ...);
// End additions by Bruce Riggins 11/14/00	

	void Trim(); //Original MString
	void Trim(char ch); //Original MString
	void Trim(char* string);

	void TrimLeft(); //Idea from CString
	void TrimLeft(char ch);
	void TrimLeft(char* string);


	void TrimRight(); //Idea from CString
	void TrimRight(char ch);
	void TrimRight(char* string);

//End Other Conversions -----------------------------------

	//Research FormatMessage

//Begin Searching -----------------------------------------

	int Find(char ch, int nStart = 0) const; //Idea from CString
	int Find(const char* string, int nStart = 0) const; //Idea from CString


	int ReverseFind(char ch) const; //Idea from CString
	int ReverseFind(char* string) const; //MString original
	int FindOneOf(char* string) const; //Idea from CStrin


//End Searching -------------------------------------------

	//Buffer Access and Windows-Specific items not included.

//Begin Buffer Access -------------------------------------
	// Additions by Bruce Riggins 11/7/00
	// Access to string implementation buffer as "C" character array
	char * GetBuffer(int nMinBufLength = -1);	//Idea from CString
	void ReleaseBuffer(int nNewLength = -1);	//Idea from CString
//	char * GetBufferSetLength(int nNewLength);	//Idea from CString
//	void FreeExtra();	//Idea from CString

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	char * LockBuffer();	//Idea from CString
	void UnlockBuffer();	//Idea from CString
	
	// End of BR additions

//End Buffer Access ---------------------------------------

private:

	MNode* GetPointerAt(int nIndex);
	void deallocate(MNode *p); // added by Bruce Riggins

	bool Gmatch( MNode* g, MNode* s ) const;
	
	char * pcStr;	
	MNode *headMNode;
	MNode *tailMNode; //New for .3b

	int precision;
	int iBufLen; // length of buffer minus 1 for null terminator
//	int iBufferInUse;

   // flag to indicate that the MString has been modified since the
   // last time char* GetBuffer() was called
   bool bModified; 

};

/*
===========================================================================
CFileString:	Class to do special File and Directory manipulation things
				with strings (because I'm tired of doing it over and over).
===========================================================================
*/
class CFileString
{
public:
	CFileString() { m_SubDirIndex = 0; }
	CFileString(MString string) { m_String = string; m_SubDirIndex = 0; };

	MString		Filename();		// C:\DATA\SUBDIR\FILE.TXT    -> FILE.TXT
	MString		Drive();		// C:\DATA\SUBDIR\FILE.TXT    -> C:
	MString		Title();		// C:\DATA\SUBDIR\FILE.TXT    -> FILE
	MString		Directory();	// C:\DATA\SUBDIR\FILE.TXT    -> \\DATA\\SUBDIR\\  //
	MString		Ext();			// C:\DATA\SUBDIR\FILE.TXT    -> .TXT
	MString		FirstSubDir();	// C:\DATA\SUBDIR\FILE.TXT    -> DATA
	MString		NextSubDir();	// C:\DATA\SUBDIR\FILE.TXT    -> SUBDIR  ...
	void		NewExt(MString ext);

	MString&	GetString(void) { return m_String; };
	MString&	operator->*(MString&) { return m_String; };
	CFileString&	operator=(const MString& string) { m_String = string; return *this; };

protected:
	int m_SubDirIndex;
	MString		m_String;
};

#endif // __cplusplus

#endif
