#include "dcmtk/dcmimgle/diutils.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmjpeg/OPJSupport.h"
#include "dcmtk/dcmdata/dcxfer.h"
#include "openjpeg.h"

#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2

#include <memory.h>
#include <stdlib.h>

#include "color.c"


// These routines are added to use memory instead of a file for input and output.
//Structure need to treat memory as a stream.

typedef struct
{
        OPJ_UINT8* pData; //Our data.
        OPJ_SIZE_T dataSize; //How big is our data.
        OPJ_SIZE_T offset; //Where are we currently in our data.
} opj_memory_stream;

//This will read from our memory to the buffer.
static OPJ_SIZE_T opj_memory_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
        opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;//Our data.
        OPJ_SIZE_T l_nb_bytes_read = p_nb_bytes;  //Amount to move to buffer.
           //Check if the current offset is outside our data buffer.
        if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T)-1;
            //Check if we are reading more than we have.
        if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset))
                l_nb_bytes_read = l_memory_stream->dataSize - l_memory_stream->offset;//Read all we have.
           //Copy the data to the internal buffer.
        memcpy(p_buffer, &(l_memory_stream->pData[l_memory_stream->offset]), l_nb_bytes_read);
        l_memory_stream->offset += l_nb_bytes_read;//Update the pointer to the new location.
        return l_nb_bytes_read;
}

//This will write from the buffer to our memory.
static OPJ_SIZE_T opj_memory_stream_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
        opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;//Our data.
        OPJ_SIZE_T l_nb_bytes_write = p_nb_bytes;//Amount to move to buffer.
           //Check if the current offset is outside our data buffer.
        if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T)-1;
           //Check if we are write more than we have space for.
        if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset))
            l_nb_bytes_write = l_memory_stream->dataSize - l_memory_stream->offset;//Write the remaining space.
           //Copy the data from the internal buffer.
        memcpy(&(l_memory_stream->pData[l_memory_stream->offset]), p_buffer, l_nb_bytes_write);
        l_memory_stream->offset += l_nb_bytes_write;//Update the pointer to the new location.
        return l_nb_bytes_write;
}

//Moves the pointer forward, but never more than we have.
static OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{       opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;
        OPJ_SIZE_T l_nb_bytes;

        if (p_nb_bytes < 0) return -1;//No skipping backwards.
        l_nb_bytes = (OPJ_SIZE_T)p_nb_bytes;//Allowed because it is positive.
            // Do not allow jumping past the end.
        if (l_nb_bytes >l_memory_stream->dataSize - l_memory_stream->offset)
            l_nb_bytes = l_memory_stream->dataSize - l_memory_stream->offset;//Jump the max.

           //Make the jump.
        l_memory_stream->offset += l_nb_bytes;
            //Returm how far we jumped.
        return l_nb_bytes;
}

//Sets the pointer to anywhere in the memory.
static OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
        opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;

        if (p_nb_bytes < 0) return OPJ_FALSE;//No before the buffer.
        if (p_nb_bytes >(OPJ_OFF_T)l_memory_stream->dataSize) return OPJ_FALSE;//No after the buffer.
        l_memory_stream->offset = (OPJ_SIZE_T)p_nb_bytes;//Move to new position.
        return OPJ_TRUE;
}

//The system needs a routine to do when finished, the name tells you what I want it to do.
static void opj_memory_stream_do_nothing(void * p_user_data)
{    OPJ_ARG_NOT_USED(p_user_data); }

//Create a stream to use memory as the input or output.
//opj_stream_t* opj_stream_create_buffer_stream(opj_buffer_info_t* p_source_buffer, OPJ_BOOL p_is_read_stream)
opj_stream_t* opj_stream_create_default_memory_stream(opj_memory_stream* p_memoryStream, OPJ_BOOL p_is_read_stream)
{ 
        opj_stream_t* l_stream;

        if (!(l_stream = opj_stream_default_create(p_is_read_stream))) return (NULL);
            //Set how to work with the frame buffer.
        if(p_is_read_stream)
                opj_stream_set_read_function(l_stream, opj_memory_stream_read);
        else
                opj_stream_set_write_function(l_stream, opj_memory_stream_write);
        opj_stream_set_seek_function(l_stream, opj_memory_stream_seek);
        opj_stream_set_skip_function(l_stream, opj_memory_stream_skip);
        opj_stream_set_user_data(l_stream, p_memoryStream, opj_memory_stream_do_nothing);
        opj_stream_set_user_data_length(l_stream, p_memoryStream->dataSize);
        return l_stream;
}

opj_stream_t* opj_stream_create_memory_stream(opj_memory_stream* p_mem,OPJ_UINT32 p_size, bool p_is_read_stream)
{
        opj_stream_t* l_stream = NULL;
        if (! p_mem)
                return NULL;

        l_stream = opj_stream_create(p_size,p_is_read_stream);
        if (!l_stream)
                return NULL;

        opj_stream_set_user_data(l_stream, p_mem, NULL);
        opj_stream_set_user_data_length(l_stream, p_mem->dataSize);
        opj_stream_set_read_function(l_stream,(opj_stream_read_fn) opj_memory_stream_read);
        opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_memory_stream_write);
        opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_memory_stream_skip);
        opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_memory_stream_seek);
        return l_stream;
}


//**************************************************  DECODE SUPPORT ***********************************************************/

typedef struct decode_info
{
        opj_codec_t *codec;
        opj_stream_t *stream;
        opj_image_t *image;
        OPJ_BOOL   deleteImage;

} decode_info_t;

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"


int stream_format(opj_memory_stream* stream_info)
{
        int magic_format;
        if (!stream_info || stream_info->dataSize < 12)
                return -1;
        if(memcmp(stream_info->pData, JP2_RFC3745_MAGIC, 12) == 0 || memcmp(stream_info->pData, JP2_MAGIC, 4) == 0)
        {
                magic_format = JP2_CFMT;
        }
        else
        {
                if(memcmp(stream_info->pData, J2K_CODESTREAM_MAGIC, 4) == 0)
                {
                        magic_format = J2K_CFMT;
                }
                else
                        return -1;
        }
        return magic_format;
}/*  stream_format() */

const char *clr_space(OPJ_COLOR_SPACE i)
{
        if(i == OPJ_CLRSPC_SRGB) return "OPJ_CLRSPC_SRGB";
        if(i == OPJ_CLRSPC_GRAY) return "OPJ_CLRSPC_GRAY";
        if(i == OPJ_CLRSPC_SYCC) return "OPJ_CLRSPC_SYCC";
        if(i == OPJ_CLRSPC_UNKNOWN) return "OPJ_CLRSPC_UNKNOWN";
        return "CLRSPC_UNDEFINED";
}



void release(decode_info_t *decodeInfo)
{

        if(decodeInfo->codec)
        {
                opj_destroy_codec(decodeInfo->codec);
                decodeInfo->codec = NULL;
        }

        if(decodeInfo->stream)
        {
                opj_stream_destroy(decodeInfo->stream);
                decodeInfo->stream = NULL;
        }

        if(decodeInfo->deleteImage && decodeInfo->image)
        {
                opj_image_destroy(decodeInfo->image);
                decodeInfo->image = NULL;

        }
}

static int copyUint32ToUint8(opj_image_t * image, uint8_t *imageFrame, uint16_t columns, uint16_t rows)
{
  if (imageFrame == NULL) return -1;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return -1;

  uint8_t *t = imageFrame;          // target
  OPJ_INT32 *g = image->comps[0].data;   // grey plane  
  for (unsigned long i=numPixels; i; i--)
        *t++ = *g++;

  return 0;
}

int copyUint32ToUint16( opj_image_t * image, uint16_t *imageFrame, uint16_t columns, uint16_t rows)
{
  if (imageFrame == NULL) return -1;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return -1;

  uint16_t *t = imageFrame;          // target
  OPJ_INT32 *g = image->comps[0].data;   // grey plane  
  for (unsigned long i=numPixels; i; i--)
        *t++ = *g++;

  return 0;
}


int copyRGBUint8ToRGBUint8( opj_image_t * image, uint8_t *imageFrame, uint16_t columns, uint16_t rows)
{
  if (imageFrame == NULL) return -1;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return -1;

  uint8_t *t = imageFrame;          // target
  OPJ_INT32 *r = image->comps[0].data; // red plane
  OPJ_INT32 *g = image->comps[1].data; // green plane
  OPJ_INT32 *b = image->comps[2].data; // blue plane
  for (unsigned long i=numPixels; i; i--)
  {
    *t++ = *r++;
    *t++ = *g++;
    *t++ = *b++;
  }

  return 0;
}


int copyRGBUint8ToRGBUint8Planar( opj_image_t * image, uint8_t *imageFrame, uint16_t columns, uint16_t rows)
{
  if (imageFrame == NULL) return -1;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return -1;

  uint8_t *t = imageFrame;          // target
  for(unsigned long j=0; j<3; j++)
  {
        OPJ_INT32 *r = image->comps[j].data; // color plane  
        for (unsigned long i=numPixels; i; i--)
          *t++ = *r++;
  }
  return 0;
}



OPJSupport::OPJSupport(){ }

OPJSupport::~OPJSupport(){ }

void* OPJSupport::decompressJPEG2K( void* jp2Data, long jp2DataSize, long *decompressedBufferSize, int planarConfig, int *colorModel)
{
    return decompressJPEG2KWithBuffer(NULL, jp2Data, jp2DataSize, decompressedBufferSize, planarConfig, colorModel);
}

void* OPJSupport::decompressJPEG2KWithBuffer(void* inputBuffer, void* jp2Data, long jp2DataSize, long *decompressedBufferSize, int planarConfig, int *colorModel)
{
        opj_dparameters_t parameters;
        OPJ_BOOL hasFile = OPJ_FALSE;

        opj_memory_stream stream_info;
        int i, decod_format;
        int width, height;
        OPJ_BOOL hasAlpha, fails = OPJ_FALSE;
        OPJ_CODEC_FORMAT codec_format;
        unsigned char rc, gc, bc, ac;

        decode_info_t decodeInfo;

        memset(&decodeInfo, 0, sizeof(decode_info_t));
        memset(&stream_info, 0, sizeof(opj_memory_stream));

        if (jp2Data != NULL)
        {
                stream_info.dataSize = jp2DataSize;
                stream_info.pData =  (OPJ_UINT8*)jp2Data;
                stream_info.offset = 0;
        }

        opj_set_default_decoder_parameters(&parameters);
        decod_format = stream_format(&stream_info);

        if(decod_format == -1)
        {
                fprintf(stderr,"%s:%d: decode format missing\n",__FILE__,__LINE__);
                release(&decodeInfo);
                return 0;
        }

        /*-----------------------------------------------*/
        if(decod_format == J2K_CFMT)
                codec_format = OPJ_CODEC_J2K;
        else if(decod_format == JP2_CFMT)
                codec_format = OPJ_CODEC_JP2;
        else if(decod_format == JPT_CFMT)
                codec_format = OPJ_CODEC_JPT;
        else
        {
                /* clarified in infile_format() : */
                release(&decodeInfo);
                return 0;
        }
        parameters.decod_format = decod_format;

        decodeInfo.stream =  opj_stream_create_default_memory_stream(&stream_info, 1);

        if(decodeInfo.stream == NULL)
        {
           fprintf(stderr,"%s:%d: NO decodeInfo.stream\n",__FILE__,__LINE__);
           fails = true;
        }
        if (!fails)
        {   decodeInfo.codec = opj_create_decompress(codec_format);
            if(decodeInfo.codec == NULL)
            {
                fprintf(stderr,"%s:%d: NO coded\n",__FILE__,__LINE__);
                fails = true;
            }
        }
        if (!fails)
        {
              //TODO uncomment out
           //opj_set_info_handler(decodeInfo.codec, error_callback, this);
           //opj_set_info_handler(decodeInfo.codec, warning_callback, this);
           //opj_set_info_handler(decodeInfo.codec, info_callback, this);

           if( !opj_setup_decoder(decodeInfo.codec, &parameters))
           {
               fprintf(stderr,"%s:%d:\n\topj_setup_decoder failed\n",__FILE__,__LINE__);
               fails = true;
           }
        }
        if (!fails)
        {
           if( !opj_read_header(decodeInfo.stream, decodeInfo.codec, &decodeInfo.image))
           {
               fprintf(stderr,"%s:%d:\n\topj_read_header failed\n",__FILE__,__LINE__);
               fails = true;
           }
        }


        if (!fails)
        {  if (!opj_set_decode_area(decodeInfo.codec, decodeInfo.image, (OPJ_INT32)parameters.DA_x0, (OPJ_INT32)parameters.DA_y0, (OPJ_INT32)parameters.DA_x1, (OPJ_INT32)parameters.DA_y1))
           {
                   fprintf(stderr,"%s:%d:\n\topj_set_decode_area failed\n",__FILE__,__LINE__);
                   fails = true;
           }
        }

        if (!fails)
        {  if (!(opj_decode(decodeInfo.codec, decodeInfo.stream, decodeInfo.image) && opj_end_decompress(decodeInfo.codec, decodeInfo.stream))) 
           {
                   fprintf(stderr,"%s:%d:\n\topj_decode failed\n",__FILE__,__LINE__);
                   fails = true;
           }  
        }
        decodeInfo.deleteImage = fails;
        release(&decodeInfo);
        if(fails)
        {
            return 0;
        }
        decodeInfo.deleteImage = OPJ_TRUE;

        if(decodeInfo.image->color_space != OPJ_CLRSPC_SYCC
           && decodeInfo.image->numcomps == 3
           && decodeInfo.image->comps[0].dx == decodeInfo.image->comps[0].dy
           && decodeInfo.image->comps[1].dx != 1) 
               decodeInfo.image->color_space = OPJ_CLRSPC_SYCC;
        else if(decodeInfo.image->numcomps <= 2)
               decodeInfo.image->color_space = OPJ_CLRSPC_GRAY;

        if(decodeInfo.image->color_space == OPJ_CLRSPC_SYCC)
        {
           color_sycc_to_rgb(decodeInfo.image);
        }

        if(decodeInfo.image->icc_profile_buf) 
        {
#if defined(OPJ_HAVE_LIBLCMS1) || defined(OPJ_HAVE_LIBLCMS2)
            if(image->icc_profile_len)
                    color_apply_icc_profile(image);
            else
                    color_cielab_to_rgb(image);
#endif
            free(decodeInfo.image->icc_profile_buf);
            decodeInfo.image->icc_profile_buf = NULL; decodeInfo.image->icc_profile_len = 0;
       }

       width = decodeInfo.image->comps[0].w;
       height = decodeInfo.image->comps[0].h;

       long depth = (decodeInfo.image->comps[0].prec + 7)/8;
       long decompressSize = width * height * decodeInfo.image->numcomps * depth;
       if (decompressedBufferSize)
            *decompressedBufferSize = decompressSize;

       if (!inputBuffer ) 
            inputBuffer =  malloc(decompressSize);

       if (colorModel)
           *colorModel = 0;

          // copy the image depending on planer configuration and bits
       if(decodeInfo.image->numcomps == 1)      // Greyscale
       {
           if(decodeInfo.image->comps[0].prec <= 8)
              copyUint32ToUint8(decodeInfo.image, (uint8_t*)inputBuffer, width, height);
           if(decodeInfo.image->comps[0].prec > 8)
              copyUint32ToUint16(decodeInfo.image, (uint16_t*)inputBuffer, width, height);
       }
       else if(decodeInfo.image->numcomps == 3)
       {
           if(planarConfig == 0)
               copyRGBUint8ToRGBUint8(decodeInfo.image, (uint8_t*)inputBuffer, width, height);
           else if(planarConfig == 1)
               copyRGBUint8ToRGBUint8Planar(decodeInfo.image, (uint8_t*)inputBuffer, width, height);
       }


       release(&decodeInfo);
       return inputBuffer;
}

//*********************************************************   ENCODE SUPPORT  *******************************************************/


int frametoimage(const Uint8 *framePointer, int planarConfiguration, EP_Interpretation photometricInterpretation, int samplesPerPixel, int width, int height, int bitsAllocated, OFBool isSigned, opj_cparameters_t *parameters, opj_image_t **ret_image)
{
        Uint8 bytesAllocated = bitsAllocated / 8;    
        // Uint32 PixelWidth = bytesAllocated * samplesPerPixel;

        if (bitsAllocated % 8 != 0)
                return -1;

        opj_image_t *image = NULL;
        opj_image_cmptparm_t *cmptparm = new opj_image_cmptparm_t[samplesPerPixel];
        memset(cmptparm, 0, samplesPerPixel * sizeof(opj_image_cmptparm_t));

        int subsampling_dx = parameters->subsampling_dx;
        int subsampling_dy = parameters->subsampling_dy;

        for (int i = 0; i < samplesPerPixel; i++)
        {
                cmptparm[i].prec = bitsAllocated;
                cmptparm[i].bpp = bitsAllocated;
                cmptparm[i].sgnd = (isSigned) ? 1 : 0;
                cmptparm[i].dx = (OPJ_UINT32)subsampling_dx;
                cmptparm[i].dy = (OPJ_UINT32)subsampling_dy;
                cmptparm[i].w = width;
                cmptparm[i].h = height;
        }

// PhotometricInterpretation:  EPI_Unknown, EPI_Missing, EPI_Monochrome1, EPI_Monochrome2, EPI_PaletteColor, EPI_RGB, EPI_HSV, EPI_ARGB, EPI_CMYK, EPI_YBR_Full, EPI_YBR_Full_422, EPI_YBR_Partial_422 
        COLOR_SPACE colorspace = OPJ_CLRSPC_UNSPECIFIED;
        if(photometricInterpretation == EPI_Monochrome1 || photometricInterpretation == EPI_Monochrome2)
                colorspace = OPJ_CLRSPC_GRAY;
        else if(photometricInterpretation == EPI_RGB)
                colorspace = OPJ_CLRSPC_SRGB;
        else if(photometricInterpretation == EPI_YBR_Full || photometricInterpretation == EPI_YBR_Full_422) 
                colorspace = OPJ_CLRSPC_SYCC;
        else
             return -1;

        image = opj_image_create(samplesPerPixel, cmptparm, colorspace);
        
        // set image offset and reference grid
        image->x0 = (OPJ_UINT32)parameters->image_offset_x0;
        image->y0 = (OPJ_UINT32)parameters->image_offset_y0;
        image->x1 = !image->x0 ? (OPJ_UINT32)(width - 1)  * (OPJ_UINT32)subsampling_dx + 1 : image->x0 + (OPJ_UINT32)(width - 1)  * (OPJ_UINT32)subsampling_dx + 1;
        image->y1 = !image->y0 ? (OPJ_UINT32)(height - 1) * (OPJ_UINT32)subsampling_dy + 1 : image->y0 + (OPJ_UINT32)(height - 1) * (OPJ_UINT32)subsampling_dy + 1;                

        if(bytesAllocated == 1)
        {
                if(planarConfiguration == 0)
                {
                        for(int i = 0; i < samplesPerPixel; i++)
                        {                        
                                OPJ_INT32 *target = image->comps[i].data;
                                const Uint8 *source = &framePointer[i * bytesAllocated];
                                for (unsigned int pos = 0; pos < height * width; pos++)
                                {
                                        *target = (isSigned) ? (Sint8) *source : *source;
                                        target++;
                                        source += samplesPerPixel;
                                }            
                        }
                }
                else
                {
                        for(int i = 0; i < samplesPerPixel; i++)
                        {                        
                                OPJ_INT32 *target = image->comps[i].data;
                                const Uint8 *source = &framePointer[i * height * width * bytesAllocated];
                                for (unsigned int pos = 0; pos < height * width; pos++)
                                {
                                        *target =  (isSigned) ? (Sint8) *source : *source;
                                        target++;
                                        source++;
                                }            
                        }
                }
        }
        else if(bytesAllocated == 2)
        {                
                if(planarConfiguration == 0)
                {                
                        for(int i = 0; i < samplesPerPixel; i++)
                        {
                                OPJ_INT32 *target = image->comps[i].data;
                                const Uint16 *source = (Uint16 *) &framePointer[i * bytesAllocated];
                                for (unsigned int pos = 0; pos < height * width; pos++)
                                {
                                        *target =  (isSigned) ? (Sint16) *source : *source;
                                        target++;
                                        source += samplesPerPixel;
                                }            
                        }
                }
                else
                {
                        for(int i = 0; i < samplesPerPixel; i++)
                        {                        
                                OPJ_INT32 *target = image->comps[i].data;
                                const Uint16 *source = (Uint16 *) &framePointer[i * height * width * bytesAllocated];
                                for (unsigned int pos = 0; pos < height * width; pos++)
                                {
                                        *target =  (isSigned) ? (Sint16) *source : *source;
                                        target++;
                                        source++;
                                }            
                        }
                }
        }
        else if(bytesAllocated == 4)
        {
                if(planarConfiguration == 0)
                {                
                        for(int i = 0; i < samplesPerPixel; i++)
                    {
                                OPJ_INT32 *target = image->comps[i].data;
                                const Uint32 *source = (Uint32 *) &framePointer[i * bytesAllocated];
                                for (unsigned int pos = 0; pos < height * width; pos++)
                                {                                        
                                        *target =  (isSigned) ? (Sint32) *source : *source;
                                        target++;
                                        source += samplesPerPixel;
                                }            
                        }
                }
                else
                {
                        for(int i = 0; i < samplesPerPixel; i++)
                    {                        
                                OPJ_INT32 *target = image->comps[i].data;
                                const Uint32 *source = (Uint32 *) &framePointer[i * height * width * bytesAllocated];
                                for (unsigned int pos = 0; pos < height * width; pos++)
                                {
                                        *target =  (isSigned) ? (Sint32) *source : *source;
                                        target++;
                                        source++;
                                }            
                        }
                }
        }
        delete [] cmptparm;

        *ret_image = image;
        return 0;

}

void* OPJSupport::compressJPEG2K(void* data, int bitsAllocated, int columns, int rows, int samplesPerPixel, int planarConfig, EP_Interpretation photometric, EP_Representation pixelRepresentation, bool lossless, long unsigned int& compressedSize)
{
        const Uint8 *framePointer = (Uint8*)data;
        OFCondition result = EC_Normal;
        Uint16 bytesAllocated = bitsAllocated / 8;
        Uint32 frameSize = columns*rows*bytesAllocated*samplesPerPixel;
        opj_cparameters_t parameters;  
        opj_image_t *image = NULL; 
        Uint8 *buffer = NULL;

        opj_set_default_encoder_parameters(&parameters);
                        
        int r= frametoimage(framePointer, planarConfig, photometric, samplesPerPixel, columns, rows, bitsAllocated, pixelRepresentation, &parameters, &image);
                
        if (r == 0)
        {
                // set lossless
                parameters.tcp_numlayers = 1;
                parameters.tcp_rates[0] = 0;                        
                parameters.cp_disto_alloc = 1;
                parameters.tcp_mct = 0;

                size_t size = frameSize + 1024;  //**more space the we could possibly want
                buffer = new Uint8[size];

                // Set up the information structure for OpenJPEG
                opj_stream_t *l_stream = NULL;
                opj_codec_t* l_codec = NULL;
                l_codec = opj_create_compress(OPJ_CODEC_J2K);

                //opj_set_info_handler(l_codec, msg_callback, NULL);
                //opj_set_warning_handler(l_codec, msg_callback, NULL);
                //opj_set_error_handler(l_codec, msg_callback, NULL);

                if (!opj_setup_encoder(l_codec, &parameters, image) )
                {  
                        opj_destroy_codec(l_codec);      
                        l_codec = NULL;
                        result = EC_MemoryExhausted;
                }

                opj_memory_stream mstream;
                mstream.pData = buffer;  mstream.dataSize = size;  mstream.offset = 0;
                l_stream = opj_stream_create_memory_stream(&mstream, size, OPJ_FALSE);

                if(!opj_start_compress(l_codec,image,l_stream))
                {
                        result = EC_CorruptedData;
                }

                if(result.good() && !opj_encode(l_codec, l_stream))
                {
                        result = EC_InvalidStream;
                }

                if(result.good() && opj_end_compress(l_codec, l_stream))
                {
                        result = EC_Normal;
                }

                opj_stream_destroy(l_stream); l_stream = NULL;
                opj_destroy_codec(l_codec); l_codec = NULL;
                opj_image_destroy(image); image = NULL;

                size = mstream.offset;

                if (result.good())
                {
                        // 'size' now contains the size of the compressed data in buffer
                        compressedSize = size;
                }

        }  
        else result = EC_IllegalCall;

        if (result == EC_Normal)
            return buffer;
        else
        {  if (buffer != NULL)
             delete[] buffer;

           return NULL;
        }
}
