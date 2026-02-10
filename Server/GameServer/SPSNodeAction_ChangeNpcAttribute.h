#ifndef __SERVER_SPSONODE_ACTION_CHANGENPCATTRIBUTE__
#define __SERVER_SPSONODE_ACTION_CHANGENPCATTRIBUTE__


#include "ControlScriptNodeAction.h"
#include "NtlObject.h"

class CSPSNodeAction_ChangeNpcAttribute : public CControlScriptNodeAction
{

public:
	CSPSNodeAction_ChangeNpcAttribute(const char* lpszName);
	virtual ~CSPSNodeAction_ChangeNpcAttribute();


	virtual sPARAMETER_INFO*	GetParameterMap() { return NULL; }

public:

	virtual bool				AddParam(CControlScriptNodeParam_String* pNode);


public:

	bool						m_bApplyRelationPc;;

	eRELATION_TYPE				m_eRelationPc;

};


#endif
