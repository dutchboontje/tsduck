//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsECMRepetitionRateDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ECM_repetition_rate_descriptor"
#define MY_CLASS    ts::ECMRepetitionRateDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_ECM_REPETITION_RATE, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::ECMRepetitionRateDescriptor::ECMRepetitionRateDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ECMRepetitionRateDescriptor::clearContent()
{
    CA_system_id = 0;
    ECM_repetition_rate = 0;
    private_data.clear();
}

ts::ECMRepetitionRateDescriptor::ECMRepetitionRateDescriptor(DuckContext& duck, const Descriptor& desc) :
    ECMRepetitionRateDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ECMRepetitionRateDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(CA_system_id);
    buf.putUInt16(ECM_repetition_rate);
    buf.putBytes(private_data);
}

void ts::ECMRepetitionRateDescriptor::deserializePayload(PSIBuffer& buf)
{
    CA_system_id = buf.getUInt16();
    ECM_repetition_rate = buf.getUInt16();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ECMRepetitionRateDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"ECM_repetition_rate", ECM_repetition_rate, false);
    root->addHexaTextChild(u"private_data", private_data, true);
}

bool ts::ECMRepetitionRateDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(CA_system_id, u"CA_system_id", true) &&
           element->getIntAttribute(ECM_repetition_rate, u"ECM_repetition_rate", true) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 6);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ECMRepetitionRateDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"CA System Id: %s", CASIdName(disp.duck(), buf.getUInt16(), NamesFlags::VALUE_NAME)) << std::endl;
        disp << margin << UString::Format(u"ECM repetition rate: %d ms", buf.getUInt16()) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}
