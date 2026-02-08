#pragma once
//-- include ------------------------------------------------------------------------
#include <string>
#include <map>
#include <deque>
#include "NtlSingleton.h"
#include "NtlMutex.h"
#include "../../Shared/NtlSharedCommon.h"
//-----------------------------------------------------------------------------------

using namespace std;

#define PROFILE_OUTPUT_FILENAME		"Profile"

//-----------------------------------------------------------------------------------------
//
class CProfileStatistic
	: public CNtlSingleton<CProfileStatistic>
{
protected:
	struct PROFILEUNIT
	{
	public:
		ntl_uint64 m_uiTotalCycle;
		ntl_uint64 m_uiMinCycle;
		ntl_uint64 m_uiMaxCycle;
		unsigned int m_uiCalledCount;
		unsigned int m_uiTurnCount_TotalCycle;
		unsigned int m_uiTurnCount_CalledCount;

	protected:
		string m_strFilename;
		unsigned int m_uiLine;

	public:
		PROFILEUNIT()
			: m_uiTotalCycle( 0 )
			, m_uiMinCycle( 0 )
			, m_uiMaxCycle( 0 )
			, m_uiCalledCount( 0 )	
			, m_strFilename( )
			, m_uiLine( 0 )
			, m_uiTurnCount_TotalCycle( 0 )
			, m_uiTurnCount_CalledCount( 0 )
		{
		}

		PROFILEUNIT( const char * szFilename, unsigned short line, ntl_uint64 cycle )
			: m_uiTotalCycle( cycle )
			, m_uiMinCycle( cycle )
			, m_uiMaxCycle( cycle )
			, m_uiCalledCount( 1 )
			, m_strFilename( szFilename )
			, m_uiLine( line )
			, m_uiTurnCount_TotalCycle( 0 )
			, m_uiTurnCount_CalledCount( 0 )
		{
		}

		~PROFILEUNIT()
		{
		}
	};

	struct FUNC
	{
		ntl_uint64 m_uiStartCycle;
		ntl_uint64 m_uiChildCycle;
	};

public:
	typedef map< string,PROFILEUNIT >		PROFILES_MAP;
	typedef PROFILES_MAP::iterator			PROFILES_ITER;
	typedef PROFILES_MAP::value_type		PROFILES_VALUE;

	typedef deque< FUNC >					FUNC_DEQUE;

public:
	CProfileStatistic( void );
	~CProfileStatistic( void );

	void BeginProfile( ntl_uint64 startCycle );
	void EndProfile( const char* strToken, ntl_uint64 endCycle, const char* strFile, unsigned short nLine );
	void Save( void );
	void SetFileName( const char* pszFileName ) { m_strFilename = pszFileName; return; }
	void CheckCpuCycle( void );
	void ClearProfile( void );

private:
	ntl_uint64	m_uCpuCycle;
	PROFILES_MAP		m_mapProfiles;
	FUNC_DEQUE			m_dequeFunc;
	string				m_strFilename;
	char				m_szFileNameTime[256];
	CNtlMutex			m_mutex;
};

//-----------------------------------------------------------------------------------------
//
class CProfile
{

public:
	CProfile( const char* szToken, const char* szFile );
	CProfile( const char* szToken, const char* szFile, unsigned short nLine );
	virtual ~CProfile( void );

public:
	void Init( const char * szToken, const char * szFile, unsigned short nLine );

public:
	void Create( unsigned short nLine );
	void Destroy( void );

private:
	ntl_uint64	m_cycle;
	const char*			m_szToken;
	const char*			m_szFilename;
	unsigned short	m_nLine;
	CNtlMutex			m_mutex;
};

//-----------------------------------------------------------------------------------------
// Profile on the head of a desired function to the PROFILE ("your name")
// Write this only being put


//-----------------------------------------------------------------------------------------
//  [8/8/2009 zeroera] : CAUTION: Do you use will cause a decrease in performance when using the profiler, turning only when necessary.
//-----------------------------------------------------------------------------------------
//
// #define __USE_PERFORMANCE_PROFILER__
//
//-----------------------------------------------------------------------------------------

#ifdef __USE_PERFORMANCE_PROFILER__
	#define PROFILE(token) CProfile _tagPROFILE_CLASS(token, __FILE__, (unsigned short)(__LINE__) )
#else
	#define PROFILE(token) ((void)0)
#endif

//-----------------------------------------------------------------------------------
//
#ifdef __USE_PERFORMANCE_PROFILER__
	#define PROFILE_FLUSH CProfileStatistic::GetInstance()->Save()
#else
	#define PROFILE_FLUSH
#endif