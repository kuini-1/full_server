#ifndef __LIB_CAPT_FONT_FILE_INCLUDE_H__
#define __LIB_CAPT_FONT_FILE_INCLUDE_H__

namespace libCapt
{
class FontFile
{
public:
	enum { MAGIC_CODE = 0x46504143, };
	 
	struct Head
	{
		unsigned int dwMagicCode;		 
		unsigned int nCodeOffset;		 
		unsigned int nCodeCounts;		 
		unsigned int nGlyphOffset;		 
		unsigned int nGlyphCounts;		 
		unsigned int nGlyphSize;		 
	};

	struct CodeGlyph
	{
		union
		{
			unsigned long long	nNextOffset;	 
			CodeGlyph*			pNext;			 
		};
		char				data[1];
	};

	struct Code
	{
		unsigned short		wCode;			 
		unsigned short		nGlyphCounts;	 
		union
		{
			unsigned long long	nFirstOffset;	 
			CodeGlyph*			pFirst;			 
		};
	};

	enum { GLPYH_EOL=-128, GLPYH_EOF=-127 };

public:
	void release(void);
	bool loadFromDataStream(const unsigned char* pStream, unsigned int streamSize);
	unsigned int getCodeCounts(void) const { return m_fileHead.nCodeCounts; }
	unsigned short getCodeFromIndex(unsigned int index) const;
	char* getCodeGlyphFromIndex(unsigned int index) const;

protected:
	void _pointRuntime(void);

private:
	bool _readStrem(unsigned char* dest, unsigned int size, const unsigned char*& pStream, const unsigned char* pStreamEnd);

protected:
	Head m_fileHead;
	Code* m_pCodeBuf;
	unsigned char* m_pGlyphBuf;

public:
	FontFile();
	virtual ~FontFile();
};

}

#endif
