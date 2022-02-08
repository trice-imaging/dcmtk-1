#define TRICE 1
/*
 *
 *  Copyright (C) 2007-2013, OFFIS e.V.
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation were developed by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module:  dcmdata
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Implements utility for converting standard image formats to DICOM
 *
 */

#ifdef TRICE
#include <sys/file.h>
#include <iostream>
#include <fstream>
#include <trice_int.h>
#ifdef _WIN32
#undef close
#undef sleep
const char* delimiter="\\";
const char* lineTerm="\r\n";
#else
const char* delimiter="/";
const char* lineTerm="\n";
#endif
#endif

#include "dcmtk/dcmdata/dctk.h"          /* for various dcmdata headers */

#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/libi2d/i2d.h"
#include "dcmtk/dcmdata/libi2d/i2djpgs.h"
#include "dcmtk/dcmdata/libi2d/i2dbmps.h"
#include "dcmtk/dcmdata/libi2d/i2dplsc.h"
#include "dcmtk/dcmdata/libi2d/i2dplvlp.h"
#include "dcmtk/dcmdata/libi2d/i2dplnsc.h"

#define OFFIS_CONSOLE_APPLICATION "img2dcm"
static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v" OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

#define SHORTCOL 4
#define LONGCOL 21

static bool trice = true;  //TRICE
static bool use_files = false;  //Use jpg files from first part of conversion
static bool uplink = false;  //TRICE/uplink replaces pixel data
static bool uplink2 = false;  //TRICE/uplink version that retains the pixel data
static OFString dcmInputFileName = "";
static bool mp4 = false;
static int numberFrames = 0;
static OFString pixDataFile = "";
static OFString pixDataFile1 = "";
static OFString currentDir = "";
static OFString  opt_processDir = "";              // Where to find processes - needed for windows
static const char* dicomPath="";

static OFLogger img2dcmLogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_APPLICATION);

#include "trice_dicom.cc"

static void cleanup()
{
    OFString dir = currentDir;
        //**Remove temp files
#ifdef __windows__
    int rc = _chdir((char*)dir.c_str());
#else
    int rc = chdir((char*)dir.c_str());
#endif

    if (rc == 0)
#ifndef __windows__
       rc = system((char*)"rm -f FrameConvert.f*.raw triceTemplate.dcm* triceDUMP.txt *.OUT FRAME.*.jpg *.rawNOTPIXEL");
#else
       rc = system((char*)"del /q /F FrameConvert.f*.raw triceTemplate.dcm* triceDUMP.txt *.OUT FRAME.*.jpg *.rawNOTPIXEL 2> NUL");
#endif
}

static OFCondition evaluateFromFileOptions(OFCommandLine& cmd, Image2Dcm& converter)
{
  OFCondition cond;
  // Parse command line options dealing with DICOM file import
  if ( cmd.findOption("--dataset-from") )
  {
    OFString tempStr;
    OFCommandLine::E_ValueStatus valStatus;
    valStatus = cmd.getValue(tempStr);
    if (valStatus != OFCommandLine::VS_Normal)
        return makeOFCondition(OFM_dcmdata, 18, OF_error, "Unable to read value of --dataset-from option");
    converter.setTemplateFile(tempStr);
    tempStr.append(".rawNOTPIXEL");  //**Look for updated template with TRICEFY_PRIVATE fields
    if (nonZeroFile((char*)tempStr.c_str()))
       converter.setTemplateFile(tempStr);
  }

  if (cmd.findOption("--study-from"))
  {
    OFString tempStr;
    OFCommandLine::E_ValueStatus valStatus;
    valStatus = cmd.getValue(tempStr);
    if (valStatus != OFCommandLine::VS_Normal)
      return makeOFCondition(OFM_dcmdata, 18, OF_error, "Unable to read value of --study-from option");
    converter.setStudyFrom(tempStr);
  }

  if (cmd.findOption("--series-from"))
  {
    OFString tempStr;
    OFCommandLine::E_ValueStatus valStatus;
    valStatus = cmd.getValue(tempStr);
    if (valStatus != OFCommandLine::VS_Normal)
      return makeOFCondition(OFM_dcmdata, 18, OF_error, "Unable to read value of --series-from option");
    converter.setSeriesFrom(tempStr);
  }

  if (cmd.findOption("--instance-inc"))
    converter.setIncrementInstanceNumber(OFTrue);

  // Return success
  return EC_Normal;
}


static void addCmdLineOptions(OFCommandLine& cmd)
{
  cmd.addParam("imgfile-in",  "image input filename");
  cmd.addParam("dcmfile-out", "DICOM output filename");

  cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
    cmd.addOption("--help",                  "-h",      "print this help text and exit", OFCommandLine::AF_Exclusive);
    cmd.addOption("--version",                          "print version information and exit", OFCommandLine::AF_Exclusive);
    OFLog::addOptions(cmd);

  cmd.addGroup("input options:", LONGCOL, SHORTCOL + 2);
    cmd.addSubGroup("general:");
      cmd.addOption("--input-format",        "-i",   1, "[i]nput file format: string", "supported formats: JPEG (default), BMP, MP4");
      cmd.addOption("--dataset-from",        "-df",  1, "[f]ilename: string",
                                                        "use dataset from DICOM file f");
      cmd.addOption("--study-from",          "-stf", 1, "[f]ilename: string",
                                                        "read patient/study from DICOM file f");
      cmd.addOption("--series-from",         "-sef", 1, "[f]ilename: string",
                                                        "read patient/study/series from DICOM file f");
      cmd.addOption("--instance-inc",        "-ii",     "increase instance number read from DICOM file");
    cmd.addSubGroup("JPEG format:");
      cmd.addOption("--disable-progr",       "-dp",     "disable support for progressive JPEG");
      cmd.addOption("--disable-ext",         "-de",     "disable support for extended sequential JPEG");
      cmd.addOption("--insist-on-jfif",      "-jf",     "insist on JFIF header");
      cmd.addOption("--keep-appn",           "-ka",     "keep APPn sections (except JFIF)");

  cmd.addGroup("processing options:", LONGCOL, SHORTCOL + 2);
    cmd.addSubGroup("attribute checking:");
      cmd.addOption("--do-checks",                      "enable attribute validity checking (default)");
      cmd.addOption("--no-checks",                      "disable attribute validity checking");
      cmd.addOption("--insert-type2",        "+i2",     "insert missing type 2 attributes (default)\n(only with --do-checks)");
      cmd.addOption("--no-type2-insert",     "-i2",     "do not insert missing type 2 attributes \n(only with --do-checks)");
      cmd.addOption("--invent-type1",        "+i1",     "invent missing type 1 attributes (default)\n(only with --do-checks)");
      cmd.addOption("--no-type1-invent",     "-i1",     "do not invent missing type 1 attributes\n(only with --do-checks)");
    cmd.addSubGroup("character set:");
      cmd.addOption("--latin1",              "+l1",     "set latin-1 as standard character set (default)");
      cmd.addOption("--no-latin1",           "-l1",     "keep 7-bit ASCII as standard character set");
    cmd.addSubGroup("other processing options:");
      cmd.addOption("--key",                 "-k",   1, "[k]ey: gggg,eeee=\"str\", path or dict. name=\"str\"",
                                                        "add further attribute");

  cmd.addGroup("output options:");
    cmd.addSubGroup("target SOP class:");
      cmd.addOption("--sec-capture",         "-sc",     "write Secondary Capture SOP class (default)");
      cmd.addOption("--new-sc",              "-nsc",    "write new Secondary Capture SOP classes");
      cmd.addOption("--vl-photo",            "-vlp",    "write Visible Light Photographic SOP class");

    cmd.addSubGroup("output file format:");
      cmd.addOption("--write-file",          "+F",      "write file format (default)");
      cmd.addOption("--write-dataset",       "-F",      "write data set without file meta information");
    cmd.addSubGroup("group length encoding:");
      cmd.addOption("--group-length-recalc", "+g=",     "recalculate group lengths if present (default)");
      cmd.addOption("--group-length-create", "+g",      "always write with group length elements");
      cmd.addOption("--group-length-remove", "-g",      "always write without group length elements");
    cmd.addSubGroup("length encoding in sequences and items:");
      cmd.addOption("--length-explicit",     "+e",      "write with explicit lengths (default)");
      cmd.addOption("--length-undefined",    "-e",      "write with undefined lengths");
    cmd.addSubGroup("data set trailing padding (not with --write-dataset):");
      cmd.addOption("--padding-off",         "-p",      "no padding (implicit if --write-dataset)");
      cmd.addOption("--padding-create",      "+p",   2, "[f]ile-pad [i]tem-pad: integer", "");
//TRICE
    cmd.addSubGroup("Trice Options:");
      cmd.addOption("--meta-json",         "-mj",   1, "[f]file containing the json blob with the metadata attributes", "");
      cmd.addOption("--meta-xml",          "-mx",   1, "[f]file containing the xml with the metadata attributes", "");
//**Read in the JSON, build internal dataset, write to a temp area (scratch), and treat like --dataset-from
      cmd.addOption("--trice",             "+tr",      "trice - will default some things (--sec-capture, +g, --dataset-from, charset)");
      cmd.addOption("--use-files",         "+fi",      "If multiframe, jpg files are coming in - don't use files from .mp4");
      cmd.addOption("--uplink",            "+up",      "uplink - will use TriceConsumerFormat private tag and have no pixel data)");
      cmd.addOption("--uplink2",           "+up2",     "uplink - will use TriceConsumerFormat private tag and have full pixel data)");
      cmd.addOption("--process-dir",       "-pr",  1, "[p]ath: string", "where to find the processes needed (dcmdump, dump2dcm)");
      cmd.addOption("--dict-path",         "-dp",  1, "[p]ath: string", "complete path (including file name) for dicom.dic");
      cmd.addOption("--help", "-?", "img2dcm .  This is the command that will convert any .jpg/.mp4 to a DCM for download.  It takes an .xml file (for meta) or an optional DICOM file that contains the meta. \n \
 \n \
 * img2dcm --trice --meta-xml <xml file> [ or -mj <json file w meta> or -df <DICOM file w meta> ]  inputFile  outputFile \n \
    * The --trice arg sets all the right options to keep the command simple.   \n \
    * Meta comes through --meta-xml (or optionally through -mj/-df if that is more convenient) \n \
    * Return info \n \
        * Command exit(0) if successful.  Any error info will come via stderr\n");
}


static OFCondition startConversion(OFCommandLine& cmd, int argc, char *argv[])
{
  // Parse command line and exclusive options
  prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, "Convert standard image formats into DICOM format", rcsid);
  if (app.parseCommandLine(cmd, argc, argv))
  {
    /* check exclusive options first */
    if (cmd.hasExclusiveOption())
    {
      if (cmd.findOption("--version"))
      {
        app.printHeader(OFTrue /*print host identifier*/);
        exit(0);
      }
    }
  }

  /* print resource identifier */
  OFLOG_DEBUG(img2dcmLogger, rcsid << OFendl);

  // Main class for controlling conversion
  Image2Dcm i2d;
  // Output plugin to use (i.e. SOP class to write)
  I2DOutputPlug *outPlug = NULL;
  // Input plugin to use (i.e. file format to read)
  I2DImgSource *inputPlug = NULL;
  // Group length encoding mode for output DICOM file
  E_GrpLenEncoding grpLengthEnc = EGL_recalcGL;
  // Item and Sequence encoding mode for output DICOM file
  E_EncodingType lengthEnc = EET_ExplicitLength;
  // Padding mode for output DICOM file
  E_PaddingEncoding padEnc = EPD_noChange;
  // File pad length for output DICOM file
  OFCmdUnsignedInt filepad = 0;
  // Item pad length for output DICOM file
  OFCmdUnsignedInt itempad = 0;
  // Write only pure dataset, i.e. without meta header
  E_FileWriteMode writeMode = EWM_fileformat;
  // Override keys are applied at the very end of the conversion "pipeline"
  OFList<OFString> overrideKeys;
  // The transfer syntax proposed to be written by output plugin
  E_TransferSyntax writeXfer;

  // Parse rest of command line options
  OFLog::configureFromCommandLine(cmd, app);

  OFString outputFile, tempStr;
  cmd.getParam(1, tempStr);  //**Image file

  if (tempStr.empty())
  {
    OFLOG_ERROR(img2dcmLogger, "No image input filename specified");
    cleanup(); return EC_IllegalCall;
  }
  else
  {   pixDataFile = tempStr;
      char dir[strlen(pixDataFile.c_str())+1];   strcpy(dir, pixDataFile.c_str());
      char* slash1 = strrchr(dir, '/');
      char* slash2 = strrchr(dir, '\\');
      char* slash = (slash1>slash2)?slash1:slash2;
      if (slash == NULL)
         currentDir = ".";
      else
      {  *slash = '\0';
         currentDir = dir;
      }
  }

  cmd.getParam(2, tempStr);
  if (tempStr.empty())
  {
    OFLOG_ERROR(img2dcmLogger, "No DICOM output filename specified");
    cleanup(); return EC_IllegalCall;
  }
  else
    outputFile = tempStr;

  if ( cmd.findOption("--uplink") )
       uplink = trice = true;
  if ( cmd.findOption("--uplink2") )
       uplink2 = trice = true;
  if ( cmd.findOption("--use-files") )
       use_files = true;
  if ( cmd.findOption("--trice") )
       trice = true;
  else if (cmd.findOption("--meta-json"))
  {
    OFLOG_ERROR(img2dcmLogger, "--meta-json support requires --trice option");
    cleanup(); return EC_IllegalCall;
  }
  else if (cmd.findOption("--meta-xml"))
  {
    OFLOG_ERROR(img2dcmLogger, "--meta-xml support requires --trice option");
    cleanup(); return EC_IllegalCall;
  }

  if (cmd.findOption("--input-format"))
  {
    app.checkValue(cmd.getValue(tempStr));
//TRICE
    if (tempStr == "MP4")
      inputPlug = new I2DJpegSource();
    else if (tempStr == "JPEG")
      inputPlug = new I2DJpegSource();
    else if (tempStr == "BMP")
      inputPlug = new I2DBmpSource();
    else
    { cleanup(); return makeOFCondition(OFM_dcmdata, 18, OF_error, "No plugin for selected input format available"); }
    if (!inputPlug)
    {  cleanup(); return EC_MemoryExhausted; }
  }
  else if (strcasestr(pixDataFile.c_str(), ".bmp") != NULL)
    inputPlug = new I2DBmpSource();
  else // default is JPEG/MP4
    inputPlug = new I2DJpegSource();
  OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Instantiated input plugin: " << inputPlug->inputFormat());

  if (tempStr == "MP4" || strcasestr(pixDataFile.c_str(), ".mp4") != NULL)
     mp4 = true;

  if (cmd.findOption("--process-dir"))
  {   app.checkValue(cmd.getValue(opt_processDir));
      if (!OFStandard::dirExists(opt_processDir))
      {   fprintf(stderr, "Process Directory %s does not exist\n", opt_processDir.c_str());
          cleanup(); exit(1);
      }
  }
  bool windows = false;
  if (cmd.findOption("--dict-path"))
  {    app.checkValue(cmd.getValue(dicomPath));
#ifndef __windows__
       (void)setenv("DCMDICTPATH", dicomPath, 1);
       windows = true;
#else
       char* envStr = (char*)malloc(strlen(dicomPath)+32);
       sprintf(envStr, "DCMDICTPATH=%s", dicomPath);
       (void)putenv(envStr);
#endif
  }

  /* make sure data dictionary is loaded */
  if (!dcmDataDict.isDictionaryLoaded())
    OFLOG_WARN(img2dcmLogger, "no data dictionary loaded, check environment variable: " << DCM_DICT_ENVIRONMENT_VARIABLE);

     //**Put in private tags and exit - leave pixel data untouched (uplink2) 
  if ((uplink2 || uplink) && cmd.findOption("--dataset-from") ) 
  {   OFString fileName;
      cmd.getValue(fileName);
      if (!fileExists((char*)fileName.c_str()))
      {   OFLOG_FATAL(img2dcmLogger, "Meta DICOM file does not exist");
          cleanup(); return EC_IllegalCall;
      }
      DcmFileFormat*  dfile = new DcmFileFormat();
      OFCondition error = dfile->loadFile(fileName);
      DcmDataset *dataset = dfile-> getDataset();  

      bool ok = true;
      u_int8_t upper = findAvailPrivateSpace(dataset, 0x0035);
      if (dataset->putAndInsertString(DcmTag(0x0035, createLowerShort(0x00, upper), EVR_LO), "TRICEFY_PRIVATE").bad()) ok = false;
           //**All elements are even length in  DICOM
      int fileSize;
      bool evenBytes = true;

      FILE* fd = ::fopen(pixDataFile.c_str(), "rb+");
      if (fd == NULL)
      {   OFLOG_ERROR(img2dcmLogger, "pixelData file does not exist");
          cleanup(); return EC_IllegalCall;
      }
      fseek(fd, 0, SEEK_END);
      fileSize = ftell(fd);
          //**DICOM only takes even lengths so add a byte and notate it so it can be removed later
      if (fileSize % 2 == 1)
         evenBytes = false;  
      fseek(fd, 0, SEEK_SET);
      Uint8* bytes = (Uint8*)malloc( evenBytes?fileSize:fileSize+1 );
      if (bytes == NULL || fread(bytes, 1, fileSize, fd) != fileSize)
      {   OFLOG_ERROR(img2dcmLogger, "cannot read consumer format pixelData into memory");
          cleanup(); return EC_IllegalCall;
      }
      if (!evenBytes)
      {   bytes[fileSize] = '\0';
          DcmTag tag(0x0035, createLowerShort(upper, 0x13), EVR_LO);    tag.setPrivateCreator("TRICEFY_PRIVATE");
          if (dataset->putAndInsertString(tag, "1").bad()) ok = false;
      }
      DcmTag tag(0x0035, createLowerShort(upper, 0x11), EVR_OB);    tag.setPrivateCreator("TRICEFY_PRIVATE");
      if (dataset->putAndInsertUint8Array(tag, bytes, evenBytes?fileSize:fileSize+1 ).bad())  ok = false;
      free(bytes);
      fclose(fd);
         //**Validate that the consumer format is safely in the dataset
      if (ok)
      {   const Uint8* value;
          unsigned long count = 0; 
          dataset->findAndGetUint8Array(tag, value, &count);
          if (count < fileSize)  
              ok = false;
      }
      if (ok == false)
      {  cleanup(); return EC_IllegalCall; }

      if (uplink2||use_files)
      {  DcmTag tag(0x0035, createLowerShort(upper, 0x15), EVR_LO);    tag.setPrivateCreator("TRICEFY_PRIVATE");  //**No pixel data is there too
         if (dataset->putAndInsertString(tag, "1").bad()) ok = false;
         if (dfile->saveFile(outputFile).bad())  ok = false;
         if (uplink2) { cleanup(); exit(ok ? 0 : 1); }
      }
      fileName.append(".rawNOTPIXEL");  //**Save template file with TRICEFY_PRIVATE fields
      if (dfile->saveFile(fileName).bad())  ok = false;
      delete dfile;
      if (ok == false)
      {  cleanup(); return EC_IllegalCall;  }
  }
  
  if (mp4 && !use_files)
  {    OFString dir = currentDir;
       OFString command("ffmpeg -i ");
       command.append(pixDataFile).append(" ");
       if (uplink)  //**Just the first frame if --uplink to get Transfer Sytnax and other pixel related fields
          command.append("-vframes 1 ");
#ifdef __windows__
       command.append(dir).append(delimiter).append("FRAME.f%d.jpg 2> NUL");
#else
       command.append(dir).append(delimiter).append("FRAME.f%d.jpg 2> /dev/null");
#endif
       OFString pgm;
       OFStandard::combineDirAndFilename(pgm, opt_processDir, command, true);
       if (system(pgm.c_str()) != 0)
       {   OFLOG_FATAL(img2dcmLogger, "ffmpeg command failed for .mp4 file");
           cleanup();
           return EC_IllegalCall;
       }
  }
  if (mp4)
  {       //**Get number of frames and the DICOM for the first frame
       pixDataFile1.append(currentDir).append(delimiter).append("FRAME.f1.jpg");
       numberFrames = ::getFileCount((char*)currentDir.c_str(), "FRAME.");
  }

//TRICE
  if (trice)
  {   
      DcmDataset *meta =  new DcmDataset();

      meta->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 192");
      meta->putAndInsertString(DCM_Modality, "OT");
      if (mp4) 
      {   
          if (!uplink && !windows)   
          {   char nf[32];  sprintf(nf, "%d", numberFrames);
              meta->putAndInsertString(DCM_NumberOfFrames, nf);
              meta->putAndInsertString(DCM_CineRate, "15");
          }
            //**NO metadata
          if (cmd.findOption("--meta-json") == false && cmd.findOption("--dataset-from") == false && cmd.findOption("--meta-xml") == false)
          {     //**Save File
             OFString dir = currentDir;
             dcmInputFileName.append(dir).append(delimiter).append("triceTemplate.dcm");

               //meta -> saveFile(dcmInputFileName);
             std::ofstream pFile;
             OFString fileName(dcmInputFileName);  fileName.append(".IN"); OFStandard::deleteFile(fileName);
             pFile.open (fileName.c_str(), std::ios::out | std::ios::app | std::ios::binary);
             meta->print(pFile);
             pFile.close();
             OFString command("dump2dcm -g -q +E --update-meta-info ");  
             if (!isnull(dicomPath))
                  command.append(" --dict-path ").append(dicomPath).append(" ");
             command.append(fileName).append(" ").append(dcmInputFileName);
             OFString pgm;  OFStandard::combineDirAndFilename(pgm, opt_processDir, command, true);
             if (system(pgm.c_str()) != 0)
             {    OFLOG_FATAL(img2dcmLogger, "system call for dump2dcm with input template failed");
                  cleanup();
                  return EC_IllegalCall;
             }
             i2d.setTemplateFile(dcmInputFileName);
             rmFile(fileName.c_str());
          }
      }
      if (cmd.findOption("--meta-json"))  //**Will go away
      {   OFString metaFile;
          app.checkValue(cmd.getValue(metaFile));
          if (!fileExists((char*)metaFile.c_str()))
          {   OFLOG_FATAL(img2dcmLogger, "Meta JSON file does not exist");
              cleanup();
              return EC_IllegalCall;
          }
    
          char* json = readTxt((char*)metaFile.c_str());
          if (populateFromJson(json, meta) == 0)
          {
                //**Save File
             OFString dir = currentDir;
             dcmInputFileName.append(dir).append(delimiter).append("triceTemplate.dcm");

               //meta -> saveFile(dcmInputFileName);
             std::ofstream pFile;
             OFString fileName(dcmInputFileName);  fileName.append(".IN"); OFStandard::deleteFile(fileName);
             pFile.open (fileName.c_str(), std::ios::out | std::ios::app | std::ios::binary);
             meta->print(pFile);
             pFile.close();
             OFString command("dump2dcm -g -q +E --update-meta-info ");  
             if (strcmp(dicomPath, "") != 0)
                  command.append(" --dict-path ").append(dicomPath).append(" ");
             command.append(fileName).append(" ").append(dcmInputFileName);
             OFString pgm; OFStandard::combineDirAndFilename(pgm, opt_processDir, command, true);
             GrowingString args;
             if (system(pgm.c_str()) != 0)
             {    OFLOG_FATAL(img2dcmLogger, "system call for dump2dcm with input template failed");
                  cleanup();
                  return EC_IllegalCall;
             }
             i2d.setTemplateFile(dcmInputFileName);
             rmFile(fileName.c_str());
         }
         else
         {   OFLOG_FATAL(img2dcmLogger, "meta JSON does not parse");
             cleanup();
             return EC_IllegalCall;
         }
      }
#ifndef __windows__
      else if (cmd.findOption("--meta-xml") && !uplink)
      {   OFString metaFile;
          app.checkValue(cmd.getValue(metaFile));
          if (!fileExists((char*)metaFile.c_str()))
          {   OFLOG_FATAL(img2dcmLogger, "Meta XML file does not exist");
              cleanup();
              return EC_IllegalCall;
          }
          char* xml = ::readTxt((char*)metaFile.c_str());
          bool xchange = false;
          if (strstr(xml, "<meta-header") == NULL)
          {   xml = Util::replace(xml, "<file-format>", "<file-format><meta-header xfer=\"1.2.840.10008.1.2.1\" name=\"Little Endian Explicit\">");
              xml = Util::replace(xml, "<data-set>", "</meta-header><data-set xfer=\"1.2.840.10008.1.2.1\" name=\"Little Endian Implicit\">");
              xml = Util::replace(xml, "<sequence tag=\"7fe0,0010\" vr=\"OB\" card=\"1\" name=\"PixelData\">\n<pixel-item len=\"0\" binary=\"hidden\"></pixel-item>\n</sequence>\n", "");  //**rm empty pixel data, no Transfer Syntax
              xchange = true;
              metaFile.append(".1.xml");
              writeTxt((char*)metaFile.c_str(), "", xml);
          }
          
                //**Save File -- linux only
          OFString dir = currentDir;
          dcmInputFileName.append(dir).append(delimiter).append("triceTemplate.dcm");
          OFString command("xml2dcm -q --update-meta-info ");  command.append(metaFile).append(" ").append(dcmInputFileName);
          if (system(command.c_str()) != 0)
          {    OFLOG_FATAL(img2dcmLogger, "system call for xml2dcm with input template failed");
               if (xchange)  rmFile(metaFile.c_str());
               cleanup();
               return EC_IllegalCall;
          }
          if (xchange)  rmFile(metaFile.c_str());
          i2d.setTemplateFile(dcmInputFileName);

             //**Set the override keys and remove private tags
          DcmFileFormat*  dfile = new DcmFileFormat();
          DcmDataset *dataset = dfile-> getDataset();  //** dfile->getMetaInfo() for the header
          OFCondition error = dfile->loadFile(dcmInputFileName);
          
#ifdef TRICE  
          u_int16_t upper = findSpecificPrivateSpace(dataset, 0x0035, "TRICEFY_PRIVATE", 0x10);  
           //**Remove Tricefy private Tags
          DcmStack stack;
          DcmObject *object = NULL;
          bool changed = false;
          while (dataset->nextObject(stack, OFTrue).good())
          {   object = stack.top();
              if (object->getGTag() == 0x0035)  //**Tricefy Private Tag
              {    u_int16_t etag = object->getETag();    
                   if (etag < 0x10 || etag / (16*16) == upper)
                   {   stack.pop();
                       delete OFstatic_cast(DcmItem *, stack.top())->remove(object);
                       changed = true;
                   }
              }
          }
          if (changed)
             if (dfile->saveFile(dcmInputFileName).bad())
             {  cleanup();  return EC_IllegalCall;  }
#endif
          const char* p = "";  dataset->findAndGetString(DCM_SOPClassUID, p);
          if (!isnull(p))
          {  OFString attr("SOPClassUID=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
          p = "";  dataset->findAndGetString(DCM_SOPInstanceUID, p);
          if (!isnull(p))
          {  OFString attr("SOPInstanceUID=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
#ifdef SOURCE_AE_TITLE
          p = "";  dfile->getMetaInfo()->findAndGetString(DCM_SourceApplicationEntityTitle, p);
          if (!isnull(p))
          {  OFString attr("SourceApplicationEntityTitle=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
#endif
          p = "";  meta->findAndGetString(DCM_NumberOfFrames, p);
          if (!isnull(p))
          {  OFString attr("NumberOfFrames=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
          i2d.setOverrideKeys(overrideKeys);
          delete dfile;
      }
#endif
      else if ( cmd.findOption("--dataset-from") ) //**Already have dataset, just do overrides based on the header
      {   OFString fileName;
          cmd.getValue(fileName);
          if (!fileExists((char*)fileName.c_str()))
          {   OFLOG_FATAL(img2dcmLogger, "Meta DICOM file does not exist");
              cleanup();
              return EC_IllegalCall;
          }
          DcmFileFormat*  dfile = new DcmFileFormat();
          OFCondition error = dfile->loadFile(fileName);
          DcmMetaInfo *meta = dfile-> getMetaInfo(); 
          DcmDataset *dataset = dfile-> getDataset();  //** dfile->getMetaInfo() for the header

          const char* p = "";  meta->findAndGetString(DCM_MediaStorageSOPClassUID, p);
          if (!isnull(p))
          {  OFString attr("SOPClassUID=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
          p = "";  meta->findAndGetString(DCM_MediaStorageSOPInstanceUID, p);
          if (!isnull(p))
          {  OFString attr("SOPInstanceUID=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
#ifdef SOURCE_AE_TITLE
          p = "";  meta->findAndGetString(DCM_SourceApplicationEntityTitle, p);
          if (!isnull(p))
          {  OFString attr("SourceApplicationEntityTitle=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
#endif
          p = "";  dataset->findAndGetString(DCM_NumberOfFrames, p);
          if (!isnull(p))
          {  OFString attr("NumberOfFrames=");  attr.append(p);
             overrideKeys.push_back(attr);
          }
             //**Append DerivationDescription
          p = "";  dataset->findAndGetString(DCM_DerivationDescription, p);
          OFString attr("DerivationDescription=Tricefy Lossy Pixel Optimization");
          if (!isnull(p))
          {         // append old Derivation Description
               attr += " [";
               attr += p;
               attr += "]";
               if (attr.length() > 1024)
               {       // ST is limited to 1024 characters, cut off tail
                    attr.erase(1020);
                    attr += "...]";
               }
          }
          overrideKeys.push_back(attr);
          i2d.setOverrideKeys(overrideKeys);
          delete dfile;
      }
      delete meta;
  }


 // Find out which plugin to use
  cmd.beginOptionBlock();
  if (cmd.findOption("--sec-capture") || cmd.findOption("--trice"))
    outPlug = new I2DOutputPlugSC();
  if (cmd.findOption("--vl-photo"))
  {
    outPlug = new I2DOutputPlugVLP();
  }
  if (cmd.findOption("--new-sc"))
    outPlug = new I2DOutputPlugNewSC();
  cmd.endOptionBlock();
  if (!outPlug) // default is the old Secondary Capture object
    outPlug = new I2DOutputPlugSC();
  if (outPlug == NULL) {  cleanup();  return EC_MemoryExhausted; }
  OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Instantiated output plugin: " << outPlug->ident());

  cmd.beginOptionBlock();
  if (cmd.findOption("--write-file"))    writeMode = EWM_fileformat;
  if (cmd.findOption("--write-dataset")) writeMode = EWM_dataset;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--group-length-recalc")) grpLengthEnc = EGL_recalcGL;
  if (cmd.findOption("--group-length-create") || cmd.findOption("--trice")) grpLengthEnc = EGL_withoutGL;
  if (cmd.findOption("--group-length-remove")) grpLengthEnc = EGL_withoutGL;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--length-explicit"))  lengthEnc = EET_ExplicitLength;
  if (cmd.findOption("--length-undefined")) lengthEnc = EET_UndefinedLength;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--padding-off"))
  {
    filepad = 0;
    itempad = 0;
  }
  else if (cmd.findOption("--padding-create"))
  {
    OFCmdUnsignedInt opt_filepad; OFCmdUnsignedInt opt_itempad;
    app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
    app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
    itempad = opt_itempad;
    filepad = opt_filepad;
  }
  cmd.endOptionBlock();

  // create override attribute dataset (copied from findscu code)
  if (cmd.findOption("--key", 0, OFCommandLine::FOM_FirstFromLeft))
  {
    const char *ovKey = NULL;
    do {
      app.checkValue(cmd.getValue(ovKey));
      overrideKeys.push_back(ovKey);
    } while (cmd.findOption("--key", 0, OFCommandLine::FOM_NextFromLeft));
  }
  i2d.setOverrideKeys(overrideKeys);

  // Test for ISO Latin 1 option
  OFBool insertLatin1 = OFTrue;
  cmd.beginOptionBlock();
  if (cmd.findOption("--latin1"))
    insertLatin1 = OFTrue;
  if (cmd.findOption("--no-latin1") || cmd.findOption("--trice"))  // want unicode
    insertLatin1 = OFFalse;
  cmd.endOptionBlock();
  i2d.setISOLatin1(insertLatin1);

  // evaluate validity checking options
  OFBool insertType2 = OFTrue;
  OFBool inventType1 = OFTrue;
  OFBool doChecks = OFTrue;
  cmd.beginOptionBlock();
  if (cmd.findOption("--no-checks"))
    doChecks = OFFalse;
  if (cmd.findOption("--do-checks") || cmd.findOption("--trice"))
    doChecks = OFTrue;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--insert-type2"))
    insertType2 = OFTrue;
  if (cmd.findOption("--no-type2-insert"))
    insertType2 = OFFalse;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--invent-type1"))
    inventType1 = OFTrue;
  if (cmd.findOption("--no-type1-invent"))
    inventType1 = OFFalse;
  cmd.endOptionBlock();
  i2d.setValidityChecking(doChecks, insertType2, inventType1);
  outPlug->setValidityChecking(doChecks, insertType2, inventType1);

  // evaluate --xxx-from options and transfer syntax options
  OFCondition cond;
  cond = evaluateFromFileOptions(cmd, i2d);
  if (cond.bad())
  {
    delete outPlug; outPlug = NULL;
    delete inputPlug; inputPlug = NULL;
    if (!isnull(dcmInputFileName.c_str()))  rmFile(dcmInputFileName.c_str());
    cleanup();
    return cond;
  }

  if (inputPlug->inputFormat() == "JPEG" || mp4)
  {
    I2DJpegSource *jpgSource = OFstatic_cast(I2DJpegSource*, inputPlug);
    if (!jpgSource)
    {
       delete outPlug; outPlug = NULL;
       delete inputPlug; inputPlug = NULL;
       if (!isnull(dcmInputFileName.c_str()))  rmFile(dcmInputFileName.c_str());
       cleanup();
       return EC_MemoryExhausted;
    }
    if ( cmd.findOption("--disable-progr") )
      jpgSource->setProgrSupport(OFFalse);
    if ( cmd.findOption("--disable-ext") )
      jpgSource->setExtSeqSupport(OFFalse);
    if ( cmd.findOption("--insist-on-jfif") )
      jpgSource->setInsistOnJFIF(OFTrue);
    if ( cmd.findOption("--keep-appn") )
      jpgSource->setKeepAPPn(OFTrue);
  }
  if (mp4)
     inputPlug->setImageFile(pixDataFile1);  
  else
     inputPlug->setImageFile(pixDataFile);  

  DcmDataset *resultObject = NULL;
  OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Starting image conversion");
  cond = i2d.convert(inputPlug, outPlug, resultObject, writeXfer);
  if (cond.bad())
  {
    delete outPlug; outPlug = NULL;
    delete inputPlug; inputPlug = NULL;
    if (!isnull(dcmInputFileName.c_str()))  rmFile(dcmInputFileName.c_str());
    cleanup();
    return cond;
  }
  DcmXfer o_xfer(writeXfer);
  bool encapsulated = o_xfer.isEncapsulated();   //**Should always be encapsulated since JPEG

        //**Convert the other frames and create a multiframe

  if (mp4)
  {   OFString dir = currentDir;
      char pixFile[strlen(dir.c_str())+128];  sprintf(pixFile, "%s%sFrameConvert.f%d.raw", dir.c_str(), delimiter, 1);
      size_t pixelCounter = 1;

      std::ofstream pFile;
      OFString fileName(outputFile);  fileName.append(".OUT"); OFStandard::deleteFile(fileName);
      pFile.open (fileName.c_str(), std::ios::out | std::ios::app | std::ios::binary);
      DcmFileFormat dcmff(resultObject);
      dcmff.getMetaInfo()->print(pFile, 0, 0, pixFile, &pixelCounter);
      dcmff.print(pFile, 0, 0, pixFile, &pixelCounter);
      pFile.close();

      int strct = 1;
      char* end = "";
      OFString tfile = "";
      char line[strlen(dir.c_str())+128];
      if (encapsulated)  //**Dump an empty lookup table (offsets in orig will be wrong) and the output transfer type
      {   tfile.append("(0002,0010) UI ").append(o_xfer.getXferID()).append("             #  22, 1 TransferSyntaxUID\r\n");
          char* ds = ::readTxt((char*)fileName.c_str());     //**Output transfer type
          char*  p =  strstr(ds, "(7fe0,0010)");             //**We want Meta data / no pixel data so we can put in the proper multiframe pixel data
          while (p && *(p-1) == ' ')   //**pixel data in a sequence;  keep looking until we fine one not in a sequence
             p =  strstr(p+1, "(7fe0,0010)");             
          if (p == NULL)  
          {     OFLOG_FATAL(img2dcmLogger, "Cannot read the dcmdump file just persisted via print()");
                delete outPlug; outPlug = NULL;
                delete inputPlug; inputPlug = NULL;
                delete resultObject; resultObject = NULL;
                cleanup();
                if (!isnull(dcmInputFileName.c_str()))  rmFile(dcmInputFileName.c_str());
                return EC_IllegalCall;
          }
          char*  px =  strstr(ds, "(7fe0,0010)");             //**We want Meta data / no pixel data so we can put in the proper multiframe pixel data
             //**Count up the pixel data tags before this one - some might be in sequences
          while (px != p)
          {   strct++;
              px =  strstr(px+1, "(7fe0,0010)");             
          }
          *p = '\0';
          end = strstr(p+1, "(fffe,e0dd)");
          if (!isnull(end))
             end = strstr(end+1, "(");   //Retain Private tags past the pixel data
          tfile.append(ds); 
             //**Add pixel data
          if (!uplink||use_files)
          {   sprintf(line, "(7fe0,0010) OB (PixelSequence #=%d)                      # u/l, 1 PixelData\r\n", numberFrames);
              tfile.append(line);
                 //**add empty lookup table
              char file0[strlen(dir.c_str())+128];  sprintf(file0, "%s%sFrameConvert.f0.raw", dir.c_str(), delimiter);
              writeData(file0, (char*)"", (void*)"", 0);  //**Drop an empty lookup table
              sprintf(line, "  (fffe,e000) pi =%s                  #   0, 1 Item\r\n", file0);
              tfile.append(line);
                 //**Add first file
              char file1[strlen(dir.c_str())+128];  sprintf(file1, "%s%sFrameConvert.f1.raw.%d.raw", dir.c_str(), delimiter,strct+1);
              if (!nonZeroFile(file1))
                 sprintf(file1, "%s%sFrameConvert.f1.raw.%d.raw", dir.c_str(), delimiter,strct+2);
              sprintf(line, "  (fffe,e000) pi =%s                  #   0, 1 Item\r\n", file1);
              tfile.append(line);
         }
      }
      //**else - unincapsulated - nothing needs to be done to the file (FrameConvert.f1.raw.1.raw)
      
       //**Do the rest of the frames
      if (!uplink || use_files) for (int i = 2;  i <= numberFrames; i++)
      {   char frameName[strlen(dir.c_str())+128];  sprintf(frameName, "%s%sFRAME.f%d.jpg", dir.c_str(), delimiter, i);
             //**format pixel data
          inputPlug->setImageFile(frameName);  
          delete resultObject; resultObject = NULL;
          cond = i2d.convert(inputPlug, outPlug, resultObject, writeXfer);  //**new resultObject

             //**drop the pixel frame file from resultObject
          DcmFileFormat dcmff1(resultObject);
          std::cout.setstate(std::ios_base::badbit);  //**Silence stdout, we are only interested in the pixel files
          sprintf(pixFile, "%s%sFrameConvert.f%d.raw", dir.c_str(), delimiter, i);
          pixelCounter = 1; dcmff1.print(std::cout, DCMTypes::PF_shortenLongTagValues, 0, pixFile, &pixelCounter);
          std::cout.clear();
             //**now we have the pixel file;  let's add to the multiframe being compiled
          if (encapsulated)
          {   sprintf(frameName, "%s%sFrameConvert.f%d.raw.1.raw", dir.c_str(), delimiter, i);  rmFile(frameName); //**empty lookup table 
              sprintf(frameName, "%s%sFrameConvert.f%d.raw.%d.raw", dir.c_str(), delimiter, i, strct+1);  
              if (!nonZeroFile(frameName))
                 sprintf(frameName, "%s%sFrameConvert.f%d.raw.%d.raw", dir.c_str(), delimiter,i,strct+2);
              sprintf(line, "  (fffe,e000) pi =%s                  #   0, 1 Item\r\n", frameName);
              tfile.append(line);
          }
          else
          {        //**Uncompressed multiframe is just a single file with contiguous frames
              char frame[strlen(dir.c_str())+128], frame1[strlen(dir.c_str())+128];
              sprintf(frame, "%s%sFrameConvert.f%d.raw.1.raw", dir.c_str(), delimiter, i);
              sprintf(frame1, "%s%sFrameConvert.f1.raw.1.raw", dir.c_str(), delimiter);
              appendFile(frame, frame1);
              rmFile(frame);
         }
         sprintf(frameName, "%s%sFRAME.f%d.jpg", dir.c_str(), delimiter, i-1);
         if (i>2) rmFile(frameName);  //Clean up the Frame file used to create previous pixel file
     }

        //**Done - make the final DICOM and remove misc files 
     if ((!uplink||use_files) && encapsulated)  //*End of sequence marker
        tfile.append("(fffe,e0dd) na (SequenceDelimitationItem)               #   0, 0 SequenceDelimitationItem\r\n"); 
     if (!isnull(end))  //**Add anything after the pixel data
        tfile.append(end);
     writeTxt((char*)dir.c_str(), (char*)"triceDUMP.txt", (char*)tfile.c_str());

     OFString command("dump2dcm -g -q +E --update-meta-info ");  
     if (strcmp(dicomPath, "") != 0)
        command.append(" --dict-path ").append(dicomPath).append(" ");
     command.append(dir.c_str()).append(delimiter).append("triceDUMP.txt").append(" ").append(outputFile);
     OFString pgm;
     OFStandard::combineDirAndFilename(pgm, opt_processDir, command, true);
     if (system(pgm.c_str()) != 0)
     {    OFLOG_FATAL(img2dcmLogger, "system call for dump2dcm with final complete data failed");
          delete outPlug; outPlug = NULL;
          delete inputPlug; inputPlug = NULL;
          delete resultObject; resultObject = NULL;
          cleanup();
          if (!isnull(dcmInputFileName.c_str()))  rmFile(dcmInputFileName.c_str());
          return EC_IllegalCall;
     }

  }
  else   //**Simgle image
  {   OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Saving output DICOM to file " << outputFile);
      DcmFileFormat dcmff(resultObject);
      if (uplink && !use_files)
          dcmff.getDataset()->findAndDeleteElement(DCM_PixelData);
      cond = dcmff.saveFile(outputFile.c_str(), writeXfer, lengthEnc,  grpLengthEnc, padEnc, OFstatic_cast(Uint32, filepad), OFstatic_cast(Uint32, itempad), writeMode);
      if (uplink && cmd.findOption("--dataset-from") ) 
      {   OFString fileName;
          cmd.getValue(fileName);
          fileName.append(".rawNOTPIXEL"); 
          rmFile((char*)fileName.c_str());
      }
  }

  // Cleanup and return
  delete outPlug; outPlug = NULL;
  delete inputPlug; inputPlug = NULL;
  delete resultObject; resultObject = NULL;
  if (!isnull(dcmInputFileName.c_str()))  rmFile(dcmInputFileName.c_str());
  cleanup();
  return cond;
}


int main(int argc, char *argv[])
{

  // variables for command line
  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, "Convert image file to DICOM", rcsid);
  OFCommandLine cmd;

  cmd.setOptionColumns(LONGCOL, SHORTCOL);
  cmd.setParamColumn(LONGCOL + SHORTCOL + 4);
  addCmdLineOptions(cmd);

  OFCondition cond = startConversion(cmd, argc, argv);
  if (cond.bad())
  {
    OFLOG_FATAL(img2dcmLogger, "Error converting file: " << cond.text());
    return 1;
  }

  return 0;
}
