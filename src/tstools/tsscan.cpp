//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  DVB network scanning utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTuner.h"
#include "tsTunerArgs.h"
#include "tsSignalState.h"
#include "tsModulation.h"
#include "tsHFBand.h"
#include "tsTSScanner.h"
#include "tsChannelFile.h"
#include "tsNIT.h"
#include "tsTransportStreamId.h"
#include "tsDescriptorList.h"
TS_MAIN(MainCode);

#define DEFAULT_PSI_TIMEOUT   10000 // ms
#define DEFAULT_MIN_STRENGTH  10
#define OFFSET_EXTEND         3


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace {
    class ScanOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(ScanOptions);
    public:
        ScanOptions(int argc, char *argv[]);

        ts::DuckContext   duck {this};
        ts::TunerArgs     tuner_args {false};
        bool              uhf_scan = false;
        bool              vhf_scan = false;
        bool              nit_scan = false;
        bool              no_offset = false;
        bool              use_best_strength = false;
        uint32_t          first_channel = 0;
        uint32_t          last_channel = 0;
        int32_t           first_offset = 0;
        int32_t           last_offset = 0;
        int64_t           min_strength = 0;
        bool              show_modulation = false;
        bool              list_services = false;
        bool              global_services = false;
        cn::milliseconds  psi_timeout {};
        const ts::HFBand* hfband = nullptr;
        ts::UString       channel_file {};
        bool              update_channel_file = false;
        bool              default_channel_file = false;
        std::vector<ts::DeliverySystem> delivery_systems {};
    };
}

ScanOptions::ScanOptions(int argc, char *argv[]) :
    Args(u"Scan a DTV network frequencies and services", u"[options]")
{
    duck.defineArgsForCharset(*this);
    duck.defineArgsForHFBand(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    tuner_args.defineArgs(*this, true);

    setIntro(u"There are three mutually exclusive types of network scanning. "
             u"Exactly one of the following options shall be specified: "
             u"--nit-scan, --uhf-band, --vhf-band.");

    // The following option replaces --delivery-system as defined in ModulationArgs (through TunerArgs).
    // We want to allow more than one value for it.
    option(u"delivery-system", 0, ts::DeliverySystemEnum(), 0, ts::Args::UNLIMITED_COUNT);
    help(u"delivery-system",
         u"Specify which delivery system to use. "
         u"By default, use the default system for the tuner.\n"
         u"With --nit-scan, this is the delivery system for the stream which contains the NIT to scan.\n"
         u"With --uhf-band and --vhf-band, the option can be specified several times. "
         u"In that case, the multiple delivery systems are tested in the specified order on each channel. "
         u"This is typically used to scan terrestrial networks using DVB-T and DVT-T2. "
         u"Be aware that the scan time is multiplied by the number of specified systems on channels without signal.");

    option(u"nit-scan", 'n');
    help(u"nit-scan",
         u"Tuning parameters for a reference transport stream must be present (frequency or channel reference). "
         u"The NIT is read on the specified frequency and a full scan of the corresponding network is performed.");

    option(u"uhf-band", 'u');
    help(u"uhf-band",
         u"Perform a complete UHF-band scanning (DVB-T, ISDB-T or ATSC). "
         u"Use the predefined UHF frequency layout of the specified region (see option --hf-band-region). "
         u"By default, scan the center frequency of each channel only. "
         u"Use option --use-offsets to scan all predefined offsets in each channel.");

    option(u"vhf-band", 'v');
    help(u"vhf-band", u"Perform a complete VHF-band scanning. See also option --uhf-band.");

    option(u"best-quality");
    help(u"best-quality", u"Obsolete option, do not use.");

    option(u"best-strength");
    help(u"best-strength",
         u"With UHF/VHF-band scanning, for each channel, use the offset with the best signal strength. "
         u"By default, use the average of lowest and highest offsets with required minimum strength. "
         u"Note that some tuners cannot report a correct signal strength, making this option useless.");

    option(u"first-channel", 0, POSITIVE);
    help(u"first-channel",
         u"For UHF/VHF-band scanning, specify the first channel to scan (default: lowest channel in band).");

    option(u"first-offset", 0, INTEGER, 0, 1, -40, +40);
    help(u"first-offset",
         u"For UHF/VHF-band scanning, specify the first offset to scan on each channel.");

    option(u"global-service-list", 'g');
    help(u"global-service-list",
         u"Same as --service-list but display a global list of services at the end "
         u"of scanning instead of per transport stream.");

    option(u"last-channel", 0, POSITIVE);
    help(u"last-channel",
         u"For UHF/VHF-band scanning, specify the last channel to scan (default: highest channel in band).");

    option(u"last-offset", 0, INTEGER, 0, 1, -40, +40);
    help(u"last-offset",
         u"For UHF/VHF-band scanning, specify the last offset to scan on each channel. "
         u"Note that tsscan may scan higher offsets. As long as some signal is found at a "
         u"specified offset, tsscan continues to check up to 3 higher offsets above the \"last\" one. "
         u"This means that if a signal is found at offset +2, offset +3 will be checked anyway, etc. up to offset +5.");

    option(u"min-quality", 0, INT64);
    help(u"min-quality", u"Obsolete option, do not use.");

    option(u"min-strength", 0, INT64);
    help(u"min-strength",
         u"Minimum signal strength. Frequencies with lower signal strength are ignored. "
         u"The value can be in milli-dB or percentage. It depends on the tuner and its driver. "
         u"Check the displayed unit. "
         u"The default is " + ts::UString::Decimal(DEFAULT_MIN_STRENGTH) + u", whatever unit it is.");

    option(u"no-offset");
    help(u"no-offset",
         u"For UHF/VHF-band scanning, scan only the central frequency of each channel. "
         u"This is now the default. Specify option --use-offsets to scan all offsets.");

    option(u"use-offsets");
    help(u"use-offsets",
         u"For UHF/VHF-band scanning, do not scan only the central frequency of each channel. "
         u"Also scan frequencies with offsets. As an example, if a signal is transmitted at offset +1, "
         u"the reception may be successful at offsets -1 to +3 (but not -2 and +4). "
         u"With this option, tsscan checks all offsets and reports that the signal is at offset +1. "
         u"By default, tsscan reports that the signal is found at the central frequency of the channel (offset zero).");

    option<cn::milliseconds>(u"psi-timeout");
    help(u"psi-timeout",
         u"Specifies the timeout, in milli-seconds, for PSI/SI table collection. "
         u"Useful only with --service-list. The default is " +
         ts::UString::Decimal(DEFAULT_PSI_TIMEOUT) + u" milli-seconds.");

    option(u"service-list", 'l');
    help(u"service-list", u"Read SDT of each channel and display the list of services.");

    option(u"show-modulation", 0);
    help(u"show-modulation",
         u"Display modulation parameters when possible. Note that some tuners "
         u"cannot report correct modulation parameters, making this option useless.");

    option(u"save-channels", 0, FILENAME);
    help(u"save-channels", u"filename",
         u"Save the description of all channels in the specified XML file. "
         u"If the file name is \"-\", use the default tuning configuration file. "
         u"See also option --update-channels.");

    option(u"update-channels", 0, FILENAME);
    help(u"update-channels", u"filename",
         u"Update the description of all channels in the specified XML file. "
         u"The content of each scanned transport stream is replaced in the file. "
         u"If the file does not exist, it is created. "
         u"If the file name is \"-\", use the default tuning configuration file. "
         u"The location of the default tuning configuration file depends on the system. "
#if defined(TS_LINUX)
         u"On Linux, the default file is $HOME/.tsduck.channels.xml. "
#elif defined(TS_WINDOWS)
         u"On Windows, the default file is %APPDATA%\\tsduck\\channels.xml. "
#endif
         u"See also option --save-channels.");

    analyze(argc, argv);
    duck.loadArgs(*this);
    tuner_args.loadArgs(duck, *this);

    // Type of scanning
    uhf_scan = present(u"uhf-band");
    vhf_scan = present(u"vhf-band");
    nit_scan = present(u"nit-scan");

    if (nit_scan + uhf_scan + vhf_scan != 1) {
        error(u"specify exactly one of --nit-scan, --uhf-band or --vhf-band");
    }
    if (nit_scan && !tuner_args.hasModulationArgs()) {
        error(u"specify the characteristics of the reference TS with --nit-scan");
    }

    // --delivery-system is fetched twice. Once in tuner_args.loadArgs() where only
    // one optional value is fetched. And once here, with multiple values.
    getIntValues(delivery_systems, u"delivery-system");
    if (nit_scan && delivery_systems.size() > 1) {
        error(u"specify at most one --delivery-system with --nit-scan");
    }

    // Type of HF band to use.
    hfband = vhf_scan ? duck.vhfBand() : duck.uhfBand();

    use_best_strength = present(u"best-strength");
    list_services = present(u"service-list");
    global_services = present(u"global-service-list");
    show_modulation = present(u"show-modulation");
    no_offset = !present(u"use-offsets");

    getIntValue(first_channel, u"first-channel", hfband->firstChannel());
    getIntValue(last_channel, u"last-channel", hfband->lastChannel());
    getIntValue(min_strength, u"min-strength", DEFAULT_MIN_STRENGTH);
    getChronoValue(psi_timeout, u"psi-timeout", cn::milliseconds(DEFAULT_PSI_TIMEOUT));
    if (no_offset) {
        first_offset = last_offset = 0;
    }
    else {
        getIntValue(first_offset, u"first-offset", hfband->firstOffset(first_channel));
        getIntValue(last_offset, u"last-offset", hfband->lastOffset(first_channel));
    }

    // Generate error messages when channels are incorrect.
    // This is only an initial check. The channel number is rechecked during the scan
    // of each channel because a band can contain "holes" (unallocated channel numbers).
    hfband->isValidChannel(first_channel, *this);
    hfband->isValidChannel(last_channel, *this);

    const bool save_channel_file = present(u"save-channels");
    update_channel_file = present(u"update-channels");
    channel_file = update_channel_file ? value(u"update-channels") : value(u"save-channels");
    default_channel_file = (save_channel_file || update_channel_file) && (channel_file.empty() || channel_file == u"-");

    if (save_channel_file && update_channel_file) {
        error(u"--save-channels and --update-channels are mutually exclusive");
    }
    else if (default_channel_file) {
        // Use default channel file.
        channel_file = ts::ChannelFile::DefaultFileName();
    }

    exitOnError();
}


//----------------------------------------------------------------------------
// UHF/VHF-band offset scanner: Scan offsets around a specific channel and
// determine offset with the best signal.
//----------------------------------------------------------------------------

class OffsetScanner
{
    TS_NOBUILD_NOCOPY(OffsetScanner);
public:
    // Constructor: Perform scanning. Keep signal tuned on best offset.
    OffsetScanner(ScanOptions& opt, ts::Tuner& tuner, uint32_t channel);

    // Check if signal found and which offset is the best one.
    bool signalFound() const { return _signal_found; }
    uint32_t channel() const { return _channel; }
    int32_t bestOffset() const { return _best_offset; }
    void getTunerParameters(ts::ModulationArgs& params) const { params = _best_params; }

private:
    ScanOptions&       _opt;
    ts::Tuner&         _tuner;
    const uint32_t     _channel;
    bool               _signal_found = false;
    int32_t            _best_offset = 0;
    int32_t            _lowest_offset = 0;
    int32_t            _highest_offset = 0;
    int64_t            _best_strength = 0;
    int32_t            _best_strength_offset = 0;
    ts::ModulationArgs _best_params {};

    // Scan the whole thing on one delivery system (DS_UNDEFINED means tuner's default).
    void scanAll(ts::DeliverySystem sys);

    // Build tuning parameters for a channel.
    void buildTuningParameters(ts::ModulationArgs& params, int32_t offset, ts::DeliverySystem sys);

    // Tune to specified offset. Return false on error.
    bool tune(int32_t offset, ts::ModulationArgs& params, ts::DeliverySystem sys);

    // Test the signal at one specific offset. Return true if signal is found.
    bool tryOffset(int32_t offset, ts::DeliverySystem sys);
};


//----------------------------------------------------------------------------
// UHF-band offset scanner constructor.
// Perform scanning. Keep signal tuned on best offset
//----------------------------------------------------------------------------

OffsetScanner::OffsetScanner(ScanOptions& opt, ts::Tuner& tuner, uint32_t channel) :
    _opt(opt),
    _tuner(tuner),
    _channel(channel)
{
    if (_opt.delivery_systems.empty()) {
        // Unspecified delivery system, use default one from the tuner.
        scanAll(ts::DS_UNDEFINED);
    }
    else {
        for (auto sys : _opt.delivery_systems) {
            scanAll(sys);
            if (_signal_found) {
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------
// Scan the whole thing on one delivery system (DS_UNDEFINED means tuner's default).
//----------------------------------------------------------------------------

void OffsetScanner::scanAll(ts::DeliverySystem sys)
{
    ts::UString desc;
    if (sys != ts::DS_UNDEFINED) {
        desc.format(u" (%s)", ts::DeliverySystemEnum().name(sys));
    }
    if (!_opt.hfband->isValidChannel(_channel, _opt)) {
        return;
    }
    _opt.verbose(u"scanning channel %'d, %'d Hz%s", _channel, _opt.hfband->frequency(_channel), desc);

    if (_opt.no_offset) {
        // Only try the central frequency
        tryOffset(0, sys);
    }
    else {
        // Scan lower offsets in descending order, starting at central frequency
        if (_opt.first_offset <= 0) {
            bool last_ok = false;
            int32_t offset = _opt.last_offset > 0 ? 0 : _opt.last_offset;
            while (offset >= _opt.first_offset - (last_ok ? OFFSET_EXTEND : 0)) {
                last_ok = tryOffset(offset, sys);
                --offset;
            }
        }

        // Scan higher offsets in ascending order, starting after central frequency
        if (_opt.last_offset > 0) {
            bool last_ok = false;
            int32_t offset = _opt.first_offset <= 0 ? 1 : _opt.first_offset;
            while (offset <= _opt.last_offset + (last_ok ? OFFSET_EXTEND : 0)) {
                last_ok = tryOffset(offset, sys);
                ++offset;
            }
        }
    }

    // If signal was found, select best offset
    if (_signal_found) {
        if (_opt.no_offset) {
            // No offset search, the best and only offset is zero.
            _best_offset = 0;
        }
        else if (_opt.use_best_strength && _best_strength > 0) {
            // Signal strength indicator is valid, use offset with best signal strength
            _best_offset = _best_strength_offset;
        }
        else {
            // Default: use average between lowest and highest offsets
            _best_offset = (_lowest_offset + _highest_offset) / 2;
        }

        // Finally, tune back to best offset
        _signal_found = tune(_best_offset, _best_params, sys) && _tuner.getCurrentTuning(_best_params, false);
    }
}


//----------------------------------------------------------------------------
// Build tuning parameters for a channel.
//----------------------------------------------------------------------------

void OffsetScanner::buildTuningParameters(ts::ModulationArgs& params, int32_t offset, ts::DeliverySystem sys)
{
    // Force frequency in tuning parameters.
    // Other tuning parameters from command line (or default values).
    params = _opt.tuner_args;
    if (sys == ts::DS_UNDEFINED) {
        params.resolveDeliverySystem(_tuner.deliverySystems(), _opt);
    }
    else {
        params.delivery_system = sys;
    }
    params.frequency = _opt.hfband->frequency(_channel, offset);
    params.setDefaultValues();
}


//----------------------------------------------------------------------------
// UHF-band offset scanner: Tune to specified offset. Return false on error.
//----------------------------------------------------------------------------

bool OffsetScanner::tune(int32_t offset, ts::ModulationArgs& params, ts::DeliverySystem sys)
{
    buildTuningParameters(params, offset, sys);
    return _tuner.tune(params);
}


//----------------------------------------------------------------------------
// UHF-band offset scanner: Test the signal at one specific offset.
//----------------------------------------------------------------------------

bool OffsetScanner::tryOffset(int32_t offset, ts::DeliverySystem sys)
{
    _opt.debug(u"trying offset %d", offset);

    // Tune to transponder and start signal acquisition.
    // Signal locking timeout is applied in start().
    ts::ModulationArgs params;
    if (!tune(offset, params, sys) || !_tuner.start()) {
        return false;
    }

    // Get signal characteristics.
    ts::SignalState state;
    bool ok = _tuner.getSignalState(state) && state.signal_locked;

    // If we get a signal and we wee need to scan offsets, check signal strength.
    // If we don't scan offsets, there is no need to consider signal strength, just use the central offset.
    if (ok && !_opt.no_offset) {

        _opt.verbose(u"%s, %s", _opt.hfband->description(_channel, offset), state);

        if (state.signal_strength.has_value()) {
            const int64_t strength = state.signal_strength.value().value;
            if (strength <= _opt.min_strength) {
                // Strength is supported but too low
                ok = false;
            }
            else if (strength > _best_strength) {
                // Best offset so far for signal strength
                _best_strength = strength;
                _best_strength_offset = offset;
                _tuner.getCurrentTuning(params, false);
            }
        }
    }

    if (ok) {
        if (!_signal_found) {
            // First offset with signal on this channel
            _signal_found = true;
            _lowest_offset = _highest_offset = offset;
        }
        else if (offset < _lowest_offset) {
            _lowest_offset = offset;
        }
        else if (offset > _highest_offset) {
            _highest_offset = offset;
        }
    }

    // Stop signal acquisition
    _tuner.stop();

    return ok;
}


//----------------------------------------------------------------------------
// Scanning context.
//----------------------------------------------------------------------------

class ScanContext
{
    TS_NOBUILD_NOCOPY(ScanContext);
public:
    // Constructor.
    ScanContext(ScanOptions&);

    // tsscan main code.
    void main();

private:
    ScanOptions&    _opt;
    ts::Tuner       _tuner;
    ts::ServiceList _services;
    ts::ChannelFile _channels;

    // Analyze a TS and generate relevant info.
    void scanTS(std::ostream& strm, const ts::UString& margin, ts::ModulationArgs& tparams);

    // UHF/VHF-band scanning
    void hfBandScan();

    // NIT-based scanning
    void nitScan();
};

// Constructor.
ScanContext::ScanContext(ScanOptions& opt) :
    _opt(opt),
    _tuner(_opt.duck),
    _services(),
    _channels()
{
}


//----------------------------------------------------------------------------
// Analyze a TS and generate relevant info.
//----------------------------------------------------------------------------

void ScanContext::scanTS(std::ostream& strm, const ts::UString& margin, ts::ModulationArgs& tparams)
{
    const bool get_services = _opt.list_services || _opt.global_services;

    // Collect info from the TS.
    // Use "PAT only" when we do not need the services or channels file.
    ts::TSScanner info(_opt.duck, _tuner, _opt.psi_timeout, !get_services && _opt.channel_file.empty());

    // Get tuning parameters again, as TSScanner waits for a lock.
    // Also keep the original frequency and polarity since satellite tuners can only report the intermediate frequency.
    const std::optional<uint64_t> saved_frequency(tparams.frequency);
    const std::optional<ts::Polarization> saved_polarity(tparams.polarity);
    info.getTunerParameters(tparams);
    if (!tparams.frequency.has_value() || tparams.frequency.value() == 0) {
        tparams.frequency = saved_frequency;
    }
    if (!tparams.polarity.has_value()) {
        tparams.polarity = saved_polarity;
    }

    std::shared_ptr<ts::PAT> pat;
    std::shared_ptr<ts::SDT> sdt;
    std::shared_ptr<ts::NIT> nit;

    info.getPAT(pat);
    info.getSDT(sdt);
    info.getNIT(nit);

    // Get network and TS Id.
    uint16_t ts_id = 0;
    uint16_t net_id = 0;
    if (pat != nullptr) {
        ts_id = pat->ts_id;
        strm << margin << ts::UString::Format(u"Transport stream id: %d, 0x%X", ts_id, ts_id) << std::endl;
    }
    if (nit != nullptr) {
        net_id = nit->network_id;
    }

    // Reset TS description in channels file.
    ts::ChannelFile::TransportStreamPtr ts_info;
    if (!_opt.channel_file.empty()) {
        ts::ChannelFile::NetworkPtr net_info(_channels.networkGetOrCreate(net_id, ts::TunerTypeOf(tparams.delivery_system.value_or(ts::DS_UNDEFINED))));
        ts_info = net_info->tsGetOrCreate(ts_id);
        ts_info->clear(); // reset all services in TS.
        ts_info->onid = sdt == nullptr ? 0 : sdt->onetw_id;
        ts_info->tune = tparams;
    }

    // Display modulation parameters
    if (_opt.show_modulation) {
        tparams.display(strm, margin, _opt.maxSeverity());
    }

    // Display or collect services
    if (get_services || ts_info != nullptr) {
        ts::ServiceList srvlist;
        if (info.getServices(srvlist)) {
            if (ts_info != nullptr) {
                // Add all services in the channels info.
                ts_info->addServices(srvlist);
            }
            if (_opt.list_services) {
                // Display services for this TS
                srvlist.sort(ts::Service::Sort1);
                strm << std::endl;
                ts::Service::Display(strm, margin, srvlist);
                strm << std::endl;
            }
            if (_opt.global_services) {
                // Add collected services in global service list
                _services.insert(_services.end(), srvlist.begin(), srvlist.end());
            }
        }
    }
}


//----------------------------------------------------------------------------
// UHF/VHF-band scanning
//----------------------------------------------------------------------------

void ScanContext::hfBandScan()
{
    // Loop on all selected UHF channels
    for (uint32_t chan = _opt.first_channel; chan <= _opt.last_channel; ++chan) {

        // Scan all offsets surrounding the channel.
        OffsetScanner offscan(_opt, _tuner, chan);
        if (offscan.signalFound()) {

            // A channel was found, report its characteristics.
            ts::SignalState state;
            _tuner.getSignalState(state);
            std::cout << "* " << _opt.hfband->description(chan, offscan.bestOffset()) << ", " << state.toString() << std::endl;

            // Analyze PSI/SI if required.
            ts::ModulationArgs tparams;
            offscan.getTunerParameters(tparams);
            scanTS(std::cout, u"  ", tparams);
        }
    }
}


//----------------------------------------------------------------------------
// NIT-based scanning
//----------------------------------------------------------------------------

void ScanContext::nitScan()
{
    // Tune to the reference transponder.
    if (!_tuner.tune(_opt.tuner_args)) {
        return;
    }

    // Collect info on reference transponder.
    ts::TSScanner info(_opt.duck, _tuner, _opt.psi_timeout, false);

    // Get the collected NIT
    std::shared_ptr<ts::NIT> nit;
    info.getNIT(nit);
    if (nit == nullptr) {
        _opt.error(u"cannot scan network, no NIT found on specified transponder");
        return;
    }

    // Process each TS descriptor list in the NIT.
    for (const auto& it : nit->transports) {
        const ts::TransportStreamId& tsid(it.first);
        const ts::DescriptorList& dlist(it.second.descs);
        ts::ModulationArgs params;
        if (params.fromDeliveryDescriptors(_opt.duck, dlist, tsid.transport_stream_id, _opt.tuner_args.delivery_system.value_or(ts::DS_UNDEFINED))) {
            // Got delivery descriptors, this is the description of one transponder.
            // Copy the local reception parameters (LNB, etc.) from the command line options
            // (we use the same reception equipment).
            params.copyLocalReceptionParameters(_opt.tuner_args);
            // Tune to this transponder.
            _opt.debug(u"* tuning to " + params.toPluginOptions(true));
            if (_tuner.tune(params)) {
                // Report channel characteristics
                ts::SignalState state;
                _tuner.getSignalState(state);
                std::cout << "* Frequency: " << params.shortDescription(_opt.duck) << ", " << state.toString() << std::endl;
                // Analyze PSI/SI if required
                scanTS(std::cout, u"  ", params);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Main code from scan context.
//----------------------------------------------------------------------------

void ScanContext::main()
{
    // Initialize tuner.
    _tuner.setSignalTimeoutSilent(true);
    if (!_opt.tuner_args.configureTuner(_tuner)) {
        return;
    }

    // Pre-load the existing channel file.
    if (_opt.update_channel_file && !_opt.channel_file.empty() && fs::exists(_opt.channel_file) && !_channels.load(_opt.channel_file, _opt)) {
        return;
    }

    // Main processing depends on scanning method.
    if (_opt.uhf_scan || _opt.vhf_scan) {
        hfBandScan();
    }
    else if (_opt.nit_scan) {
        nitScan();
    }
    else {
        _opt.fatal(u"inconsistent options, internal error");
    }

    // Report global list of services if required
    if (_opt.global_services) {
        _services.sort(ts::Service::Sort1);
        std::cout << std::endl;
        ts::Service::Display(std::cout, u"", _services);
    }

    // Save channel file. Create intermediate directories when it is the default file.
    if (!_opt.channel_file.empty()) {
        _opt.verbose(u"saving %s", _opt.channel_file);
        _channels.save(_opt.channel_file, _opt.default_channel_file, _opt);
    }
}


//----------------------------------------------------------------------------
// Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ScanOptions opt(argc, argv);
    ScanContext ctx(opt);
    ctx.main();
    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
