#ifndef __LIB_CAPT_RLE_INCLUDE_H__
#define __LIB_CAPT_RLE_INCLUDE_H__

 
namespace libCapt
{

 
bool rleCompress(const unsigned char* imageBuf, int width, int height, unsigned char* rleBuf, unsigned int& rleBufSize);

 
bool rleDecompress(const unsigned char* rleBuf, unsigned int rleBufSize, int width, int height, unsigned char* imageBuf, unsigned int& imageBufSize);

}

#endif

