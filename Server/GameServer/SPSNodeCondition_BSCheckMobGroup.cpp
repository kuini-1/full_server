#include "stdafx.h"
#include "SPSNodeCondition_BSCheckMobGroup.h"
#include "ControlScriptNodeParam_Number.h"
#include "ControlScriptNodeParam_String.h"



CSPSNodeCondition_BSCheckMobGroup::CSPSNodeCondition_BSCheckMobGroup(const char* lpszName)
:CControlScriptNodeCondition(SPS_NODE_CONDITION_BS_CHECK_MOBGROUP, lpszName)
{
	m_spawnGroupId = INVALID_SPAWNGROUPID;
	m_byMaxCount = 0;
	m_byMinCount = 0;
	m_bIsInRange = true;
}

CSPSNodeCondition_BSCheckMobGroup::~CSPSNodeCondition_BSCheckMobGroup()
{
}


bool CSPSNodeCondition_BSCheckMobGroup::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "group") == 0)
	{
		m_spawnGroupId = (SPAWNGROUPID)floor(pNode->GetValue());
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "max count") == 0)
	{
		m_byMaxCount = (BYTE)floor(pNode->GetValue());
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "min count") == 0)
	{
		m_byMinCount = (BYTE)floor(pNode->GetValue());
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CSPSNodeCondition_BSCheckMobGroup: [%s] not exist \n", name);
	return false;
}

bool CSPSNodeCondition_BSCheckMobGroup::AddParam(CControlScriptNodeParam_String* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "is in range") == 0)
	{
		const char* sub = pNode->GetName();

		if (NTL_STRICMP(sub, "true") == 0)
		{
			m_bIsInRange = true;
			return CControlScriptNode::AddParam(pNode);
		}

		if (NTL_STRICMP(sub, "false") == 0)
		{
			m_bIsInRange = false;
			return CControlScriptNode::AddParam(pNode);
		}

		printf("CSPSNodeCondition_BSCheckMobGroup: sub [%s] not exist \n", sub);
		return false;
	}

	printf("CSPSNodeCondition_BSCheckMobGroup: [%s] not exist \n", name);
	return false;
}