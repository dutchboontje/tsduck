//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCAEMMTSDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"CA_EMM_TS_descriptor"
#define MY_CLASS    ts::CAEMMTSDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_CA_EMM_TS, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CAEMMTSDescriptor::CAEMMTSDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::CAEMMTSDescriptor::CAEMMTSDescriptor(DuckContext& duck, const Descriptor& desc) :
    CAEMMTSDescriptor()
{
    deserialize(duck, desc);
}

void ts::CAEMMTSDescriptor::clearContent()
{
    CA_system_id = 0;
    transport_stream_id = 0;
    original_network_id = 0;
    power_supply_period = 0;
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::CAEMMTSDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(CA_system_id);
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.putUInt8(power_supply_period);
}

void ts::CAEMMTSDescriptor::deserializePayload(PSIBuffer& buf)
{
    CA_system_id = buf.getUInt16();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();
    power_supply_period = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CAEMMTSDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "CA System Id: " << CASIdName(disp.duck(), buf.getUInt16(), NamesFlags::VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"Transport stream id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Original network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Power-on time: %d minutes", buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::CAEMMTSDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"power_supply_period", power_supply_period);
}

bool ts::CAEMMTSDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(CA_system_id, u"CA_system_id", true) &&
           element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
           element->getIntAttribute(original_network_id, u"original_network_id", true) &&
           element->getIntAttribute(power_supply_period, u"power_supply_period", true);
}
