#ifndef __OPJSUPPORT__
#define __OPJSUPPORT__

#include <iostream>


class OPJSupport {

public:
    OPJSupport();
    ~OPJSupport();

    void* decompressJPEG2K( void* jp2Data, long jp2DataSize, long *decompressedBufferSize, int planarConfig, int *colorModel);
    void* decompressJPEG2KWithBuffer( void* inputBuffer, void* jp2Data, long jp2DataSize, long *decompressedBufferSize, int planarConfig, int *colorModel);
    void* compressJPEG2K(void* data, int bitsAllocated, int columns, int rows, int samplesPerPixel, int planarConfig, EP_Interpretation photometric, EP_Representation pixelRepresentation, bool lossless, long unsigned int& compressedSize);

};





#endif 
