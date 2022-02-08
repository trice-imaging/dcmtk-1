/*
 *
 *  Copyright (C) 1997-2005, OFFIS
 *
 *  This software and supporting documentation were developed by
 *
 *    Kuratorium OFFIS e.V.
 *    Healthcare Information and Communication Systems
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
 *  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
 *  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
 *  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
 *  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
 *
 *  Module:  dcmjpeg
 *
 *  Author:  Marco Eichelberg, Norbert Olges
 *
 *  Purpose: compression routines of the IJG JPEG library configured for 12 bits/sample. 
 *
 *  Last Update:      $Author: lpysher $
 *  Update Date:      $Date: 2006/03/01 20:15:44 $
 *  Source File:      $Source: /cvsroot/osirix/osirix/Binaries/dcmtk-source/dcmjpeg/djeijg12.cc,v $
 *  CVS/RCS Revision: $Revision: 1.1 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djeijg2k.h"
#include "dcmtk/dcmjpeg/djcparam.h"
#include "dcmtk/ofstd/ofconsol.h"


#include <sys/types.h>
//#include <sys/sysctl.h>

#define INCLUDE_CSTDIO
#define INCLUDE_CSETJMP
#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmjpeg/OPJSupport.h"

// These two macros are re-defined in the IJG header files.
// We undefine them here and hope that IJG's configure has
// come to the same conclusion that we have...
#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif
#ifdef HAVE_STDDEF_H
#undef HAVE_STDDEF_H
#endif

// use 16K blocks for temporary storage of compressed JPEG data
#define IJGE12_BLOCKSIZE 16384

//#include "openjpeg.h"
/**
sample error callback expecting a FILE* client object
*/
static void error_callback(const char *msg, void *a)
{
	printf( "%s", msg);
}
/**
sample warning callback expecting a FILE* client object
*/
static void warning_callback(const char *msg, void *a)
{
	printf( "%s", msg);
}
/**
sample debug callback expecting no client object
*/
static void info_callback(const char *msg, void *a)
{
//	NSLog( @"%s", msg);
}

static inline int int_ceildivpow2(int a, int b)
{
	return (a + (1 << b) - 1) >> b;
}


DJCompressJP2K::DJCompressJP2K(const DJCodecParameter& cp, EJ_Mode mode, Uint8 theQuality, Uint8 theBitsPerSample)
: DJEncoder()
, cparam(&cp)
, quality(theQuality)
, bitsPerSampleValue(theBitsPerSample)
, modeofOperation(mode) //EJM_JP2K_lossy, EJM_JP2K_lossless
{

}

DJCompressJP2K::~DJCompressJP2K()
{

}


OFCondition DJCompressJP2K::encode( Uint16 columns, Uint16 rows, EP_Interpretation photometric, Uint16 samplesPerPixel, Uint8 * image_buffer, Uint8 * & to, Uint32 & length, EP_Representation pixel_rep)
{
	//return encode( columns, rows, colorSpace, samplesPerPixel, (Uint8*) image_buffer, to, length);
	OPJSupport* supp = new OPJSupport();
        int planarConfig = cparam->getPlanarConfiguration();
        //E_DecompressionColorSpaceConversion cc = cparam->getDecompressionColorSpaceConversion();  // { ECC_lossyYCbCr, ECC_lossyRGB, ECC_monochrome }
        unsigned long compressedSize;
	void* res = supp->compressJPEG2K(image_buffer, bitsPerSampleValue, columns, rows, samplesPerPixel, planarConfig, photometric, pixel_rep, (modeofOperation == EJM_JP2K_lossless)?true:false, compressedSize);
	if (!res)
		return EC_MemoryExhausted;
        to = (Uint8*)res;
        length = compressedSize;
	delete supp;
	return EC_Normal;
}

OFCondition DJCompressJP2K::encode( Uint16  columns , Uint16  rows , EP_Interpretation  photometric , Uint16  samplesPerPixel , Uint16 *  image_buffer , Uint8 *&  to , Uint32 &  length, EP_Representation pixel_rep)
{
	OPJSupport* supp = new OPJSupport();
        int planarConfig = cparam->getPlanarConfiguration();
        //E_DecompressionColorSpaceConversion cc = cparam->getDecompressionColorSpaceConversion();  // { ECC_lossyYCbCr, ECC_lossyRGB, ECC_monochrome }
        unsigned long compressedSize;
	void* res = supp->compressJPEG2K(image_buffer, bitsPerSampleValue, columns, rows, samplesPerPixel, planarConfig, photometric, pixel_rep, (modeofOperation == EJM_JP2K_lossless)?true:false, compressedSize);
	if (!res)
		return EC_MemoryExhausted;
        to = (Uint8*)res;
        length = compressedSize;
	delete supp;
	return EC_Normal;
}

Uint16 DJCompressJP2K::bytesPerSample() const
{
	if( bitsPerSampleValue <= 8)
		return 1;
	else
		return 2;
}

Uint16 DJCompressJP2K::bitsPerSample() const
{
	return bitsPerSampleValue;
}
