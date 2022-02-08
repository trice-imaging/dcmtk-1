#define TRICE 1
/*
 *
 *  Copyright (C) 2002-2013, OFFIS e.V.
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
 *  Module:  dcmimage
 *
 *  Authors: Joerg Riesmeier
 *
 *  Purpose: Scale DICOM images
 *
 */

#ifdef TRICE
#include <sys/file.h>
#include <iostream>
#include <fstream>
#include <trice_int.h>
#include <box.h>
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

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDIO
#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"

#ifdef HAVE_GUSI_H
#include <GUSI.h>
#endif

#include "dcmtk/dcmdata/dctk.h"          /* for various dcmdata headers */
#include "dcmtk/dcmdata/cmdlnarg.h"      /* for prepareCmdLineArgs */
#include "dcmtk/dcmdata/dcuid.h"         /* for dcmtk version name */

#include "dcmtk/ofstd/ofconapp.h"        /* for OFConsoleApplication */
#include "dcmtk/ofstd/ofcmdln.h"         /* for OFCommandLine */

#include "dcmtk/oflog/oflog.h"           /* for OFLogger */
#include "dcmtk/dcmimgle/dcmimage.h"     /* for DicomImage */
#include "dcmtk/dcmimage/diregist.h"     /* include to support color images */
#include "dcmtk/dcmdata/dcrledrg.h"      /* for DcmRLEDecoderRegistration */

#ifdef TRICE
#include "dcmtk/dcmjpeg/djdecode.h"      /* for dcmjpeg decoders */
#include "dcmtk/dcmjpeg/dipijpeg.h"      /* for dcmimage JPEG plugin */
#include "dcmtk/dcmjpeg/djencode.h"  /* for dcmjpeg encoders */
#include "dcmtk/dcmjpeg/djrplol.h"   /* for DJ_RPLossless */
#include "dcmtk/dcmjpeg/djrploss.h"  /* for DJ_RPLossy */
#include "dcmtk/dcmjpls/djdecode.h"      /* for dcmjpls decoders */
#include "dcmtk/dcmjpls/djencode.h"      /* for dcmjpls encoders */

#include "dcmtk/dcmdata/dcrledrg.h"        // rle decoder
#include "dcmtk/dcmdata/dcrleerg.h" 
#include "trice_dicom.cc"
char* genInstanceFromUID(const char*, const char*);
#endif

#ifdef WITH_ZLIB
#include <zlib.h>          /* for zlibVersion() */
#endif

#define OFFIS_CONSOLE_DESCRIPTION "Scale DICOM images"

#ifdef TRICE
#define OFFIS_CONSOLE_APPLICATION "dcmaudscale"
#define ROOT_UID "1.3.6.1.4.1.35190."
#else
#define OFFIS_CONSOLE_APPLICATION "dcmscale"
#endif

static OFLogger dcmaudscaleLogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_APPLICATION);

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

#define SHORTCOL 4
#define LONGCOL 21

#ifdef TRICE
bool                opt_vp6 = false;                  // VP6
OFString            opt_idCache;
void 
trice_anonymize(DcmDataset* datasetSV, bool opt_uplink, long int frameCount, bool strict=true, const char* realFileName=NULL)
{
     //**Find the relocatable part of the private tags
   u_int16_t kp_upper = findSpecificPrivateSpace(datasetSV, 0x6301, "KRETZ_PRIVATE", 0x10);
   u_int16_t ku_upper1 = findSpecificPrivateSpace(datasetSV, 0x7fe1, "KRETZ_US", 0x11);
   u_int16_t ku_upper2 = findSpecificPrivateSpace(datasetSV, 0x8001, "KRETZ_US", 0x11);
   u_int16_t tp_upper = findSpecificPrivateSpace(datasetSV, 0x0035, "TRICEFY_PRIVATE", 0x10);

/* All TM and UI attributes types are handled globally 
   and therefore are no longer enumerated
*/
   struct
   {  DcmTag sAttr;
      const char* value;
      bool strict;
   } tags[]  = {  
            {  DCM_RETIRED_AcquisitionComments, "", true },
            {  DCM_AcquisitionContextSequence, "", true },
            {  DCM_AcquisitionDeviceProcessingDescription, "", true },
            {  DCM_AcquisitionProtocolDescription, "", true },
            {  DCM_ActualHumanPerformersSequence, "", true },
            {  DCM_AdditionalPatientHistory, "", true },
            {  DCM_AdmissionID, "", true },
            {  DCM_AdmittingDiagnosesCodeSequence, "", true },
            {  DCM_AdmittingDiagnosesDescription, "", true },
            {  DCM_Allergies, "", true },
            {  DCM_RETIRED_Arbitrary, "", true },
            {  DCM_AuthorObserverSequence, "", true },
            {  DCM_BranchOfService, "", true },
            {  DCM_CassetteID, "", true },
            {  DCM_CommentsOnThePerformedProcedureStep, "", true },
            {  DCM_ConfidentialityConstraintOnPatientDataDescription, "", true },
            {  DCM_ContentCreatorIdentificationCodeSequence, "", true },
            {  DCM_ContributionDescription, "", true },
            {  DCM_CountryOfResidence, "", true },
            {  DCM_CurrentPatientLocation, "", true },
            {  DCM_RETIRED_CurveData, "", true },
            {  DCM_RETIRED_CurveDate, "", true },
            {  DCM_CustodialOrganizationSequence, "", true },
            {  DCM_DataSetTrailingPadding, "", true },
            {  DCM_DerivationDescription, "", true },
            {  DCM_DetectorID, "", true },
            {  DCM_DeviceSerialNumber, "", true },
            {  DCM_DigitalSignaturesSequence, "", true },
            {  DCM_RETIRED_DischargeDiagnosisDescription, "", true },
            {  DCM_RETIRED_DistributionAddress, "", true },
            {  DCM_RETIRED_DistributionName, "", true },
            {  DCM_EthnicGroup, "", true },
            {  DCM_FailedSOPInstanceUIDList, "", true },
            {  DCM_FrameComments, "", true },
            {  DCM_GantryID, "", true },
            {  DCM_GeneratorID, "", true },
            {  DCM_GraphicAnnotationSequence, "", true },
            {  DCM_HumanPerformerName, "", true },
            {  DCM_HumanPerformerOrganization, "", true },
            {  DCM_IconImageSequence, "", true },
            {  DCM_RETIRED_IdentifyingComments, "", true },
            {  DCM_ImageComments, "", true },
            {  DCM_RETIRED_ImagePresentationComments, "", true },
            {  DCM_ImagingServiceRequestComments, "", true },
            {  DCM_RETIRED_Impressions, "", true },
            {  DCM_InstitutionAddress, "", true },
            {  DCM_InstitutionCodeSequence, "", true },
            {  DCM_InstitutionName, "", false },
            {  DCM_InstitutionalDepartmentName, "", true },
            {  DCM_RETIRED_InsurancePlanIdentification, "", true },
            {  DCM_IntendedRecipientsOfResultsIdentificationSequence, "", true },
            {  DCM_RETIRED_InterpretationApproverSequence, "", true },
            {  DCM_RETIRED_InterpretationAuthor, "", true },
            {  DCM_RETIRED_InterpretationDiagnosisDescription, "", true },
            {  DCM_RETIRED_InterpretationIDIssuer, "", true },
            {  DCM_RETIRED_InterpretationRecorder, "", true },
            {  DCM_RETIRED_InterpretationText, "", true },
            {  DCM_RETIRED_InterpretationTranscriber, "", true },
            {  DCM_RETIRED_IssuerOfAdmissionID, "", true },
            {  DCM_IssuerOfPatientID, "", true },
            {  DCM_RETIRED_IssuerOfServiceEpisodeID, "", true },
            {  DCM_LastMenstrualDate, "", true },
            {  DCM_MAC, "", true },
            {  DCM_MedicalAlerts, "", true },
            {  DCM_MedicalRecordLocator, "", true },
            {  DCM_MilitaryRank, "", true },
            {  DCM_ModifiedAttributesSequence, "", true },
            {  DCM_RETIRED_ModifiedImageDescription, "", true },
            {  DCM_RETIRED_ModifyingDeviceID, "", true },
            {  DCM_RETIRED_ModifyingDeviceManufacturer, "", true },
            {  DCM_NamesOfIntendedRecipientsOfResults, "", true },
            {  DCM_NameOfPhysiciansReadingStudy, "", true },
            {  DCM_Occupation, "", true },
            {  DCM_OperatorsName, "", false },
            {  DCM_OperatorIdentificationSequence, "", true },
            {  DCM_OriginalAttributesSequence, "", true },
            {  DCM_OrderCallbackPhoneNumber, "", true },
            {  DCM_OrderEnteredBy, "", true },
            {  DCM_OrderEntererLocation, "", true },
            {  DCM_OtherPatientIDs, "", true },
            {  DCM_OtherPatientIDsSequence, "", true },
            {  DCM_OtherPatientNames, "", true },
            {  DCM_RETIRED_OverlayComments, "", true },
            {  DCM_OverlayData, "", true },
            {  DCM_RETIRED_OverlayDate, "", true },
            {  DCM_ParticipantSequence, "", true },
            {  DCM_PatientAddress, "", false },
            {  DCM_PatientComments, "", true },
            {  DCM_PatientSexNeutered, "", true },
            {  DCM_PatientState, "", true },
            {  DCM_PatientAge, "", true },
            {  DCM_PatientInstitutionResidence, "", true },
            {  DCM_PatientInsurancePlanCodeSequence, "", true },
            {  DCM_PatientMotherBirthName, "", true },
            {  DCM_PatientPrimaryLanguageCodeSequence, "", true },
            {  DCM_PatientPrimaryLanguageModifierCodeSequence, "", true },
            {  DCM_PatientReligiousPreference, "", true },
            {  DCM_PatientSize, "", true },
            {  DCM_PatientTelephoneNumbers, "", false },
            {  DCM_PatientWeight, "", true },
            {  DCM_PerformingPhysicianName, "", true },
            {  DCM_PhysiciansReadingStudyIdentificationSequence, "", true },
            {  DCM_PhysiciansOfRecordIdentificationSequence, "", true },
            {  DCM_PreMedication, "", true },
            {  DCM_ReferringPhysicianAddress, "", false },
            {  DCM_ReferringPhysicianIdentificationSequence, "", true },
            {  DCM_ReferringPhysicianTelephoneNumbers, "", false },
            {  DCM_PatientTransportArrangements, "", true },
            {  DCM_PerformedLocation, "", true },
            {  DCM_PerformedProcedureStepDescription, "", true },
            {  DCM_PerformedProcedureStepEndDate, "", true },
            {  DCM_PerformedProcedureStepID, "", true },
            {  DCM_PerformedProcedureStepStartDate, "", true },
            {  DCM_PerformedStationAETitle, "", true },
            {  DCM_PerformedStationGeographicLocationCodeSequence, "", true },
            {  DCM_PerformedStationName, "", true },
            {  DCM_PerformedStationNameCodeSequence, "", true },
            {  DCM_PerformingPhysicianIdentificationSequence, "", true },
            {  DCM_PersonAddress, "", true },
            {  DCM_PersonTelephoneNumbers, "", false },
            {  DCM_RETIRED_PhysicianApprovingInterpretation, "", true },
            {  DCM_PlateID, "", true },
            {  DCM_PregnancyStatus, "", true },
            {  DCM_ProtocolName, "", true },
            {  DCM_RETIRED_ReasonForTheImagingServiceRequest, "", true },
            {  DCM_RETIRED_ReasonForStudy, "", true },
            {  DCM_ReferencedDigitalSignatureSequence, "", true },
            {  DCM_ReferencedImageSequence, "", true },
            {  DCM_ReferencedPatientAliasSequence, "", true },
            {  DCM_ReferencedPatientSequence, "", true },
            {  DCM_ReferencedPerformedProcedureStepSequence, "", true },
            {  DCM_ReferencedSOPInstanceMACSequence, "", true },
            {  DCM_RequestingPhysician, "", true },
            {  DCM_RequestingService, "", true },
            {  DCM_ResponsibleOrganization, "", true },
            {  DCM_ResponsiblePerson, "", true },
            {  DCM_RETIRED_ResultsComments, "", true },
            {  DCM_RETIRED_ResultsDistributionListSequence, "", true },
            {  DCM_RETIRED_ResultsIDIssuer, "", true },
            {  DCM_ReviewerName, "", true },
            {  DCM_ScheduledHumanPerformersSequence, "", true },
            {  DCM_RETIRED_ScheduledPatientInstitutionResidence, "", true },
            {  DCM_ScheduledPerformingPhysicianIdentificationSequence, "", true },
            {  DCM_ScheduledPerformingPhysicianName, "", true },
            {  DCM_ScheduledProcedureStepEndDate, "", true },
            {  DCM_ScheduledProcedureStepDescription, "", true },
            {  DCM_ScheduledProcedureStepLocation, "", true },
            {  DCM_ScheduledProcedureStepStartDate, "", true },
            {  DCM_ScheduledStationAETitle, "", true },
            {  DCM_ScheduledStationGeographicLocationCodeSequence, "", true },
            {  DCM_ScheduledStationName, "", true },
            {  DCM_ScheduledStationNameCodeSequence, "", true },
            {  DCM_RETIRED_ScheduledStudyLocation, "", true },
            {  DCM_RETIRED_ScheduledStudyLocationAETitle, "", true },
            {  DCM_SeriesDescription, "", false },
            {  DCM_ServiceEpisodeDescription, "", true },
            {  DCM_ServiceEpisodeID, "", true },
            {  DCM_SmokingStatus, "", true },
            {  DCM_SourceImageSequence, "", true },
            {  DCM_SpecialNeeds, "", true },
            {  DCM_StationName, "", true },
            {  DCM_RETIRED_StudyComments, "", true },
            {  DCM_StudyDescription, "", false },
            {  DCM_StudyID, "", true },
            {  DCM_RETIRED_StudyIDIssuer, "", true },
            {  DCM_RETIRED_TextComments, "", true },
            {  DCM_TextString, "", true },
            {  DCM_TimezoneOffsetFromUTC, "", true },
            {  DCM_RETIRED_TopicAuthor, "", true },
            {  DCM_RETIRED_TopicKeywords, "", true },
            {  DCM_RETIRED_TopicSubject, "", true },
            {  DCM_RETIRED_TopicTitle, "", true },
            {  DCM_AffectedSOPInstanceUID, "", true },
            {  DCM_DigitalSignatureUID, "", true },
            {  DCM_SourceApplicationEntityTitle, "", true },
            {  DCM_StudyInstanceUID, "__GENUID__", false },
            {  DCM_ReferencedStudySequence, "", true },
            {  DCM_RegionOfResidence, "", true },
            {  DCM_RequestAttributesSequence, "", true },
            {  DCM_RequestedContrastAgent, "", true },
            {  DCM_RequestedProcedureComments, "", true },
            {  DCM_RequestedProcedureDescription, "", true },
            {  DCM_RequestedProcedureID, "", true },
            {  DCM_RequestedProcedureLocation, "", true },
            {  DCM_VerifyingObserverSequence, "", true },
            {  DCM_VerifyingOrganization, "", true },
            {  DCM_VisitComments, "", true },
            {  DCM_PatientIdentityRemoved, "", true },
            {  DCM_DeidentificationMethod, "", true },
            {  DCM_VerifyingObserverName, "ANONYMOUS", true },
            {  DCM_PatientName, "ANONYMOUS", true },
            {  DCM_PatientSex,  "O", false },
            {  DCM_PatientBirthName , "ANONYMOUS", true },
            {  DCM_AcquisitionDateTime, "19700101000000.00", false },
            {  DCM_AcquisitionDate, "19700101", false },
            {  DCM_InstanceCreationDate, "19700101", false },
            {  DCM_AdmittingDate, "19700101", true },
            {  DCM_StudyDate, "19700101", false },
            {  DCM_SeriesDate, "19700101", false },
            {  DCM_ContentDate, "19700101", false },
            {  DCM_PersonName, "ANONYMOUS", true },
            {  DCM_ReferringPhysicianName, "ANONYMOUS", false },
            {  DCM_PlacerOrderNumberImagingServiceRequest, "0", true },
            {  DCM_PatientBirthDate, "19000101", true },
#ifndef PRESERVE_PID
            {  DCM_PatientID, "000000", true },
#endif
            {  DCM_AccessionNumber, "0", false },
            {  DCM_ContentCreatorName, "ANONYMOUS", true },
            {  DCM_ContrastBolusAgent, "XX", true },
            {  DCM_FillerOrderNumberImagingServiceRequest, "0", true },
            {  DCM_RETIRED_FillerOrderNumberImagingServiceRequestRetired , "0", true },
            {  DCM_PersonIdentificationCodeSequence, "0", true },
            {  DCM_VerifyingObserverIdentificationCodeSequence, "0", true },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x10), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x11), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x12), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x13), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x22), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x23), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x24), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x6301,createLowerShort(kp_upper, 0x25), "KRETZ_PRIVATE"), "", false },
            {  DcmTag(0x7fe1,createLowerShort(ku_upper1, 0x01), "KRETZ_US"), "", true},
            {  DcmTag(0x8001,createLowerShort(ku_upper2, 0x01), "KRETZ_US"), "", true},
            {  DcmTag(0x0035,createLowerShort(tp_upper, 0x11), "TRICEFY_PRIVATE"), "", true},
            {  DcmTag(0x0035,createLowerShort(tp_upper, 0x13), "TRICEFY_PRIVATE"), "", true},
            {  DcmTag(0x0035,createLowerShort(tp_upper, 0x15), "TRICEFY_PRIVATE"), "", true},
               //**do these last
            {  DcmTag(0x6301,kp_upper), "", false},
            {  DcmTag(0x6301,0x0000), "", true},
       };
    
   bool SR = false;

   char birthDate[16] = "";
   const char* p = "";  datasetSV->findAndGetString(DCM_PatientBirthDate, p);
   if (!isnull(p) && strlen(p) == 8)
      sprintf(birthDate, "%.3s00101", p);    //**Maintain the decade only

   if (frameCount == 0)  //**See if SR. e don't want to remove all of the SR measurement data
   {   const char* c = "";  datasetSV->findAndGetString(DCM_SOPClassUID, c);
       if (!isnull(c) && strncmp(c, "1.2.840.10008.5.1.4.1.1.88", 26) == 0)
          SR = true;
   }

      //**Grab and cache old/new StudyInstanceUID
   char oldStudyID[100] = "";  
   char newStudyID[100] = "";  
   const char* c = ""; datasetSV->findAndGetString(DCM_StudyInstanceUID, c);
   if (!isnull(c))  strcpy(oldStudyID, (char*)c);
   if (!isnull(opt_idCache.c_str()))
   {  char* c = Util::readTxt((char*)opt_idCache.c_str(), oldStudyID);
      if (!isnull(c))
         strcpy(newStudyID, c);
   }   

   for (unsigned int i = 0; i < sizeof(tags)/sizeof(*tags); i++)
   {  const char* c, *uid;
      if (!isnull(tags[i].value))
      {   if (datasetSV->findAndGetString(tags[i].sAttr, c).good() && !isnull(c))
          {  
             if ((strict && !opt_vp6) && strcmp(tags[i].value, "__GENUID__") == 0 && tags[i].sAttr == DCM_StudyInstanceUID && !isnull(newStudyID))
               datasetSV->putAndInsertString(tags[i].sAttr, newStudyID);
                 
             else if (strcmp(tags[i].value, "__GENUID__") == 0)
             {  const char* p2 = "";  datasetSV->findAndGetString(DCM_PatientName, p2);
                const char* p3 = "";  datasetSV->findAndGetString(DCM_StudyDate, p3);
                const char* pT = "";  datasetSV->findAndGetString(DCM_StudyTime, pT);
                const char* p4 = "";  datasetSV->findAndGetString(DCM_PatientID, p4);
                const char* p5 = "";  datasetSV->findAndGetString(DCM_PatientBirthDate, p5);
                const char* pU = oldStudyID;
                char dt[32];  memset(dt, '\0', sizeof(dt)); //**yymmddhhmm
                if (strlen(p3) >= 8 && !isnull(pT))
                   strncpy(dt,p3+2,6);
                else if (isnull(pT))
                   strncpy(dt,p3,10);
                else strncpy(dt, p3, 6);
                if (!isnull(pT)) strncat(dt, pT, 4);
                if (strict && !opt_vp6) 
                {   datasetSV->putAndInsertString(tags[i].sAttr,  uid=genInstanceFromParts(c, isnull(p2)?pU:p2, dt, isnull(p4)?p5:p4));
                    if (tags[i].sAttr == DCM_StudyInstanceUID && !isnull(opt_idCache.c_str()) && !isnull(uid))
                      ::writeTxt((char*)opt_idCache.c_str(), oldStudyID, (char*)uid);
                }
             }
             else if (strncmp(tags[i].value, "19700101", 8) == 0 && (strict || tags[i].strict))   //**Preserve year only of the date in question
             {    const char* p = "";  datasetSV->findAndGetString(tags[i].sAttr, p);
                  char nDate[strlen(tags[i].value)+1];  strcpy(nDate, tags[i].value);
                  strncpy(nDate, p, 4);  //**Copy year
                  datasetSV->putAndInsertString(tags[i].sAttr,  nDate);
             }
             else if (strcmp(tags[i].value, "19000101") == 0 && !isnull(birthDate)) 
                  datasetSV->putAndInsertString(tags[i].sAttr,  birthDate);  //**Preserve decade only of the birthdate
             else if (strict || tags[i].strict)
                  datasetSV->putAndInsertString(tags[i].sAttr,  tags[i].value);
          }
      }
      else  if (strict || tags[i].strict)
          datasetSV->findAndDeleteElement(tags[i].sAttr, true, true);  //*Deleted in sequences too
   }
   if (!SR || strict)
      datasetSV->findAndDeleteElement(DCM_ContentSequence, true, true);
   {      //**remove embedded pixel elements before pixel data is manipulated
       Memory mem;
       Array<DcmTag> tags1;
       DcmStack stack;
       DcmTag tag1;
       bool gonner = false;
          //**Loop through looking for embedded pixel elements
       while (datasetSV->nextObject(stack, OFTrue).good())
       {  if (stack.top()->isNested() == false)
          {   tag1 = stack.top()->getTag();   //**Save top-level tag
              gonner = false;
          }
          else
          {   if (stack.top()->getTag() == DCM_PixelData) //**Found one;  note the top-level tag to delete
                 gonner = true;
                 //**Remove embedded attributes that could be self-identifying
              else if (!SR||strict) for (unsigned int i = 0; i < sizeof(tags)/sizeof(*tags); i++)
                 if (tags[i].sAttr == stack.top()->getTag() && (strict || tags[i].strict) && stack.top()->getVR() != EVR_TM && stack.top()->getVR() != EVR_UI)  
                 {  gonner = true; break; }
          }
          if (gonner)
          {   DcmTag* t = (DcmTag*)mem.malloc(sizeof(DcmTag));  
              memcpy(t, &tag1, sizeof(DcmTag));
              tags1.add(t);
              gonner = false;
          }
       }
          //**Remove any tags found above
       for (int i = 0; i < tags1.size(); i++)
           datasetSV->findAndDeleteElement(*tags1.get(i), true, true);
   }

   datasetSV->putAndInsertString(DCM_PatientIdentityRemoved, "YES");
   datasetSV->putAndInsertString(DCM_LongitudinalTemporalInformationModified, "MODIFIED");
   datasetSV->putAndInsertString(DCM_BurnedInAnnotation , "NO");
   if (strict && isnull(realFileName))
       datasetSV->putAndInsertString(DCM_DeidentificationMethod, "Application Level Confidentiality Profile (for Clinical Trials) + Clean Pixel Data Option + Clean Structured Content Option by Tricefy");  //**Will search for "Tricefy" to determine if we anonymized it
   if (opt_uplink)
      datasetSV->putAndInsertString(DCM_ManufacturerModelName, "TRICEFY UPLINK");
   if (frameCount > 1)
   {   char numBuf[20];
       sprintf(numBuf, "%ld", frameCount);
       datasetSV->putAndInsertString(DCM_NumberOfFrames, numBuf);
   }
        //*Pseudinomization : subject-id_site-id_study-seq_unique-stuff.dcm
   if (!isnull(realFileName))
   {  int num;  char** pieces = Util::split(Util::replace((char*)realFileName, ".dcm", ""), '_', num);
      OFString name;
      if (num > 0)
      {   name.append(pieces[0]).append("^");
          datasetSV->putAndInsertString(DCM_PatientID, pieces[0]);
      }
      if (num > 1)
          name.append(pieces[1]);
      if (num > 2)
          datasetSV->putAndInsertString(DCM_AccessionNumber, pieces[2]);
      if (num > 0)
          datasetSV->putAndInsertString(DCM_PatientName, name.c_str());
   }
}

template <typename T>
static void FillRegionWithColor(T *p, const unsigned int *dims, const unsigned int * region, T color=0)
{
    unsigned int xmin = region[0];
    unsigned int xmax = region[1]-1;
    unsigned int ymin = region[2];
    unsigned int ymax = region[3]-1;

    for( unsigned int x = xmin; x <= xmax; ++x)
      for( unsigned int y = ymin; y <= ymax; ++y)
         p[ x+y*dims[0] ] = color;  //**black
}

template <typename T>
static void FillLastRegionWithColor(T *p, const unsigned int *dims, int subheight, T color = 0)
{
    int xmin = 0;
    int ymin = 0;
    int width = dims[0];
    int height = dims[1];

        //**Traverse from the bottom looking for the first non-white char
    int x; int y; bool found = false;
    for(y = height-1; !found && y >= 0; --y)
       for(x=width-1 ; !found && x >= 0; --x)
          if (p[ x+y*dims[0] ] != color)
               found = true;

    int ymax = y;   
    if (ymax < 0 )  ymax = 0;
    ymin = ymax - subheight;   //**Subheight in pixels
    if (ymin < 0)  ymin = 0;

    for( unsigned int x = 0; x <= width; ++x)
      for( unsigned int y = ymin; y <= height; ++y)
         p[ x+y*dims[0] ] = color;  
}

#endif
// ********************************************

int main(int argc, char *argv[])
{
    OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, OFFIS_CONSOLE_DESCRIPTION, rcsid);
    OFCommandLine cmd;

    OFBool opt_uidCreation = OFFalse;   //**already done as part of anonymization
    E_FileReadMode opt_readMode = ERM_autoDetect;
    E_FileWriteMode opt_writeMode = EWM_fileformat;
    E_TransferSyntax opt_ixfer = EXS_Unknown;
    E_TransferSyntax opt_oxfer = EXS_Unknown;
    E_GrpLenEncoding opt_oglenc = EGL_recalcGL;
    E_EncodingType opt_oenctype = EET_ExplicitLength;
    E_PaddingEncoding opt_opadenc = EPD_noChange;
    OFCmdUnsignedInt opt_filepad = 0;
    OFCmdUnsignedInt opt_itempad = 0;

#ifdef TRICE
       // JPEG parameters  decompress
    //E_SubSampling opt_sampling = ESS_422;              /* default: 4:2:2 sub-sampling */
    E_DecompressionColorSpaceConversion opt_decompCSconversion = EDC_photometricInterpretation;
       // JPEG options
    E_TransferSyntax opt_oxferJ = EXS_JPEGProcess1;  
    //OFCmdUnsignedInt opt_selection_value = 6;
    //OFCmdUnsignedInt opt_point_transform = 0;
    OFBool           opt_huffmanOptimize = OFTrue;
    OFCmdUnsignedInt opt_smoothing = 0;
    int              opt_compressedBits = 0; // 0=auto, 8/12/16=force
    E_CompressionColorSpaceConversion opt_compCSconversion = ECC_lossyYCbCr;
    E_SubSampling    opt_sampleFactors = ESS_444;
    OFBool           opt_useYBR422 = OFFalse;
    OFCmdUnsignedInt opt_fragmentSize = 0; // 0=unlimited
    OFBool           opt_createOffsetTable = OFFalse;
    int              opt_windowType = 0;  /* default: no windowing; 1=Wi, 2=Wl, 3=Wm, 4=Wh, 5=Ww, 6=Wn, 7=Wr */
    OFCmdUnsignedInt opt_windowParameter = 0;
    OFCmdFloat       opt_windowCenter=0.0, opt_windowWidth=0.0;
    E_UIDCreation    opt_uidcreation = EUC_never;
    OFBool           opt_secondarycapture = OFFalse;
    OFCmdUnsignedInt opt_roiLeft = 0, opt_roiTop = 0, opt_roiWidth = 0, opt_roiHeight = 0;
    OFBool           opt_usePixelValues = OFTrue;
    OFBool           opt_useModalityRescale = OFFalse;
    OFBool           opt_trueLossless = OFTrue;
    OFString         opt_realName; 
    //OFBool           lossless = OFTrue;  /* see opt_oxfer */
#endif

    int opt_useAspectRatio = 1;                        /* default: use aspect ratio for scaling */
    OFCmdUnsignedInt opt_useInterpolation = 1;         /* default: use interpolation method '1' for scaling */
    int opt_scaleType = 0;                             /* default: no scaling */
                                                       /* 1 = X-factor, 2 = Y-factor, 3=X-size, 4=Y-size */
    OFCmdFloat opt_scale_factor = 1.0;
    OFCmdUnsignedInt opt_scale_size = 1;

    OFBool           opt_useClip = OFFalse;            /* default: don't clip */
    OFCmdSignedInt   opt_left = 0, opt_top = 0;        /* clip region (origin) */
    OFCmdUnsignedInt opt_width = 0, opt_height = 0;    /* clip region (extension) */

    const char *opt_ifname = NULL;
    const char *opt_ofname = NULL;

#ifdef TRICE
    const char*         opt_regionDef = "";               /* clip region, possibly in percents x1|y1|x2|y2 */
    const char*         opt_lastRegion = "";              /* clip end of page for pdf anon */
    OFString            opt_processDir = "";              // Where to find processes, it nullstring, use PATH
    char*               opt_dir = (char*)"";                     // directory with files
#if defined(GE) || defined(UNC) 
    bool                opt_anonymize = true;            // Full anonymization
    bool                opt_anon_strict = true;        //  strict anonymzation
    bool                opt_uplink = true;               // anonymized in the uplink
#else
    bool                opt_anonymize = false;            // Full anonymization
    bool                opt_anon_strict = false;        //  strict anonymzation
    bool                opt_uplink = false;               // anonymized in the uplink
#endif
    OFBool              opt_pixelRubOut = false;          // RubOut pixles instead of clipping
    unsigned int topRub[6], bottomRub[6], rightRub[6], leftRub[6]; //Rubout regions
    bool do_topRub = false, do_rightRub = false, do_leftRub = false, do_bottomRub = false, opt_use_bg_color = false;
#endif

    prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
    cmd.setOptionColumns(LONGCOL, SHORTCOL);

    cmd.addParam("dcmfile-in",  "DICOM input filename to be scaled");
    cmd.addParam("dcmfile-out", "DICOM output filename to be written");

    cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
     cmd.addOption("--help",                "-h",       "print this help text and exit", OFCommandLine::AF_Exclusive);
     cmd.addOption("--version",                         "print version information and exit", OFCommandLine::AF_Exclusive);
     OFLog::addOptions(cmd);

    cmd.addGroup("input options:");
     cmd.addSubGroup("input file format:");
      cmd.addOption("--read-file",          "+f",       "read file format or data set (default)");
      cmd.addOption("--read-file-only",     "+fo",      "read file format only");
      cmd.addOption("--read-dataset",       "-f",       "read data set without file meta information");
     cmd.addSubGroup("input transfer syntax:");
      cmd.addOption("--read-xfer-auto",     "-t=",      "use TS recognition (default)");
      cmd.addOption("--read-xfer-detect",   "-td",      "ignore TS specified in the file meta header");
      cmd.addOption("--read-xfer-little",   "-te",      "read with explicit VR little endian TS");
      cmd.addOption("--read-xfer-big",      "-tb",      "read with explicit VR big endian TS");
      cmd.addOption("--read-xfer-implicit", "-ti",      "read with implicit VR little endian TS");

    cmd.addGroup("image processing and encoding options:");
#ifdef TRICE
     cmd.addSubGroup("color space conversion options (compressed images only):");
      cmd.addOption("--conv-photometric",   "+cp",      "convert if YCbCr photometric interpr. (default)");
      cmd.addOption("--conv-lossy",         "+cl",      "convert YCbCr to RGB if lossy JPEG");
      cmd.addOption("--conv-guess",         "+cg",      "convert to RGB if YCbCr is guessed by library");
      cmd.addOption("--conv-guess-lossy",   "+cgl",     "convert to RGB if lossy JPEG and YCbCr is\nguessed by the underlying JPEG library");
      cmd.addOption("--conv-always",        "+ca",      "always convert YCbCr to RGB");
      cmd.addOption("--conv-never",         "+cn",      "never convert color space");
#endif
     cmd.addSubGroup("scaling:");
      cmd.addOption("--recognize-aspect",    "+a",      "recognize pixel aspect ratio (default)");
      cmd.addOption("--ignore-aspect",       "-a",      "ignore pixel aspect ratio when scaling");
      cmd.addOption("--interpolate",         "+i",   1, "[n]umber of algorithm: integer",
                                                        "use interpolation when scaling (1..4, def: 1)");
      cmd.addOption("--no-interpolation",    "-i",      "no interpolation when scaling");
      cmd.addOption("--no-scaling",          "-S",      "no scaling, ignore pixel aspect ratio (default)");
      cmd.addOption("--scale-x-factor",      "+Sxf", 1, "[f]actor: float",
                                                        "scale x axis by factor, auto-compute y axis");
      cmd.addOption("--scale-y-factor",      "+Syf", 1, "[f]actor: float",
                                                        "scale y axis by factor, auto-compute x axis");
      cmd.addOption("--scale-x-size",        "+Sxv", 1, "[n]umber: integer",
                                                        "scale x axis to n pixels, auto-compute y axis");
      cmd.addOption("--scale-y-size",        "+Syv", 1, "[n]umber: integer",
                                                        "scale y axis to n pixels, auto-compute x axis");
     cmd.addSubGroup("other transformations:");
      cmd.addOption("--clip-region",         "+C",   4, "[l]eft [t]op [w]idth [h]eight: integer",
                                                        "clip rectangular image region (l, t, w, h)");
     cmd.addSubGroup("SOP Instance UID:");
      cmd.addOption("--uid-always",          "+ua",     "always assign new SOP Instance UID (default)");
      cmd.addOption("--uid-never",           "+un",     "never assign new SOP Instance UID");

  cmd.addGroup("output options:");
    cmd.addSubGroup("output file format:");
      cmd.addOption("--write-file",          "+F",      "write file format (default)");
      cmd.addOption("--write-dataset",       "-F",      "write data set without file meta information");
    cmd.addSubGroup("output transfer syntax:");
      cmd.addOption("--write-xfer-same",     "+t=",     "write with same TS as input (default)");
      cmd.addOption("--write-xfer-little",   "+te",     "write with explicit VR little endian TS");
      cmd.addOption("--write-xfer-big",      "+tb",     "write with explicit VR big endian TS");
      cmd.addOption("--write-xfer-implicit", "+ti",     "write with implicit VR little endian TS");
    cmd.addSubGroup("post-1993 value representations:");
      cmd.addOption("--enable-new-vr",       "+u",      "enable support for new VRs (UN/UT) (default)");
      cmd.addOption("--disable-new-vr",      "-u",      "disable support for new VRs, convert to OB");
    cmd.addSubGroup("group length encoding:");
      cmd.addOption("--group-length-recalc", "+g=",     "recalculate group lengths if present (default)");
      cmd.addOption("--group-length-create", "+g",      "always write with group length elements");
      cmd.addOption("--group-length-remove", "-g",      "always write without group length elements");
    cmd.addSubGroup("length encoding in sequences and items:");
      cmd.addOption("--length-explicit",     "+e",      "write with explicit lengths (default)");
      cmd.addOption("--length-undefined",    "-e",      "write with undefined lengths");
    cmd.addSubGroup("data set trailing padding (not with --write-dataset):");
      cmd.addOption("--padding-retain",      "-p=",     "do not change padding\n(default if not --write-dataset)");
      cmd.addOption("--padding-off",         "-p",      "no padding (implicit if --write-dataset)");
      cmd.addOption("--padding-create",      "+p",   2, "[f]ile-pad [i]tem-pad: integer",
                                                        "align file on multiple of f bytes\nand items on multiple of i bytes");
#ifdef TRICE
    cmd.addGroup("trice options:");
      cmd.addOption("--dict-path",          "-dp",  1, "[p]ath: string", "complete path (including file name) for dicom.dic");
      cmd.addOption("--clip-percent",       "+CP",  1, "[P]ipe delimited region definition: string",
                                                       "clip image region x1|y1|x2|y2");
      cmd.addOption("--anonymize",          "+an",     "anonymize DICOM fully");
      cmd.addOption("--uplink",             "+up",     "anonymize in the uplink");
      cmd.addOption("--vp6",                "+vp",     "VP6 add-on");
      cmd.addOption("--high-priority",      "+hp",     "run without low priority");
      cmd.addOption("--process-dir",        "-pr",  1, "[p]ath: string", "where to find the processes needed (dcmdump, dump2dcm)");
      cmd.addOption("--strict-anon",        "+sa",     "Strict anonymization");
      cmd.addOption("--non-strict-anon",    "+na",     "Non-Strict anonymization");
      cmd.addOption("--pseud",              "-ps",  1, "[f]fileName; string",  "name as id_site_sequence_other.dcm");
      cmd.addOption("--cache",              "-ca",  1, "[d]irectoryName; string",  "directory/ for idCache ");
      cmd.addOption("--last-region",        "+lr",  1, "height of region at end of page to anonymize", "height in pixels or as a percent");
      cmd.addOption("--bg-color",           "+bg",     "Use blackground color (pixel 0) to anonymize");
      cmd.addOption("--help", "-?", "dcmaudscale.  This is the logic that will do anonymization cropping on any DICOM .  It handles all of the decoding issues - so you should be able to call it without any preprocessing of the DICOM \n \
 \n \
 * dcmaudscale --anonymize --clip-percent  \"x|y|width|length\" inputFile outputFile \n \
    * the --clip-percent should be the exact thing from the anon table.  You don't need to do any processing of them .   \n \
    * inputFile - the location of the input file.   \n \
    * outputFile - the location of the output file. \n \
    * Return Info \n \
        * Command exit(0) if successful.  Any error info will come via stderr\n");

#endif

    if (app.parseCommandLine(cmd, argc, argv))
    {
      /* check exclusive options first */
      if (cmd.hasExclusiveOption())
      {
          if (cmd.findOption("--version"))
          {
              app.printHeader(OFTrue /*print host identifier*/);
              COUT << OFendl << "External libraries used:";
#if !defined(WITH_ZLIB) && !defined(TRICE)
              COUT << " none" << OFendl;
#else
              COUT << OFendl;
#endif
#ifdef WITH_ZLIB
              COUT << "- ZLIB, Version " << zlibVersion() << OFendl;
#endif
#ifdef TRICE
              COUT << "- " << DiJPEGPlugin::getLibraryVersionString() << OFendl;
#endif
              return 0;
          }
      }

      /* command line parameters */

      cmd.getParam(1, opt_ifname);
      cmd.getParam(2, opt_ofname);

      /* general options */

      OFLog::configureFromCommandLine(cmd, app);

      /* input options */
#ifdef TRICE
      if (cmd.findOption("--anonymize"))
          opt_anonymize = true;
      if (cmd.findOption("--uplink"))
      {   opt_uplink = opt_anonymize = true;
          opt_uidCreation = OFFalse;
#ifdef __windows__
          if (!cmd.findOption("--high-priority"))
             SetPriorityClass(GetCurrentProcess(), 1048576);  //**Set to background priority
#endif
      }
      else
          opt_anon_strict = true;
      if (cmd.findOption("--strict-anon"))
      {   opt_anon_strict = opt_anonymize = true;   }
      if (cmd.findOption("--non-strict-anon"))
      {   opt_anonymize = true;   opt_anon_strict = false; }
      if (cmd.findOption("--last-region"))
          app.checkValue(cmd.getValue(opt_lastRegion));
      if (cmd.findOption("--pseud"))
          app.checkValue(cmd.getValue(opt_realName));
      if (cmd.findOption("--cache"))
          app.checkValue(cmd.getValue(opt_idCache));
      if (cmd.findOption("--bg-color"))
          opt_use_bg_color = true;
      if (cmd.findOption("--vp6"))
          opt_vp6 = true;
#endif

      cmd.beginOptionBlock();
      if (cmd.findOption("--read-file")) opt_readMode = ERM_autoDetect;
      if (cmd.findOption("--read-file-only")) opt_readMode = ERM_fileOnly;
      if (cmd.findOption("--read-dataset")) opt_readMode = ERM_dataset;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--read-xfer-auto"))
          opt_ixfer = EXS_Unknown;
      if (cmd.findOption("--read-xfer-detect"))
          dcmAutoDetectDatasetXfer.set(OFTrue);
#ifdef TRICE
      dcmAutoDetectDatasetXfer.set(OFTrue);
#endif
      if (cmd.findOption("--read-xfer-little"))
      {
          app.checkDependence("--read-xfer-little", "--read-dataset", opt_readMode == ERM_dataset);
          opt_ixfer = EXS_LittleEndianExplicit;
      }
      if (cmd.findOption("--read-xfer-big"))
      {
          app.checkDependence("--read-xfer-big", "--read-dataset", opt_readMode == ERM_dataset);
          opt_ixfer = EXS_BigEndianExplicit;
      }
      if (cmd.findOption("--read-xfer-implicit"))
      {
          app.checkDependence("--read-xfer-implicit", "--read-dataset", opt_readMode == ERM_dataset);
          opt_ixfer = EXS_LittleEndianImplicit;
      }
      cmd.endOptionBlock();

      /* image processing options: color space conversion */

#ifdef TRICE
      cmd.beginOptionBlock();
      if (cmd.findOption("--conv-photometric"))
          opt_decompCSconversion = EDC_photometricInterpretation;
      if (cmd.findOption("--conv-lossy"))
          opt_decompCSconversion = EDC_lossyOnly;
      if (cmd.findOption("--conv-guess"))
          opt_decompCSconversion = EDC_guess;
      if (cmd.findOption("--conv-guess-lossy"))
          opt_decompCSconversion = EDC_guessLossyOnly;
      if (cmd.findOption("--conv-always"))
          opt_decompCSconversion = EDC_always;
      if (cmd.findOption("--conv-never"))
          opt_decompCSconversion = EDC_never;
      cmd.endOptionBlock();
#endif

      /* image processing options: scaling */

      cmd.beginOptionBlock();
      if (cmd.findOption("--recognize-aspect"))
          opt_useAspectRatio = 1;
      if (cmd.findOption("--ignore-aspect"))
          opt_useAspectRatio = 0;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--interpolate"))
          app.checkValue(cmd.getValueAndCheckMinMax(opt_useInterpolation, 1, 4));
      if (cmd.findOption("--no-interpolation"))
          opt_useInterpolation = 0;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--no-scaling"))
          opt_scaleType = 0;
      if (cmd.findOption("--scale-x-factor"))
      {
          opt_scaleType = 1;
          app.checkValue(cmd.getValueAndCheckMin(opt_scale_factor, 0.0, OFFalse));
      }
      if (cmd.findOption("--scale-y-factor"))
      {
          opt_scaleType = 2;
          app.checkValue(cmd.getValueAndCheckMin(opt_scale_factor, 0.0, OFFalse));
      }
      if (cmd.findOption("--scale-x-size"))
      {
          opt_scaleType = 3;
          app.checkValue(cmd.getValueAndCheckMin(opt_scale_size, 1));
      }
      if (cmd.findOption("--scale-y-size"))
      {
          opt_scaleType = 4;
          app.checkValue(cmd.getValueAndCheckMin(opt_scale_size, 1));
      }
      cmd.endOptionBlock();

      /* image processing options: other transformations */

#ifdef TRICE
      if (cmd.findOption("--clip-percent"))
      {   app.checkValue(cmd.getValue(opt_regionDef));
               //**Check to ensure 3 pipes
            bool ok = true;  char* p = (char*)opt_regionDef;
            for (int i = 0; i < 3; i++)
            {  char* pp = strchr(p, '|');
               if (pp == NULL)  {  ok = false;  break; }
               p = pp+1;
            }
            if (!ok)
            {   OFLOG_FATAL(dcmaudscaleLogger, "Format of --clip-percent incorrect (x1|y1|x2|y2)\n");
                return 1;
            }
      }
#endif
      if (cmd.findOption("--clip-region"))
      {
          app.checkValue(cmd.getValue(opt_left));
          app.checkValue(cmd.getValue(opt_top));
          app.checkValue(cmd.getValue(opt_width));
          app.checkValue(cmd.getValue(opt_height));
          opt_useClip = OFTrue;
      }

      /* image processing options: SOP Instance UID options */

      cmd.beginOptionBlock();
      if (cmd.findOption("--uid-always")) opt_uidCreation = OFTrue;
      if (cmd.findOption("--uid-never")) opt_uidCreation = OFFalse;
      cmd.endOptionBlock();

      /* output options */

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-file")) opt_writeMode = EWM_fileformat;
      if (cmd.findOption("--write-dataset")) opt_writeMode = EWM_dataset;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--write-xfer-same")) opt_oxfer = EXS_Unknown;
      if (cmd.findOption("--write-xfer-little")) opt_oxfer = EXS_LittleEndianExplicit;
      if (cmd.findOption("--write-xfer-big")) opt_oxfer = EXS_BigEndianExplicit;
      if (cmd.findOption("--write-xfer-implicit")) opt_oxfer = EXS_LittleEndianImplicit;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--enable-new-vr"))
      {
          dcmEnableUnknownVRGeneration.set(OFTrue);
          dcmEnableUnlimitedTextVRGeneration.set(OFTrue);
          dcmEnableOtherFloatStringVRGeneration.set(OFTrue);
          dcmEnableOtherDoubleStringVRGeneration.set(OFTrue);
      }
      if (cmd.findOption("--disable-new-vr"))
      {
          dcmEnableUnknownVRGeneration.set(OFFalse);
          dcmEnableUnlimitedTextVRGeneration.set(OFFalse);
          dcmEnableOtherFloatStringVRGeneration.set(OFFalse);
          dcmEnableOtherDoubleStringVRGeneration.set(OFFalse);
      }
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--group-length-recalc")) opt_oglenc = EGL_recalcGL;
      if (cmd.findOption("--group-length-create")) opt_oglenc = EGL_withGL;
      if (cmd.findOption("--group-length-remove")) opt_oglenc = EGL_withoutGL;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--length-explicit")) opt_oenctype = EET_ExplicitLength;
      if (cmd.findOption("--length-undefined")) opt_oenctype = EET_UndefinedLength;
      cmd.endOptionBlock();

      cmd.beginOptionBlock();
      if (cmd.findOption("--padding-retain"))
      {
          app.checkConflict("--padding-retain", "--write-dataset", opt_writeMode == EWM_dataset);
          opt_opadenc = EPD_noChange;
      }
      if (cmd.findOption("--padding-off")) opt_opadenc = EPD_withoutPadding;
      if (cmd.findOption("--padding-create"))
      {
          app.checkConflict("--padding-create", "--write-dataset", opt_writeMode == EWM_dataset);
          app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
          app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
          opt_opadenc = EPD_withPadding;
      }
      cmd.endOptionBlock();
    }

    /* print resource identifier */
    OFLOG_DEBUG(dcmaudscaleLogger, rcsid << OFendl);

#ifdef TRICE
    if (cmd.findOption("--process-dir"))
    {   app.checkValue(cmd.getValue(opt_processDir));
        if (!OFStandard::dirExists(opt_processDir))
        {   fprintf(stderr, "Process Directory %s does not exist\n", opt_processDir.c_str());
            exit(1);
        }
    }

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
#endif

    /* make sure data dictionary is loaded */
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFLOG_WARN(dcmaudscaleLogger, "no data dictionary loaded, check environment variable: " << DCM_DICT_ENVIRONMENT_VARIABLE);
    }

#ifdef TRICE
    DcmRLEEncoderRegistration::registerCodecs();    
    // register RLE decompression codec
    DcmRLEDecoderRegistration::registerCodecs();
    // register JPEG-LS decompression codecs
    DJLSDecoderRegistration::registerCodecs();
    // register JPEG-LS compression codecs
    DJLSEncoderRegistration::registerCodecs();
    // register global decompression codecs
    DJDecoderRegistration::registerCodecs(opt_decompCSconversion);
     // register global compression codecs
    DJEncoderRegistration::registerCodecs(
      opt_compCSconversion,
      opt_uidcreation,
      opt_huffmanOptimize,
      OFstatic_cast(int, opt_smoothing),
      opt_compressedBits,
      OFstatic_cast(Uint32, opt_fragmentSize),
      opt_createOffsetTable,
      opt_sampleFactors,
      opt_useYBR422,
      opt_secondarycapture,
      OFstatic_cast(Uint32, opt_windowType),
      OFstatic_cast(Uint32, opt_windowParameter),
      opt_windowCenter,
      opt_windowWidth,
      OFstatic_cast(Uint32, opt_roiLeft),
      OFstatic_cast(Uint32, opt_roiTop),
      OFstatic_cast(Uint32, opt_roiWidth),
      OFstatic_cast(Uint32, opt_roiHeight),
      opt_usePixelValues,
      opt_useModalityRescale,
      OFFalse,
      OFFalse,
      opt_trueLossless);
#endif

    // ======================================================================
    // read input file

    if (isnull(opt_ifname) || (!OFStandard::fileExists(opt_ifname)) || isnull(opt_ofname))
    {
        OFLOG_FATAL(dcmaudscaleLogger, "invalid input filename: <empty string>");
        return 1;
    }

    opt_dir = new char[strlen(opt_ifname)+1];   strcpy(opt_dir, opt_ifname);
    char* slash = strrchr(opt_dir, delimiter[0]);
#ifdef __windows__
    char* slash1 = strrchr(opt_dir, '/');
    if (slash1 > slash)
        slash = slash1;
#endif
    if (slash != NULL)
        *slash = '\0';
    else 
       strcpy(opt_dir, "");
    if (isnull(opt_dir))
    {   char cwd[350];  
#ifdef _WIN32
        GetCurrentDirectory(sizeof(cwd), cwd);
#else
        getcwd(cwd, sizeof(cwd));
#endif
        delete[] opt_dir;
        opt_dir = new char[strlen(cwd)+1];  strcpy(opt_dir, cwd);
    }

/**
    // no clipping/scaling 
    if (!opt_scaleType && !opt_useClip && strcmp(opt_regionDef, "") == 0)
    {
        OFLOG_FATAL(dcmaudscaleLogger, "nothing to do");
        return 1;
    }
**/

    OFLOG_INFO(dcmaudscaleLogger, "open input file " << opt_ifname);

    DcmFileFormat*  dfile = new DcmFileFormat();
    DcmDataset *dataset = dfile-> getDataset();  //** dfile->getMetaInfo() for the header

    bool retry = false;  //**Try once to repair any invalid DICOM encountered
LOAD:
    OFCondition error = dfile->loadFile(opt_ifname, opt_ixfer, EGL_noChange, DCM_MaxReadLength, opt_readMode);
#ifdef TRICE
    OFString ifnameIn, ifname1;
    if (!error.good() && isDicom((char*)opt_ifname))
    {    OFString pgm;
         OFString command = "dcmdump -q +E -vr  -ml +uc +Ep +rd --convert-to-utf8 +L --write-pixel ";
         command.append(opt_dir).append(" ");
         OFStandard::combineDirAndFilename(pgm, opt_processDir, command, true);
         if (!isnull(dicomPath))
            pgm.append(" --dict-path ").append(dicomPath);
         ifnameIn.append(opt_ifname).append(".dump");
         pgm.append(opt_ifname).append(" > ").append(ifnameIn);
         if (system(pgm.c_str()) == 0 || nonZeroFile((char*)ifnameIn.c_str()))  //**Worked
         {    OFString command1 = "dump2dcm -q +E --update-meta-info";
              OFStandard::combineDirAndFilename(pgm, opt_processDir, command1, true);
              if (strcmp(dicomPath, "") != 0)
                  pgm.append(" --dict-path ").append(dicomPath);
              ifname1.append(opt_ifname).append(".OUT");
              pgm.append(" ").append(ifnameIn);
              pgm.append(" ").append(ifname1);
              if (system(pgm.c_str()) == 0)  //**Worked 
              {   mvFile((char*)ifname1.c_str(), (char*)opt_ifname);
#ifndef __windows__
                  fprintf(stderr, "Repairing the file and continuing to anonymize...\n");
#endif
                  error = dfile->loadFile(opt_ifname, opt_ixfer, EGL_noChange, DCM_MaxReadLength, opt_readMode);
              }
         }
         rmFile(ifnameIn.c_str());
         rmFile(ifname1.c_str());
         char cmd1[strlen(opt_dir)+100];
#ifdef __windows__
         sprintf(cmd1, "del /q /F %s\\*.raw* 2> NUL", opt_dir);
         char* c = Util::replace(cmd1, "\\\\", "\\");   strcpy(cmd1, c);
#else
         sprintf(cmd1, "rm -f %s/*.raw*", opt_dir);
#endif
         system(cmd1);
    }
#endif

    if (error.bad())
    {
        OFLOG_FATAL(dcmaudscaleLogger, error.text() << ": reading file: " <<  opt_ifname);
        return 1;
    }

    if (opt_oxfer == EXS_Unknown || opt_anonymize)
    {
        OFLOG_INFO(dcmaudscaleLogger, "set output transfer syntax to input transfer syntax");
        opt_oxfer = dataset->getOriginalXfer();
    }

      //**See if there is any pixel data or any pixel conversions
    const char* ts = "";
    OFCondition result = dataset->findAndGetString(DCM_ManufacturerModelName, ts);
    if (!isnull(ts) && strcmp(ts, "TRICEFY UPLINK") == 0)  //**We've already anonymized this one
    {  cpFile((char*)opt_ifname, (char*)opt_ofname);
       return 0;
    }
    ts = "";
    result = dataset->findAndGetString(DCM_PatientIdentityRemoved, ts);
    if (!isnull(ts) && strcmp(ts, "YES") == 0)  //**See if we have
    {   result = dataset->findAndGetString(DCM_DeidentificationMethod, ts);
        if (!isnull(ts) && strcasestr(ts, "Tricefy") != NULL )  //**anonymized it
        {  cpFile((char*)opt_ifname, (char*)opt_ofname);
           return 0;
        }
    }

    DcmElement* element = NULL;
    result = dataset->findAndGetElement(DCM_PixelData, element);
    if (result.bad() || element == NULL || (!opt_scaleType && !opt_useClip && strcmp(opt_regionDef, "") == 0))  //**No pixel data to process
    {
             //**See if SR
        bool SR = false;
        const char* c = "";  dataset->findAndGetString(DCM_SOPClassUID, c);
        if (!isnull(c) && strncmp(c, "1.2.840.10008.5.1.4.1.1.88", 26) == 0)
           SR = true;

        char birthDate[16] = "";
        const char* p = "";  dataset->findAndGetString(DCM_PatientBirthDate, p);
        if (!isnull(p) && strlen(p) == 8)
           sprintf(birthDate, "[%s]", p);

        char patientID[100] = "";
        char patientID1[100] = "";
        int pidLgth = 0;
        p = "";  dataset->findAndGetString(DCM_PatientID, p);
        if (!isnull(p) && strlen(p) >= 6 && strlen(p) < 100)
        {   strcpy(patientID, p);  pidLgth = strlen(patientID);
            memset(patientID1, '0', pidLgth);
        }
        
        if (opt_anonymize)   //**Anonymize attributes
           trice_anonymize(dataset, opt_uplink, 0, opt_anon_strict, opt_realName.c_str());

            //**Remove all tags w unknown data
        size_t pixelCounter = 0;
        std::ofstream pFile;
        OFString fileName(opt_ofname);  fileName.append(".IN");   OFStandard::deleteFile(fileName);
        pFile.open (fileName.c_str(), std::ios::out | std::ios::app | std::ios::binary); 
        dfile->print(pFile, 0, 0, opt_ofname, &pixelCounter); //**Print both metaheader + dataset
        pFile.close();

              //** remove lines with unknown tags (and full sequences that are unknown)
              //**This anonymizes SR data (DCM_ContentSequence) if it has not been removed above
              //**Also filters out Unk and private attrs and misc Date/Times
        if (opt_anonymize)
        {   OFString realFile;  Memory mem;
            const char* sop = "";  dataset->findAndGetString(DCM_StudyInstanceUID, sop);
            if (isnull(sop)) sop = genSopInstanceId();
            char* meta = readTxt((char*)fileName.c_str());  mem.add(meta);
            int n;  char** lines = Util::split(meta, '\n', n, &mem);
            int seq = 0;
            for (int i = 0; i < n; i++)
            {   char* p;   
                if (SR && !opt_anon_strict && (p=strstr(lines[i], " PN [")) != NULL)    //Anonymize names
                {  char* e = strrchr(lines[i], ']');
                   if (!isnull(e) && e < p+15)  e = p+15;
                   strcpy(p, " PN [ANONYMOUS]");  
                   if (!isnull(e))
                     strcat(lines[i], e+1);
                }
                if (SR && !opt_anon_strict && (p=strstr(lines[i], " UT [")) != NULL)    //Anonymize text fields
                {  char* e = strrchr(lines[i], ']');
                   if (!isnull(e) && e < p+7)  e = p+7;
                   strcpy(p, " UT [ ]");  
                   if (!isnull(e))
                     strcat(lines[i], e+1);
                }
                else if ((p=strstr(lines[i], " TM [")) != NULL)    //Anonymize times across the board
                {  char* e = strrchr(lines[i], ']');
                   if (!isnull(e) && e < p+15)  e = p+15;
		   strcpy(p, " TM [000000.00]");
                   if (!isnull(e))
                     strcat(lines[i], e+1);
                }
                else if (!isnull(birthDate) && (p=strstr(lines[i], birthDate)) != NULL)   //Anonymize birthdate - decade only
                   strncpy(p+3, "00101", 4);
                else if (opt_anon_strict && (p=strstr(lines[i], " DA [")) != NULL)    //Anonymize Dates - year only is allowed
                   strncpy(p+9, "0101", 4);
                else if (opt_anon_strict && (p=strstr(lines[i], " DT [")) != NULL)    //Anonymize Date/time 
                   strncpy(p+9, "010100000000", 10);
                    //**Anonymize other UIDs that could have embedded date info in them
                else if (opt_anon_strict && !opt_vp6 && strstr(lines[i], ROOT_UID) == NULL && strstr(lines[i], "ClassUID") == 0 && (p=strstr(lines[i], " UI [")) != NULL)    
                {   char uid[65]; strncpy(uid, p+5, 64); uid[64] = '\0';
                    char* br = strstr(uid, "]");
                    if (!isnull(br)) *br = '\0';
                    char* uid1 = genInstanceFromUID(uid, sop); 
                    *(p+4) = '\0';  GrowingString l(lines[i]); *(p+4) = '[';
                    l.add("[").add(uid1).add("] # " );
                    char* ll = l.get();
                    if (strlen(ll) < strlen(lines[i]))  strncpy(lines[i], ll, strlen(ll));
                    else  lines[i] = ll; 
                }
#ifndef PRESERVE_PID
                else if (!isnull(patientID) && (p=strstr(lines[i], patientID)) != NULL)   //Anon patientID
                   strncpy(p, patientID1, pidLgth);
#endif
                if (strstr(lines[i], "Unknown Tag & Data") != NULL && strstr(lines[i], ") SQ ") != NULL)
                   seq++;
                else if (seq > 0 && strstr(lines[i], ") SQ ") != NULL)
                   seq++; 
                if ((strstr(lines[i], "Unknown Tag & Data") == NULL && seq == 0 &&
                    strstr(lines[i], "PrivateCreator") == NULL && strstr(lines[i], "PrivateGroupLength") == NULL) ||
                    strstr(lines[i], "(6301,00") != NULL)
                   realFile.append(lines[i]).append("\n");
                else if (seq > 0 && strstr(lines[i], "(fffe,e0dd)") != NULL)
                   seq--; 
            }
            writeTxt((char*)fileName.c_str(), (char*)"", (char*)realFile.c_str());  //**Write out file with no unk. tags
        }

            //**recreate anon DICOM
        OFString pgm;
        OFStandard::combineDirAndFilename(pgm, opt_processDir, "dump2dcm -g -q +E --update-meta-info", true);
          
        (void)rmFile(opt_ofname);
        OFString command(pgm);
        if (strcmp(dicomPath, "") != 0)
            command.append(" --dict-path ").append(dicomPath);
        command.append(" ").append(fileName.c_str()).append(" ").append(opt_ofname);
        if (system(command.c_str()) != 0)
        {   OFString reason("dump2dcm failed ( ");  reason.append(command).append(" )\n");  
            fprintf(stderr, "dcmaudscale failed:  %s\n", reason.c_str());
            rmFile(fileName.c_str());
            exit(1);
        }
        char cmd1[strlen(opt_dir)+100];
#ifdef __windows__
        sprintf(cmd1, "del /q /F %s.*.raw* 2> NUL", opt_ofname);
        char* cc = Util::replace(cmd1, "\\\\", "\\");   strcpy(cmd1, cc);
#else
        sprintf(cmd1, "rm -f %s.*.raw*", opt_ofname);
#endif
        system(cmd1);
        rmFile(fileName.c_str());
        return 0;
    }

    OFString derivationDescription;

    Sint32 frameCount;
    if (dataset->findAndGetSint32(DCM_NumberOfFrames, frameCount).bad() || frameCount < 1)
        frameCount = 1;

    if (opt_anonymize)
        trice_anonymize(dataset, opt_uplink, frameCount, opt_anon_strict, opt_realName.c_str());
       //**Grab SOPInstanceUID to use later in the anon process
    char sop[128] = "";  
    {  const char* c = NULL; dataset->findAndGetString(DCM_StudyInstanceUID, c);
       if (!isnull(c))  strlcpy(sop, c, sizeof(sop));
       else strlcpy(sop, genSopInstanceId(), sizeof(sop));
    }

    unsigned long opt_compatibilityMode  = (opt_scaleType > 0) ? CIF_MayDetachPixelData : 0;

    if (frameCount > 1)
        opt_compatibilityMode |= CIF_UsePartialAccessToPixelData; // | CIF_TakeOverExternalDataset;

bool regionDefCheck = false;
   //**Calculate the frame count
int usedFrameCount = 1;
if (frameCount < 128)
   usedFrameCount = frameCount;
else
   usedFrameCount = 128;  //*Do 128 frames at a time

       //**Write it out as a temporary file
DcmXfer o_xfer(opt_oxfer);
bool encapsulated = o_xfer.isEncapsulated();
char pixelFileName[64+strlen(opt_dir)];  sprintf(pixelFileName, "%s%sUplink.f", opt_dir, delimiter);
size_t pixelCounter = 0;
int delta = 1;
for (int opt_frame = 1;  opt_frame <= frameCount; opt_frame+=delta)
{  
    if (opt_frame == 1)
        delta = 1;
    else
        delta = usedFrameCount;
    if (opt_frame+delta > frameCount)
        delta = frameCount-opt_frame+1;
    if (delta < 1) delta = 1;

    DcmFileFormat* dfileSV = new DcmFileFormat(*dfile);
    DcmDataset* datasetSV = dfileSV->getDataset();

    // ======================================================================
    // image processing starts here

    OFLOG_INFO(dcmaudscaleLogger, "preparing pixel data");
    DicomImage *di = new DicomImage(dfileSV, opt_oxfer, opt_compatibilityMode, opt_frame - 1, delta); 
    if (di == NULL)
    {
        OFLOG_FATAL(dcmaudscaleLogger, "Out of memory");
        return 1;
    }

    if (di->getStatus() != EIS_Normal && !retry)
    {      //**Try to repair the DICOM
        fprintf(stderr, "DICOM is invalid - will attempt to repair it\n");
        char out[strlen(opt_ifname)+128];  sprintf(out, "%s.OUT", opt_ifname);
        char c[strlen(opt_ifname)*2+128];
        sprintf(c, "gdcmconv --jpeg --check-meta %s %s", opt_ifname, out);
        OFString pgm;  OFStandard::combineDirAndFilename(pgm, opt_processDir, c, true);
        if (system(pgm.c_str()) == 0)
           mvFile(out, (char*)opt_ifname);
        else //** Cannot repair
        {  OFLOG_FATAL(dcmaudscaleLogger, "Repair Unsuccessful; " <<  DicomImage::getString(di->getStatus()));
           return 1;
        }
        fprintf(stderr, "Repair successful;  retrying the anonymization\n");
        retry = true;
        goto LOAD;
    }

    DicomImage *newimage = NULL;

    if (opt_useClip)
        OFLOG_INFO(dcmaudscaleLogger, "clipping image to (" << opt_left << "," << opt_top
            << "," << opt_width << "," << opt_height << ")");
    // perform clipping (without scaling)

    unsigned int origWidth = di->getWidth();
    unsigned int origHeight = di->getHeight();
    if (opt_scaleType <= 0)
    {
#ifdef TRICE
        if (!regionDefCheck && strcmp(opt_regionDef, "") != 0)  //**Convert region definition to clip definition
        {    unsigned int width = di->getWidth();
             unsigned int height = di->getHeight();
             char* coord = (char*)opt_regionDef;
             char* pipe = strchr(coord, '|');  *pipe = '\0';
             opt_left = (strchr(coord, '%') != NULL) ? width * atof(coord) * .01 + .5 : atof(coord) + .5;
             if (opt_left > width)  opt_left = 0;
             coord = pipe+1; pipe = strchr(coord, '|');  *pipe = '\0';
             opt_top = (strchr(coord, '%') != NULL) ? height * atof(coord) * .01  + .5 : atof(coord) + .5;
             if (opt_top > height)  opt_top = height * .1 + .5;
             coord = pipe+1; pipe = strchr(coord, '|');  *pipe = '\0';
             opt_width = (strchr(coord, '%') != NULL) ? width * atof(coord) * .01 - opt_left: atof(coord) - opt_left;
             if (opt_width > width)  opt_width = width - opt_left;
             coord = pipe+1; 
             opt_height = (strchr(coord, '%') != NULL) ? height * atof(coord) * .01 - opt_top: atof(coord) - opt_top;
             if (opt_height > height)  opt_height = height - opt_top;
           
             opt_pixelRubOut = 1;
             regionDefCheck = true;

                //**For rubout there could be as many as 4 rectangles to rub out (top, left, right, bottom)
             int left_xmin, left_ymin, left_xmax, left_ymax;
             leftRub[0] = left_xmin = 0;
             leftRub[1] = left_xmax = opt_left;
             leftRub[2] = left_ymin = 0;
             leftRub[3] = left_ymax = height;
             do_leftRub = (left_xmax > 0) ? true : false;

             int top_xmin, top_ymin, top_xmax, top_ymax;
             topRub[0] = top_xmin = 0;
             topRub[1] = top_xmax = width;
             topRub[2] = top_ymin = 0;
             topRub[3] = top_ymax = opt_top;
             do_topRub = (top_ymax > 0) ? true : false;

             int right_xmin, right_ymin, right_xmax, right_ymax;
             rightRub[0] = right_xmin = opt_width+opt_left;
             rightRub[1] = right_xmax = width; 
             rightRub[2] = right_ymin = 0;
             rightRub[3] = right_ymax = height;
             do_rightRub = (right_xmin < width-1) ? true : false;

             int bottom_xmin, bottom_ymin, bottom_xmax, bottom_ymax;
             bottomRub[0] = bottom_xmin = 0;
             bottomRub[1] = bottom_xmax = width;
             bottomRub[2] = bottom_ymin = opt_height+opt_top;
             bottomRub[3] = bottom_ymax = height;
             do_bottomRub = (bottom_ymin < height-1) ? true : false;
        }
        if (opt_width == 0)  opt_width = di->getWidth();
        if (opt_height == 0) opt_height = di->getHeight();
#endif

        if (opt_useClip)
        {
            newimage = di->createClippedImage(opt_left, opt_top, opt_width, opt_height);
            derivationDescription = "Clipped rectangular image region";
            if (opt_anonymize && !isnull(opt_realName.c_str()))
               derivationDescription = "Pixels rubbed out for pseudonimization";
            else if (opt_anonymize)
               derivationDescription = "Clipped rectangular image region for anonymization";
        }
#ifdef TRICE
        else if (opt_pixelRubOut)
        {    unsigned int dims[] = { di->getWidth(), di->getHeight() };
             newimage = di->createClippedImage(0, 0, dims[0], dims[1]);
             DiPixel* pixels = (DiPixel*)newimage->getInterData();
             void* fullPixelData = (void*)pixels->getData();
             EP_Representation rep = pixels->getRepresentation();
             int samplesPerPixel = pixels->getPlanes();

                // get pointer to internal raw representation of image data
             void *planes[3] = {NULL, NULL, NULL};
             if (samplesPerPixel == 3)
             {    // for color images, dinter->getData() returns a pointer to an array
                  // of pointers pointing to the real plane data
                 void** draw_array = (void**)fullPixelData;
                 planes[0] = draw_array[0];
                 planes[1] = draw_array[1];
                 planes[2] = draw_array[2];
             }
             else
             {    // for monochrome images, dinter->getData() directly returns a pointer
                  // to the single monochrome plane.
                 planes[0] = fullPixelData;
             }

                //**Calculate what is black for the various color schemes
             int lr = 0;
             if (!isnull(opt_lastRegion))
             {   lr = atoi(opt_lastRegion);
                 if (strchr(opt_lastRegion, '%') != NULL)
                    lr = dims[1] * lr * .01  + .5;
             }
             OFString photo;
             dataset->findAndGetOFString(DCM_PhotometricInterpretation, photo);
             for (int i = 0; i < delta; i++)   //loop per frame
             {  for (int j = 0; j < samplesPerPixel; j++)  //loop per sample
                {   if (rep == EPR_Sint8 || rep == EPR_Uint8)
                    {   Uint8* pixelData = (Uint8 *)planes[j] + i*dims[0]*dims[1];
                        Uint8 black = 0;
                        if (photo == "MONOCHROME1")
                            black = 255;
                        if (opt_use_bg_color)
                           black = pixelData[0];
                        //if (photo == "YBR_FULL" && j > 0) black = 128;
                        if (do_topRub)
                           FillRegionWithColor<Uint8>(pixelData, dims, topRub, black);
                        if (do_rightRub)
                           FillRegionWithColor<Uint8>(pixelData, dims, rightRub, black);
                        if (do_leftRub)
                           FillRegionWithColor<Uint8>(pixelData, dims, leftRub, black);
                        if (do_bottomRub)
                           FillRegionWithColor<Uint8>(pixelData, dims, bottomRub, black);
                        if (do_bottomRub && !isnull(opt_lastRegion))
                           FillLastRegionWithColor<Uint8>(pixelData, dims, lr, black);
                    }
                    else if (rep == EPR_Sint16 || rep == EPR_Uint16)
                    {   Uint16* pixelData = (Uint16 *)planes[j] + i*dims[0]*dims[1];
                        Uint16 black = 0;
                        if (photo == "MONOCHROME1")
                            black = 65535;
                        if (opt_use_bg_color)
                           black = pixelData[0];
                        //if (photo == "YBR_FULL" && j > 0) black = 32768;
                        if (do_topRub)
                           FillRegionWithColor<Uint16>(pixelData, dims, topRub, black);
                        if (do_rightRub)
                           FillRegionWithColor<Uint16>(pixelData, dims, rightRub, black);
                        if (do_leftRub)
                           FillRegionWithColor<Uint16>(pixelData, dims, leftRub, black);
                        if (do_bottomRub)
                           FillRegionWithColor<Uint16>(pixelData, dims, bottomRub, black);
                        if (do_bottomRub && !isnull(opt_lastRegion))
                           FillLastRegionWithColor<Uint16>(pixelData, dims, lr, black);
                    }
                    else if (rep == EPR_Sint32 || rep == EPR_Uint32)
                    {   Uint32* pixelData = (Uint32 *)planes[j] + i*dims[0]*dims[1];
                        Uint32 black = 0;
                        if (photo == "MONOCHROME1")
                            black = 4294967295;
                        if (opt_use_bg_color)
                           black = pixelData[0];
                        //if (photo == "YBR_FULL" && j > 0) black = 2147483648;
                        if (do_topRub)
                           FillRegionWithColor<Uint32>(pixelData, dims, topRub, black);
                        if (do_rightRub)
                           FillRegionWithColor<Uint32>(pixelData, dims, rightRub, black);
                        if (do_leftRub)
                           FillRegionWithColor<Uint32>(pixelData, dims, leftRub, black);
                        if (do_bottomRub)
                           FillRegionWithColor<Uint32>(pixelData, dims, bottomRub, black);
                        if (do_bottomRub && !isnull(opt_lastRegion))
                           FillLastRegionWithColor<Uint32>(pixelData, dims, lr, black);
                    }
                }
             }
             derivationDescription = "Pixels rubbed out in image region(s)";
             if (opt_anonymize && !isnull(opt_realName.c_str()))
                derivationDescription = "Pixels rubbed out for pseudonimization";
             else if (opt_anonymize)
                derivationDescription = "Pixels rubbed out for anonymization";
        }
#endif
    }
    // perform scaling (and possibly clipping)
    else if (opt_scaleType <= 4)
    {
        switch (opt_scaleType)
        {
            case 1:
                OFLOG_INFO(dcmaudscaleLogger, "scaling image, X factor=" << opt_scale_factor
                    << ", Interpolation=" << OFstatic_cast(int, opt_useInterpolation)
                    << ", Aspect Ratio=" << (opt_useAspectRatio ? "yes" : "no"));
                if (opt_useClip)
                    newimage = di->createScaledImage(opt_left, opt_top, opt_width, opt_height, opt_scale_factor, 0.0,
                        OFstatic_cast(int, opt_useInterpolation), opt_useAspectRatio);
                else
                    newimage = di->createScaledImage(opt_scale_factor, 0.0, OFstatic_cast(int, opt_useInterpolation),
                        opt_useAspectRatio);
                break;
            case 2:
                OFLOG_INFO(dcmaudscaleLogger, "scaling image, Y factor=" << opt_scale_factor
                    << ", Interpolation=" << OFstatic_cast(int, opt_useInterpolation)
                    << ", Aspect Ratio=" << (opt_useAspectRatio ? "yes" : "no"));
                if (opt_useClip)
                    newimage = di->createScaledImage(opt_left, opt_top, opt_width, opt_height, 0.0, opt_scale_factor,
                        OFstatic_cast(int, opt_useInterpolation), opt_useAspectRatio);
                else
                    newimage = di->createScaledImage(0.0, opt_scale_factor, OFstatic_cast(int, opt_useInterpolation),
                        opt_useAspectRatio);
                break;
            case 3:
                OFLOG_INFO(dcmaudscaleLogger, "scaling image, X size=" << opt_scale_size
                    << ", Interpolation=" << OFstatic_cast(int, opt_useInterpolation)
                    << ", Aspect Ratio=" << (opt_useAspectRatio ? "yes" : "no"));
                if (opt_useClip)
                    newimage = di->createScaledImage(opt_left, opt_top, opt_width, opt_height, opt_scale_size, 0,
                        OFstatic_cast(int, opt_useInterpolation), opt_useAspectRatio);
                else
                    newimage = di->createScaledImage(opt_scale_size, 0, OFstatic_cast(int, opt_useInterpolation),
                        opt_useAspectRatio);
                break;
            case 4:
                OFLOG_INFO(dcmaudscaleLogger, "scaling image, Y size=" << opt_scale_size
                    << ", Interpolation=" << OFstatic_cast(int, opt_useInterpolation)
                    << ", Aspect Ratio=" << (opt_useAspectRatio ? "yes" : "no"));
                if (opt_useClip)
                    newimage = di->createScaledImage(opt_left, opt_top, opt_width, opt_height, 0, opt_scale_size,
                        OFstatic_cast(int, opt_useInterpolation), opt_useAspectRatio);
                else
                    newimage = di->createScaledImage(0, opt_scale_size, OFstatic_cast(int, opt_useInterpolation),
                        opt_useAspectRatio);
                break;
            default:
                break;
        }
        if (opt_useClip)
            derivationDescription = "Scaled rectangular image region";
        else
            derivationDescription = "Scaled image";
    }
    if (opt_scaleType > 4)
        OFLOG_ERROR(dcmaudscaleLogger, "internal error: unknown scaling type");
    else if (newimage == NULL)
    {
#ifdef TRICE
           ;   //**Nothing to do - the old image has been properly modified
#else
        OFLOG_FATAL(dcmaudscaleLogger, "cannot create new image");
        return 1;
#endif
    }
    else if (newimage->getStatus() != EIS_Normal)
    {
        OFLOG_FATAL(dcmaudscaleLogger, DicomImage::getString(newimage->getStatus()));
        return 1;
    }
    //* write scaled image to dataset (update attributes of Image Pixel Module) 
    else if (!newimage->writeImageToDataset(*datasetSV))
    {
        OFLOG_FATAL(dcmaudscaleLogger, "cannot write new image to dataset");
        return 1;
    }


    if (encapsulated) 
        datasetSV->chooseRepresentation(opt_oxfer, NULL);   //**restore original compression

    if (opt_frame == 1)  //**Grab the full resulting meta data, anonymize and save
    {
        if (frameCount > 1)
        {   char numBuf[20];
            sprintf(numBuf, "%ld", frameCount);
            datasetSV->putAndInsertString(DCM_NumberOfFrames, numBuf);
        }

        // ======================================================================
        // update some header attributes
        // update Derivation Description
        if (!derivationDescription.empty())
        {
            const char *oldDerivation = NULL;
            if (datasetSV->findAndGetString(DCM_DerivationDescription, oldDerivation).good())
            {
                 // append old Derivation Description, if any
                derivationDescription += " [";
                derivationDescription += oldDerivation;
                derivationDescription += "]";
                if (derivationDescription.length() > 1024)
                {
                    // ST is limited to 1024 characters, cut off tail
                    derivationDescription.erase(1020);
                    derivationDescription += "...]";
                }
            }
            datasetSV->putAndInsertString(DCM_DerivationDescription, derivationDescription.c_str());
        }
    
        // update Image Type
        OFString imageType = "DERIVED";
        const char *oldImageType = NULL;
        if (datasetSV->findAndGetString(DCM_ImageType, oldImageType).good())
        {
            if (oldImageType != NULL)
            {
                // append old image type information beginning with second entry
                const char *pos = strchr(oldImageType, '\\');
                if (pos != NULL)
                    imageType += pos;
            }
        }
        datasetSV->putAndInsertString(DCM_ImageType, imageType.c_str());
    
          // update SOP Instance UID
        if (opt_uidCreation)
        {
            // add reference to source image
            DcmItem *ditem = NULL;
            const char *sopClassUID = NULL;
            const char *sopInstanceUID = NULL;
            if (datasetSV->findAndGetString(DCM_SOPClassUID, sopClassUID).good() &&
                datasetSV->findAndGetString(DCM_SOPInstanceUID, sopInstanceUID).good())
            {
                datasetSV->findAndDeleteElement(DCM_SourceImageSequence);
                if (datasetSV->findOrCreateSequenceItem(DCM_SourceImageSequence, ditem).good())
                {
                    ditem->putAndInsertString(DCM_SOPClassUID, sopClassUID);
                    ditem->putAndInsertString(DCM_SOPInstanceUID, sopInstanceUID);
                }
            }
            // create new SOP instance UID
            char new_uid[100];
            datasetSV->putAndInsertString(DCM_SOPInstanceUID, dcmGenerateUniqueIdentifier(new_uid));
            // force meta-header to refresh SOP Instance UID
            if (opt_writeMode == EWM_fileformat)
                opt_writeMode = EWM_updateMeta;
        }
    
        if (opt_anonymize)   
        {  
           std::ofstream pFile;
           OFString fileName(opt_ifname);  fileName.append(".META");   OFStandard::deleteFile(fileName);
           pFile.open (fileName.c_str(), std::ios::out | std::ios::app | std::ios::binary); 
           dfileSV->getMetaInfo()->print(pFile, 0, 0, pixelFileName, &pixelCounter);
           datasetSV->print(pFile, 0, 0, pixelFileName, &pixelCounter);
           pFile.close();
        }
    }
    else //**Subsequent frames
    {   std::cout.setstate(std::ios_base::badbit);  //**Silence stdout, we are only interested in the pixel files
        int pixelCounterSV = pixelCounter = opt_frame;
        datasetSV->print(std::cout, DCMTypes::PF_shortenLongTagValues, 0, pixelFileName, &pixelCounter);
        std::cout.clear();
           //**rename the files
        if (encapsulated) 
        {   for (int i = pixelCounterSV;  i < pixelCounterSV+delta; i++)
            {   char f1[strlen(pixelFileName)+128], f2[strlen(pixelFileName)+128];
                sprintf(f1, "%s%sUplink.f.%d.raw", opt_dir, delimiter, i+1);
                sprintf(f2, "%s%sUplink.f.%d.raw", opt_dir, delimiter, i);
                   //**Remove the lookup table
                if (i == pixelCounterSV)
                   rmFile(f2);
                mvFile(f1, f2);
            }
        }
        else if (!encapsulated)  //**contiguous frames for uncompressed multiframe
        {    char f0[strlen(pixelFileName)+128], f1[strlen(pixelFileName)+128];
             sprintf(f0, "%s%sUplink.f.0.raw", opt_dir, delimiter);
             sprintf(f1, "%s%sUplink.f.%d.raw", opt_dir, delimiter, (int)pixelCounterSV);
             appendFile(f1, f0);
        }
    }
    
    if (newimage)
       delete newimage;
    /* cleanup original image */
    delete di;
    delete dfileSV;
}


    // ======================================================================
    // write back output file

/**** No reason to write the file
    OFLOG_INFO(dcmaudscaleLogger, "create output file " << opt_ofname);

    error = dfile->saveFile(opt_ofname, opt_oxfer, opt_oenctype, opt_oglenc, opt_opadenc,
        OFstatic_cast(Uint32, opt_filepad), OFstatic_cast(Uint32, opt_itempad), opt_writeMode);

    if (error.bad())
    {
        OFLOG_FATAL(dcmaudscaleLogger, error.text() << ": writing file: " <<  opt_ofname);
        return 1;
    }
*****/
        //**Instead let's build up the file
#ifdef TRICE

    bool success = true;
    OFString reason;

    pixelCounter = 0;
    std::ofstream pFile;
    OFString fileName(opt_ifname);  fileName.append(".OUT");   OFStandard::deleteFile(fileName);
    pFile.open (fileName.c_str(), std::ios::out | std::ios::app | std::ios::binary); 
    dfile->print(pFile, 0, 0, opt_ifname, &pixelCounter); //**Print both metaheader + dataset
    pFile.close();

    delete dfile;

        //**Switch the dump files with the scale/clip files and rebuild the image
 
//***** 1)  Remove zero-length files of the form Uplink.f*.raw
//******2)  rename files to be consistent with file names heading into dump2dcm (opt_ifName.%d.raw)

    if (success)
    {   struct stat sbuff;
#ifdef _WIN32
        HANDLE hFind;   WIN32_FIND_DATA FindFileData;
        char zFiles[strlen(opt_dir)+1];  sprintf(zFiles, "%s%sUplink.f*.raw", opt_dir, delimiter);
        if((hFind = FindFirstFile(zFiles, &FindFileData)) != INVALID_HANDLE_VALUE)
        {
            do
            {   char* name = FindFileData.cFileName;
#else
        DIR* dir = opendir(opt_dir);
        if (dir != NULL)
        {   struct dirent* dirent = readdir(dir);
            if (dirent != NULL)  do
            {   char* name = dirent->d_name;
#endif
                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 )
                   continue;
                if (strstr(name, "Uplink.f") == NULL || strstr(name, ".raw") == NULL)  //wrong file
                   continue;
                char buffer[strlen(name)+strlen(opt_dir)+32];      sprintf(buffer, "%s%s%s", opt_dir, delimiter, name);
                if (stat(buffer, &sbuff) == 0 && sbuff.st_size == 0)
                   (void)rmFile(buffer);
                else if (strstr(name, ".rawNOTPIXEL") != NULL)   //**mv -f Uplink.*  opt_ifname.*
                {    char* extension = strstr(buffer, "Uplink.f");
                     extension += 7;
                     char toFile[strlen(buffer)+strlen(opt_ifname)];
                     sprintf(toFile, "%s.%s", opt_ifname, extension);
                     (void)rmFile(toFile);  
                     int rc = mvFile(buffer, toFile);
                     if (rc != 0)
                     {  cpFile(buffer, toFile);
                        if (!nonZeroFile(toFile))
                        {   success = false;  reason.append("Cannot move ").append(buffer).append(" to ").append(toFile).append(" \n");   break; }
                        rmFile(buffer);
                     }
                }
                else   //**mv -f Uplink.f.$i.raw.*.raw opt_ifname.$i.raw
                {
                     int num = atoi(&name[9]);
                     char toFile[strlen(opt_ifname)+strlen(opt_dir)+32];  
                     sprintf(toFile, "%s.%d.raw", opt_ifname, num);
                     (void)rmFile(toFile);  
                     int rc = mvFile(buffer, toFile);
                     if (rc != 0)
                     {  cpFile(buffer, toFile);
                        if (!nonZeroFile(toFile))
                        {   success = false;  reason.append("Cannot move ").append(buffer).append(" to ").append(toFile).append(" \n");   break; }
                        rmFile(buffer);
                     }
               }
#ifdef _WIN32
            } while(FindNextFile(hFind, &FindFileData));
        FindClose(hFind);
        }
#else
           } while ((dirent = readdir(dir)) != NULL);
        closedir(dir);
        }
#endif
        if (success)
        {      //**Combine metadata from first clip and pixel data from per-frame processing.
            Memory mem;
            OFString fileName(opt_ifname);   fileName.append(".META");  
            OFString fileName1(opt_ifname);  fileName1.append(".OUT");  
            OFString fileNameF(opt_ifname);  fileNameF.append(".FINAL");  
            char* meta = readTxt((char*)fileName.c_str());   mem.add(meta);
            char* mEnd = strstr((char*)meta, "(7fe0,0010)");
            if (mEnd != NULL) *mEnd = '\0';  //**Cut off pixel lines
            char* pixels = readTxt((char*)fileName1.c_str());   
            if (!isnull(pixels)) mem.add(pixels);
            char* pStart = strstr((char*)pixels, "(7fe0,0010)");

            if (!isnull(pStart))
            {      //**Build composite
                 OFString composite(meta);
                   //**Add pixel data
                 composite.append(pStart);
                    //**Edit/Remove lines with unknown tags (and full sequences that are unknown), DT, DA, TI and UI
                 if (opt_anonymize)   
                 {   int n;  char** lines = Util::split((char*)composite.c_str(), '\n', n, &mem);
                     OFString composite1;
                     int seq = 0;
                     for (int i = 0; i < n; i++)
                     {   char* p;   
                         if ((p=strstr(lines[i], " TM [")) != NULL)    //Anonymize times across the board
                         {  char* e = strrchr(lines[i], ']');
                            if (!isnull(e) && e < p+15)  e = p+15;
                            strcpy(p, " TM [000000.00]");
                            if (!isnull(e))
                              strcat(lines[i], e+1);
                         }
                         else if (opt_anon_strict && (p=strstr(lines[i], " DA [")) != NULL) //Anonymize Dates - year only is allowed
                            strncpy(p+9, "0101", 4);
                         else if (opt_anon_strict && (p=strstr(lines[i], " DT [")) != NULL) //Anonymize Date/time 
                            strncpy(p+9, "010100000000", 10);
                         else if (opt_anon_strict && !opt_vp6 && strstr(lines[i], ROOT_UID) == NULL && strstr(lines[i], "ClassUID") == 0 && (p=strstr(lines[i], " UI [")) != NULL)    
                         {   char uid[65]; strncpy(uid, p+5, 64); uid[64] = '\0';
                             char* br = strstr(uid, "]");
                             if (!isnull(br)) *br = '\0';
                             char* uid1 = genInstanceFromUID(uid, sop); 
                             *(p+4) = '\0';  GrowingString l(lines[i]); *(p+4) = '[';
                             l.add("[").add(uid1).add("] # " );
                             char* ll = l.get();
                             if (strlen(ll) < strlen(lines[i]))  strncpy(lines[i], ll, strlen(ll));
                             else  lines[i] = ll; 
                         }
                         if (strstr(lines[i], "Unknown Tag & Data") != NULL && strstr(lines[i], ") SQ ") != NULL)
                             seq++;
                         else if (seq > 0 && strstr(lines[i], ") SQ ") != NULL)
                            seq++; 
                         if ((strstr(lines[i], "Unknown Tag & Data") == NULL && seq == 0 &&
                              strstr(lines[i], "PrivateCreator") == NULL && strstr(lines[i], "PrivateGroupLength") == NULL) ||
                              strstr(lines[i], "(6301,00") != NULL)
                            composite1.append(lines[i]).append("\n");
                         else if (seq > 0 && strstr(lines[i], "(fffe,e0dd)") != NULL)
                            seq--; 
                     }
                     writeTxt((char*)fileNameF.c_str(), (char*)"", (char*)composite1.c_str());  //**Write out combined file
                 }
                 else
                     writeTxt((char*)fileNameF.c_str(), (char*)"", (char*)composite.c_str());  //**Write out combined file
                 if (encapsulated)  //**Dump an empty lookup table (offsets in orig will be wrong)
                 {   char file0[strlen(opt_ifname)+256];  sprintf(file0, "%s.%d.raw", opt_ifname, 0);
                     writeData(file0, "", (void*)"", 0);
                 }
            }
            else
            {    success = false;
                 OFLOG_WARN(dcmaudscaleLogger, "No Pixel Data to scale/clip");
                 reason.append("Cannot create composite DICOM file - there is no pixel data\n");
            }
            rmFile((char*)fileName.c_str());
            rmFile((char*)fileName1.c_str());
        }

           //**Recreate the file:   dump2dcm --dict-path <dictionary> opt_ifname.OUT  opt_ifname
        if (success)
        {    OFString pgm;
             OFStandard::combineDirAndFilename(pgm, opt_processDir, "dump2dcm -g -q +E --update-meta-info", true);
          
             (void)rmFile(opt_ofname);
             OFString command(pgm);
             if (strcmp(dicomPath, "") != 0)
                command.append(" --dict-path ").append(dicomPath);
             command.append(" ").append(opt_ifname).append(".FINAL ").append(opt_ofname);
             if (system(command.c_str()) != 0)
             {   success = false;   reason.append("dump2dcm failed ( ").append(command).append(" )\n");  }
        }
     }

     char cmd1[strlen(opt_dir)+100];
#ifdef __windows__
     sprintf(cmd1, "del /q /F %s\\*.raw* 2> NUL", opt_dir);
     char* c = Util::replace(cmd1, "\\\\", "\\");   strcpy(cmd1, c);
#else
     sprintf(cmd1, "rm -f %s/*.raw*", opt_dir);
#endif
     system(cmd1);

     OFString file(opt_ifname);  file.append(".FINAL");
     OFStandard::deleteFile(file); 
#endif

    OFLOG_INFO(dcmaudscaleLogger, "conversion successful");

#ifdef TRICE
    // deregister global decompression codecs
    DJDecoderRegistration::cleanup();
    DJEncoderRegistration::cleanup();
    // deregister JPEG-LS decompression codecs
    DJLSDecoderRegistration::cleanup();
    DJLSEncoderRegistration::cleanup();
    DcmRLEEncoderRegistration::cleanup();    
    DcmRLEDecoderRegistration::cleanup();                

#endif

    if (!success)
    {   fprintf(stderr, "dcmaudscale failed:  %s\n", reason.c_str());
        (void)rmFile(opt_ofname);
        return 1;
    }

    return 0;
}
