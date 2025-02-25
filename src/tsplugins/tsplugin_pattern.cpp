//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Replace packet payload with a binary pattern on selected PID's
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsByteBlock.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PatternPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PatternPlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        uint8_t   _offset_pusi = 0;      // Start offset in packets with PUSI
        uint8_t   _offset_non_pusi = 0;  // Start offset in packets without PUSI
        ByteBlock _pattern {};           // Binary pattern to apply
        PIDSet    _pid_list {};          // Array of pid values to filter
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pattern", ts::PatternPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PatternPlugin::PatternPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Replace packet payload with a binary pattern on selected PID's", u"[options] pattern")
{
    option(u"", 0, HEXADATA, 1, 1, 1, PKT_MAX_PAYLOAD_SIZE);
    help(u"",
         u"Specifies the binary pattern to apply on TS packets payload. "
         u"The value must be a string of hexadecimal digits specifying any "
         u"number of bytes.");

    option(u"negate", 'n');
    help(u"negate",
         u"Negate the PID filter: modify packets on all PID's, expect the "
         u"specified ones.");

    option(u"offset-non-pusi", 'o', INTEGER, 0, 1, 0, PKT_SIZE - 4);
    help(u"offset-non-pusi",
         u"Specify starting offset in payload of packets with the PUSI (payload. "
         u"unit start indicator) not set. By default, the pattern replacement "
         u"starts at the beginning of the packet payload (offset 0).");

    option(u"offset-pusi", 'u', INTEGER, 0, 1, 0, PKT_SIZE - 4);
    help(u"offset-pusi",
         u"Specify starting offset in payload of packets with the PUSI (payload. "
         u"unit start indicator) set. By default, the pattern replacement "
         u"starts at the beginning of the packet payload (offset 0).");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Select packets with these PID values. Several -p or --pid options "
         u"may be specified to select multiple PID's. If no such option is "
         u"specified, packets from all PID's are modified.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PatternPlugin::start()
{
    getHexaValue(_pattern);
    getIntValue(_offset_pusi, u"offset-pusi", 0);
    getIntValue(_offset_non_pusi, u"offset-non-pusi", 0);
    getIntValues(_pid_list, u"pid", true);

    if (present(u"negate")) {
        _pid_list.flip();
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PatternPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // If the packet has no payload, or not in a selected PID, leave it unmodified
    if (!pkt.hasPayload() || !_pid_list[pkt.getPID()]) {
        return TSP_OK;
    }

    // Compute start of payload area to replace
    uint8_t* pl = pkt.b + pkt.getHeaderSize() + (pkt.getPUSI() ? _offset_pusi : _offset_non_pusi);

    // Compute remaining size to replace (maybe negative if starting offset is beyond the end of the packet).
    int remain = int(pkt.b + PKT_SIZE - pl);

    // Replace the payload with the pattern
    while (remain > 0) {
        int cursize = std::min(remain, int(_pattern.size()));
        MemCopy(pl, _pattern.data(), cursize);
        pl += cursize;
        remain -= cursize;
    }

    return TSP_OK;
}
