/*
 *
 *  Copyright (C) 2001-2014, OFFIS e.V.
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
 *  Module:  dcmjpeg
 *
 *  Author:  Marco Eichelberg
 *
 *  Purpose: Change metadata field value if it is different
 *
 */

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDLIB
#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

#ifdef HAVE_GUSI_H
#include <GUSI.h>
#endif

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"       /* for dcmtk version name */
#include "util.h"

#define OFFIS_CONSOLE_APPLICATION "dcmvalue"

static OFLogger dcmValueLogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_APPLICATION);

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

// ********************************************


int main(int argc, char *argv[])
{
  bool study_uid = true;

#ifdef HAVE_GUSI_H
  GUSISetup(GUSIwithSIOUXSockets);
  GUSISetup(GUSIwithInternetSockets);
#endif

  const char *opt_value = NULL;
  const char *opt_fname = NULL;

  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, "Change Field Value if different", rcsid);
  OFCommandLine cmd;

  cmd.addParam("value",    "DICOM value");
  cmd.addParam("dcmfile",  "DICOM filename");

  cmd.addOption("--study-uid", "+s",     "altering study_uid");
  cmd.addOption("--dict-path",          "-dp",  1, "[p]ath: string", "complete path (including file name) for dicom.dic");

    /* evaluate command line */
  prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
  if (app.parseCommandLine(cmd, argc, argv))
    {

      /* command line parameters */

      cmd.getParam(1, opt_value);
      cmd.getParam(2, opt_fname);

      /* options */

      OFLog::configureFromCommandLine(cmd, app);

      if (cmd.findOption("--study_uid")) study_uid = true;

    }

    /* print resource identifier */
    OFLOG_DEBUG(dcmValueLogger, rcsid << OFendl);

    static const char* dicomPath="";
    if (cmd.findOption("--dict-path"))
    {    app.checkValue(cmd.getValue(dicomPath));
#ifndef __windows__
         (void)setenv("DCMDICTPATH", dicomPath, 1);
#else
         char* envStr = (char*)malloc(strlen(dicomPath)+32);
         sprintf(envStr, "DCMDICTPATH=%s", dicomPath);
         (void)putenv(envStr);
#endif
    }
    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFLOG_WARN(dcmValueLogger, "no data dictionary loaded, check environment variable: "
            << DCM_DICT_ENVIRONMENT_VARIABLE);
    }

    // validate new value
    if ((opt_value == NULL) || (strlen(opt_value) == 0))
    {
        OFLOG_FATAL(dcmValueLogger, "invalid new Value <empty string>");
        return 1;
    }
    if ((opt_fname == NULL) || (strlen(opt_fname) == 0))
    {
        OFLOG_FATAL(dcmValueLogger, "invalid filename: <empty string>");
        return 1;
    }

    OFCondition error = EC_Normal;

    DcmFileFormat fileformat;

    OFLOG_INFO(dcmValueLogger, "reading input file " << opt_fname);

    error = fileformat.loadFile(opt_fname);
    if (error.bad())
    {
        OFLOG_FATAL(dcmValueLogger, error.text() << ": reading file: " <<  opt_fname);
        return 1;
    }

    DcmDataset *dataset = fileformat.getDataset();
    
    bool change = false;
    if (study_uid)
    {    const char* c = "";   
         if (dataset->findAndGetString(DCM_StudyInstanceUID, c).good() && !isnull(c))
             if (strcmp(c, opt_value) != 0)
                change = true;

             if (change)
                dataset->putAndInsertString(DCM_StudyInstanceUID, opt_value);
    }


    if (change)
    {
       OFString tfile = opt_fname;
       tfile += ".TMP";

           //fileformat.loadAllDataIntoMemory();
       OFCondition error = fileformat.saveFile(tfile); 
       if (error != EC_Normal)
       {
           OFLOG_FATAL(dcmValueLogger, error.text() << ": writing file: " <<  opt_fname);
           rmFile ((char*)tfile.c_str());
           return 1;
       }

       mvFile((char*)tfile.c_str(), (char*)opt_fname);
       if (nonZeroFile((char*)tfile.c_str()))
       {  fileformat.saveFile(opt_fname);
          rmFile((char*)tfile.c_str());  
       }
    }
    return 0;
}
