//***********************************************************************************
//
//	File		:	ModelToolCharDataTable.cpp
//
//	Begin		:	2007-03-08
//
//	Copyright	:	NTL-Inc Co., Ltd
//
//	Author		:	Hong Ho Dong   ( battery@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#include "StdAfx.h"
#include "ModelToolCharDataTable.h"
#include "NtlXMLDoc.h"
#include "NtlItem.h"
#include "NtlCharacter.h"
#include "NtlFileSerializer.h"


#define	XML_PROP_LIST_ELEMENT								"/CHARACTER_PROPERTY/CHARACTER_ELEMENT/ELEMENT"
#define XML_PROP_LIST_ELEMENT_NAME_ATTRIBUTE				"NAME"
	
#define XML_PROP_NAME_ATTRIBUTE								"/MODEL_SCRIPT/HEADER/NAME"
#define XML_PROP_ANIM_DATA_ELEMENT							"/MODEL_SCRIPT/ANIM_TABLE/ANIM_DATA"
//#define XML_PROP_ANIM_DATA_CHILD_ELEMENT					"ANIM_EVENT"

#define XML_PROP_ANIM_DATA_ELEMENT_KEYID_ATTRIBUTE			"KEYID"
#define XML_PROP_ANIM_DATA_ELEMENT_PLAYTIME_ATTRIBUTE		"PLAYTIME"
#define XML_PROP_ANIM_DATA_ELEMENT_EVENTID_ATTRIBUTE		"ID"
#define XML_PROP_ANIM_DATA_ELEMENT_HITTIME_ATTRIBUTE		"TIME"
#define XML_PROP_ANIM_DATA_ELEMENT_HITBEHAVIOR_ATTRIBUTE	"BEHAVIOR"
#define XML_PROP_ANIM_DATA_ELEMENT_HITKB2PUSH_ATTRIBUTE		"KB2PUSH"




int sMODELTOOL_CHAR_TBLDAT::GetRemapAnimKeyID( int nAnimKeyID)
{
	int nRemappingAnimKeyID = NTL_MAX_SIZE_ANIM_ALL;

	if( nAnimKeyID >= COMMON_ANIMATION_START && nAnimKeyID <= COMMON_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - COMMON_ANIMATION_START + NTL_ST_INDEX_ANIM_COMMON;
	}
	else if( nAnimKeyID >= BATTLE_ANIMATION_START && nAnimKeyID <= BATTLE_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - BATTLE_ANIMATION_START + NTL_ST_INDEX_ANIM_BATTLE;
	}
	else if( nAnimKeyID >= ATTACK_ANIMATION_START && nAnimKeyID <= ATTACK_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - ATTACK_ANIMATION_START + NTL_ST_INDEX_ANIM_ATTACK;
	}
	else if(nAnimKeyID >= SOCIAL_ANIMATION_START && nAnimKeyID <= SOCIAL_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - SOCIAL_ANIMATION_START + NTL_ST_INDEX_ANIM_SOCIAL;
	}
	else if(nAnimKeyID >= SKILL_ANIMATION_START && nAnimKeyID <= SKILL_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - SKILL_ANIMATION_START + NTL_ST_INDEX_ANIM_SKILL;
	}
	else if(nAnimKeyID >= HTB_ANIMATION_START && nAnimKeyID <= HTB_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - HTB_ANIMATION_START + NTL_ST_INDEX_ANIM_HTB;
	}
	else if(nAnimKeyID >= PC_TRIGGER_ANIMATION_START && nAnimKeyID <= PC_TRIGGER_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - PC_TRIGGER_ANIMATION_START + NTL_ST_INDEX_ANIM_TRIGGER;
	}
	else if(nAnimKeyID >= TRANSFORM_ANIMATION_START && nAnimKeyID <= TRANSFORM_ANIMATION_END)
	{
		nRemappingAnimKeyID = nAnimKeyID - TRANSFORM_ANIMATION_END + NTL_ST_INDEX_ANIM_TRANSFORM;
	}
    else if(nAnimKeyID >= VEHICLE_SRP1_ANIMATION_START && nAnimKeyID <= VEHICLE_SRP1_ANIMATION_END)
    {
        nRemappingAnimKeyID = nAnimKeyID - VEHICLE_SRP1_ANIMATION_END + NTL_ST_INDEX_ANIM_VEHICLE_SRP1;
    }
    else if(nAnimKeyID >= VEHICLE_SRP2_ANIMATION_START && nAnimKeyID <= VEHICLE_SRP2_ANIMATION_END)
    {
        nRemappingAnimKeyID = nAnimKeyID - VEHICLE_SRP2_ANIMATION_END + NTL_ST_INDEX_ANIM_VEHICLE_SRP2;
    }
    else if(nAnimKeyID >= ITEM_USE_ANIMATION_START && nAnimKeyID <= ITEM_USE_ANIMATION_END)
    {
        nRemappingAnimKeyID = nAnimKeyID - ITEM_USE_ANIMATION_END + NTL_ST_INDEX_ANIM_ITEM_USE;
    }
    
	
	return nRemappingAnimKeyID;
}

bool sMODELTOOL_CHAR_TBLDAT::Add(int nAnimKeyID, float fDurationTime, BYTE byPushCount, BYTE byKnockDownCount, BYTE byHitCount, float *pHitTime, bool bKB2Push /* = false */)
{
	int nRemapAnimKeyID = GetRemapAnimKeyID(nAnimKeyID);

	_ASSERT(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL);
	if( !(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL) )
		return false;

	m_aCharTbldat[nRemapAnimKeyID].fDurationTime	= fDurationTime;
	m_aCharTbldat[nRemapAnimKeyID].byPushCount		= byPushCount;
	m_aCharTbldat[nRemapAnimKeyID].byKnockDownCount	= byKnockDownCount;
	m_aCharTbldat[nRemapAnimKeyID].bDataLoaded		= true;
	m_aCharTbldat[nRemapAnimKeyID].byHitCount		= byHitCount;
    m_aCharTbldat[nRemapAnimKeyID].bKB2Push         = bKB2Push;
	
	memcpy(m_aCharTbldat[nRemapAnimKeyID].aHitTime, pHitTime, sizeof(float) * byHitCount);
	return true;
}

bool sMODELTOOL_CHAR_TBLDAT::AddSkillCancelTime( int nAnimKeyID, float fTime ) 
{
    int nRemapAnimKeyID = GetRemapAnimKeyID(nAnimKeyID);

    _ASSERT(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL);
    if( !(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL) )
        return false;
    
    m_aCharTbldat[nRemapAnimKeyID].fSkillCancelTime = fTime;

    return TRUE;
}

sCHAR_DATA_INFO *sMODELTOOL_CHAR_TBLDAT::GetChainAttack(BYTE byClass, BYTE byWeapon, BYTE byChainSequence)
{
	int nRemapAnimKeyID = 0;
	switch( byWeapon )
	{
		case ITEM_TYPE_UNKNOWN: 
			nRemapAnimKeyID = NTL_ST_INDEX_ANIM_ATTACK_DEF + byChainSequence - NTL_ST_CHAIN_SEQUENCE;
			break;

		case ITEM_TYPE_STAFF: 
			nRemapAnimKeyID = NTL_ST_INDEX_ANIM_ATTACK_STAFF + byChainSequence - NTL_ST_CHAIN_SEQUENCE;
			break;

		default:
			nRemapAnimKeyID = NTL_ST_INDEX_ANIM_ATTACK_DEF + byChainSequence - NTL_ST_CHAIN_SEQUENCE;
			break;
	}
	
	if(byClass == PC_CLASS_STREET_FIGHTER)
		nRemapAnimKeyID = NTL_ST_INDEX_ANIM_ATTACK_GLOVE + byChainSequence - NTL_ST_CHAIN_SEQUENCE;
	
	_ASSERT(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL);
	if( !(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL) )
		return NULL;

	return &m_aCharTbldat[nRemapAnimKeyID];
}

sCHAR_DATA_INFO *sMODELTOOL_CHAR_TBLDAT::GetAnim(BYTE byWeapon, ANIMATIONID AnimKeyID)
{
	UNREFERENCED_PARAMETER( byWeapon );

	int nRemapAnimKeyID = GetRemapAnimKeyID(AnimKeyID);

//	_ASSERT(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL);
	if( !(nRemapAnimKeyID >= 0 && nRemapAnimKeyID < NTL_MAX_SIZE_ANIM_ALL) )
		return NULL;

	return &m_aCharTbldat[nRemapAnimKeyID];
}



CModelToolCharDataTable::CModelToolCharDataTable()
{
}

CModelToolCharDataTable::~CModelToolCharDataTable()
{
	Destroy();
}

bool CModelToolCharDataTable::Create(const char *lpszPath, bool bIsSharedAccess, char *szCryptPassword)
{
	UNREFERENCED_PARAMETER(bIsSharedAccess);
	UNREFERENCED_PARAMETER(szCryptPassword);
	std::string strPath( lpszPath );

	//if(LoadPropertyList(lpszPath, "CharacterPropertyList.xml") == false)
	//	return false;

	//if(LoadPropertyList(lpszPath, "MobPropertyList.xml") == false)
	//	return false;

	//if(LoadPropertyList(lpszPath, "NPCPropertyList.xml") == false)
	//	return false;

	//if(LoadPropertyList(lpszPath, "VehiclePropertyList.xml") == false)
	//	return false;

	strPath += "\\characterproperty.rdf";
	if (!LoadPropertySerialize(strPath.c_str(), false, NULL))
	{
		return false;
	}

	return true;
}

bool CModelToolCharDataTable::LoadPropertyList(const char *lpszPath, const char * lpszFileName)
{
#if !defined(_WIN32)
	(void)lpszPath;
	(void)lpszFileName;
	return false;
#else
	CNtlXMLDoc doc;
	std::string strPath( lpszPath );
	std::string strFileName;
	
	if(doc.Create() == false)
	{
		_ASSERT(0);
		return false;
	}
	
	strFileName = strPath;
	strFileName += "/";
	strFileName += lpszFileName;

	if(doc.Load( const_cast<char *>(strFileName.c_str()) ) == false)
	{
		_ASSERT(0);
		return false;
	}
	
	

	IXMLDOMNodeList *pList = doc.SelectNodeList(XML_PROP_LIST_ELEMENT);
	IXMLDOMNode* pNode = NULL;
	char szBuffer[1024];

	long  lNum;
	pList->get_length(&lNum);
	for(long i = 0; i < lNum ; ++i)
	{
		pList->get_item(i,&pNode);

		if(!doc.GetTextWithAttributeName(pNode, XML_PROP_LIST_ELEMENT_NAME_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
		{
			_ASSERT(0);
			return false;
		}
		strFileName = strPath + "/" + szBuffer;
		
		if( LoadProperty( strFileName.c_str() ) == false )
		{
			_ASSERT(0);
			return false;
		}

		if(pNode)
		{
			pNode->Release(); 
			pNode = NULL;
		}
	}

	if(pList)
	{
		pList->Release(); 
		pList = NULL;
	}

	return true;
#endif // _WIN32
}

bool CModelToolCharDataTable::LoadProperty(const char * lpszFullPathFileName)
{
#if !defined(_WIN32)
	(void)lpszFullPathFileName;
	return false;
#else
	CNtlXMLDoc doc;
	if(doc.Create() == false)
	{
		_ASSERT(0);
		return false;
	}
	
	if(doc.Load( const_cast<char *>(lpszFullPathFileName) ) == false)
	{
		_ASSERT(0);
		return false;
	}
	
	char szBuffer[1024];

	if( !doc.GetDataWithXPath(const_cast<char *>(XML_PROP_NAME_ATTRIBUTE), szBuffer, sizeof(szBuffer)) )
	{
		_ASSERT(0);
		return false;
	}

	sMODELTOOL_CHAR_TBLDAT	*pModelToolCharData = new sMODELTOOL_CHAR_TBLDAT;

	//Add(szBuffer, pModelToolCharData);

	IXMLDOMNodeList *pAnimDataList = doc.SelectNodeList(XML_PROP_ANIM_DATA_ELEMENT);
	IXMLDOMNode* pAnimDataNode = NULL;

	IXMLDOMNodeList *pAnimEventList = NULL;
	IXMLDOMNode* pAnimEventNode = NULL;

	long	lAnimDataNodeNum = 0;
	long	lAnimEventNodeNum = 0;

	int		nAnimKeyID = 0;
	float	fDurationTime = 0.f;

	BYTE	byPushCount = 0;
	BYTE	byKnockDownCount = 0;
    bool    bKB2Push = false;
	BYTE	byHitCount = 0;
	float	aHitTime[NTL_MAX_SIZE_HIT];

	pAnimDataList->get_length(&lAnimDataNodeNum);
	for(long m = 0; m < lAnimDataNodeNum ; ++m)
	{
		pAnimDataList->get_item(m,&pAnimDataNode);

		if(!doc.GetTextWithAttributeName(pAnimDataNode, XML_PROP_ANIM_DATA_ELEMENT_KEYID_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
		{
			_ASSERT(0);
			return false;
		}
		nAnimKeyID = atoi(szBuffer);

		if(!doc.GetTextWithAttributeName(pAnimDataNode, XML_PROP_ANIM_DATA_ELEMENT_PLAYTIME_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
		{
			_ASSERT(0);
			return false;
		}

		fDurationTime = (float)atof(szBuffer);
		
		pAnimDataNode->selectNodes(L"ANIM_EVENT", &pAnimEventList);
		pAnimEventList->get_length(&lAnimEventNodeNum);

		memset(aHitTime, 0, sizeof(aHitTime));
		for(long n = 0; n < lAnimEventNodeNum; ++n)
		{
			pAnimEventList->get_item(n, &pAnimEventNode);

			if(!doc.GetTextWithAttributeName(pAnimEventNode, XML_PROP_ANIM_DATA_ELEMENT_EVENTID_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
			{
				_ASSERT(0);
				return false;
			}
	
			int nAnimEventID = EVENT_ANIM_NONE;
			int nBehavior = TARGET_BEHAVIOR_NONE;

			nAnimEventID = atoi(szBuffer);

			if(nAnimEventID == EVENT_ANIM_HIT)
			{
				if(!doc.GetTextWithAttributeName(pAnimEventNode, XML_PROP_ANIM_DATA_ELEMENT_HITBEHAVIOR_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
				{
					_ASSERT(0);
					return false;
				}
				
				nBehavior = atoi(szBuffer);

				if(nBehavior == TARGET_BEHAVIOR_PUSH)
				{
					byPushCount++;
				}
				else if(nBehavior == TARGET_BEHAVIOR_KNOCK_DOWN)
				{
					byKnockDownCount++;
					if( byKnockDownCount > 1 )
					{
						_ASSERT(0);
						return false;
					}
				}

				// Hit time
				if(!doc.GetTextWithAttributeName(pAnimEventNode, XML_PROP_ANIM_DATA_ELEMENT_HITTIME_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
				{
					_ASSERT(0);
					return false;
				}

				aHitTime[byHitCount] = (float)atof(szBuffer);
				byHitCount++;

				if(byHitCount > NTL_MAX_SIZE_HIT)
					_ASSERT(0);


				// KB2PUSH
				if(!doc.GetTextWithAttributeName(pAnimEventNode, XML_PROP_ANIM_DATA_ELEMENT_HITKB2PUSH_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
				{
					_ASSERT(0);
					return false;
				}

				if(atoi(szBuffer) == 0)
				{
					bKB2Push = false;
				}
				else
				{
					bKB2Push = true;
				}
			}
            else if(nAnimEventID == EVENT_ANIM_SKILL_CANCEL)
            {
                // time
                if(!doc.GetTextWithAttributeName(pAnimEventNode, XML_PROP_ANIM_DATA_ELEMENT_HITTIME_ATTRIBUTE, szBuffer, sizeof(szBuffer)))
                {
                    _ASSERT(0);
                    return false;
                }

                pModelToolCharData->AddSkillCancelTime(nAnimKeyID, (float)atof(szBuffer));
            }
			
			if(pAnimEventNode)
			{
				pAnimEventNode->Release();
				pAnimEventNode = NULL;
			}
		}

		if( nAnimKeyID != -1 )
			pModelToolCharData->Add(nAnimKeyID, fDurationTime, byPushCount, byKnockDownCount, byHitCount, aHitTime, bKB2Push);

		nAnimKeyID = 0;
		fDurationTime = 0;
		byPushCount = 0;
		byKnockDownCount = 0;
		byHitCount = 0;

		if(pAnimEventList)
		{
			pAnimEventList->Release();
			pAnimEventList = NULL;
		}
		
		if(pAnimDataNode)
		{
			pAnimDataNode->Release(); 
			pAnimDataNode = NULL;
		}
	}

	//pModelToolCharData->GetChainAttack(PC_CLASS_HUMAN_FIGHTER, ITEM_TYPE_UNKNOWN, 6);
	if(pAnimDataList)
	{
		pAnimDataList->Release(); 
		pAnimDataList = NULL;
	}

	char *pTokenStart = NULL;
	char *pTokenEnd = NULL;
	char szToken[64];

	if( !doc.GetDataWithXPath("MODEL_SCRIPT/HEADER/ANIM_BBOX", szBuffer, sizeof(szBuffer)) )
	{
		_ASSERT(0);
		return false;
	}
	
	CNtlVector vTemp;
	pTokenStart = szBuffer;
	pTokenEnd = strstr( szBuffer, ";");
	strncpy_s(szToken, sizeof(szToken), pTokenStart, pTokenEnd - pTokenStart);
	vTemp.x = (float) atof(szToken);

	pTokenStart = pTokenEnd + 1;
	pTokenEnd = strstr( pTokenStart, ";");
	strncpy_s(szToken, sizeof(szToken), pTokenStart, pTokenEnd - pTokenStart);
	vTemp.y = (float) atof(szToken);

	pTokenStart = pTokenEnd + 1;
	strncpy_s(szToken, sizeof(szToken), pTokenStart, strlen(szBuffer));
	vTemp.z = (float) atof(szToken);

	pModelToolCharData->m_vMin.x = -(vTemp.x / 2.0f);
	pModelToolCharData->m_vMin.y = 0.0f;
	pModelToolCharData->m_vMin.z = -(vTemp.z / 2.0f);
	
	pModelToolCharData->m_vMax.x = vTemp.x / 2.0f;
	pModelToolCharData->m_vMax.y = vTemp.y;
	pModelToolCharData->m_vMax.z = vTemp.z / 2.0f;

	if( !doc.GetDataWithXPath("MODEL_SCRIPT/BONE_DATA/BASE_SCALE", szBuffer, sizeof(szBuffer)) )
	{
		_ASSERT(0);
		return false;
	}

	pModelToolCharData->m_fScale = (float) atof(szBuffer);
	
	return true;
#endif // _WIN32
}

void CModelToolCharDataTable::Destroy()
{
	TABLEIT stTableIT  = m_mapTableList.begin();
	TABLEIT endTableIT = m_mapTableList.end();
	for(;stTableIT != endTableIT;)
	{
		sMODELTOOL_CHAR_TBLDAT *pModelToolCharData = stTableIT->second;
		if(pModelToolCharData != NULL)
		{
			delete pModelToolCharData;
			pModelToolCharData = NULL;
		}
		++stTableIT;
	}
	m_mapTableList.clear();
}

bool CModelToolCharDataTable::Add(std::string &strModelName, sMODELTOOL_CHAR_TBLDAT *pModelToolCharData)
{
	//printf("Add Model: [%s]\n", strModelName.c_str());
	if (m_mapTableList.find(strModelName) != m_mapTableList.end())
	{
		printf("model already added \n");
		_ASSERT(0);
		return false;
	}
	
	
	//m_mapTableList.insert(TABLEVAL(strModelName, pModelToolCharData));
	//m_mapTableList.insert(std::make_pair(strModelName, pModelToolCharData));
	m_mapTableList[strModelName] = pModelToolCharData;
	return true;
}


bool CModelToolCharDataTable::LoadPropertySerialize(const char * lpszFullPathFileName, bool bEncrypt, char *szCryptPassword)
{
	UNREFERENCED_PARAMETER(bEncrypt);
	UNREFERENCED_PARAMETER(szCryptPassword);

	CNtlFileSerializer nsl;
	if (nsl.LoadFile(const_cast<char *>(lpszFullPathFileName), false))
	{
		unsigned int uiVer;
		std::string strCharacterDataPath;
		unsigned int uiPropertySize;

		nsl.operator>>(uiVer);
		nsl.operator>>(strCharacterDataPath);
		nsl.operator>>(uiPropertySize);

		for (unsigned int j = 0; j < uiPropertySize; j++)
			LoadCharacterProperty(&nsl);
	}

	return true;
}

bool CModelToolCharDataTable::LoadCharacterProperty(CNtlFileSerializer *nsl)
{
	std::string strName;
	bool bBaseScale;
	bool bBoneScale;
	float fInitRunFrontAnimSpeed;
	std::string strTypeMeshFileName;
	std::string strMeshPath;
	unsigned int uiExtraBBoxSize;
	unsigned int uiLinkEffectSize;

	nsl->operator>>(strName);
	sMODELTOOL_CHAR_TBLDAT* pModelToolCharData = new sMODELTOOL_CHAR_TBLDAT;

	if (this->Add(strName, pModelToolCharData))
	{
		nsl->operator>>(bBaseScale);
		nsl->operator>>(bBoneScale);
		nsl->operator>>(fInitRunFrontAnimSpeed);
		nsl->operator>>(strTypeMeshFileName);
		nsl->operator>>(strMeshPath);
		nsl->operator>>(uiExtraBBoxSize);
		nsl->operator>>(uiLinkEffectSize);

		//if (strName.compare("A_HFI_M_CH") == 0)
		//	printf("LoadCharacterProperty: [%s] [%s] [%s] [%u] [%u] %f %i %i\n", strName.c_str(), strTypeMeshFileName.c_str(), strMeshPath.c_str(), uiExtraBBoxSize, uiLinkEffectSize, fInitRunFrontAnimSpeed, bBaseScale, bBoneScale);

		if (uiLinkEffectSize > 0)
		{
			SEventLinkEffect* pEventLinkEffect = new SEventLinkEffect;
			for (UINT i = 0; i < uiLinkEffectSize; i++)
				nsl->Out(pEventLinkEffect, sizeof(SEventLinkEffect));

			delete pEventLinkEffect;
		}

		LoadAnimTable(nsl, pModelToolCharData);

		sVECTOR3 vFaceCameraPos;
		sVECTOR3 vBaseScale;
		sVECTOR3 vPosOffset;
		BOOL bIdk;
		BOOL nIdk2;
		int nIdk3;

		nsl->operator>>(bIdk);
		nsl->operator>>(nIdk2);
		nsl->operator>>(nIdk3);
		nsl->Out(&vFaceCameraPos, sizeof(sVECTOR3));

		if (bIdk == TRUE)
		{
		//	printf("nIdk3 %i \n", nIdk3);

			BYTE* sv = new BYTE;
			for (int i = 0; i < nIdk3*20; i++)
				nsl->Out(sv, sizeof(BYTE));
			delete sv;
		}

		sVECTOR3 vScale;
		vScale.x = 1.0f;
		if (nIdk2 == TRUE)
		{
			nsl->Out(&vScale, sizeof(sVECTOR3));
		}

		pModelToolCharData->m_fScale = vScale.x;

		nsl->Out(&vPosOffset, sizeof(sVECTOR3));
		pModelToolCharData->m_vMax.x = vPosOffset.x;
		pModelToolCharData->m_vMax.y = vPosOffset.y;
		pModelToolCharData->m_vMax.z = vPosOffset.z;
		//	if (strName.compare("A_HFI_M_CH") == 0)
		//	printf("strName %s m_vMax %f %f %f\n", strName.c_str(), vPosOffset.x, vPosOffset.y, vPosOffset.z);

		nsl->Out(&vPosOffset, sizeof(sVECTOR3));
		pModelToolCharData->m_vMin.x = vPosOffset.x;
		pModelToolCharData->m_vMin.y = 0.0f;
		pModelToolCharData->m_vMin.z = vPosOffset.z;
	//	if (strName.compare("A_HFI_M_CH") == 0)
	//	printf("strName %s m_vMin %f %f %f\n", strName.c_str(), vPosOffset.x, vPosOffset.y, vPosOffset.z);

		int nIddkk = 0;
		nsl->operator>>(nIddkk);
		for (int x = 0; x < nIddkk; x++)
		{
			UINT32 fsdfsdf;
			nsl->operator>>(fsdfsdf);
			nsl->Out(&vBaseScale, sizeof(sVECTOR3));
			nsl->Out(&vBaseScale, sizeof(sVECTOR3));
		}

		nsl->Out(&vBaseScale, sizeof(sVECTOR3));
		nsl->Out(&vBaseScale, sizeof(sVECTOR3));

		float isfsafsadk;
		nsl->operator>>(isfsafsadk);
		nsl->operator>>(isfsafsadk);
	}
	else
	{
		delete pModelToolCharData;
		return false;
	}

	return true;
}

bool CModelToolCharDataTable::LoadAnimTable(CNtlFileSerializer *nsl, sMODELTOOL_CHAR_TBLDAT *pModelToolCharData)
{
	std::string strAnimPath;
	int nAnimDataCount = 0;

	nsl->operator>>(strAnimPath);
	nsl->operator>>(nAnimDataCount);

	//printf("LoadAnimTable: [%s]|[%u]\n", strAnimPath.c_str(), nAnimDataCount);

	for (int i = 0; i < nAnimDataCount; i++)
	{
		std::string strAnimName;
		unsigned int uiAnimKey;
		float fPlayTime;
		unsigned int flagAnim;
		int nAnimEventSize;

		nsl->operator>>(strAnimName);
		nsl->operator>>(uiAnimKey);
		nsl->operator>>(fPlayTime);
		nsl->operator>>(flagAnim);
		nsl->operator>>(nAnimEventSize);

		BYTE byPushCount = 0;
		BYTE byKnockDownCount = 0;
		BYTE byHitCount = 0;
		float pHitTime[NTL_MAX_SIZE_HIT];
		bool bKB2Push = false;

	//	printf("AnimData: [%s]|[%u] |[%f]|[%u]|[%u]\n", strAnimName.c_str(), uiAnimKey, fPlayTime, flagAnim, nAnimEventSize);

	//	if (nAnimEventSize > 25)
	//		break;

		for (int ii = 0; ii < nAnimEventSize; ii++)
		{
			int nEventID = 0;
			nsl->operator>>(nEventID);
		//	printf("nEventID %u ", nEventID);
			switch (nEventID)
			{
				case 2:
				{
					SEventAnimHit* pEventAnimHit = new SEventAnimHit;
					LoadAnimHitEvent(pEventAnimHit, nsl);

					if (pEventAnimHit->eTargetBehavior == TARGET_BEHAVIOR_PUSH)
						byPushCount++;
					else if (pEventAnimHit->eTargetBehavior == TARGET_BEHAVIOR_KNOCK_DOWN)
						byKnockDownCount++;

					pHitTime[byHitCount++] = pEventAnimHit->fTime;
					
					bKB2Push = pEventAnimHit->bKB2Push != 0;

					delete pEventAnimHit;
				}
				break;
				case 3:
				{
					SEventVisualEffect* pEventAnim = new SEventVisualEffect;
					nsl->Out(pEventAnim, sizeof(SEventVisualEffect));

					delete pEventAnim;
				}
				break;
				case 4:
				{
					SEventSound* pEventAnim = new SEventSound;
					nsl->Out(pEventAnim, sizeof(SEventSound));

					delete pEventAnim;
				}
				break;
				case 5:
				{
					SEventFootStep* pEventAnim = new SEventFootStep;
					nsl->Out(pEventAnim, sizeof(SEventFootStep));

					delete pEventAnim;
				}
				break;
				case 6:
				{
					SEventLinkEffect* pEventAnim = new SEventLinkEffect;
					nsl->Out(pEventAnim, sizeof(SEventLinkEffect));

					delete pEventAnim;
				}
				break;
				case 7:
				{
					SEventWeightTime* pEventAnim = new SEventWeightTime;
					nsl->Out(pEventAnim, sizeof(SEventWeightTime));

					delete pEventAnim;
				}
				break;
				case 8:
				{
					SEventTrace* pEventAnim = new SEventTrace;
					nsl->Out(pEventAnim, sizeof(SEventTrace));

					delete pEventAnim;
				}
				break;
				case 9:
				{
					SEventSubWeapon* pEventAnim = new SEventSubWeapon;
					nsl->Out(pEventAnim, sizeof(SEventSubWeapon));

					delete pEventAnim;
				}
				break;
				case 10:
				{
					SEventPostEffect* pEventAnim = new SEventPostEffect;
					nsl->Out(pEventAnim, sizeof(SEventPostEffect));

					delete pEventAnim;
				}
				break;
				case 11:
				{
					SEventSummonPet* pEventAnim = new SEventSummonPet;
					nsl->Out(pEventAnim, sizeof(SEventSummonPet));

					delete pEventAnim;
				}
				break;
				case 12:
				{
					SEventAnimCinematic* pEventAnim = new SEventAnimCinematic;
					nsl->Out(pEventAnim, sizeof(SEventAnimCinematic));

					delete pEventAnim;
				}
				break;
				case 13:
				{
					SEventAlpha* pEventAnim = new SEventAlpha;
					nsl->Out(pEventAnim, sizeof(SEventAlpha));

					delete pEventAnim;
				}
				break;
				case 14:
				{
					SEventExplosion* pEventAnim = new SEventExplosion;
					nsl->Out(pEventAnim, sizeof(SEventExplosion));

					delete pEventAnim;
				}
				break;
				case 15:
				{
					SEventDirect* pEventAnim = new SEventDirect;
					nsl->Out(pEventAnim, sizeof(SEventDirect));

					delete pEventAnim;
				}
				break;
				case 16:
				{
					SEventColorChange* pEventAnim = new SEventColorChange;
					nsl->Out(pEventAnim, sizeof(SEventColorChange));

					delete pEventAnim;
				}
				break;
				case 17:
				{
					SEventStretch* pEventAnim = new SEventStretch;
					nsl->Out(pEventAnim, sizeof(SEventStretch));

					delete pEventAnim;
				}
				break;
				case 18:
				{
					SEventTrigger* pEventAnim = new SEventTrigger;
					nsl->Out(pEventAnim, sizeof(SEventTrigger));

					delete pEventAnim;
				}
				break;
				case 19:
				{
					SEventSkillCancel* pEventAnim = new SEventSkillCancel;
					nsl->Out(pEventAnim, sizeof(SEventSkillCancel));
					pModelToolCharData->AddSkillCancelTime(uiAnimKey, pEventAnim->fTime);

					delete pEventAnim;
				}
				break;
			
				default: printf("LoadAnimTable: nEventID NOT FOUND %u \n", nEventID);
			}

		}

		if (uiAnimKey != INVALID_TBLIDX)
			pModelToolCharData->Add(uiAnimKey, fPlayTime, byPushCount, byKnockDownCount, byHitCount, pHitTime, bKB2Push);
	}

	return true;
}

void CModelToolCharDataTable::LoadAnimHitEvent(SEventAnimHit *pEventAnimHit, CNtlFileSerializer *sIn)
{
	sIn->operator>>(pEventAnimHit->fTime);
	sIn->operator>>((int&)pEventAnimHit->eEventID);
	sIn->operator>>(pEventAnimHit->uiDamage);
	sIn->operator>>(pEventAnimHit->bPowerEffect);
	sIn->operator>>((int&)pEventAnimHit->eAttackType);
	sIn->operator>>((int&)pEventAnimHit->eTargetBehavior);
	sIn->operator>>(pEventAnimHit->bKB2Push);
	sIn->operator>>((int&)pEventAnimHit->eHandType);
	sIn->operator>>((int&)pEventAnimHit->uiProjectileEffectType);
	sIn->operator>>((int&)pEventAnimHit->eProjectileShotType);
	sIn->operator>>(pEventAnimHit->nSubWeaponFlag);
	sIn->Out(pEventAnimHit->chBoneName, 32);
	sIn->Out(pEventAnimHit->chProjectileEffectName, 32);
	sIn->operator>>(pEventAnimHit->fProjectileSpeed);
	sIn->operator>>(pEventAnimHit->bTargetAttach);
	sIn->operator>>(pEventAnimHit->fTargetHeight);
	sIn->Out(pEventAnimHit->chTargetEffectName, 32);
	sIn->operator>>( (int&)pEventAnimHit->eTargetEffectType );
	sIn->Out(pEventAnimHit->chSubTargetEffect, 32);
	sIn->Out(pEventAnimHit->chTargetSoundName, 32);
	sIn->operator>>(pEventAnimHit->eSoundType);
	sIn->operator>>(pEventAnimHit->bHitSoundEcho);
	sIn->operator>>(pEventAnimHit->bCameraShake);
	sIn->Out(pEventAnimHit->chWordEffect, 32);

	switch (pEventAnimHit->uiProjectileEffectType)
	{
		case 2002:
		{
			sIn->operator>>(pEventAnimHit->uEffectTypeExtraData.hissidanData.bApplyAngle);
			sIn->Out(&pEventAnimHit->uEffectTypeExtraData.hissidanData.v2dAngle, sizeof(sVECTOR2));
		}
		break;
		case 2003:
		{
			sIn->Out(&pEventAnimHit->uEffectTypeExtraData.hellZoneData.vTargetEffectOffset, sizeof(sVECTOR3));
			sIn->operator>>(pEventAnimHit->uEffectTypeExtraData.hellZoneData.fTargetEffectStartWaitTime);
			sIn->operator>>(pEventAnimHit->uEffectTypeExtraData.hellZoneData.fTargetEffectSpeed);
		}
		break;
		case 2004:
		{
			sIn->operator>>(pEventAnimHit->uEffectTypeExtraData.multiHissidanData.nCount);
			for (int i = 0; i < pEventAnimHit->uEffectTypeExtraData.multiHissidanData.nCount; i++)
			{
				sVECTOR2 x;
				sIn->Out(&x, sizeof(sVECTOR2));
			}
		}
		break;

		default: break;
	}

	if (pEventAnimHit->bCameraShake == TRUE)
	{
		sIn->operator>>(pEventAnimHit->fCameraShakeFactor);
		sIn->operator>>(pEventAnimHit->fCameraShakeMaxHeight);
	}
}


sMODELTOOL_CHAR_TBLDAT *CModelToolCharDataTable::FindData(const char* strModelName)
{
	if(m_mapTableList.find( strModelName ) == m_mapTableList.end())
		return NULL;

	return m_mapTableList[strModelName];
}



//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
DWORD sMODELTOOL_CHAR_TBLDAT::GetAnimationLength(ANIMATIONID skillAnimationId)
{
	sCHAR_DATA_INFO * pCharAnimData = GetAnim( ITEM_TYPE_UNKNOWN, skillAnimationId );
	if( NULL == pCharAnimData )
	{
		return 0;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return 0;
	}

	return (DWORD) ( 1000.0f * pCharAnimData->fDurationTime );
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
DWORD sMODELTOOL_CHAR_TBLDAT::GetChainAttackDurationTime(BYTE byClass, BYTE byWeapon, BYTE byChainSequence)
{
	sCHAR_DATA_INFO * pCharAnimData = GetChainAttack( byClass, byWeapon, byChainSequence );
	if( NULL == pCharAnimData )
	{
		return 0;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return 0;
	}

	return (DWORD) ( 1000.0f * pCharAnimData->fDurationTime );
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
BYTE sMODELTOOL_CHAR_TBLDAT::GetChainAttackPushCount(BYTE byClass, BYTE byWeapon, BYTE byChainSequence)
{
	sCHAR_DATA_INFO * pCharAnimData = GetChainAttack( byClass, byWeapon, byChainSequence );
	if( NULL == pCharAnimData )
	{
		return 0;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return 0;
	}

	return pCharAnimData->byPushCount;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool sMODELTOOL_CHAR_TBLDAT::GetChainAttackKnockDown(BYTE byClass, BYTE byWeapon, BYTE byChainSequence)
{
	sCHAR_DATA_INFO * pCharAnimData = GetChainAttack( byClass, byWeapon, byChainSequence );
	if( NULL == pCharAnimData )
	{
		return false;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return false;
	}

	return pCharAnimData->byKnockDownCount ? true : false;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool sMODELTOOL_CHAR_TBLDAT::GetSkillKnockDown(ANIMATIONID skillAnimationId)
{
	sCHAR_DATA_INFO * pCharAnimData = GetAnim( ITEM_TYPE_UNKNOWN, skillAnimationId );
	if( NULL == pCharAnimData )
	{
		return false;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return false;
	}

	return pCharAnimData->byKnockDownCount ? true : false;
}


bool sMODELTOOL_CHAR_TBLDAT::GetSkillKB2Push( ANIMATIONID skillAnimationId ) 
{
    sCHAR_DATA_INFO * pCharAnimData = GetAnim( ITEM_TYPE_UNKNOWN, skillAnimationId );
    if( NULL == pCharAnimData )
    {
        return false;
    }
    if (false == pCharAnimData->bDataLoaded)
    {
        return false;
    }

    return pCharAnimData->bKB2Push;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
DWORD sMODELTOOL_CHAR_TBLDAT::GetSkillAnimationLength(ANIMATIONID skillAnimationId)
{
	return GetAnimationLength( skillAnimationId );
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
BYTE sMODELTOOL_CHAR_TBLDAT::GetSkillAnimationPushCount(ANIMATIONID skillAnimationId)
{
	sCHAR_DATA_INFO * pCharAnimData = GetAnim( ITEM_TYPE_UNKNOWN, skillAnimationId );
	if( NULL == pCharAnimData )
	{
		return 0;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return 0;
	}

	return pCharAnimData->byPushCount;
}


//-----------------------------------------------------------------------------------
//		Purpose	: Animation Hit Time.                
//		Return	: Hit Time (Skill Hit)
//-----------------------------------------------------------------------------------
DWORD sMODELTOOL_CHAR_TBLDAT::GetSkillAnimationFirstHitTime(ANIMATIONID skillAnimationId)
{
	sCHAR_DATA_INFO * pCharAnimData = GetAnim( ITEM_TYPE_UNKNOWN, skillAnimationId );
	if( NULL == pCharAnimData )
	{
		return 0;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return 0;
	}

	return (DWORD) ( 1000.0f * pCharAnimData->aHitTime[0] );
}


//-----------------------------------------------------------------------------------
//		Purpose	: Animation Hit Time.                
//		Return	: Hit Time (Skill Hit)
//-----------------------------------------------------------------------------------
DWORD sMODELTOOL_CHAR_TBLDAT::GetSkillAnimationSkillCancelTime(ANIMATIONID skillAnimationId)
{
	sCHAR_DATA_INFO * pCharAnimData = GetAnim( ITEM_TYPE_UNKNOWN, skillAnimationId );
	if( NULL == pCharAnimData )
	{
		return 0;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return 0;
	}

	if (0 == pCharAnimData->byHitCount)
	{
		return 0;
	}

	if (NTL_MAX_SIZE_HIT < pCharAnimData->byHitCount)
	{
		_ASSERT(0);
		return 0;
	}

	return (DWORD) ( 1000.0f * pCharAnimData->fSkillCancelTime );
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
DWORD sMODELTOOL_CHAR_TBLDAT::GetNpcAttackDurationTime(BYTE byAttackType)
{
	sCHAR_DATA_INFO * pCharAnimData = GetAnim( ITEM_TYPE_UNKNOWN, (ANIMATIONID) ( byAttackType + ATTACK_ANIMATION_START ) ); // �ӽ�
	if( NULL == pCharAnimData )
	{
		return 0;
	}
	if (false == pCharAnimData->bDataLoaded)
	{
		return 0;
	}

	return (DWORD) ( 1000.0f * pCharAnimData->fDurationTime );
}

