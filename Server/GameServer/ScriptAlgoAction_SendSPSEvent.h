#ifndef __SCRIPT_ALGO_DBOG_ACTION_SEND_SPS_EVENT_H__
#define __SCRIPT_ALGO_DBOG_ACTION_SEND_SPS_EVENT_H__

#include "ScriptAlgoAction.h"

#include "NtlObject.h"

class CWpsAlgoAction_SendSPSEvent : public CScriptAlgoAction
{

public:

	CWpsAlgoAction_SendSPSEvent(CWpsAlgoObject* pObject);
	virtual	~CWpsAlgoAction_SendSPSEvent();

public:

	virtual bool				AttachControlScriptNode(CControlScriptNode* pControlScriptNode);

	virtual int					OnUpdate(DWORD dwTickDiff, float fMultiple);

private:

	eOBJTYPE					m_eObjType;

	TBLIDX						m_targetTblidx;

	DWORD						m_eventID;

};

#endif