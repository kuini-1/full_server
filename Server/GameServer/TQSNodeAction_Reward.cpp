#include "stdafx.h"
#include "TQSNodeAction_Reward.h"
#include "ControlScriptNodeParam_Number.h"
#include "ControlScriptNodeParam_String.h"


CTQSNodeAction_Reward::CTQSNodeAction_Reward(const char* lpszName)
:CControlScriptNodeAction(TQS_NODE_ACTION_REWARD, lpszName)
{
}

CTQSNodeAction_Reward::~CTQSNodeAction_Reward()
{

}

CTQSNodeAction_Reward::sPARAMETER_INFO* CTQSNodeAction_Reward::GetParameterMap()
{
	return NULL;
}


bool CTQSNodeAction_Reward::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "event id") == 0)
	{
		m_teid = (WORD)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}
	if (NTL_STRICMP(name, "time") == 0)
	{
		m_dwWaitTime = (DWORD)pNode->GetValue() * 1000;
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CTQSNodeAction_Reward: [%s] not exist \n", name);
	return false;
}

bool CTQSNodeAction_Reward::AddParam(CControlScriptNodeParam_String* pNode)
{
	const char* name = pNode->GetName();
	const char* value = pNode->GetValue();


	if (NTL_STRICMP(name, "trigger type") == 0)
	{
		if (NTL_STRICMP(value, "quest") == 0)
			m_byTriggerType = 0;
		return CControlScriptNode::AddParam(pNode);
	}


	printf("CTQSNodeAction_Reward: [%s] not exist \n", name);
	return false;
}