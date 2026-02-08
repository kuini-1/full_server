#include "precomp_navi.h"
#include "NtlNaviPathEngine.h"
#include "NtlNaviLog.h"
#if !defined(_WIN32)
#include <dlfcn.h>
#include <string>
#endif


class CPEErrorHandler : public iErrorHandler
{
public:
	virtual eAction handle(const char* type, const char* description, const char *const* attributes)
	{
		std::string displayString(description);
		if(attributes && *attributes)
		{
			displayString += "\nAttributes:\n";
			do
			{
				displayString += "attribute ";
				displayString += *attributes;
				displayString += "=";
				attributes++;
				displayString += *attributes;
				displayString += "\n";
				attributes++;
			}
			while(*attributes);
		}

		char title[1000];
		snprintf(title, 1000, "Error, type: %s", type);

		CNtlNaviLog::GetInstance()->Log( title );
		CNtlNaviLog::GetInstance()->Log( displayString.c_str() );
		
		return iErrorHandler::CONTINUE;
	}
};

static CPEErrorHandler g_clErrorHandler;


CPathDataOutStream::CPathDataOutStream( const char* pFileName )
{
	if (!NTL_FOPEN(&m_pFile, pFileName, "wb"))
		m_pFile = NULL;
}

CPathDataOutStream::~CPathDataOutStream( void )
{
	if ( m_pFile )
	{
		fclose( m_pFile );
		m_pFile = NULL;
	}
}

void CPathDataOutStream::put( const char* data, tUnsigned32 dataSize )
{
	if ( m_pFile )
	{
		fwrite( data, dataSize, 1, m_pFile );
	}
}



CNtlNaviPathEngine* CNtlNaviPathEngine::GetInstance( void )
{
	static CNtlNaviPathEngine g_clPathEngine;
	return &g_clPathEngine;
}

CNtlNaviPathEngine::CNtlNaviPathEngine( void )
{
	m_hInstance = NULL;
	m_pPathEngine = NULL;
}

CNtlNaviPathEngine::~CNtlNaviPathEngine( void )
{
	Delete();
}

iPathEngine* CNtlNaviPathEngine::GetPathEngine( void )
{
	return m_pPathEngine;
}

bool CNtlNaviPathEngine::Create( const char* pPathDllName )
{
	if ( NULL == pPathDllName )
	{
		return true;
	}

#if defined(_WIN32)
	m_hInstance = LoadLibrary( pPathDllName );

	if ( NULL == m_hInstance )
	{
		return false;
	}

	FARPROC procAddr;
	procAddr = GetProcAddress( m_hInstance, (LPCSTR)1 );

	if ( NULL == procAddr )
	{
		FreeLibrary( m_hInstance );
		m_hInstance = NULL;
		return false;
	}

	tGetInterfaceFunction getInterfaceFunction = (tGetInterfaceFunction) procAddr;

	m_pPathEngine = getInterfaceFunction( this );

	if ( NULL == m_pPathEngine )
	{
		FreeLibrary( m_hInstance );
		m_hInstance = NULL;
		return false;
	}
#else
	/* Linux: use dlopen for .so; convert .dll path to .so if needed */
	m_hInstance = dlopen( pPathDllName, RTLD_NOW );
	if ( NULL == m_hInstance )
	{
		/* Try .so extension if path ends with .dll */
		std::string soPath( pPathDllName );
		size_t pos = soPath.rfind( ".dll" );
		if ( pos != std::string::npos )
		{
			soPath.replace( pos, 4, ".so" );
			m_hInstance = dlopen( soPath.c_str(), RTLD_NOW );
		}
	}
	if ( NULL == m_hInstance )
	{
		return false;
	}

	/* PathEngine exports getInterface (Windows uses ordinal 1); dlsym by name on Linux */
	void* procAddr = dlsym( m_hInstance, "getInterface" );
	if ( NULL == procAddr )
	{
		dlclose( m_hInstance );
		m_hInstance = NULL;
		return false;
	}

	tGetInterfaceFunction getInterfaceFunction = (tGetInterfaceFunction) procAddr;

	m_pPathEngine = getInterfaceFunction( this );

	if ( NULL == m_pPathEngine )
	{
		dlclose( m_hInstance );
		m_hInstance = NULL;
		return false;
	}
#endif

	m_pPathEngine->setErrorHandler( &g_clErrorHandler );

	return true;
}

void CNtlNaviPathEngine::Delete( void )
{
	if ( m_pPathEngine )
	{
		m_pPathEngine->deleteAllObjects();
#if defined(_WIN32)
		FreeLibrary( m_hInstance );
#else
		dlclose( m_hInstance );
#endif

		m_hInstance = NULL;
		m_pPathEngine = NULL;
	}
}

iErrorHandler::eAction CNtlNaviPathEngine::handle( const char* type, const char* description, const char *const* attributes )
{
	std::string displayString(description);
	if(attributes && *attributes)
	{
		displayString += "\nAttributes:\n";
		do
		{
			displayString += "attribute ";
			displayString += *attributes;
			displayString += "=";
			attributes++;
			displayString += *attributes;
			displayString += "\n";
			attributes++;
		}
		while(*attributes);
	}

	char title[1000];
	snprintf(title, 1000, "Error, type: %s", type);

	CNtlNaviLog::GetInstance()->Log( title );
	CNtlNaviLog::GetInstance()->Log( displayString.c_str() );
	return CONTINUE;
}
