//***********************************************************************************
//
//	File		:	NtlIniFile.cpp
//
//	Begin		:	2006-01-05
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Myoung Jin, Choi		( yoshiki@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "NtlIniFile.h"
#include "NtlError.h"

#if !defined(_WIN32)

#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>
#include <sys/stat.h>

const unsigned int MAX_BUFFER = 256;

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlIniFile::CNtlIniFile()
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlIniFile::~CNtlIniFile()
{
}

// Helper function to trim whitespace
static std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlIniFile::Create(const char * lpszFullName)
{
	if (!lpszFullName)
		return NTL_FAIL;

	// Check if file exists
	struct stat fileStat;
	if (stat(lpszFullName, &fileStat) != 0)
		return NTL_FAIL;

	m_strConfigFileName = lpszFullName;
	m_strLastReadGroup.GetString().clear();
	m_strLastReadKey.GetString().clear();
	m_iniData.clear();

	// Parse the INI file
	std::ifstream file(lpszFullName);
	if (!file.is_open())
		return NTL_FAIL;

	std::string currentSection;
	std::string line;

	while (std::getline(file, line))
	{
		// Remove comments (everything after ; or #)
		size_t commentPos = line.find(';');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);
		commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);

		line = trim(line);
		if (line.empty())
			continue;

		// Check for section [section]
		if (line[0] == '[' && line[line.length() - 1] == ']')
		{
			currentSection = trim(line.substr(1, line.length() - 2));
			continue;
		}

		// Parse key=value
		size_t eqPos = line.find('=');
		if (eqPos != std::string::npos)
		{
			std::string key = trim(line.substr(0, eqPos));
			std::string value = trim(line.substr(eqPos + 1));
			
			if (!key.empty())
			{
				m_iniData[currentSection][key] = value;
			}
		}
	}

	file.close();
	return NTL_SUCCESS;
}

int CNtlIniFile::Create(const char * lpszPathName, const char * lpszFileName)
{
	if (!lpszPathName || !lpszFileName)
		return NTL_FAIL;

	std::string fullPath = std::string(lpszPathName) + "/" + std::string(lpszFileName);
	return Create(fullPath.c_str());
}

bool CNtlIniFile::Read(const char *group, const char *key, CNtlString &val)
{
	if (!group || !key)
		return false;

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	std::string section(group);
	std::string keyStr(key);

	std::map<std::string, std::map<std::string, std::string> >::iterator sectionIt = m_iniData.find(section);
	if (sectionIt == m_iniData.end())
		return false;

	std::map<std::string, std::string>::iterator keyIt = sectionIt->second.find(keyStr);
	if (keyIt == sectionIt->second.end())
		return false;

	val = keyIt->second.c_str();
	return true;
}

CNtlString CNtlIniFile::Read(const char *group, const char *key)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		strTemp = "";
	return strTemp;
}

bool CNtlIniFile::Read(const char *group, const char *key, bool &bFlag)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;

	std::string val = strTemp.c_str();
	// Convert to lowercase for comparison
	for (size_t i = 0; i < val.length(); ++i)
		val[i] = tolower(val[i]);

	bFlag = (val == "true" || val == "1" || val == "yes");
	return true;
}

bool CNtlIniFile::Read(const char *group, const char *key, char &num)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;
	num = (char)atoi(strTemp.c_str());
	return true;
}

bool CNtlIniFile::Read(const char *group, const char *key, short &num)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;
	num = (short)atoi(strTemp.c_str());
	return true;
}

bool CNtlIniFile::Read(const char *group, const char *key, int &num)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;
	num = atoi(strTemp.c_str());
	return true;
}

bool CNtlIniFile::Read(const char *group, const char *key, float &num)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;
	num = (float)atof(strTemp.c_str());
	return true;
}

bool CNtlIniFile::Read(const char *group, const char *key, unsigned char &num)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;
	num = (unsigned char)atoi(strTemp.c_str());
	return true;
}

bool CNtlIniFile::Read(const char *group, const char *key, unsigned short &num)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;
	num = (unsigned short)atoi(strTemp.c_str());
	return true;
}

bool CNtlIniFile::Read(const char *group, const char *key, unsigned int &num)
{
	CNtlString strTemp;
	if (!Read(group, key, strTemp))
		return false;
	num = (unsigned int)atoi(strTemp.c_str());
	return true;
}

bool CNtlIniFile::Read(const char *pszGroup, const char *pszKey, DWORD &dwNumber)
{
	CNtlString strTemp;
	if (!Read(pszGroup, pszKey, strTemp))
		return false;
	dwNumber = (DWORD)atoi(strTemp.c_str());
	return true;
}

#else // _WIN32

const unsigned int MAX_BUFFER = 256;

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlIniFile::CNtlIniFile()
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlIniFile::~CNtlIniFile()
{
}



//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlIniFile::Create(const char * lpszFullName)
{
	WIN32_FIND_DATA FindFileData;

	HANDLE hFind = FindFirstFile( lpszFullName, &FindFileData );
	if ( INVALID_HANDLE_VALUE == hFind ) 
	{
		return GetLastError();
	} 


	FindClose( hFind );

	m_strConfigFileName = lpszFullName;
	m_strLastReadGroup.GetString().clear();
	m_strLastReadKey.GetString().clear();


	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlIniFile::Create(const char * lpszPathName, const char * lpszFileName)
{
	m_strConfigFileName.Format("%s/%s", lpszPathName, lpszFileName);

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, CNtlString &val)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	char buffer[MAX_BUFFER + 1] = { 0x00, };
	if( 0 == GetPrivateProfileString(group, key, NULL, buffer, MAX_BUFFER, m_strConfigFileName.c_str() ) )
	{
		return false;
	}

	val = buffer;
	
	return true;
}

#endif // _WIN32

#if defined(_WIN32)
//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlString CNtlIniFile::Read(const char *group, const char *key)
{
	CNtlString strTemp;

	if( NULL == group || NULL == key )
	{
		return strTemp;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	if( !Read(group, key, strTemp) )
	{
		strTemp = "";
	}

	return strTemp;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, bool &bFlag)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	if( 0 == strTemp.GetString().compare("true") || 0 == strTemp.GetString().compare("TRUE") )
	{
		bFlag = true;
	}
	else
	{
		bFlag = false;
	}

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, char &num)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	num = (char) atoi( strTemp.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, short &num)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	num = (short) atoi( strTemp.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, int &num)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	num = atoi( strTemp.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, float &num)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	num = (float) atof( strTemp.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, unsigned char &num)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	num = (unsigned char) atoi( strTemp.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, unsigned short &num)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	num = (unsigned short) atoi( strTemp.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *group, const char *key, unsigned int &num)
{
	if( NULL == group || NULL == key )
	{
		return false;
	}

	m_strLastReadGroup = group;
	m_strLastReadKey = key;

	CNtlString strTemp;
	if( !Read(group, key, strTemp) )
	{
		return false;
	}

	num = (unsigned int) atoi( strTemp.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlIniFile::Read(const char *pszGroup, const char *pszKey, DWORD &dwNumber)
{
	if( NULL == pszGroup || NULL == pszKey )
	{
		return false;
	}

	m_strLastReadGroup = pszGroup;
	m_strLastReadKey = pszKey;

	CNtlString strTemp;
	if ( false == Read(pszGroup, pszKey, strTemp) )
	{
		return false;
	}

	dwNumber = (DWORD)(atoi(strTemp.c_str()));

	return true;
}
#endif // _WIN32