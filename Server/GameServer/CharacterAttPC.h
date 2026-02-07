#ifndef __INC_DBOG_CHARATTRIBUTE_PC_H__
#define __INC_DBOG_CHARATTRIBUTE_PC_H__


#include "CharacterAtt.h"

class CPlayer;

class CCharacterAttPC : public CCharacterAtt
{

public:
	CCharacterAttPC();
	virtual	~CCharacterAttPC();

public:

	bool				Create(CPlayer* pPlayer);

	virtual void		Reset();

	virtual void		CalculateBaseAtt();
	virtual void		CalculateLastAtt();

public:

	virtual void		CalculateNewItems();
	virtual void		ApplySubBuff(int usebuff);

	virtual void		CalculateBaseStr(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateBaseCon(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateBaseFoc(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateBaseDex(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateBaseSol(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateBaseEng(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void		CalculateLastStr(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateLastCon(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateLastFoc(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateLastDex(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateLastSol(float fValue, BYTE byApplyType, bool bIsPlus);
	virtual void		CalculateLastEng(float fValue, BYTE byApplyType, bool bIsPlus);

	virtual void		CalculateBattleAttribute(float fValue, BYTE byApplyType, bool bIsPlus);


private:

	void				CalcSecondWeaponOffence();


private:

	CPlayer*			m_pPlayerRef;

};

#endif