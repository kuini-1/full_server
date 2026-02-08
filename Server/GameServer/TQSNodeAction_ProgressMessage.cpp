#include "stdafx.h"
#include "TQSNodeAction_ProgressMessage.h"
#include "ControlScriptNodeParam_Number.h"
#include "ControlScriptNodeParam_String.h"
#include "NtlDirection.h"
#include "NtlTimeQuest.h"


CTQSNodeAction_ProgressMessage::CTQSNodeAction_ProgressMessage(const char* lpszName)
:CControlScriptNodeAction(TQS_NODE_ACTION_PROGRESS_MESSAGE, lpszName)
{
}

CTQSNodeAction_ProgressMessage::~CTQSNodeAction_ProgressMessage()
{

}

CTQSNodeAction_ProgressMessage::sPARAMETER_INFO* CTQSNodeAction_ProgressMessage::GetParameterMap()
{
	return NULL;
}


bool CTQSNodeAction_ProgressMessage::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "message value") == 0)
	{
		m_byMessageValue = (BYTE)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CTQSNodeAction_ProgressMessage: [%s] not exist \n", name);
	return false;
}

bool CTQSNodeAction_ProgressMessage::AddParam(CControlScriptNodeParam_String* pNode)
{
	const char* name = pNode->GetName();
	const char* value = pNode->GetValue();

	if (NTL_STRICMP(name, "progress type") == 0)
	{
		if (NTL_STRICMP(value, "tmq") == 0)
			m_eProgressType = PROGRESS_MESSAGE_TYPE_TMQ;
		else if (NTL_STRICMP(value, "tmq stage") == 0)
			m_eProgressType = PGOGRESS_MESSAGE_TYPE_TMQ_STAGE;

		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "message type") == 0)
	{
		if (NTL_STRICMP(value, "start") == 0)
			m_eMessageType = TIMEQUEST_PROGRESS_MESSAGE_TYPE_START;
		else if (NTL_STRICMP(value, "end") == 0)
			m_eMessageType = TIMEQUEST_PROGRESS_MESSAGE_TYPE_END;
		else if (NTL_STRICMP(value, "clear") == 0)
			m_eMessageType = TIMEQUEST_PROGRESS_MESSAGE_TYPE_CLEAR;
		else if (NTL_STRICMP(value, "fail") == 0)
			m_eMessageType = TIMEQUEST_PROGRESS_MESSAGE_TYPE_FAIL;

		return CControlScriptNode::AddParam(pNode);
	}

	printf("CTQSNodeAction_ProgressMessage: [%s] not exist \n", name);
	return false;
}