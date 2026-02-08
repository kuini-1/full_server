#include "stdafx.h"
#include "WPSNodeCondition_CheckTime.h"
#include "ControlScriptNodeParam_Number.h"


CWPSNodeCondition_CheckTime::CWPSNodeCondition_CheckTime(const char* lpszName)
:CControlScriptNodeCondition(WPS_NODE_CONDITION_CHECK_TIME, lpszName)
{
}

CWPSNodeCondition_CheckTime::~CWPSNodeCondition_CheckTime()
{

}

bool CWPSNodeCondition_CheckTime::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "time") == 0)
	{
		m_dwWaitTime = (DWORD)std::floor(1000.0 * pNode->GetValue());
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CWPSNodeCondition_CheckTime: [%s] not exist \n", name);

	return false;
}
