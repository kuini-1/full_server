#include "stdafx.h"
#include "TQSNodeAction_RemoveMobGroup.h"
#include "ControlScriptNodeParam_Number.h"
#include "ControlScriptNodeParam_String.h"
#include "NtlCharacter.h"


CTQSNodeAction_RemoveMobGroup::CTQSNodeAction_RemoveMobGroup(const char* lpszName)
:CControlScriptNodeAction(TQS_NODE_ACTION_REMOVE_MOB_GROUP, lpszName)
{
}

CTQSNodeAction_RemoveMobGroup::~CTQSNodeAction_RemoveMobGroup()
{

}

CTQSNodeAction_RemoveMobGroup::sPARAMETER_INFO* CTQSNodeAction_RemoveMobGroup::GetParameterMap()
{
	return NULL;
}


bool CTQSNodeAction_RemoveMobGroup::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "group") == 0)
	{
		m_spawnGroupId = (TBLIDX)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CTQSNodeAction_RemoveMobGroup: [%s] not exist \n", name);
	return false;
}

bool CTQSNodeAction_RemoveMobGroup::AddParam(CControlScriptNodeParam_String* pNode)
{
	const char* name = pNode->GetName();
	const char* value = pNode->GetValue();

	if (NTL_STRICMP(name, "type") == 0)
	{
		if (NTL_STRICMP(value, "clear") == 0)
			m_eSpawnRemoveType = SPAWN_REMOVE_TYPE_CLEAR;
		else if (NTL_STRICMP(value, "despawn") == 0)
			m_eSpawnRemoveType = SPAWN_REMOVE_TYPE_DESPAWN;
		else if (NTL_STRICMP(value, "faint") == 0)
			m_eSpawnRemoveType = SPAWN_REMOVE_TYPE_FAINT;

		return CControlScriptNode::AddParam(pNode);
	}

	printf("CTQSNodeAction_RemoveMobGroup: [%s] not exist \n", name);
	return false;
}