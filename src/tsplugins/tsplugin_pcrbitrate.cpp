//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Permanently recompute bitrate based on PCR analysis
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPCRAnalyzer.h"

#define DEF_MIN_PCR_CNT  128
#define DEF_MIN_PID        1


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRBitratePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCRBitratePlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        PCRAnalyzer _pcr_analyzer {}; // PCR analysis context
        BitRate     _bitrate = 0;     // Last remembered bitrate (keep it signed)
        UString     _pcr_name {};     // Time stamp type name

        // PCR analysis is done permanently. Typically, the analysis of a
        // constant stream will produce different results quite often. But
        // the results vary by a few bits only. This is a normal behavior
        // which would generate useless activity if reported. Consequently,
        // once a bitrate is statistically computed, we keep it as long as
        // the results are not significantly different. We ignore new results
        // which vary only by less than the following factor.

        static constexpr BitRate::int_t REPORT_THRESHOLD = 500000; // 100 b/s on a 50 Mb/s stream
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcrbitrate", ts::PCRBitratePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRBitratePlugin::PCRBitratePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Permanently recompute bitrate based on PCR analysis", u"[options]")
{
    option(u"dts", 'd');
    help(u"dts",
         u"Use DTS (Decoding Time Stamps) from video PID's instead of PCR "
         u"(Program Clock Reference) from the transport layer.");

    option(u"ignore-errors", 'i');
    help(u"ignore-errors",
         u"Ignore transport stream errors such as discontinuities. When errors are "
         u"not ignored (the default), the bitrate of the original stream (before corruptions) "
         u"is evaluated. When errors are ignored, the bitrate of the received stream is "
         u"evaluated, missing packets being considered as non-existent.");

    option(u"min-pcr", 0, POSITIVE);
    help(u"min-pcr",
         u"Stop analysis when that number of PCR are read from the required "
         u"minimum number of PID (default: " TS_STRINGIFY(DEF_MIN_PCR_CNT) u").");

    option(u"min-pid", 0, POSITIVE);
    help(u"min-pid",
         u"Minimum number of PID to get PCR from (default: " TS_STRINGIFY(DEF_MIN_PID) u").");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRBitratePlugin::start()
{
    _pcr_analyzer.setIgnoreErrors(present(u"ignore-errors"));
    const size_t min_pcr = intValue<size_t>(u"min-pcr", DEF_MIN_PCR_CNT);
    const size_t min_pid = intValue<size_t>(u"min-pid", DEF_MIN_PID);
    if (present(u"dts")) {
        _pcr_analyzer.resetAndUseDTS (min_pid, min_pcr);
        _pcr_name = u"DTS";
    }
    else {
        _pcr_analyzer.reset (min_pid, min_pcr);
        _pcr_name = u"PCR";
    }
    _bitrate = 0;
    return true;
}


//----------------------------------------------------------------------------
// Bitrate reporting method
//----------------------------------------------------------------------------

ts::BitRate ts::PCRBitratePlugin::getBitrate()
{
    return _bitrate;
}

ts::BitRateConfidence ts::PCRBitratePlugin::getBitrateConfidence()
{
    // The returned bitrate is based on continuous evaluation of PCR.
    return BitRateConfidence::PCR_CONTINUOUS;
}



//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRBitratePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed the packet into the PCR analyzer.

    if (_pcr_analyzer.feedPacket(pkt)) {
        // A new bitrate is available, get it and restart analysis
        BitRate new_bitrate = _pcr_analyzer.bitrate188();
        _pcr_analyzer.reset();

        // If the new bitrate is too close to the previous recorded one, no need to signal it.
        if (new_bitrate != _bitrate && (new_bitrate / (new_bitrate - _bitrate)).abs() < REPORT_THRESHOLD) {
            // New bitrate is significantly different, signal it.
            verbose(u"new bitrate from %s analysis: %'d b/s", _pcr_name, new_bitrate);
            _bitrate = new_bitrate;
            pkt_data.setBitrateChanged(true);
        }
    }
    return TSP_OK;
}
