#include	"vtobj.h"
#include	"MString.h"
#include	"MStringArray.h"
#include	"rpn_eqn.h"

IMPLEMENT_DYNCREATE(CRpnEquation, VTObject);

void CRpnEquation::ResetContent(void)
{
	for (int c = m_OperationArray.GetSize() - 1; c >= 0; c--)
	{
		delete (CRpnOperation*) m_OperationArray.GetAt(c);
	}								  
	m_OperationArray.RemoveAll();
}

void CRpnEquation::Add(int operation)
{
	CRpnOperation*	op = new CRpnOperation;

	// Fill operation with operation code
	op->m_Operation = operation;

	// Add operation to array
	m_OperationArray.Add(op);
}

void CRpnEquation::Add(int operation, const char *var)
{
	CRpnOperation*	op = new CRpnOperation;

	// Fill operation with operation code
	op->m_Operation = operation;
	op->m_Variable = var;

	// Add operation to array
	m_OperationArray.Add(op);
}

void CRpnEquation::Add(double value)
{
	CRpnOperation*	op = new CRpnOperation;

	// Fill operation with appropriate data
	op->m_Operation = RPN_VALUE;
	op->m_Value = value;

	// Add operation to array
	m_OperationArray.Add(op);
}

void CRpnEquation::Add(const char *var)
{
	CRpnOperation*	op = new CRpnOperation;

	// Fill operation with appropriate data
	op->m_Operation = RPN_VARIABLE;
	op->m_Variable = var;

	// Add operation to array
	m_OperationArray.Add(op);
}

void CRpnEquation::Add(VTObject* pMacro)
{
	CRpnOperation*	op = new CRpnOperation;

	// Fill operation with operation code
	op->m_Operation = RPN_MACRO;
	op->m_Macro = pMacro;

	// Add operation to array
	m_OperationArray.Add(op);
}

