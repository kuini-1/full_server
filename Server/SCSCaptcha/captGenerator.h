#ifndef __LIB_CAPT_GENERATOR_INCLUDE_H__
#define __LIB_CAPT_GENERATOR_INCLUDE_H__

namespace libCapt
{
class FontFile;

 
struct Question
{
	
	enum { ANASWER_LENGTH = 4 };			
	enum { IMAGE_WIDTH=64, IMAGE_HEIGHT=100, IMAGE_PITCH=IMAGE_WIDTH>>1 };
	enum { IMAGE_BUF_LENGTH = IMAGE_PITCH*IMAGE_HEIGHT };
	enum { FLAG_RLE = 1<<16, };

	inline bool isCompressed(void) const { return (nFlags & FLAG_RLE) !=0; }
	inline unsigned int getSize(void) const { return nFlags&0xFFFF; }

	unsigned int nFlags;
	unsigned short	wAnswer0[ANASWER_LENGTH];		 
	unsigned short	wAnswer1[ANASWER_LENGTH];		 
	unsigned short	wAnswer2[ANASWER_LENGTH];		 
	unsigned short	wAnswer3[ANASWER_LENGTH];		 
	unsigned char imageBuf[IMAGE_BUF_LENGTH];

	//---------------------------
	int	nCorrectAnswer;
};

 
class Generator
{
public:
 
	void generateQuestion(Question& question);

	/*****************************
	******************************/
private:
	enum { NOISE_POINT_COUNTS = 100, NOISE_CURVES_COUNTS = 10};
	typedef unsigned char RandomBuf[Question::IMAGE_WIDTH];
	typedef unsigned char IMAGE_BUF[Question::IMAGE_WIDTH*Question::IMAGE_HEIGHT];
	enum { CURVES_LENGTH = 256 };
	static const char CURVES_DATA[CURVES_LENGTH];

	int _drawCharacter(int codeIndex, int x_pos, int y_pos, IMAGE_BUF& imageBuf, int _sk1, int _sk2);
	void _drawLine(IMAGE_BUF& im);
	void _drawDots(IMAGE_BUF& im);
	void _blur(IMAGE_BUF& im);
	void _fillRandCurves(IMAGE_BUF& imageBuf);
	void _generatorAnswer(Question& question, unsigned int* answerIndex);
	

private:
	FontFile* m_fontFile;	 

public:
	Generator(FontFile* fontFile);
	~Generator();
};

}

#endif
