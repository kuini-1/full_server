//***********************************************************************************
//
//	File		:	NtlThread.h
//
//	Begin		:	2005-11-30
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	NTL Thread Class
//
//***********************************************************************************

#ifndef __NTLTHREAD_H__
#define __NTLTHREAD_H__

#define LOKI_CLASS_LEVEL_THREADING
#include "Singleton.h"

#include "NtlLinkList.h"
#include "NtlString.h"
#include "NtlEvent.h"
#include "NtlMutex.h"


class CThreadKey;
class CThreadHelper;
class CNtlThread;

//---------------------------------------------------------------------------------------
// CNtlRunObject : ?????? ???? ??ü ?????
//---------------------------------------------------------------------------------------
class CNtlRunObject
{
friend class CNtlThread;

public:

	CNtlRunObject()
		:m_arg(0), m_pOwner(0), m_bRunning(true) {}

	virtual ~CNtlRunObject() {}


public:

	// ?????? ???????? ?? ????? Override??? Application Logic?? ???
	virtual void			Run() {}

	//
	virtual void			Close() { m_bRunning = false; }


public:

	// ?????? ??? ???? ?? ?????? ???? ????? ????
	void					Terminate();

	//
	void *					GetArg() const { return m_arg; }

	//
	void 					SetArg(void * arg) { m_arg = arg; }

	// ???? ?????? ???
	CNtlThread *			GetThread() const { return m_pOwner; }

	// unconditional wait
	void					Wait();

	// conditional wait
	int						Wait(unsigned int millisecs);

	// ???? ?????? ????
	void					Exit();

	// ???? ???????? ????? ??????
	const char *			GetName() const;

	//
	bool					IsRunnable() { return m_bRunning; }

	//
	void					SetRunnable(bool bRunning) { m_bRunning = bRunning; }


private:

	// ???? ??????
	void					SetThread(CNtlThread * pOwner) { m_pOwner = pOwner; }


protected:

	void *					m_arg;

private:

	CNtlThread *			m_pOwner;

	bool					m_bRunning;

};


//---------------------------------------------------------------------------------------
// CNtlThread : ?????? ??ü ?????
//---------------------------------------------------------------------------------------
class CNtlThread : public CNtlLinkObject
{
friend class CThreadkey;
friend class CThreadHelper;
friend class CNtlThreadFactory;

public:

	// ?????? ????
	enum STATUS
	{
		eSTATUS_NOT_RUNNING = 0,	// ???????? ??? ???? ( ??? ???? )
		eSTATUS_PREPARING_TO_RUN,	// ???? ??? ????
		eSTATUS_RUNNING,			// ???? ????
		eSTATUS_PAUSED,				// ???? ????
		eSTATUS_DEAD,				// ???? ????

		MAX_STATUS,
	};


public:

	// AutoDelete?? ??? RunObject?? ???? Heap?? ??????? ????? ?????? ??. 
	CNtlThread(CNtlRunObject * pRunObject, const char * name = "Unknown Thread", bool bAutoDelete = false);

	virtual ~CNtlThread(void);


private:

	CNtlThread(const CNtlThread &);

	void operator=(const CNtlThread &);


public:

	// Thread?? ??????????? ?????? ????? ??? ???
	void					Close();

public:

	// Thread ????
	void					CleanUp();

	// ???? Thread Loop???? ????? ???, ?????????? Runnable?? run?? ??????
	void					Execute();


public:

	// ????
	void					Init();

	// Get Next Thread ( in Current Linked List )
	CNtlThread *			GetNextThread() const { return (CNtlThread*) this->GetNext(); }

	// conditional wait
	void					Wait();

	// unconditional wait
	int						Wait(unsigned int millisecs);

	// Thread?? ????????
	void					Exit();

	// ?????? Thread?? ?????????? ???????
	void					Join();

	// ?????? ??????? ???? ???????? ???????. ( ???? Thread ???? )
	void					Start();

	// Thread??? ???
	const char *			GetName() { return m_strName.c_str(); }

	// Thread?? ????? Runnable ??ü ???
	CNtlRunObject *			GetRunObject() { return m_pRunObject; }

	// attribute ????
	void					SetArg(void *arg) { m_pRunObject->SetArg(arg); }

	// attribute ???
	void *					GetArg() { return m_pRunObject->GetArg(); }

	//
	bool					IsAutoDelete() { return m_bAutoDelete; }

	// ????ü?
	bool					IsStatus(STATUS status) { return status == m_status; }

	//
	const char *			GetStatusString();

	//
	void					SetSignaled(bool bSignaled) { m_bSignaled = bSignaled; }

	//
	bool					IsSignaled() { return m_bSignaled; }




	// ????? Thread?? ?ñ???? ?????? ( static )
	static void				Notify(CNtlThread * pThread);

	// ???? Thread?? ?????? ( static )
	static CNtlThread *		GetCurrentThread();

	// Main Thread?? ??????
	static CNtlThread *		GetMainThread() { return m_pMainThread; }

	//
	static void				UnitTest();
	

protected:


	CNtlString				m_strName;

	STATUS					m_status;


	HANDLE					m_hThread;

	unsigned				m_threadID;

	CNtlEvent				m_event;

	bool					m_bSignaled;

	bool					m_bAutoDelete;

	CNtlRunObject *			m_pRunObject;


protected:

	static CThreadKey *		m_pThreadKey;

	static CNtlThread *		m_pMainThread;
};




//---------------------------------------------------------------------------------------
// NtlThreadFactory : ?????? ???? ?? ??? ó???? ?????? Factory ?????
//---------------------------------------------------------------------------------------
class CNtlThreadFactory
{
friend class CNtlThread;
friend class CThreadHelper;	// ????? ?????
friend class CNtlRunObject;

public:

	CNtlThreadFactory();

	virtual ~CNtlThreadFactory();


public:


	void					GarbageCollect(bool bShutDown = false);

	void					SingleGarbageCollect(CNtlThread* pGarbageThread);

	//close


public:
	
	void					Shutdown();

	void					CloseAll();

	void					JoinAll();

	CNtlThread *			CreateThread(CNtlRunObject * pRunObject, const char * name = "Unnamed Thread", bool bAutoDelete = false);
	
	void					AllThreadDump(); //new


protected:

	CNtlLinkList 		m_ThreadList;

	CNtlMutex 			m_Mutex;

	bool				m_bClosed;

};

typedef Loki::SingletonHolder<CNtlThreadFactory, Loki::CreateUsingNew, Loki::DefaultLifetime, Loki::ClassLevelLockable> tThreadFactory;

#if defined(_WIN32)
inline void CNtlRunObject::Wait()
{
	m_pOwner->Wait();
}

inline void CNtlRunObject::Terminate()
{
	Close();

	if (GetThread())
	{
		if (GetThread()->IsStatus(CNtlThread::eSTATUS_RUNNING) ||
			GetThread()->IsStatus(CNtlThread::eSTATUS_PAUSED) ||
			GetThread()->IsStatus(CNtlThread::eSTATUS_DEAD))
		{
			GetThread()->Join();
		}

		tThreadFactory::Instance().SingleGarbageCollect(GetThread());
	}
}

inline int CNtlRunObject::Wait(unsigned int millisecs)
{
	return m_pOwner->Wait( millisecs );
}

inline void CNtlRunObject::Exit()
{
	m_pOwner->Exit();
}

inline const char * CNtlRunObject::GetName() const
{
	return m_pOwner->GetName();
}
#endif // _WIN32


#endif // __NTLTHREAD_H__

