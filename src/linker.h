#ifndef		_VTLinkerer_
#define		_VTLinkerer_

#include "vtobj.h"
#include "rpn_eqn.h"
#include "elf.h"
#include <stdio.h>

#define		SYM_LABEL		1
#define		SYM_EQUATE		2
#define		SYM_SET			3
#define		SYM_EXTERN		4
#define		SYM_DEFINE		5
#define		SYM_CSEG		0x0100
#define		SYM_DSEG		0x0200
#define		SYM_8BIT		0x0400
#define		SYM_16BIT		0x0800
#define		SYM_PUBLIC		0x1000
#define		SYM_ISREG		0x2000
#define		SYM_ISEQ		0x4000
#define		SYM_HASVALUE	0x8000
	
#define		LKR_CMD_NONE		0
#define		LKR_CMD_CODE		1
#define		LKR_CMD_DATA		2
#define		LKR_CMD_OBJPATH		3
#define		LKR_CMD_OBJFILE		4
#define		LKR_CMD_ORDER		5
#define		LKR_CMD_CONTAINS	6
#define		LKR_CMD_ENDSWITH	7
#define		LKR_CMD_ENTRY		8
#define		LKR_CMD_DEFINE		9
#define		LKR_CMD_MIXED		10
#define		LKR_CMD_CD_DONE		0x80
#define		LKR_CMD_ERROR		98
#define		LKR_CMD_COMPLETE	99

#define		START_ADDR_CODEEND	-2
#define		START_ADDR_DATAEND	-3

#define		PAGE			1
#define		INPAGE			2

#define		ASEG			0
#define		CSEG			1
#define		DSEG			2

#define		MAX_LOADER_LINE_LEN		220

#define	VT_ISDIGIT(x)  (((x) >= '0') && ((x) <= '9'))

#ifdef WIN32
#define	LINE_ENDING		"\r\n"
#else
#define	LINE_ENDING		"\n"
#endif
// Support classes for VTAssembler objects...

typedef struct sLinkAddrRange {
	int						startAddr;
	int						endAddr;
	int						nextLocateAddr;
	int						endLocateAddr;
	struct sLinkAddrRange*	pNext;
	struct sLinkAddrRange*	pPrev;
} LinkAddrRange;

class CObjSegment : public VTObject
{
public:
	CObjSegment(const char *name, int type);
	~CObjSegment();

	MString				m_Name;				// Name of segment
	int					m_Type;				// ASEG, CSEG or DSEG type
	int					m_Page;
	int					m_Index;			// Current index for listing
	int					m_Count;			// Instruction count for listing
	int					m_sh_offset;		// Offset in .obj file of segment name
	unsigned short		m_Address;			// Address counter for each segment
	VTObArray			m_Reloc;
};

class CObjExtern : public VTObject
{
public:
	CObjExtern()			{ m_Address = 0; };

	MString				m_Name;
	unsigned short		m_Address;
	unsigned short		m_Segment;
	unsigned short		m_SymIdx;
	unsigned char		m_Size;
};

class CObjRelocation : public VTObject
{
public:
	CObjRelocation()	{ m_Address = 0; m_Segment = 0;};

	unsigned short		m_Address;
	CObjSegment*		m_Segment;
};

class CObjSymbol : public VTObject
{
public:
	CObjSymbol() { m_Line = -1; m_Value = -1; m_SymType = 0; 
			m_StrtabOffset = 0; m_Segment = NULL; }

// Attributes

	MString				m_Name;
	long				m_Line;
	long				m_Value;
	CObjSegment*		m_Segment;
	unsigned short		m_SymType;
	unsigned short		m_FileIndex;
	long				m_StrtabOffset;
	unsigned long		m_Off8;
	unsigned long		m_Off16;
};

class CObjFileSection : public VTObject
{
public:
	CObjFileSection() { m_pStrTab = NULL; m_pObjSegment = NULL; m_pProgBytes = NULL; m_Located = false;
						m_LocateAddr = -1; m_Size = 0; m_Index = -1; };
	~CObjFileSection();

	Elf32_Shdr			m_ElfHeader;		// The ELF header as read from the file
	char*				m_pStrTab;			// Pointer to the string table, if any
	CObjSegment*		m_pObjSegment;		// Pointer to the Segment data
	VTObArray			m_Symbols;			// Symbols from the symbol table
	VTObArray			m_Reloc;			// Relocation entries 
	char*				m_pProgBytes;		// Pointer to program bytes
	int					m_Size;				// Size of the ProgBytes
	int					m_Index;			// Index of this section in the object file
	bool				m_Located;			// Indicates if this section has been located
	int					m_LocateAddr;		// Address where the segment was located by linker
	MString				m_Name;				// Name of the section
	int					m_Type;				// Segment type for segment sections
};

class CLinkRgn : public VTObject
{
public:
	CLinkRgn(int type, MString& name, int startAddr, int endAddr, int prot, const char* pAtEndName, int atend);
	~CLinkRgn();

// Attributes
	MString				m_Name;
	LinkAddrRange*		m_pFirstAddrRange;
	LinkAddrRange*		m_pLastAddrRange;
	MString				m_AtEndRgn;
	int					m_Type;
	int					m_Protected;
	int					m_AtEnd;
	int					m_NextLocateAddr;
	int					m_EndLocateAddr;
	int					m_LocateComplete;
};

typedef void (*stdOutFunc_t)(void *pContext, const char *msg);

class CObjFile : public VTObject
{
public:
	CObjFile(const char* name) { m_Name = name; m_pShStrTab = NULL; m_pStrTab = NULL;
				m_pSymSect = NULL; }
	~CObjFile();

// Attributes
	MString				m_Name;				// Filename
	VTObArray			m_FileSections;		// Array of file sections
	Elf32_Ehdr			m_Ehdr;				// The main ELF header
	char*				m_pStrTab;			// Pointer to the string table, if any (not alloced or freed by CObjFile)
	char*				m_pShStrTab;		// Pointer to the string table, if any (not alloced or freed by CObjFile)
	CObjFileSection*	m_pSymSect;			// Object file Symbol Table section pointer
};

class CObjSymFile : public VTObject
{
public:
	CObjSymFile() { m_pObjFile = NULL; m_pObjSection = NULL; }

// Attributes
	CObjFile*			m_pObjFile;			// Pointer to the ObjFile where the symbol came from
	CObjFileSection*	m_pObjSection;		// Pointer to the File Section
	Elf32_Sym*			m_pSym;				// Pointer to the symbol data
	const char *		m_pName;			// Pointer to the name string
};

class VTLinker : public VTObject
{
public:
	VTLinker();
	~VTLinker();

// Attributes
private:
	VTMapStringToOb		m_UndefSymbols;		// Map of undefined symbols
	VTMapStringToOb		m_Symbols;			// Map of Symbols
	VTMapStringToOb		m_ObjFiles;			// Array of CObjFile objects
	VTMapStringToOb		m_LinkRegions;		// Map of CLinkRgn objects where we can link to
	MStringArray		m_LinkFiles;		// Array of filenames to be linked
	MStringArray		m_Errors;			// Array of error messages during parsing
	MStringArray*		m_ActiveLinkOrder;	// Array of error messages during parsing
	VTMapStringToOb		m_LinkOrderList;	// List of link order arrays per segment
	VTMapStringToOb		m_LinkContainsList;	// List of link order arrays per segment
	VTMapStringToOb		m_LinkEndsWithList;	// List of link order arrays per segment
	MString				m_SegName;			// Active segment name
	stdOutFunc_t		m_pStdoutFunc;		// Standard out message routine
	void*				m_pStdoutContext;   // Opaque context for stdout
	int					m_Map;				// Create a map file?
	int					m_Hex;				// Create a HEX file?
	int					m_DebugInfo;		// Include debug info in .obj?
	int					m_ProjectType;
	int					m_FileIndex;
	int					m_Command;			// Script command during file parsing
	int					m_TotalCodeSpace;	// Total space used by code
	int					m_TotalDataSpace;	// Total space used for data
	int					m_TargetModel;		// Target model we are linking for
	unsigned short		m_StartAddress;		// Start address of generated code
	unsigned short		m_EntryAddress;		// Entry address of generated code
	MString				m_OutputName;		// Name of output file
	MString				m_ObjFileList;		// Comma separated list of files
	MString				m_ObjPath;			// Comma separated list of directories
	MString				m_LinkOptions;		// Linker options
	MString				m_RootPath;			// Root path of project.
	MString				m_LoaderFilename;	// Filename of loader to be generated, if any
	MString				m_LinkerScript;		// Name of the linker script
	MString				m_EntryLabel;		// Label or address of program entry
	MStringArray		m_ObjDirs;			// Array of '/' terminated object dirs
	CObjFileSection*	m_SegMap[65536];	// Map of Segment assignements

// Operations
	int					GetValue(MString & string, int & value);
	int					LookupSymbol(MString& name, CObjSymbol *& symbol);
	CObjSymbol*			LookupSymOtherFile(MString& name, CObjSegment** pSeg = NULL);
	void				ResetContent(void);
	int					CreateObjFile(const char *filename);
	void				MakeBinary(int val, int length, MString& binary);
	void				CreateHex(MString& filename);
	void				CreateMap(MString& filename, MString& asmFilename);
	void				CalcObjDirs();
	MString				PreprocessDirectory(const char *pDir);
	int					MapScriptCommand(const char *pStr, int lineNo);
	void				ProcScriptField2(const char *pStr, int lineNo, MString &segname);
	void				ProcScriptField3(char *pStr, int lineNo, int& startAddr, char*& pSectName);
	void				ProcScriptField4(const char *pStr, int lineNo, int& endAddr);
	void				ProcScriptField5(const char *pStr, int lineNo, int& prot, int& atend);
	void				AddOrderedSegment(const char *pStr, int lineNo);
	void				AddContainsSegment(const char *pStr, int lineNo);
	void				AddEndsWithSegment(const char *pStr, int lineNo);
	void				NewLinkRegion(int type, int lineNo, int startAddr,
							int endAddr, int prot, const char* pAtEndName, int atend);
	int					EvaluateScriptAddress(const char *pStr, int lineNo);
	void				ProcessArgs(MString &str, const char *pDelim);
	int					ReadLinkerScript(void);
	int					ReadObjFiles(void);
	int					AssignSectionNames(void);
	int					ReadElfHeaders(FILE* fd, MString& filename);
	int					ReadSectionData(FILE* fd, CObjFile* pFile, 
							CObjFileSection* pFileSection);
	int					LocateSegments(void);
	int					LocateNondependantSegments(void);
	int					LocateOrderedSegments(void);
	int					LocateEndsWithSegments(void);
	int					LocateContainsSegments(void);
	int					LocateSegmentIntoRegion(MString& region, MString& segment,
							bool atEnd = FALSE);
	int					LocateSegmentIntoRegion(MString& region, CObjFileSection* pFileSection, 
							bool atEnd = FALSE);
	CObjFileSection*	FindRelSection(CObjFile* pObjFile, Elf32_Rel* pRel);
	int					ResolveLocals();
	int					ResolveExterns();
	MString				MakeTitle(const MString& path);
	int					GenerateOutputFile(void);
	int					GenerateMapFile(void);
	int					BackAnnotateListingFiles(void);
	int					CreateQuintuple(FILE* fd, unsigned long& ascii85, int& lineNo, 
							int& dataCount, int& lastDataFilePos, int& lineCount, int& checksum);
	int					GenerateLoaderFile(int startAddr, int endAddr, int entryAddr);

public:
// Public Access functions
	int					Link();
	unsigned short		GetEntryAddress(void);
	unsigned short		GetStartAddress(void);
	void				SetLinkOptions(const MString& options);
	void				SetObjFiles(const MString& options);
	void				SetLinkerScript(const MString& script);
	void				SetObjDirs(const MString& dirs);
	void				SetRootPath(const MString& rootPath);
	void				SetLoaderFilename(const MString& loaderFilename);
	void				SetProjectType(int type);
	void				SetStdoutFunction(void *pContext, stdOutFunc_t pFunc);
	void				SetTargetModel(int targetModel) { m_TargetModel = targetModel; }
	const MStringArray&		GetErrors() { return m_Errors; };
	void				SetOutputFile(const MString& outFile );
	int					TotalCodeSpace(void) { return m_TotalCodeSpace; }
	int					TotalDataSpace(void) { return m_TotalDataSpace; }
};

#endif
