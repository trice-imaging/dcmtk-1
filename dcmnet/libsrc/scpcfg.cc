/*
 *
 *  Copyright (C) 2012-2013, OFFIS e.V.
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
 *  Module:  dcmnet
 *
 *  Author:  Michael Onken, Jan Schlamelcher
 *
 *  Purpose: Class representing configuration of an SCP.
 *
 */

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */

#include "dcmtk/dcmnet/scpcfg.h"
#include "dcmtk/dcmnet/diutil.h"

// ----------------------------------------------------------------------------

DcmSCPConfig::DcmSCPConfig() :
  m_assocConfig(),
  m_assocCfgProfileName("DEFAULT"),
  m_port(104),
  m_aetitle("DCMTK_SCP"),
  m_refuseAssociation(OFFalse),
  m_maxReceivePDULength(ASC_DEFAULTMAXPDU),
  m_connectionBlockingMode(DUL_BLOCK),
  m_dimseBlockingMode(DIMSE_BLOCKING),
  m_dimseTimeout(0),
  m_acseTimeout(30),
  m_verbosePCMode(OFFalse),
  m_connectionTimeout(1000),
  m_respondWithCalledAETitle(OFTrue),
  m_progressNotificationMode(OFTrue)
{
}

// ----------------------------------------------------------------------------

DcmSCPConfig::~DcmSCPConfig()
{
  // nothing to do
}

// ----------------------------------------------------------------------------

DcmSCPConfig::DcmSCPConfig(const DcmSCPConfig &old) :
  m_assocConfig(old.m_assocConfig),
  m_assocCfgProfileName(old.m_assocCfgProfileName),
  m_port(old.m_port),
  m_aetitle(old.m_aetitle),
  m_refuseAssociation(old.m_refuseAssociation),
  m_maxReceivePDULength(old.m_maxReceivePDULength),
  m_connectionBlockingMode(old.m_connectionBlockingMode),
  m_dimseBlockingMode(old.m_dimseBlockingMode),
  m_dimseTimeout(old.m_dimseTimeout),
  m_acseTimeout(old.m_acseTimeout),
  m_verbosePCMode(old.m_verbosePCMode),
  m_connectionTimeout(old.m_connectionTimeout),
  m_respondWithCalledAETitle(old.m_respondWithCalledAETitle),
  m_progressNotificationMode(old.m_progressNotificationMode)
{
  // nothing more to do
}

// ----------------------------------------------------------------------------

DcmSCPConfig& DcmSCPConfig::operator=(const DcmSCPConfig &obj)
{
  /* No self assignment */
  if (this != &obj)
  {
    m_assocConfig = obj.m_assocConfig; // performs deep copy
    m_assocCfgProfileName = obj.m_assocCfgProfileName;
    m_port = obj.m_port;
    m_aetitle = obj.m_aetitle;
    m_refuseAssociation = obj.m_refuseAssociation;
    m_maxReceivePDULength = obj.m_maxReceivePDULength;
    m_connectionBlockingMode = obj.m_connectionBlockingMode;
    m_dimseBlockingMode = obj.m_dimseBlockingMode;
    m_dimseTimeout = obj.m_dimseTimeout;
    m_acseTimeout = obj.m_acseTimeout;
    m_verbosePCMode = obj.m_verbosePCMode;
    m_connectionTimeout = obj.m_connectionTimeout;
    m_respondWithCalledAETitle = obj.m_respondWithCalledAETitle;
    m_progressNotificationMode = obj.m_progressNotificationMode;
  }
  return *this;
}

// ----------------------------------------------------------------------------

OFCondition DcmSCPConfig::evaluateIncomingAssociation(T_ASC_Association &assoc) const
{
  return m_assocConfig.evaluateAssociationParameters(m_assocCfgProfileName.c_str(), assoc);
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::forceAssociationRefuse(const OFBool doRefuse)
{
  m_refuseAssociation = doRefuse;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setMaxReceivePDULength(const Uint32 maxRecPDU)
{
  m_maxReceivePDULength = maxRecPDU;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setPort(const Uint16 port)
{
  m_port = port;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setAETitle(const OFString &aetitle)
{
  m_aetitle = aetitle;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setRespondWithCalledAETitle(const OFBool useCalled)
{
  m_respondWithCalledAETitle = useCalled;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setConnectionBlockingMode(const DUL_BLOCKOPTIONS blockingMode)
{
  m_connectionBlockingMode = blockingMode;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setDIMSEBlockingMode(const T_DIMSE_BlockingMode blockingMode)
{
  m_dimseBlockingMode = blockingMode;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setDIMSETimeout(const Uint32 dimseTimeout)
{
  m_dimseTimeout = dimseTimeout;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setACSETimeout(const Uint32 acseTimeout)
{
  m_acseTimeout = acseTimeout;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setConnectionTimeout(const Uint32 timeout)
{
  m_connectionTimeout = timeout;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setVerbosePCMode(const OFBool mode)
{
  m_verbosePCMode = mode;
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setHostLookupEnabled(const OFBool mode)
{
  dcmDisableGethostbyaddr.set(!mode);
}

// ----------------------------------------------------------------------------

void DcmSCPConfig::setProgressNotificationMode(const OFBool mode)
{
  m_progressNotificationMode = mode;
}

// ----------------------------------------------------------------------------

/* Get methods for SCP settings and current association information */


OFBool DcmSCPConfig::getRefuseAssociation() const
{
  return m_refuseAssociation;
}

// ----------------------------------------------------------------------------

Uint32 DcmSCPConfig::getMaxReceivePDULength() const
{
  return m_maxReceivePDULength;
}

// ----------------------------------------------------------------------------

Uint16 DcmSCPConfig::getPort() const
{
  return m_port;
}

// ----------------------------------------------------------------------------

const OFString &DcmSCPConfig::getAETitle() const
{
  return m_aetitle;
}

// ----------------------------------------------------------------------------

OFBool DcmSCPConfig::getRespondWithCalledAETitle() const
{
  return m_respondWithCalledAETitle;
}

// ----------------------------------------------------------------------------

DUL_BLOCKOPTIONS DcmSCPConfig::getConnectionBlockingMode() const
{
  return m_connectionBlockingMode;
}

// ----------------------------------------------------------------------------

T_DIMSE_BlockingMode DcmSCPConfig::getDIMSEBlockingMode() const
{
  return m_dimseBlockingMode;
}

// ----------------------------------------------------------------------------

Uint32 DcmSCPConfig::getDIMSETimeout() const
{
  return m_dimseTimeout;
}

// ----------------------------------------------------------------------------

Uint32 DcmSCPConfig::getConnnectionTimeout() const
{
  return m_connectionTimeout;
}

// ----------------------------------------------------------------------------

Uint32 DcmSCPConfig::getACSETimeout() const
{
  return m_acseTimeout;
}

// ----------------------------------------------------------------------------

OFBool DcmSCPConfig::getVerbosePCMode() const
{
  return m_verbosePCMode;
}

// ----------------------------------------------------------------------------

OFBool DcmSCPConfig::getHostLookupEnabled() const
{
  return dcmDisableGethostbyaddr.get();
}

// ----------------------------------------------------------------------------

OFBool DcmSCPConfig::getProgressNotificationMode() const
{
  return m_progressNotificationMode;
}

// ----------------------------------------------------------------------------

// Reads association configuration from config file
OFCondition DcmSCPConfig::loadAssociationCfgFile(const OFString &assocFile)
{
  OFCondition result = EC_InvalidFilename;
  // delete any previous association configuration
  m_assocConfig.clear();
  if (!assocFile.empty())
  {
    OFString profileName;
    DCMNET_DEBUG("Loading association configuration file: " << assocFile);
    result = DcmAssociationConfigurationFile::initialize(m_assocConfig, assocFile.c_str());
    if (result.bad())
    {
      DCMNET_ERROR("Unable to parse association configuration file: " << assocFile << ": " << result.text());
      m_assocConfig.clear();
    }
  }
  return result;
}

// ----------------------------------------------------------------------------

OFCondition DcmSCPConfig::setAndCheckAssociationProfile(const OFString &profileName)
{
  if (profileName.empty())
    return EC_IllegalParameter;

  DCMNET_TRACE("Setting and checking SCP association profile");
  OFString mangledName;
  OFCondition result;

  /* perform name mangling for config file key */
  const unsigned char *c = OFreinterpret_cast(const unsigned char *, profileName.c_str());
  while (*c)
  {
    if (! isspace(*c)) mangledName += OFstatic_cast(char, toupper(*c));
    ++c;
  }
  /* check profile */
  if (result.good() && !m_assocConfig.isKnownProfile(mangledName.c_str()))
  {
    DCMNET_ERROR("No association profile named \"" << profileName << "\" in association configuration");
    result = EC_IllegalParameter; // TODO: need to find better error code
  }
  if (result.good() && !m_assocConfig.isValidSCPProfile(mangledName.c_str()))
  {
    DCMNET_ERROR("The association profile named \"" << profileName << "\" is not a valid SCP association profile");
    result = EC_IllegalParameter; // TODO: need to find better error code
  }
  if (result.good())
    m_assocCfgProfileName = mangledName;

  return result;
}

// ----------------------------------------------------------------------------

OFCondition DcmSCPConfig::addPresentationContext(const OFString &abstractSyntax,
                                                 const OFList<OFString> xferSyntaxes,
                                                 const T_ASC_SC_ROLE role,
                                                 const OFString &profile)
{
  if (profile.empty())
    return EC_IllegalParameter;

  const char *DCMSCP_TS_KEY = "DCMSCP_GEN_TS_KEY";
  const char *DCMSCP_PC_KEY = "DCMSCP_GEN_PC_KEY";
  const char *DCMSCP_RO_KEY = "DCMSCP_GEN_RO_KEY";
  OFListConstIterator(OFString) it = xferSyntaxes.begin();
  OFListConstIterator(OFString) endOfList = xferSyntaxes.end();
  OFCondition result;
  // the association configuration needs key names for transfer syntaxes,
  // presentation contexts and roles. Use predefined key names.
  while ((it != endOfList) && result.good())
  {
    result = m_assocConfig.addTransferSyntax(DCMSCP_TS_KEY, (*it).c_str());
    it++;
  }
  if (result.good())
  {
    result = m_assocConfig.addPresentationContext(DCMSCP_PC_KEY, abstractSyntax.c_str(), DCMSCP_TS_KEY);
  }
  if (result.good())
  {
    result = m_assocConfig.addRole(DCMSCP_RO_KEY, abstractSyntax.c_str(), role);
  }
  /* perform name mangling for config file key */
  const unsigned char *c = OFreinterpret_cast(const unsigned char *, profile.c_str());
  OFString mangledName;
  while (*c)
  {
    if (! isspace(*c)) mangledName += OFstatic_cast(char, toupper(*c));
    ++c;
  }
  if (result.good() && !m_assocConfig.isKnownProfile(mangledName.c_str()))
  {
    result = m_assocConfig.addProfile(mangledName.c_str(), DCMSCP_PC_KEY, DCMSCP_RO_KEY);
  }

  return result;
}
