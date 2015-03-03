// RpnEquation.h:	This file defines the CRpnEquation class

#ifndef		_RPN_EQUATION_H_
#define		_RPN_EQUATION_H_

#include "MString.h"
#include "MStringArray.h"

#ifndef NULL
#define NULL 0
#endif

// Define operation codes
const	int	RPN_VALUE		= 1;
const	int	RPN_VARIABLE	= 2;
const	int	RPN_MULTIPLY	= 3;
const	int	RPN_DIVIDE		= 4;
const	int	RPN_SUBTRACT	= 5;
const	int	RPN_ADD			= 6;
const	int	RPN_EXPONENT	= 7;
const	int	RPN_MODULUS		= 8;
const	int	RPN_FLOOR		= 9;
const	int	RPN_CEIL		= 10;
const	int	RPN_LN			= 11;
const	int	RPN_LOG			= 12;
const	int	RPN_BITAND		= 13;
const	int	RPN_BITOR		= 14;
const	int RPN_BITXOR		= 15;
const	int	RPN_LEFTSHIFT	= 16;
const	int	RPN_RIGHTSHIFT	= 17;
const	int RPN_IP			= 18;
const	int	RPN_FP			= 20;
const	int	RPN_SQRT		= 21;
const	int	RPN_LAST		= 22;
const	int	RPN_NOT			= 23;
const	int	RPN_BITNOT		= 24;
const	int	RPN_HIGH		= 25;
const	int	RPN_LOW			= 26;
const	int RPN_DEFINED		= 27;
const	int	RPN_NEGATE		= 28;
const	int	RPN_MACRO		= 29;
const	int RPN_PAGE		= 30;

class CRpnOperation : public VTObject
{
public:
	CRpnOperation()		{ m_Value = 0.0; m_Operation = 0; m_Macro = NULL; };
	~CRpnOperation()	{ if (m_Macro != NULL) delete m_Macro; }

// Attributes
	int				m_Operation;
	MString			m_Variable;		// Used only for variable operations
	double			m_Value;
	VTObject*		m_Macro;		// Used to save macros in the code
};

class CRpnOpArray : public VTObArray
{
public:
	CRpnOpArray()	{};

// Attributes
	CRpnOperation& operator [] (int index) { return *((CRpnOperation *) GetAt(index)); }
};

class CRpnEquation : public VTObject
{
	DECLARE_DYNCREATE(CRpnEquation);
public:
	CRpnEquation()		{ m_EqPtr = 0; }
	~CRpnEquation()		{ ResetContent(); }


	void				ResetContent(void);
	void				Add(const char * var);
	void				Add(double value);
	void				Add(int operation);
	void				Add(int operation, const char *var);
	void				Add(VTObject *macro);

// Attributes
	CRpnOpArray			m_OperationArray;
    int                 m_Line;
	int					m_EqPtr;
};

#endif  /* _RPN_EQUATION_H_ */

