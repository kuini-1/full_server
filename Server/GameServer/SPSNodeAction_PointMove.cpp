#include "stdafx.h"
#include "SPSNodeAction_PointMove.h"
#include "ControlScriptNodeParam_Number.h"


CSPSNodeAction_PointMove::CSPSNodeAction_PointMove(const char* lpszName)
:CControlScriptNodeAction(SPS_NODE_ACTION_POINT_MOVE, lpszName)
{
	m_bRunMode = false;
	m_fMoveSpeed = INVALID_FLOAT;
	m_vDestLoc.Reset();
	m_vDestDir.Reset();
}

CSPSNodeAction_PointMove::~CSPSNodeAction_PointMove()
{
}

CControlScriptNode::sPARAMETER_INFO* CSPSNodeAction_PointMove::GetParameterMap()
{
	return NULL;
}

bool CSPSNodeAction_PointMove::AddParam(CControlScriptNodeParam_Number* pNode)
{
	const char* name = pNode->GetName();

	if (NTL_STRICMP(name, "loc x") == 0)
	{
		m_vDestLoc.x = (float)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "loc y") == 0)
	{
		m_vDestLoc.y = (float)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "loc z") == 0)
	{
		m_vDestLoc.z = (float)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "dir x") == 0)
	{
		m_vDestDir.x = (float)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "dir y") == 0)
	{
		m_vDestDir.y = (float)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "dir z") == 0)
	{
		m_vDestDir.z = (float)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "run mode") == 0)
	{
		m_bRunMode = pNode->GetValue() != 0.0f;
		return CControlScriptNode::AddParam(pNode);
	}

	if (NTL_STRICMP(name, "move speed") == 0)
	{
		m_fMoveSpeed = (float)pNode->GetValue();
		return CControlScriptNode::AddParam(pNode);
	}

	printf("CSPSNodeAction_PointMove: [%s] not exist \n", name);
	return false;
}

