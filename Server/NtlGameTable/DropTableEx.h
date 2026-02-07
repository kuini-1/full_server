#pragma once

#include <list>
#include <vector>
#include "TableTmp.h"

class CItemTable;

class CDropTableEx
{
public:

#pragma pack(push, 4)
	struct sTBLDAT_DropItem : public sTBLDAT
	{

	public:

		TBLIDX			m_idxBag;
		TBLIDX			m_idxItem;
		TBLIDX			m_idxItemMin;
		TBLIDX			m_idxItemMax;
		BYTE			m_byLevelMin;
		BYTE			m_byLevelMax;
		DWORD			m_dwProb1;
		TBLIDX			m_idxEnchantRate;
		DWORD			m_dwInterval;

	public:

		virtual int GetDataSize()
		{
			return sizeof(*this) - sizeof(void*);
		}
	};

	struct sTBLDAT_DropBag: public sTBLDAT
	{

	public:

		TBLIDX			m_idxBigBag;
		TBLIDX			m_idxBag;
		BYTE			m_byGroup;
		DWORD			m_dwProb1;
		WORD			m_wTryCount;
		DWORD			m_dwInterval;

	public:

		virtual int GetDataSize()
		{
			return sizeof(*this) - sizeof(void*);
		}
	};
#pragma pack(pop)


public:

	struct DropInfo
	{
		DropInfo() : idxItem(0), idxEnchant(0) {}

		TBLIDX		idxItem;
		TBLIDX		idxEnchant;
	};

	typedef std::list<DropInfo>			LstDropInfo;


public:

	class CNorBag
	{
	public:
		CNorBag(TBLIDX idx) : m_idx(idx), m_dwTimeDroped(0) {}
		virtual ~CNorBag() {}

	private:

		TBLIDX									m_idx;
		DWORD									m_dwTimeDroped;

	public:

		std::vector<sTBLDAT_DropItem *>			m_vecDroper;

		int										GetDrops(CItemTable *pItemTable, LstDropInfo & lstDropInfo);
	};


public:

	class CGrpBag
	{
	public:
		CGrpBag(TBLIDX idx) : m_idx(idx), m_byType(0), m_nTryCount(0) {}
		virtual ~CGrpBag() {}

	private:

		TBLIDX									m_idx;
		BYTE									m_byType;
		int										m_nTryCount;
		std::vector<sTBLDAT_DropBag *>			m_vecDroper;

		struct sGROUP_INFO
		{
			BYTE		byGroupNo;
			int			nTryCount;
		};

		std::map<BYTE, sGROUP_INFO>				m_mapGroupNo;
		std::multimap<BYTE, sTBLDAT_DropBag *>	m_mmapBagByGroupNo;

	public:

		int										Push(sTBLDAT_DropBag* pTBLDAT_DropBag);
		int										GetDrops(CItemTable *pItemTable, CDropTableEx *pTable, LstDropInfo & lstDropInfo);
		int										GetDropsFromGrpBag_Unq(CItemTable *pItemTable, CDropTableEx *pTable, LstDropInfo & lstDropInfo);

	protected:

		int										GetDropsHelper(CItemTable *pItemTable, CDropTableEx *pTable, LstDropInfo & lstDropInfo, std::multimap<BYTE, sTBLDAT_DropBag *>::iterator & iterLower, std::multimap<BYTE, sTBLDAT_DropBag *>::iterator & iterUpper);

	};



private:

	std::map<TBLIDX, CNorBag*>		m_mapNorBag;
	std::map<TBLIDX, CGrpBag*>		m_mapGrpBag;


public:

	CDropTableEx() {}
	virtual ~CDropTableEx() {}

public:

	void				Init(CTableTmp<sTBLDAT_DropItem> * pDropItem, CTableTmp<sTBLDAT_DropBag> * pDropBag);

private:

	void				Init(CTableTmp<sTBLDAT_DropItem> * pDropItem);
	void				Init(CTableTmp<sTBLDAT_DropBag> * pDropBag);

	void				ClearNorBag();
	void				ClearGrpBag();

	int					Add(sTBLDAT_DropItem * pObj);
	int					Add(sTBLDAT_DropBag * pObj);

public:

	void				Clear();

	int					GetDrops(CItemTable * pItemTable, TBLIDX idxBag, LstDropInfo & lstDropInfo);

	bool				HasAnyDropBag(TBLIDX tblidx);
	bool				HasAnyDropBigBag(TBLIDX tblidx);

	static int			IsDrop(DWORD & rdwRand, DWORD dwProb);

	CNorBag*			FindDropBag(TBLIDX idx);
	CGrpBag*			FindDropBigBag(TBLIDX idx);

};