//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBAccessControlDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_access_control_descriptor"
#define MY_CLASS    ts::ISDBAccessControlDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_CA, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBAccessControlDescriptor::ISDBAccessControlDescriptor(uint16_t id, PID p) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    CA_system_id(id)
{
}

void ts::ISDBAccessControlDescriptor::clearContent()
{
    CA_system_id = 0;
    transmission_type = 7;  // broadcast route
    pid = PID_NULL;
    private_data.clear();
}

ts::ISDBAccessControlDescriptor::ISDBAccessControlDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBAccessControlDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(CA_system_id);
    buf.putBits(transmission_type, 3);
    buf.putPID(pid);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::deserializePayload(PSIBuffer& buf)
{
    CA_system_id = buf.getUInt16();
    buf.getBits(transmission_type, 3);
    pid = buf.getPID();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        const TID tid = context.getTableId();
        const UChar* const dtype = tid == TID_CAT ? u"EMM" : (tid == TID_PMT ? u"ECM" : u"CA");
        disp << margin << "CA System Id: " << CASIdName(disp.duck(), buf.getUInt16(), NamesFlags::VALUE_NAME) << std::endl;
        disp << margin << "Transmission type: " << DataName(MY_XML_NAME, u"CATransmissionType", buf.getBits<uint8_t>(3), NamesFlags::DEC_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"%s PID: %n", dtype, buf.getPID()) << std::endl;
        disp.displayPrivateData(u"Private CA data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"transmission_type", transmission_type);
    root->setIntAttribute(u"PID", pid, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBAccessControlDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(CA_system_id, u"CA_system_id", true) &&
           element->getIntAttribute(transmission_type, u"transmission_type", false, 7, 0, 7) &&
           element->getIntAttribute<PID>(pid, u"PID", true, 0, 0x0000, 0x1FFF) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 4);
}
