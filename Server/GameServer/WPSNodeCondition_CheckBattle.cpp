#include "stdafx.h"
#include "WPSNodeCondition_CheckBattle.h"
#include "ControlScriptNodeParam_Number.h"
#include "ControlScriptNodeParam_String.h"
#include "NtlObject.h"


CWPSNodeCondition_CheckBattle::CWPSNodeCondition_CheckBattle(const char* lpszName)
:CControlScriptNodeCondition(WPS_NODE_CONDITION_CHECK_BATTLE, lpszName)
{
	m_eObjType = INVALID_OBJTYPE;
	m_index = INVALID_TBLIDX;
	m_group = INVALID_SPAWNGROUPID;
	m_bIsBattle = true;
}

CWPSNodeCondition_CheckBattle::~CWPSNodeCondition_CheckBattle()
{

}

bool CWPSNodeCondition_CheckBattle::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "group") == 0)
	{
		m_group = (unsigned int)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CWPSNodeCondition_CheckBattle: [%s] not exist \n", name);
	return false;
}

bool CWPSNodeCondition_CheckBattle::AddParam(CControlScriptNodeParam_String* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "type") == 0)
	{
		if (NTL_STRICMP(pNode->GetValue(), "mob") == 0)
		{
			m_eObjType = OBJTYPE_MOB;
			return CControlScriptNode::AddParam(pNode);
		}
		printf("value '%s' dont exist \n", pNode->GetValue());
	}

	if (NTL_STRICMP(name, "is battle") == 0)
	{
		if (NTL_STRICMP(pNode->GetValue(), "true") == 0)
		{
			m_bIsBattle = true;
			return CControlScriptNode::AddParam(pNode);
		}
		if (NTL_STRICMP(pNode->GetValue(), "false") == 0)
		{
			m_bIsBattle = false;
			return CControlScriptNode::AddParam(pNode);
		}
		printf("value '%s' dont exist \n", pNode->GetValue());
	}

	printf("CWPSNodeCondition_CheckBattle: [%s] not exist \n", name);
	return false;
}