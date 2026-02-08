#include "stdafx.h"
#include "WPSNodeCondition_NpcFaint.h"
#include "ControlScriptNodeParam_String.h"
#include "ControlScriptNodeParam_Number.h"


CWPSNodeCondition_NpcFaint::CWPSNodeCondition_NpcFaint(const char* lpszName)
:CControlScriptNodeCondition(WPS_NODE_CONDITION_NPC_FAINT, lpszName)
{
	m_bWpsFail = false;
}

CWPSNodeCondition_NpcFaint::~CWPSNodeCondition_NpcFaint()
{

}

bool CWPSNodeCondition_NpcFaint::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "npc index") == 0)
	{
		m_npcTblidx = (TBLIDX)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CWPSNodeCondition_NpcFaint: [%s] not exist \n", name);

	return false;
}

bool CWPSNodeCondition_NpcFaint::AddParam(CControlScriptNodeParam_String* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "wps fail") == 0)
	{
		if (NTL_STRICMP(pNode->GetValue(), "true") == 0)
			m_bWpsFail = true;
		else if (NTL_STRICMP(pNode->GetValue(), "false") == 0)
			m_bWpsFail = false;
		else
		{
			printf("CWPSNodeCondition_NpcFaint::AddParam [%s] not available \n", pNode->GetValue());
			return false;
		}

		return CControlScriptNode::AddParam(pNode);
	}

	printf("CWPSNodeCondition_NpcFaint: [%s] not exist \n", name);

	return false;
}