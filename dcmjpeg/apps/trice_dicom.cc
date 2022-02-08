#include "dcmtk/dcmdata/dcpxitem.h"
#include "dcmtk/dcmdata/dcpixseq.h"
#include <trice_int.h>

/**  Populate the Dataset from Json

     Private tag support.  Dcmtk supports this only halfway
**/
   //**create lower short based on the location of the private tag reservation
static u_int16_t createLowerShort(u_int8_t high, u_int8_t low)  {   return (high*16*16) + low;  }

   //**Relocatable private tags
static u_int8_t findAvailPrivateSpace(DcmDataset* dset, u_int16_t group ) 
{   u_int16_t start = 0x10;
        //**Look for the first unused space (by reservation) for the group in question
    while (start <= 0xff)
    {   DcmTag tag(group, start, EVR_LO);
        DcmTagKey searchKey(group, start);
        DcmStack stack;
        if (dset->search(searchKey, stack, ESM_fromHere, OFTrue) != EC_Normal)
           return start;
        else
           start++;
    }
    return 0xff;
}

  /**Find the location of a specific private tag
     The findAndGet() routines don't work for private reservation numbers
   */
static u_int16_t findSpecificPrivateSpace(DcmDataset* dset, u_int16_t group, char* name, u_int8_t defaultVal )  
{   u_int16_t start = 0x10;
    const char *c = NULL;
    while (start <= 0xff)
    {   c = NULL; 
        DcmTag tag(group, start, EVR_LO);
        DcmTagKey searchKey(group, start);
        DcmStack stack;
          //**Look for the reservation .  If it's there compare the identifying strings to to see if this is it
        if (dset->search(searchKey, stack, ESM_fromHere, OFTrue) == EC_Normal)
        {   char* val;
            DcmObject* obj = stack.pop();
            char* c;   ((DcmElement*)obj)->getString(c);
            if (strcmp(c, name) == 0)  //**Found it
                 return start;
            else if (defaultVal == start)  defaultVal++;  //**default is taken w something else, incr it
        }
        start++;
    }
    return defaultVal;
}


//--------------

//TRICE
static int populateFromJson(char* json, DcmDataset* meta, bool worklist=false, bool queryRetrieve=false)
{      // parse the JSON
    bool privateTag = false;
    int num;  unsigned int num1;
    JsnParser p(json, strlen(json));
    if (p.jsonOk())
    {  OFString attr = OFString(p.getString(0, (char*)"sop_instance_uid"));
       if (!isnull(attr.c_str()))
          meta->putAndInsertString(DCM_SOPInstanceUID, attr.c_str());
       attr = OFString(p.getString(0, (char*)"study_date"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_StudyDate, attr.c_str());
       attr = OFString(p.getString(0, (char*)"class_uid"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_SOPClassUID, attr.c_str());
       attr = OFString(p.getString(0, (char*)"study_time"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_StudyTime  , attr.c_str());
       attr = OFString(p.getString(0, (char*)"study_description"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_StudyDescription, attr.c_str());
       attr = OFString(p.getString(0, (char*)"manufacturer"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_Manufacturer, attr.c_str());
       attr = OFString(p.getString(0, (char*)"manufacturer_model_name"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_ManufacturerModelName, attr.c_str());
       attr = OFString(p.getString(0, (char*)"patient_name"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_PatientName  , attr.c_str());
       attr = OFString(p.getString(0, (char*)"patient_id"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_PatientID, attr.c_str());
       attr = OFString(p.getString(0, (char*)"patient_sex"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_PatientSex, attr.c_str());
       attr = OFString(p.getString(0, (char*)"patients_birth_date"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_PatientBirthDate, attr.c_str());
       attr = OFString(p.getString(0, (char*)"patient_address"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_PatientAddress, attr.c_str());
       attr = OFString(p.getString(0, (char*)"performing_physician_name"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_PerformingPhysicianName , attr.c_str());
       attr = OFString(p.getString(0, (char*)"referring_physician"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_ReferringPhysicianName, attr.c_str());
       attr = OFString(p.getString(0, (char*)"referring_physician_address"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_ReferringPhysicianAddress, attr.c_str());
       attr = OFString(p.getString(0, (char*)"study_instance_uid"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_StudyInstanceUID, attr.c_str());
       attr = OFString(p.getString(0, (char*)"series_instance_uid"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_SeriesInstanceUID, attr.c_str());
       num1 = p.getInt(0, (char*)"series_number");
       if (num1 != 0)
       {  char buff[16];  sprintf(buff, "%u", num1);
          meta-> putAndInsertString(DCM_SeriesNumber, buff);
       }
       num1 = p.getInt(0, (char*)"instance_number");
       if (num1 != 0)
       {  char buff[16];  sprintf(buff, "%u", num1);
          meta-> putAndInsertString(DCM_InstanceNumber, buff);
       }
       attr = OFString(p.getString(0, (char*)"cine_rate"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString( DCM_CineRate , attr.c_str());
       attr = OFString(p.getString(0, (char*)"frame_time"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString( DCM_FrameTime , attr.c_str());
       attr = OFString(p.getString(0, (char*)"pixel_spacing"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString( DCM_PixelSpacing , attr.c_str());
       attr = OFString(p.getString(0, (char*)"ge_referring_physician_email"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, 0x1011, EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       attr = OFString(p.getString(0, (char*)"ge_performing_physician_email"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, 0x1012, EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       attr = OFString(p.getString(0, (char*)"ge_performing_physician_phone"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, 0x1022, EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       attr = OFString(p.getString(0, (char*)"ge_patient_email"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, 0x1010, EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       attr = OFString(p.getString(0, (char*)"patient_phone"));
       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_PatientTelephoneNumbers, attr.c_str());
       attr = OFString(p.getString(0, (char*)"referring_physician_phone"));

       if (!isnull(attr.c_str()))
          meta-> putAndInsertString(DCM_ReferringPhysicianTelephoneNumbers, attr.c_str());  
           //**Private tags
       u_int16_t upper = findSpecificPrivateSpace(meta, 0x6301, "KRETZ_PRIVATE", 0x10);
       attr = OFString(p.getString(0, (char*)"ge_sonographer_email"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, createLowerShort(upper, 0x13), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       attr = OFString(p.getString(0, (char*)"ge_sonographer_phone"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, createLowerShort(upper, 0x23), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       attr = OFString(p.getString(0, (char*)"ge_patient_phone"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, createLowerShort(upper, 0x24), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       attr = OFString(p.getString(0, (char*)"ge_referring_physician_phone"));
       if (!isnull(attr.c_str()))
       {   DcmTag tag(0x6301, createLowerShort(upper, 0x25), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag,  attr.c_str());  privateTag = true;  
       }
       if (privateTag)
           meta-> putAndInsertString( DcmTag(0x6301, createLowerShort(0x00,upper), EVR_LO) , "KRETZ_PRIVATE");

       if (worklist || queryRetrieve)
       {   bool sequence = false;
           num = p.getInt(0, (char*)"number_of_studies");
           if (num > 0)
           {  char buff[16];  sprintf(buff, "%d", num);
              meta->putAndInsertString(DCM_NumberOfPatientRelatedStudies, buff);
           }
           num = p.getInt(0, (char*)"num_study_instances");
           if (num > 0)
           {  char buff[16];  sprintf(buff, "%d", num);
              meta->putAndInsertString(DCM_NumberOfStudyRelatedInstances, buff);
           }
           num = p.getInt(0, (char*)"num_series_instances");
           if (num > 0)
           {  char buff[16];  sprintf(buff, "%d", num);
              meta->putAndInsertString(DCM_NumberOfSeriesRelatedInstances, buff);
           }
           num = p.getInt(0, (char*)"num_series");
           if (num > 0)
           {  char buff[16];  sprintf(buff, "%d", num);
              meta->putAndInsertString(DCM_NumberOfStudyRelatedSeries, buff);
           }
           unsigned int num1 = p.getInt(0, (char*)"study_id");
           if (num1 != 0)
           {  char buff[16];  sprintf(buff, "%u", num1);
              meta->putAndInsertString(DCM_StudyID, buff);
           }
           attr = OFString(p.getString(0, (char*)"modalities_in_study"));
           if (!isnull(attr.c_str()))
              meta->putAndInsertString(DCM_ModalitiesInStudy, attr.c_str());
           attr = OFString(p.getString(0, (char*)"series_description"));
           if (!isnull(attr.c_str()))
              meta->putAndInsertString(DCM_SeriesDescription, attr.c_str());
           attr = OFString(p.getString(0, (char*)"other_patient_names"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_OtherPatientNames, attr.c_str());
           attr = OFString(p.getString(0, (char*)"other_patient_ids"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_OtherPatientIDs, attr.c_str());
           attr = OFString(p.getString(0, (char*)"admitting_diagnosis_description"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_AdmittingDiagnosesDescription , attr.c_str());
           attr = OFString(p.getString(0, (char*)"accession_number"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_AccessionNumber , attr.c_str());
           attr = OFString(p.getString(0, (char*)"filler_order_number"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_FillerOrderNumberImagingServiceRequest, attr.c_str());
           attr = OFString(p.getString(0, (char*)"placer_order_number"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_PlacerOrderNumberImagingServiceRequest, attr.c_str());
           attr = OFString(p.getString(0, (char*)"requesting_physician"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_RequestingPhysician , attr.c_str());
           attr = OFString(p.getString(0, (char*)"requested_procedure_comments"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_RequestedProcedureComments , attr.c_str());
           attr = OFString(p.getString(0, (char*)"imaging_service_request_comments"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_ImagingServiceRequestComments , attr.c_str());
           attr = OFString(p.getString(0, (char*)"modality"));
           if (!isnull(attr.c_str()))
           {    if (worklist) sequence = true;
                else if (queryRetrieve)
                   meta -> putAndInsertString(DCM_Modality, attr.c_str());
           }
           attr = OFString(p.getString(0, (char*)"scheduled_procedure_comments"));
           if (!isnull(attr.c_str()))
               if (worklist) sequence = true;
           attr = OFString(p.getString(0, (char*)"scheduled_performing_physician_name"));
           if (!isnull(attr.c_str()))
               if (worklist) sequence = true;
           attr = OFString(p.getString(0, (char*)"scheduled_procedure_start_date"));
           if (!isnull(attr.c_str()))
               if (worklist) sequence = true;
           attr = OFString(p.getString(0, (char*)"scheduled_procedure_start_time"));
           if (!isnull(attr.c_str()))
               if (worklist) sequence = true;
           attr = OFString(p.getString(0, (char*)"scheduled_station_ae_title"));
           if (!isnull(attr.c_str()))
               if (worklist) sequence = true;
           if (sequence)
           {   DcmItem *item = NULL;
               if (meta->findOrCreateSequenceItem(DCM_ScheduledProcedureStepSequence, item, -2 /* append */).good())
               {    attr = OFString(p.getString(0, (char*)"modality"));
                    if (!isnull(attr.c_str()))
                        item -> putAndInsertString(DCM_Modality, attr.c_str());
                    attr = OFString(p.getString(0, (char*)"scheduled_performing_physician_name"));
                    if (!isnull(attr.c_str()))
                        item -> putAndInsertString(DCM_ScheduledPerformingPhysicianName, attr.c_str());
                    attr = OFString(p.getString(0, (char*)"scheduled_procedure_start_date"));
                    if (!isnull(attr.c_str()))
                        item -> putAndInsertString(DCM_ScheduledProcedureStepStartDate, attr.c_str());
                    attr = OFString(p.getString(0, (char*)"scheduled_procedure_start_time"));
                    if (!isnull(attr.c_str()))
                        item -> putAndInsertString(DCM_ScheduledProcedureStepStartTime, attr.c_str());
                    attr = OFString(p.getString(0, (char*)"scheduled_station_ae_title"));
                    if (!isnull(attr.c_str()))
                        item -> putAndInsertString(DCM_ScheduledStationAETitle, attr.c_str());
                     attr = OFString(p.getString(0, (char*)"scheduled_procedure_comments"));
                     if (!isnull(attr.c_str()))
                        item -> putAndInsertString(DCM_CommentsOnTheScheduledProcedureStep, attr.c_str());
               }
           }
       }
       else
       {   attr = OFString(p.getString(0, (char*)"study_id"));
           if (!isnull(attr.c_str()))
              meta-> putAndInsertString( DCM_StudyID , attr.c_str());
       }
       return 0;
    }
    return 1;
}

   /** limit dataset to just these attributes:  (for mini-anonymization)
                     "OtherPatientNames", "6301,1010", "6301,1011 ", "6301,1012 ", "6301,1013",
                     "6301,1022", "6301,1023", "SOPClassUID", "Rows", "Columns", "CineRate", "FrameTime", "Modality", "StudyDate", "StudyTime",
                     "StudyDescription", "Manufacturer", "ManufacturerModelName", "PatientName", "PatientID", "PatientBirthDate",
                     "PatientAddress", "PerformingPhysicianName", "ReferringPhysicianName", "ReferringPhysicianAddress",
                     "StudyInstanceUID", "SeriesInstanceUID", "SeriesNumber", "InstanceNumber", "NumberOfFrames", "PixelSpacing", accession_number
                     "ReferringPhysicianTelephoneNumbers", "PatientTelephoneNumbers", SeriesDescription,SeriesInstanceUID, AdmittingDiagnosesDescription
**/
static DcmDataset* limitDataset(DcmDataset* dset, bool anonymize=false)
{   DcmDataset* meta = new DcmDataset();  //**Just put in the attributes we want

    const char *c = NULL;
    c = NULL; if (dset->findAndGetString(DCM_NumberOfFrames, c).good() && c) 
          meta-> putAndInsertString(DCM_NumberOfFrames, c);
    c = NULL; if (dset->findAndGetString(DCM_Modality, c).good() && c) 
          meta-> putAndInsertString(DCM_Modality, c);
    u_int16_t lRows = 0; 
    u_int16_t lCols = 0;
    dset->findAndGetUint16(DCM_Rows, lRows); 
    dset->findAndGetUint16(DCM_Columns, lCols); 
    if (lRows > 0) meta->putAndInsertUint16(DCM_Columns, lCols); 
    if (lCols > 0) meta->putAndInsertUint16(DCM_Rows, lRows); 
    c = NULL; if (dset->findAndGetString(DCM_SpecificCharacterSet, c).good() && c) 
          meta-> putAndInsertString(DCM_SpecificCharacterSet, c);
    c = NULL; if (dset->findAndGetString(DCM_SOPClassUID, c).good() && c) 
          meta-> putAndInsertString(DCM_SOPClassUID, c);
    c = NULL; if (dset->findAndGetString(DCM_SOPInstanceUID, c).good() && c) 
          meta-> putAndInsertString(DCM_SOPInstanceUID, c);
    c = NULL; if (dset->findAndGetString(DCM_StudyDate, c).good() && c) 
          meta-> putAndInsertString(DCM_StudyDate, c);
    c = NULL; if (dset->findAndGetString(DCM_StudyTime, c).good() && c) 
          meta-> putAndInsertString(DCM_StudyTime  , c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_StudyDescription, c).good() && c) 
          meta-> putAndInsertString(DCM_StudyDescription, c);
    c = NULL; if (dset->findAndGetString(DCM_Manufacturer, c).good() && c) 
          meta-> putAndInsertString(DCM_Manufacturer, c);
    c = NULL; if (dset->findAndGetString(DCM_ManufacturerModelName, c).good() && c) 
          meta-> putAndInsertString(DCM_ManufacturerModelName, c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_PatientName, c).good() && c) 
          meta-> putAndInsertString(DCM_PatientName  , c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_PatientID, c).good() && c) 
          meta-> putAndInsertString(DCM_PatientID, c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_PatientSex, c).good() && c) 
          meta-> putAndInsertString(DCM_PatientSex, c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_PatientBirthDate, c).good() && c) 
          meta-> putAndInsertString(DCM_PatientBirthDate, c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_PatientAddress, c).good() && c) 
          meta-> putAndInsertString(DCM_PatientAddress, c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_PerformingPhysicianName, c).good() && c) 
          meta-> putAndInsertString(DCM_PerformingPhysicianName , c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_ReferringPhysicianName, c).good() && c) 
          meta-> putAndInsertString(DCM_ReferringPhysicianName, c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_ReferringPhysicianAddress, c).good() && c) 
          meta-> putAndInsertString(DCM_ReferringPhysicianAddress, c);
    c = NULL; if (dset->findAndGetString(DCM_StudyInstanceUID, c).good() && c) 
          meta-> putAndInsertString(DCM_StudyInstanceUID, c);
    c = NULL; if (dset->findAndGetString(DCM_SeriesInstanceUID, c).good() && c) 
          meta-> putAndInsertString(DCM_SeriesInstanceUID, c);
    c = NULL; if (dset->findAndGetString(DCM_InstanceNumber, c).good() && c) 
          meta-> putAndInsertString(DCM_InstanceNumber, c);
    c = NULL; if (dset->findAndGetString(DCM_SeriesNumber, c).good() && c) 
          meta-> putAndInsertString(DCM_SeriesNumber, c);
    c = NULL; if (dset->findAndGetString(DCM_SeriesInstanceUID, c).good() && c) 
          meta-> putAndInsertString(DCM_SeriesInstanceUID, c);
    c = NULL; if (dset->findAndGetString(DCM_CineRate, c).good() && c) 
          meta-> putAndInsertString( DCM_CineRate , c);
    c = NULL; if (dset->findAndGetString(DCM_FrameTime, c).good() && c) 
          meta-> putAndInsertString( DCM_FrameTime , c);
    c = NULL; if (dset->findAndGetString(DCM_PixelSpacing, c).good() && c) 
          meta-> putAndInsertString( DCM_PixelSpacing , c);
    if (dset->tagExistsWithValue(DCM_PixelData))
    {    DcmPixelSequence* sequence = new DcmPixelSequence(DcmTag(DCM_PixelData, EVR_OB));
         DcmPixelItem* newby = new DcmPixelItem(DcmTag(DCM_Item, EVR_OB));
         sequence->insert(newby);
         meta->insert(sequence);
    }
    u_int16_t upper = findSpecificPrivateSpace(dset, 0x0035, "TRICEFY_PRIVATE", 0x10);
    DcmTagKey tag(0x0035,createLowerShort(upper, 0x11));
    if (dset->tagExistsWithValue(tag))
    {     DcmTag tag(0x0035, createLowerShort(upper, 0x11), EVR_OB);    tag.setPrivateCreator("TRICEFY_PRIVATE");
          meta->putAndInsertUint8Array(tag, (const u_int8_t*)"Yes", 3);
          meta-> putAndInsertString( DcmTag(0x0035, createLowerShort(0x00, upper), EVR_LO) , "TRICEFY_PRIVATE");
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_PatientTelephoneNumbers, c).good() && c) 
          meta-> putAndInsertString(DCM_PatientTelephoneNumbers, c);
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_ReferringPhysicianTelephoneNumbers, c).good() && c) 
          meta-> putAndInsertString(DCM_ReferringPhysicianTelephoneNumbers, c);  
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_AdmittingDiagnosesDescription, c).good() && c) 
          meta-> putAndInsertString(DCM_AdmittingDiagnosesDescription, c);  
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_SeriesDescription, c).good() && c) 
          meta-> putAndInsertString(DCM_SeriesDescription, c);  
    c = NULL; if (!anonymize && dset->findAndGetString(DCM_AccessionNumber, c).good() && c) 
          meta-> putAndInsertString(DCM_AccessionNumber, c);  
           //**Private tags
    bool privateTag = false;
    findSpecificPrivateSpace(dset, 0x6301, "KRETZ_PRIVATE", 0x10);
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x11), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x11), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x12), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x12), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x22), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x22), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x10), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x10), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x13), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x13), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x23), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x23), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x24), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x24), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    c = NULL; if (!anonymize && dset->findAndGetString(DcmTag(0x6301, createLowerShort(upper, 0x25), EVR_LO), c).good() && c) 
    {      DcmTag tag(0x6301, createLowerShort(upper, 0x25), EVR_LO);    tag.setPrivateCreator("KRETZ_PRIVATE");
           meta-> putAndInsertString( tag, c);  privateTag = true;  
    }
    if (privateTag)
          meta-> putAndInsertString( DcmTag(0x6301, createLowerShort(0x00, upper), EVR_LO) , "KRETZ_PRIVATE");
    return meta;
}
