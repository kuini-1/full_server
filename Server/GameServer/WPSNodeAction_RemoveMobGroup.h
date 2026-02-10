#ifndef __SERVER_WPSNODE_ACTION_REMOVEMOBGROUP__
#define __SERVER_WPSNODE_ACTION_REMOVEMOBGROUP__


#include "ControlScriptNodeAction.h"
#include "NtlCharacter.h"

class CWPSNodeAction_RemoveMobGroup : public CControlScriptNodeAction
{

public:
	CWPSNodeAction_RemoveMobGroup(const char* lpszName);
	virtual ~CWPSNodeAction_RemoveMobGroup();


	virtual sPARAMETER_INFO*	GetParameterMap();

public:

	virtual bool				AddParam(CControlScriptNodeParam_Number* pNode);

	virtual bool				AddParam(CControlScriptNodeParam_String* pNode);


public:

	SPAWNGROUPID				m_spawnGroupId;

	eSPAWN_REMOVE_TYPE			m_eSpawnRemoveType;

};


#endif
