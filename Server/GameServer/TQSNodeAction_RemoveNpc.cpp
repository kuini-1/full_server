#include "stdafx.h"
#include "TQSNodeAction_RemoveNpc.h"
#include "ControlScriptNodeParam_Number.h"
#include "ControlScriptNodeParam_String.h"
#include "NtlCharacter.h"


CTQSNodeAction_RemoveNpc::CTQSNodeAction_RemoveNpc(const char* lpszName)
:CControlScriptNodeAction(TQS_NODE_ACTION_REMOVE_NPC, lpszName)
{
}

CTQSNodeAction_RemoveNpc::~CTQSNodeAction_RemoveNpc()
{
}

CTQSNodeAction_RemoveNpc::sPARAMETER_INFO* CTQSNodeAction_RemoveNpc::GetParameterMap()
{
	return NULL;
}


bool CTQSNodeAction_RemoveNpc::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "index") == 0)
	{
		m_npcTblidx = (TBLIDX)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CTQSNodeAction_RemoveNpc: [%s] not exist \n", name);
	return false;
}

bool CTQSNodeAction_RemoveNpc::AddParam(CControlScriptNodeParam_String* pNode)
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

	printf("CTQSNodeAction_RemoveNpc: [%s] not exist \n", name);
	return false;
}