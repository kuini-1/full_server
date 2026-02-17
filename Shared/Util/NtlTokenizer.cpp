#include "stdafx.h"
#include "NtlTokenizer.h"
#include <cwchar>
#if !defined(_WIN32)
#include <iconv.h>
#include <cstring>
#endif

CNtlTokenizer::CNtlTokenizer(const std::string &strFileName, CallTokenPack fnCallPack /* = NULL */)
{
	m_pData		 = NULL;

	m_bSuccess = Load(strFileName.data(), fnCallPack);
	if(!m_bSuccess)
		return;

	m_strFileName = strFileName;
	m_iPeekPos = 0;
	m_iLastLine = 0;
	m_bInRemark = FALSE;

	Tokenize();
}

CNtlTokenizer::CNtlTokenizer(const char *pBuffer)
{
	m_pData		 = NULL;
	m_bSuccess	 = TRUE;

	m_iTotalSize = (int)strlen(pBuffer);
	m_pData = new char[m_iTotalSize+1];
	m_pData[m_iTotalSize] = '\0';
	memcpy(m_pData, pBuffer, m_iTotalSize);
		
	m_iPeekPos = 0;
	m_iLastLine = 0;
	m_bInRemark = FALSE;

	Tokenize();
}

CNtlTokenizer::~CNtlTokenizer()
{
	if(m_pData)
	{
		delete [] m_pData;
		m_pData = NULL;
	}
}

BOOL CNtlTokenizer::Load(const char *pFileName, CallTokenPack fnCallPack)
{
	if( m_pData )
	{
		delete [] m_pData;
		m_pData = NULL;
	}

	if( fnCallPack )
	{
		(*fnCallPack)( pFileName, (void**)&m_pData, &m_iTotalSize );
	}
	else
	{
		FILE *fp = NULL;
		if (!NTL_FOPEN(&fp, pFileName, "rb"))
			return FALSE;

		fseek(fp, 0, SEEK_END);
		int iSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		m_iTotalSize = iSize;
		m_pData = new char[iSize+1];
		m_pData[iSize] = '\0';

		fread(m_pData,1,iSize,fp);
		fclose(fp);
	}

	return TRUE;
}

BOOL CNtlTokenizer::IsSuccess(void)
{
	return m_bSuccess;
}

BOOL CNtlTokenizer::IsSpace(char c)
{
	const char *pSpace = " \t\r\n";

	return strchr(pSpace, c) != NULL;
}


BOOL CNtlTokenizer::IsOperator(char c)
{
	const char *pOperators = ",=();{}<+-*/>";

	return strchr(pOperators, c) != NULL;
}


BOOL CNtlTokenizer::IsRemark(char c, int iPosition)
{
	if (m_bInRemark)
	{
		if (c == '\n') 
		{	
			m_bInRemark = FALSE;
		}
		return TRUE;
	}
	else
	{
		if (c == '/')
		{
			if (
				iPosition+1 < m_iTotalSize &&
				m_pData[iPosition+1] == '/')
			{
				m_bInRemark = TRUE;
				return TRUE;
			}
		}
		/*
		else if(c == '-')
		{
			if (
				iPosition+1 < m_iTotalSize &&
				m_pData[iPosition+1] == '-')
			{
				m_bInRemark = TRUE;
				return TRUE;
			}
		}
		*/
		else if (c == '#')
		{
			m_bInRemark = TRUE;
			return TRUE;
		}
	}

	return FALSE;
}




void CNtlTokenizer::Tokenize(void)
{
	int iCurPos = 0;
	int iCurLine = 0;
	while (iCurPos < m_iTotalSize)
	{
		while (
			iCurPos < m_iTotalSize && 
			(
				IsRemark(m_pData[iCurPos], iCurPos) ||
				IsSpace(m_pData[iCurPos])
			))
		{
			if (m_pData[iCurPos] == '\n') iCurLine++;
			iCurPos++;
		}
		if (iCurPos == m_iTotalSize) break;

		if (IsOperator(m_pData[iCurPos]))
		{
			m_dqTokens.push_back(CNtlToken(std::string(&m_pData[iCurPos], 1), iCurPos, iCurLine));
			iCurPos++;
		}
		else
		{
			int iTempPos = iCurPos;
			if (m_pData[iTempPos] == '"')
			{
				int iNumChars = 0;
				iTempPos++;
				while (iTempPos < m_iTotalSize)
				{
					if (m_pData[iTempPos] == '"')
					{
						if (iTempPos+1>=m_iTotalSize || m_pData[iTempPos+1] != '"') break;
						else
						{
							iTempPos++;
						}
					}
					iTempPos++;
					iNumChars++;
				}
				if (iTempPos == m_iTotalSize)
				{
					WriteError("Missing '""' following '""'-begin");
					break;
				}

				_ASSERTE(iNumChars < NTL_TOKEN_BUFF_LEN);
//				char *temp = new char[iNumChars+1];
				m_pTemp[iNumChars] = 0;
				int ofs = 1;
				for (int i=0;i<iNumChars;i++)
				{
					m_pTemp[i] = m_pData[iCurPos+ofs+i];
					if (m_pData[iCurPos+ofs+i] == '"') 
					{
						ofs++;
					}
				}
				iCurPos = iTempPos+1;

				m_dqTokens.push_back(CNtlToken(std::string(m_pTemp), iCurPos, iCurLine));
//				delete temp;
			}
			else
			{
				while (iTempPos < m_iTotalSize &&
					!IsSpace(m_pData[iTempPos]) &&
					!IsOperator(m_pData[iTempPos]) &&
					!IsRemark(m_pData[iTempPos], iCurPos))
				{
					iTempPos++;
				}

				int iTokSize = iTempPos-iCurPos;
				m_dqTokens.push_back(CNtlToken(std::string(&m_pData[iCurPos], iTokSize), iCurPos, iCurLine));
				iCurPos += iTokSize;
			}
		}
	}
}

std::string CNtlTokenizer::PeekNextToken(int *pOffset/*=NULL*/, int *pLine /*= 0*/)
{
	if(m_iPeekPos >= (int) m_dqTokens.size())
	{
		return "";
	}
	if(pOffset != NULL) 
		*pOffset = m_dqTokens[m_iPeekPos].iOffset;
	if(pLine != NULL)
		*pLine = m_dqTokens[m_iPeekPos].iLine;

	return m_dqTokens[m_iPeekPos++].strToken;
}


std::string CNtlTokenizer::GetNextToken(int *pOffset/*=NULL*/, int *pLine /*= 0*/)
{
	m_iPeekPos = 0;
	m_iLastLine = m_dqTokens[0].iLine;
	std::string token = m_dqTokens[0].strToken;
	if(pOffset != NULL)
		*pOffset = m_dqTokens[0].iOffset;

	if(pLine != NULL)
		*pLine = m_dqTokens[0].iLine;

	m_dqTokens.pop_front();

	return token;
}

void CNtlTokenizer::PopToPeek(void)
{
	while (m_iPeekPos > 0)
	{
		m_iPeekPos--;
		m_dqTokens.pop_front();
	}
}


std::string CNtlTokenizer::WriteError(std::string strErrMsg)
{
	std::string str = "core tokenizer Tokenize() error";
	return str;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Unicode Tokenizer
//////////////

// Helper functions for WCHAR (UTF-16) to wchar_t (UTF-32) conversion
// Note: WCHARLen is now defined in NtlPortable.h, so we use that instead

static std::wstring WCHARToWString(const WCHAR* src, size_t len)
{
	if (!src || len == 0)
		return std::wstring();
	
#if defined(_WIN32)
	// On Windows, WCHAR == wchar_t (both 2 bytes), so direct conversion works
	return std::wstring((const wchar_t*)src, len);
#else
	// On Linux, WCHAR is UTF-16LE (2 bytes), wchar_t is UTF-32 (4 bytes)
	// Use iconv to convert UTF-16LE to UTF-32 (native endianness)
	// Try UTF-32 first (uses system endianness), fall back to UTF-32LE if needed
	iconv_t cd = iconv_open("UTF-32", "UTF-16LE");
	if (cd == (iconv_t)-1)
	{
		cd = iconv_open("UTF-32LE", "UTF-16LE");
	}
	if (cd == (iconv_t)-1)
	{
		// Fallback: simple ASCII conversion
		std::wstring result;
		result.reserve(len);
		for (size_t i = 0; i < len; i++)
		{
			if (src[i] < 128)
				result += (wchar_t)src[i];
			else
				result += L'?';
		}
		return result;
	}
	
	size_t inbytesleft = len * sizeof(WCHAR);
	size_t outbytesleft = len * sizeof(wchar_t); // UTF-32: 4 bytes per character
	wchar_t* outbuf = new wchar_t[len + 1];
	if (!outbuf)
	{
		iconv_close(cd);
		return std::wstring();
	}
	
	char* inbuf = (char*)src;
	char* outbuf_char = (char*)outbuf;
	
	size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf_char, &outbytesleft);
	iconv_close(cd);
	
	if (result == (size_t)-1)
	{
		delete[] outbuf;
		// Fallback: simple ASCII conversion
		std::wstring wstr;
		wstr.reserve(len);
		for (size_t i = 0; i < len; i++)
		{
			if (src[i] < 128)
				wstr += (wchar_t)src[i];
			else
				wstr += L'?';
		}
		return wstr;
	}
	
	// Calculate actual length
	size_t actualLen = (len * sizeof(wchar_t) - outbytesleft) / sizeof(wchar_t);
	outbuf[actualLen] = L'\0';
	
	std::wstring wstr(outbuf, actualLen);
	delete[] outbuf;
	return wstr;
#endif
}

CNtlTokenizerW::CNtlTokenizerW(const std::string &strFileName, CallTokenPack fnCallPack /* = NULL */)
{
	m_pData		 = NULL;

	m_bSuccess = Load(strFileName.data(), fnCallPack);
	if(!m_bSuccess)
		return;

	m_strFileName = strFileName;
	m_iPeekPos = 0;
	m_iLastLine = 0;
	m_bInRemark = FALSE;

	Tokenize();
}

CNtlTokenizerW::CNtlTokenizerW(const WCHAR *pBuffer)
{
	m_pData		 = NULL;
	m_bSuccess	 = TRUE;

	m_iTotalSize = (int)WCHARLen(pBuffer);
	m_pData = new WCHAR[m_iTotalSize+1];
	m_pData[m_iTotalSize] = 0;
	memcpy(m_pData, pBuffer, m_iTotalSize * sizeof(WCHAR));

	m_iPeekPos = 0;
	m_iLastLine = 0;	
	m_bInRemark = FALSE;

	Tokenize();
}

CNtlTokenizerW::~CNtlTokenizerW()
{
	if(m_pData)
	{
		delete [] m_pData;
		m_pData = NULL;
	}
}

BOOL CNtlTokenizerW::Load(const char *pFileName, CallTokenPack fnCallPack)
{
	if( m_pData )
	{
		delete [] m_pData;
		m_pData = NULL;
	}

	

	const BYTE byBomOffset = 2;
	const BYTE abyUnicodeBom[2] = { 0xFF, 0xFE };
	
	if( fnCallPack )
	{
		CHAR* pData = NULL;
		INT iSize = 0;

		(*fnCallPack)( pFileName, (VOID**)&pData, &iSize );

		if( abyUnicodeBom[0] == (BYTE)pData[0] &&
			abyUnicodeBom[1] == (BYTE)pData[1] )
		{// UTF-16(Little-Endian)
			WCHAR* pwData = (WCHAR*)pData;
			pwData++;
			m_iTotalSize = ( iSize - 2 ) / 2;
			m_pData = new WCHAR[m_iTotalSize+1];
			m_pData[m_iTotalSize] = '\0';

			memcpy( m_pData, pwData, m_iTotalSize * 2 );			
		}
		else
		{// Ansi
			m_iTotalSize = iSize;
			m_pData = new WCHAR[m_iTotalSize+1];
			m_pData[m_iTotalSize] = 0;

			::MultiByteToWideChar( GetACP(), 0, pData, -1, m_pData, m_iTotalSize + 1 );
		}

		delete [] pData;
	}
	else
	{
		FILE *fp = NULL;
		if (!NTL_FOPEN(&fp, pFileName, "rb"))
			return FALSE;
		BYTE abyFileBom[2] = { 0, };
		fread( abyFileBom, 1, 2, fp );


		// UTF-16(Little-Endian)
		if( abyUnicodeBom[0] == abyFileBom[0] &&
			abyUnicodeBom[1] == abyFileBom[1] )
		{
			fseek(fp, byBomOffset, SEEK_END);
			int nSize = ftell(fp);

			fseek(fp, 2, SEEK_SET);

			int nStrLen = (nSize/2);
			m_iTotalSize = nSize/2;
			m_pData = new WCHAR[nStrLen];
			m_pData[nStrLen-1] = 0;

			fread(m_pData, nSize , 1, fp);
		}
		// Ansi
		else 
		{
			fseek(fp, 0, SEEK_END);
			int nSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char* pData = new char[nSize+1];
			pData[nSize] = 0;

			fread(pData, nSize, 1, fp);

			m_pData = new WCHAR[nSize];
			m_iTotalSize = nSize;

			::MultiByteToWideChar( GetACP(), 0, pData, -1, m_pData, nSize );

			delete [] pData;
			pData = NULL;
		}

		fclose(fp);
	}

	return TRUE;
}

BOOL CNtlTokenizerW::IsSuccess(void)
{
	return m_bSuccess;
}

BOOL CNtlTokenizerW::IsSpace(WCHAR c)
{
	// WCHAR is 2 bytes, can compare directly with ASCII characters
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}


BOOL CNtlTokenizerW::IsOperator(WCHAR c)
{
	// WCHAR is 2 bytes, can compare directly with ASCII characters
	return (c == ',' || c == '=' || c == '(' || c == ')' || c == ';' || 
	        c == '{' || c == '}' || c == '<' || c == '+' || c == '-' || 
	        c == '*' || c == '/' || c == '>');
}


BOOL CNtlTokenizerW::IsRemark(WCHAR c, int iPosition)
{
	if (m_bInRemark)
	{
		if (c == '\n') 
		{	
			m_bInRemark = FALSE;
		}
		return TRUE;
	}
	else
	{
		if (c == '/')
		{
			if (
				iPosition+1 < m_iTotalSize &&
				m_pData[iPosition+1] == '/')
			{
				m_bInRemark = TRUE;
				return TRUE;
			}
		}
		/*
		else if(c == '-')
		{
		if (
		iPosition+1 < m_iTotalSize &&
		m_pData[iPosition+1] == '-')
		{
		m_bInRemark = TRUE;
		return TRUE;
		}
		}
		*/
		else if (c == '#')
		{
			m_bInRemark = TRUE;
			return TRUE;
		}
	}

	return FALSE;
}




void CNtlTokenizerW::Tokenize(void)
{
	int iCurPos = 0;
	int iCurLine = 0;
	while (iCurPos < m_iTotalSize)
	{
		while (
			iCurPos < m_iTotalSize && 
			(
			IsRemark(m_pData[iCurPos], iCurPos) ||
			IsSpace(m_pData[iCurPos])
			))
		{
			if (m_pData[iCurPos] == '\n') iCurLine++;
			iCurPos++;
		}
		if (iCurPos == m_iTotalSize) break;

		if (IsOperator(m_pData[iCurPos]))
		{
			m_dqTokens.push_back(CNtlTokenW(WCHARToWString(&m_pData[iCurPos], 1), iCurPos, iCurLine));
			iCurPos++;
		}
		else
		{
			int iTempPos = iCurPos;
			if (m_pData[iTempPos] == '"')
			{
				int iNumChars = 0;
				iTempPos++;
				while (iTempPos < m_iTotalSize)
				{
					if (m_pData[iTempPos] == '"')
					{
						if (iTempPos+1>=m_iTotalSize || m_pData[iTempPos+1] != '"') break;
						else
						{
							iTempPos++;
						}
					}
					iTempPos++;
					iNumChars++;
				}
				if (iTempPos == m_iTotalSize)
				{
					WriteError("Missing '""' following '""'-begin");
					break;
				}

				_ASSERTE(iNumChars < NTL_TOKEN_BUFF_LEN);
				//				char *temp = new char[iNumChars+1];
				m_pTemp[iNumChars] = 0;
				int ofs = 1;
				for (int i=0;i<iNumChars;i++)
				{
					m_pTemp[i] = m_pData[iCurPos+ofs+i];
					if (m_pData[iCurPos+ofs+i] == '"') 
					{
						ofs++;
					}
				}
				iCurPos = iTempPos+1;

				m_dqTokens.push_back(CNtlTokenW(WCHARToWString(m_pTemp, iNumChars), iCurPos, iCurLine));
				//				delete temp;
			}
			else
			{
				while (iTempPos < m_iTotalSize &&
					!IsSpace(m_pData[iTempPos]) &&
					!IsOperator(m_pData[iTempPos]) &&
					!IsRemark(m_pData[iTempPos], iCurPos))
				{
					iTempPos++;
				}

				int iTokSize = iTempPos-iCurPos;
				m_dqTokens.push_back(CNtlTokenW(WCHARToWString(&m_pData[iCurPos], iTokSize), iCurPos, iCurLine));
				iCurPos += iTokSize;
			}
		}
	}
}

std::wstring CNtlTokenizerW::PeekNextToken(int *pOffset/*=NULL*/, int *pLine /*= 0*/)
{
	if(m_iPeekPos >= (int) m_dqTokens.size())
	{
		return std::wstring();
	}
	if(pOffset != NULL) 
		*pOffset = m_dqTokens[m_iPeekPos].iOffset;
	if(pLine != NULL)
		*pLine = m_dqTokens[m_iPeekPos].iLine;

	return m_dqTokens[m_iPeekPos++].wstrToken;
}


std::wstring CNtlTokenizerW::GetNextToken(int *pOffset/*=NULL*/, int *pLine /*= 0*/)
{
	m_iPeekPos = 0;
	m_iLastLine = m_dqTokens[0].iLine;
	std::wstring token = m_dqTokens[0].wstrToken;
	if(pOffset != NULL)
		*pOffset = m_dqTokens[0].iOffset;

	if(pLine != NULL)
		*pLine = m_dqTokens[0].iLine;

	m_dqTokens.pop_front();

	return token;
}

void CNtlTokenizerW::PopToPeek(void)
{
	while (m_iPeekPos > 0)
	{
		m_iPeekPos--;
		m_dqTokens.pop_front();
	}
}


std::string CNtlTokenizerW::WriteError(std::string strErrMsg)
{
	std::string str = "core tokenizer Tokenize() error";
	return str;
}