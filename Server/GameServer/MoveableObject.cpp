#include "stdafx.h"
#include "MoveableObject.h"


CMoveableObject::CMoveableObject(eOBJTYPE eObjType)
	:CSpawnObject(eObjType)
{
	Init();
}

CMoveableObject::~CMoveableObject()
{
	if (!m_bIsDeleted)
		this->Destroy();
}



bool CMoveableObject::Create()
{
	return CSpawnObject::Create();
}


void CMoveableObject::Destroy()
{
	m_listRemainingDestLoc.clear();
	m_fFollowDistance = 0.0f;
}


void CMoveableObject::Init()
{
	m_vDestLoc.Reset();
	m_vSecondDestLoc.Reset();
	m_bHaveSecondDestLoc = false;
	m_listRemainingDestLoc.clear();
	m_vDestTurningDir.Reset();
	m_vJumpHeading.Reset();
	m_byMoveDirection = NTL_MOVE_NONE;
	m_byMoveStatus = NTL_MOVE_STATUS_NONE;
	m_byMoveFlag = NTL_MOVE_FLAG_WALK;
	m_fFollowDistance = 0.0f;
	m_bIsCollision = false;
	m_dwWorldUpadateTick = 0;
	m_byFollowMoveReason = DBO_MOVE_FOLLOW_INVALID;
}


void CMoveableObject::UpdateMove(DWORD dwTickDiff, float fMultiple)
{
	UNREFERENCED_PARAMETER(fMultiple);

	if (GetCollision())
		return;

	CNtlVector rvNewDir;
	CNtlVector rvNewLoc;
	CNtlVector rLoc;
	std::list<sVECTOR3> rlistConsumedDestLoc;
	float fActualSpeed = 0.0f;
	if (PredictMovement(dwTickDiff, rvNewDir, rvNewLoc, rLoc, &rlistConsumedDestLoc, &fActualSpeed))
	{
		//if(IsPC() )
		//	printf("IsPC %i | cur loc x %f new loc x %f [speed %f] [direction %u]\n", IsPC(), GetCurLoc().x, rvNewLoc.x, fActualSpeed, GetMoveDirection());

		SetCurDir(rvNewDir.x, rvNewDir.y, rvNewDir.z);
		SetCurLoc(rvNewLoc, GetCurWorld());

		this->SetDestLoc(rLoc);
		this->ConsumeDestLoc(rlistConsumedDestLoc);
	}
}

bool CMoveableObject::UpdateFollow(DWORD dwTickDiff, float fMultiple)
{
	UpdateMove(dwTickDiff, fMultiple);
	return true;
}

void CMoveableObject::TickProcess(DWORD dwTickDiff, float fMultiple)
{
	CGameObject::TickProcess(dwTickDiff, fMultiple);

	if (m_pObjectController && GetCurWorld() /*&& GetID() == 1997242*/)
		m_pObjectController->OnUpdate(dwTickDiff, fMultiple);

	m_dwWorldUpadateTick += dwTickDiff;

	if (m_pCurWorldCell)
	{
		if (/*!IsPC() || (IsPC() &&*/ m_dwWorldUpadateTick >= 100) //update every X ms
		{
			m_pCurWorldCell->MoveObject(this);
			m_dwWorldUpadateTick = 0;
		}
	}
}

bool CMoveableObject::SetMoveDirection(BYTE byMoveDirection)
{
	if (byMoveDirection < NTL_MOVE_COUNT)
	{
		m_byMoveDirection = byMoveDirection;
		return true;
	}

	return false;
}

BYTE CMoveableObject::GetDestLocCount()
{
	return (BYTE)m_listRemainingDestLoc.size() + 1;
}

bool CMoveableObject::SetMultipleDestLoc(BYTE byDestLocCount, sVECTOR3* pavDestLoc)
{
	if (byDestLocCount > 0)
	{
		if (pavDestLoc)
		{
			SetDestLoc(pavDestLoc[0]);
			m_listRemainingDestLoc.clear();
			for (int i = 1; i < byDestLocCount; i++)
				m_listRemainingDestLoc.push_back(pavDestLoc[i]);

			return true;
		}
	}

	return false;
}

bool CMoveableObject::IsMoveComplete()
{
	if (GetMoveDirection() == NTL_MOVE_MOUSE_MOVEMENT)
	{
		if (m_listRemainingDestLoc.size() == 0)
		{
			if (m_vDestLoc.operator==(GetCurLoc()))
				return true;
		}
	}

	return false;
}

void CMoveableObject::SetFollowDistance(float fFollowDistance)
{
	if (fFollowDistance >= 0.0f)
		this->m_fFollowDistance = fFollowDistance;
	else
		fFollowDistance = 0.0f;
}

bool CMoveableObject::IsPassiveDashComplete()
{
	if (IsDashPossible(GetMoveDirection()))
	{
		if (m_vDestLoc.operator==(GetCurLoc()))
			return true;
	}
	else ERR_LOG(LOG_USER, "NTL_MOVE_DASH_PASSIVE_FIRST > m_byMoveDirection || NTL_MOVE_DASH_PASSIVE_LAST < m_byMoveDirection, m_byMoveDirection = %u", GetMoveDirection());

	return false;
}

bool CMoveableObject::JumpStart(CNtlVector& rvCurDir)
{
	if (m_byMoveStatus == NTL_MOVE_STATUS_JUMP)
		return false;

	CNtlVector rDir(rvCurDir);
	rDir.y = 0.0f;

	if (!rDir.SafeNormalize())
		return false;

	SetJumpHeading(rDir);
	SetMoveStatus(NTL_MOVE_STATUS_JUMP);

	return true;
}

bool CMoveableObject::JumpEnd()
{
	if (m_byMoveStatus > NTL_MOVE_STATUS_NONE)
	{
		BYTE byNewMoveDirection = NTL_MOVE_UNKNOWN;
		switch (GetMoveDirection())
		{
		case NTL_MOVE_NONE:
		case NTL_MOVE_F:
		case NTL_MOVE_B:
		case NTL_MOVE_L:
		case NTL_MOVE_R:
		case NTL_MOVE_TURN_L:
		case NTL_MOVE_TURN_R: byNewMoveDirection = GetMoveDirection(); break;

		case NTL_MOVE_L_TURN_L_JUMP: byNewMoveDirection = NTL_MOVE_L_TURN_L; break;
		case NTL_MOVE_L_TURN_R_JUMP: byNewMoveDirection = NTL_MOVE_L_TURN_R; break;
		case NTL_MOVE_R_TURN_L_JUMP: byNewMoveDirection = NTL_MOVE_R_TURN_L; break;
		case NTL_MOVE_R_TURN_R_JUMP: byNewMoveDirection = NTL_MOVE_R_TURN_R; break;
		case NTL_MOVE_F_TURN_L_JUMP: byNewMoveDirection = NTL_MOVE_F_TURN_L; break;
		case NTL_MOVE_F_TURN_R_JUMP: byNewMoveDirection = NTL_MOVE_F_TURN_R; break;
		case NTL_MOVE_B_TURN_L_JUMP: byNewMoveDirection = NTL_MOVE_B_TURN_L; break;
		case NTL_MOVE_B_TURN_R_JUMP: byNewMoveDirection = NTL_MOVE_B_TURN_R; break;

		default: break;
		}

		if (byNewMoveDirection == NTL_MOVE_UNKNOWN)
			return false;

		SetMoveDirection(byNewMoveDirection);
		SetMoveStatus(NTL_MOVE_STATUS_NONE);
		return true;
	}

	return false;
}

bool CMoveableObject::PredictMovement(DWORD dwTickDiff, CNtlVector& rvNewDir, CNtlVector& rvNewLoc, CNtlVector& rvNewDestLoc, std::list<sVECTOR3> *rlistConsumedDestLoc, float *rfActualSpeed)
{
	if (GetCurWorld() == NULL)
	{
		return false;
	}

	rvNewDir = GetCurDir();
	rvNewLoc = GetCurLoc();
	rvNewDestLoc = GetDestLoc();
	rlistConsumedDestLoc->clear();

	float rfTurningSpeedRatio = 1.0f;
	AdjustCurSpeed(rfActualSpeed, &rfTurningSpeedRatio);

	if (GetMoveDirection() >= NTL_MOVE_NONE && GetMoveDirection() <= NTL_MOVE_KEYBOARD_LAST)
	{
		if (GetMoveStatus() <= NTL_MOVE_STATUS_AIR_MOVE)
		{
			NtlGetDestination_Keyboard(GetCurDir().x, GetCurDir().y, GetCurDir().z, *rfActualSpeed, GetCurLoc().x, GetCurLoc().y, GetCurLoc().z, GetMoveDirection(), dwTickDiff, rfTurningSpeedRatio, &rvNewDir, &rvNewLoc);
		}
		else if (GetMoveStatus() == NTL_MOVE_STATUS_JUMP)
		{
			NtlGetDestination_Jump(GetCurDir().x, GetCurDir().y, GetCurDir().z, *rfActualSpeed, GetCurLoc().x, GetCurLoc().y, GetCurLoc().z, GetJumpHeading().x, GetJumpHeading().z, GetMoveDirection(), dwTickDiff, rfTurningSpeedRatio, &rvNewDir, &rvNewLoc);
		}
		else if (GetMoveStatus() == NTL_MOVE_STATUS_DASH)
		{
			CNtlVector vDashMove = (GetDestLoc() - GetCurLoc());
			vDashMove.y = 0.0f;
			if (!vDashMove.SafeNormalize())
				vDashMove = GetCurDir();

			NtlGetDestination_Dash(vDashMove.x, vDashMove.z, Dbo_GetDashSpeed(GetWalkingSpeed(), GetLastRunSpeed()), GetCurLoc().x, GetCurLoc().y, GetCurLoc().z, GetDestLoc().x, GetDestLoc().y, GetDestLoc().z, dwTickDiff, &rvNewLoc);
		}
		else
		{
			ERR_LOG(LOG_USER, "invalid move status(%u) in predict movement", GetMoveStatus());
			return false;
		}
	}
	else if (GetMoveDirection() == NTL_MOVE_MOUSE_MOVEMENT)
	{
		NtlGetDestination_Mouse(GetCurDir().x, GetCurDir().z, *rfActualSpeed, GetCurLoc().x, GetCurLoc().y, GetCurLoc().z, GetDestLoc().x, GetDestLoc().y, GetDestLoc().z, dwTickDiff, &rvNewDir, &rvNewLoc);
		if (rvNewLoc == m_vDestLoc)
		{
			std::list<sVECTOR3>::iterator iterRemainingDestLoc = m_listRemainingDestLoc.begin();
			if (iterRemainingDestLoc != m_listRemainingDestLoc.end())
			{
				rvNewDestLoc = (*iterRemainingDestLoc);
				rlistConsumedDestLoc->push_back(*iterRemainingDestLoc);
			}
		}
	}
	else if (GetMoveDirection() == NTL_MOVE_FOLLOW_MOVEMENT)
	{
		if (GetMoveStatus() == NTL_MOVE_STATUS_NONE)
		{
			NtlGetDestination_Follow(GetCurDir().x, GetCurDir().z, *rfActualSpeed, GetCurLoc().x, GetCurLoc().y, GetCurLoc().z, GetDestLoc().x, GetDestLoc().y, GetDestLoc().z, dwTickDiff, m_fFollowDistance, &rvNewDir, &rvNewLoc);
		}
		else if (GetMoveStatus() == NTL_MOVE_STATUS_AIR_MOVE)
		{
			NtlGetDestination_AirFollow(GetCurDir(), *rfActualSpeed, GetCurLoc().x, GetCurLoc().y, GetCurLoc().z, GetDestLoc().x, GetDestLoc().y, GetDestLoc().z, dwTickDiff, m_fFollowDistance, &rvNewDir, &rvNewLoc);
		}
		else
		{
		//	ERR_LOG(LOG_USER, "invalid move status(%u) in predict movement", GetMoveStatus()); //dont need to enable this or show error when player is following someone and jumps
			return false;
		}
	}
	else
	{
	//	ERR_LOG(LOG_USER, "Invalid Move Direction(%u)!! Move Status(%u) from PredictMovement(Obj-Type %u)", GetMoveDirection(), GetMoveStatus(), GetObjType());
		return false;
	}

	if (rvNewDir.Length() < 0.95f || rvNewDir.Length() > 1.001f)
	{
	//	ERR_LOG(LOG_USER, "fLength < 0.95f || fLength > 1.001f, fLength = %f, worldId[%u], GetCurLoc() = (%f, %f, %f), m_vCurDir = (%f, %f, %f), m_vDestLoc = (%f, %f, %f), m_byMoveDirection = %u, m_fFollowDistance = %f"
	//		, rvNewDir.Length(), GetWorldTblidx(), GetCurLoc().x, GetCurLoc().y, GetCurLoc().z, GetCurDir().x, GetCurDir().y, GetCurDir().z, m_vDestLoc.x, m_vDestLoc.y, m_vDestLoc.z, GetMoveDirection(), m_fFollowDistance);
		return false;
	}

	//m_pCurWorld->AdjustLocation(rvNewLoc);

	if (m_pCurWorld->CanMoveTo(rvNewLoc, this) == false)
	{
		rvNewLoc = GetCurLoc();
		rvNewDir = GetCurDir();

		if (GetMoveDirection() == NTL_MOVE_MOUSE_MOVEMENT
			|| GetMoveDirection() == NTL_MOVE_F
			|| GetMoveDirection() == NTL_MOVE_B
			|| GetMoveDirection() == NTL_MOVE_L
			|| GetMoveDirection() == NTL_MOVE_R)
		{
				rvNewDestLoc = GetCurLoc();
		}
	}

	return true;
}


bool CMoveableObject::CalculateJumpDir(CNtlVector* pvNewCurDir, CNtlVector* pvJumpHeading)
{
	switch (GetMoveDirection())
	{
	case NTL_MOVE_NONE:
	case NTL_MOVE_F:
	{
		pvJumpHeading->operator=(*pvNewCurDir);
	}
	break;
	case NTL_MOVE_B:
	{
		pvJumpHeading->operator=(RotateVector180Degree(pvNewCurDir));
	}
	break;
	case NTL_MOVE_TURN_L:
	case NTL_MOVE_TURN_R:
	case NTL_MOVE_F_TURN_L:
	case NTL_MOVE_F_TURN_R:
	{
		pvJumpHeading->operator=(*pvNewCurDir);
	}
	break;

	case NTL_MOVE_B_TURN_L:
	case NTL_MOVE_B_TURN_R:
	{
		pvJumpHeading->operator=(RotateVector180Degree(pvNewCurDir));
	}
	break;

	case NTL_MOVE_MOUSE_MOVEMENT:
	case NTL_MOVE_FOLLOW_MOVEMENT:
	{
		pvJumpHeading->operator=(*pvNewCurDir);
	}
	break;

	default: return false; break;
	}

	return true;
}

bool CMoveableObject::ConsumeDestLoc(std::list<sVECTOR3>& rlistConsumedDestLoc)
{
	std::list<sVECTOR3>::iterator it2;

	if (!m_listRemainingDestLoc.size())
		return true;

	CNtlVector vConsumedDestLoc;
	CNtlVector vRemainingDestLoc;

	for (std::list<sVECTOR3>::iterator it = rlistConsumedDestLoc.begin(); it != rlistConsumedDestLoc.end(); it++)
	{
		if (m_listRemainingDestLoc.size() == 0)
			return true;

		it2 = m_listRemainingDestLoc.begin();

		vConsumedDestLoc = *it;
		vRemainingDestLoc = *it2;

		if (vConsumedDestLoc != vRemainingDestLoc)
		{
			ERR_LOG(LOG_USER, "ConsumedDestLoc data is not equal to RemainingDestLoc., vConsumedDestLoc = (%f, %f, %f), vRemainingDestLoc = (%f, %f, %f)", vConsumedDestLoc.x, vConsumedDestLoc.y, vConsumedDestLoc.z, vRemainingDestLoc.x, vRemainingDestLoc.y, vRemainingDestLoc.z);
			return false;
		}

		m_listRemainingDestLoc.erase(it2);
	}

	return true;
}


void CMoveableObject::AdjustCurSpeed(float *fActualSpeed, float *fTurningSpeedRatio)
{
	if (IsLocInWorldBoundary(GetCurLoc(), m_pCurWorld))
	{
		*fActualSpeed = GetCurSpeed();

		// todo: check if player is swimming
		//*rfTurningSpeedRatio = 0.5f;
	}
}




