#include "stdafx.h"
#include "SPSNodeAction_SitDown.h"
#include "ControlScriptNodeParam_String.h"


CSPSNodeAction_SitDown::CSPSNodeAction_SitDown(const char* lpszName)
:CControlScriptNodeAction(SPS_NODE_ACTION_SITDOWN, lpszName)
{
	m_bSitDown = true;
}

CSPSNodeAction_SitDown::~CSPSNodeAction_SitDown()
{
}


CControlScriptNode::sPARAMETER_INFO* CSPSNodeAction_SitDown::GetParameterMap()
{
	return NULL;
}

bool CSPSNodeAction_SitDown::AddParam(CControlScriptNodeParam_String* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "sitdown") == 0)
	{
		if (NTL_STRICMP(pNode->GetValue(), "true") == 0)
			m_bSitDown = true;
		else if (NTL_STRICMP(pNode->GetValue(), "false") == 0)
			m_bSitDown = false;
		else
		{
			printf("CSPSNodeAction_SitDown: sitdown is not true or false. [%s] \n", pNode->GetValue());
			return false;
		}

		return CControlScriptNode::AddParam(pNode);
	}

	printf("CSPSNodeAction_SitDown: [%s] not exist \n", name);
	return false;
}