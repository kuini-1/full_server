//***********************************************************************************
//
//	File		:	NtlEvent.h
//
//	Begin		:	2005-11-30
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	Event ????ȭ ??????Ʈ Ŭ????
//
//***********************************************************************************

#ifndef __NTLEVENT_H__
#define __NTLEVENT_H__

#include "../../Shared/NtlSharedCommon.h"
#if defined(_WIN32)
#include <process.h>
#endif

class CNtlEvent
{
public:

	CNtlEvent();

	virtual ~CNtlEvent(void);


	//
	CNtlEvent(const CNtlEvent & ev);

	CNtlEvent & operator=(const CNtlEvent &ev);



	void			Reset();

	void			Signal();

	void			Wait();

	int				Wait( unsigned int millisecs );


private:

	HANDLE			m_hEvent;
};

#endif // __NTLEVENT_H__
