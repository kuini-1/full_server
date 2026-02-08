#include "precomp_trigger.h"
#include "NtlTSScrTokenizer.h"
#include "NtlTSCoreDefine.h"
#include "NtlTSLog.h"


/** 
	Script tokenizer
*/


CNtlTSScrTokenizer::CNtlTSScrTokenizer( const std::string& strFileName, const char* pBuff, int nSize )
: m_nMode( eTOKENIZER_MODE_INVALID),
  m_strFileName( strFileName ),
  m_pFileBuffer( 0 ),
  m_nFileSize( 0 ),
  m_nPeekPos( 0 ),
  m_nLastLine( 0 ),
  m_bComment( false )
{
	if ( !Load( strFileName, pBuff, nSize ) )
	{
		Unload();
	}
	else
	{
		if ( !Tokenize() )
		{
			Unload();
		}
	}
}

CNtlTSScrTokenizer::~CNtlTSScrTokenizer( void )
{
	Unload();
}

bool CNtlTSScrTokenizer::IsSpace( char c )
{
	const char *pSpace = " \t\r\n";

	return ( strchr( pSpace, c ) != NULL );
}

bool CNtlTSScrTokenizer::IsOperator( char c )
{
	const char *pOperators = ",=();{}<+-*/>";

	return ( strchr( pOperators, c ) != NULL );
}

bool CNtlTSScrTokenizer::IsComment( char c, int nPos )
{
	if ( m_bComment )
	{
		if ( '\n' == c ) 
		{	
			m_bComment = false;
			return false;
		}
		return true;
	}
	else
	{
		if ( '/' == c )
		{
			if ( nPos + 1 < m_nFileSize && '/' == m_pFileBuffer[nPos + 1] )
			{
				m_bComment = true;
				return true;
			}
		}
		return false;
	}	
}

std::string CNtlTSScrTokenizer::GetNextToken( int *pnOffset )
{
	m_nPeekPos = 0;
	m_nLastLine = m_defTokens[0].m_nLine;
	std::string strToken = m_defTokens[0].m_strToken;
	if ( 0 != pnOffset ) *pnOffset = m_defTokens[0].m_nOffset;

	m_defTokens.pop_front();

	return strToken;
}

std::string CNtlTSScrTokenizer::PeekNextToken( int *pnOffset )
{
	if ( m_nPeekPos >= (int)m_defTokens.size() ) return "";
	m_nLastLine = m_defTokens[m_nPeekPos].m_nLine;
	if ( 0 != pnOffset ) *pnOffset = m_defTokens[m_nPeekPos].m_nOffset;

	return m_defTokens[m_nPeekPos++].m_strToken;
}

void CNtlTSScrTokenizer::PopToPeek( void )
{
	while ( m_nPeekPos > 0 )
	{
		m_nPeekPos--;
		m_defTokens.pop_front();
	}
}

bool CNtlTSScrTokenizer::Load( const std::string& strFileName, const char* pBuff, int nSize )
{
	if ( pBuff && 0 != nSize )
	{
		m_nMode = eTOKENIZER_MODE_OUT_BUFFER;

		m_nFileSize = nSize;
		m_pFileBuffer = (char*)pBuff;
	}
	else
	{
		m_nMode = eTOKENIZER_MODE_INNER_BUFFER;

		// ??? ?????? ????
		FILE *pFile;
		if ( !NTL_FOPEN( &pFile, strFileName.c_str(), "rb" ) )
		{
			printf( "Can not open the file. Info[%s]. [%s]", strFileName.c_str(), TS_CODE_TRACE() );
			return false;
		}

		// ?????? ??? ??????
		fseek( pFile, 0, SEEK_END );
		m_nFileSize = ftell( pFile );
		fseek( pFile, 0, SEEK_SET );

		// ?????? ???? ????? ??????
		m_pFileBuffer = (char*)malloc( m_nFileSize*sizeof(char) );
		if ( 0 == m_pFileBuffer )
		{
			printf( "Allocating a memory is failed. Info[%zu]. [%s]", (size_t)m_nFileSize*sizeof(char), TS_CODE_TRACE() );
			return false;
		}

		// ???? ??? ???????? ??????? ???????
		fread( m_pFileBuffer, 1, m_nFileSize, pFile );

		fclose( pFile );
	}

	return true;
}

void CNtlTSScrTokenizer::Unload( void )
{
	if ( eTOKENIZER_MODE_INNER_BUFFER == m_nMode )
	{
		if ( m_pFileBuffer )
		{
			free( m_pFileBuffer );
		}
	}
	m_pFileBuffer = 0;
	m_nFileSize = 0;
	m_nPeekPos = 0;
	m_nLastLine = 0;
	m_bComment = false;

	m_defTokens.clear();
}

bool CNtlTSScrTokenizer::Tokenize( void )
{
	int nCurPos = 0;
	int nCurLine = 0;
	while ( nCurPos < m_nFileSize )
	{
		// Comment, Space?? ?????
		while ( nCurPos < m_nFileSize && ( IsComment( m_pFileBuffer[nCurPos], nCurPos ) || IsSpace( m_pFileBuffer[nCurPos] ) ) )
		{
			if ( '\n' == m_pFileBuffer[nCurPos] ) nCurLine++;
			nCurPos++;
		}

		// nCurPos ?? ???? ?????? ??? ????? while?? ???? ??????
		if ( nCurPos == m_nFileSize ) break;

		// Operator ????
		if ( IsOperator( m_pFileBuffer[nCurPos] ) )
		{
			// Operator??? ???? ????? ????, ????? ??? Operator?? Token???? ???????
			m_defTokens.push_back( CToken( std::string( &m_pFileBuffer[nCurPos], 1 ), nCurPos, nCurLine ) );
			nCurPos++;
		}
		else
		{
			int nTempPos = nCurPos;

			// ????? ó??
			if ( '"' == m_pFileBuffer[nTempPos] )
			{
				int nNumChars = 0;
				++nTempPos;

				while ( nTempPos < m_nFileSize )
				{
					if ( '"' == m_pFileBuffer[nTempPos] )
					{
						if ( (nTempPos+1 >= m_nFileSize) || ( '"' != m_pFileBuffer[nTempPos+1] ) )
						{
							break;
						}
						else
						{
							++nTempPos;
						}
					}
					++nTempPos;
					++nNumChars;
				}

				if ( nTempPos == m_nFileSize )
				{
					printf( "Parsing the string is failed. Info[%d]. [%s]", nTempPos, TS_CODE_TRACE() );
					return false;
				}

				char *pTemp = NULL;
				pTemp = (char*)malloc( (nNumChars+1)*sizeof(char) );
				if ( 0 == pTemp )
				{
					printf( "Allocating a memory is failed. Info[%zu]. [%s]", (size_t)(nNumChars+1)*sizeof(char), TS_CODE_TRACE() );
					return false;
				}
				pTemp[nNumChars] = 0;

				int nTempOffset = 1;
				for( int i = 0; i < nNumChars; ++i )
				{
					pTemp[i] = m_pFileBuffer[nCurPos+nTempOffset+i];
					if ( '"' == m_pFileBuffer[nCurPos+nTempOffset+i] ) 
					{
						nTempOffset++;
					}
				}
				nCurPos = nTempPos + 1;
				m_defTokens.push_back( CToken(std::string(pTemp), nCurPos, nCurLine) );

				free( pTemp );
			}
			else
			{
				while ( nTempPos < m_nFileSize && 
						!IsSpace( m_pFileBuffer[nTempPos] ) && 
						!IsOperator( m_pFileBuffer[nTempPos] ) && 
						!IsComment( m_pFileBuffer[nTempPos], nCurPos ) )
				{
					++nTempPos;
				}

				int nTokenSize = nTempPos - nCurPos;
				m_defTokens.push_back( CToken(std::string(&m_pFileBuffer[nCurPos], nTokenSize), nCurPos, nCurLine) );
				nCurPos += nTokenSize;
			}
		}
	}

	return true;
}
