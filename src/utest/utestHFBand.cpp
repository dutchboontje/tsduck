//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for HFBand class.
//
//----------------------------------------------------------------------------

#include "tsHFBand.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class HFBandTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(DefaultRegion);
    TSUNIT_DECLARE_TEST(Bands);
    TSUNIT_DECLARE_TEST(Empty);
    TSUNIT_DECLARE_TEST(Europe);
    TSUNIT_DECLARE_TEST(USA);
    TSUNIT_DECLARE_TEST(VHF);
    TSUNIT_DECLARE_TEST(BS);
    TSUNIT_DECLARE_TEST(CS);

private:
    ts::Report& report();
};

TSUNIT_REGISTER(HFBandTest);


//----------------------------------------------------------------------------
// Common tools.
//----------------------------------------------------------------------------

ts::Report& HFBandTest::report()
{
    if (tsunit::Test::debugMode()) {
        return CERR;
    }
    else {
        return NULLREP;
    }
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(DefaultRegion)
{
    debug() << "HFBandTest::testDefaultRegion: default region: \"" << ts::HFBand::DefaultRegion(report()) << "\"" << std::endl;
    TSUNIT_ASSERT(!ts::HFBand::DefaultRegion(report()).empty());
}

TSUNIT_DEFINE_TEST(Bands)
{
    TSUNIT_EQUAL(u"UHF, VHF", ts::UString::Join(ts::HFBand::GetAllBands(u"Europe")));
    TSUNIT_EQUAL(u"BS, CS, UHF, VHF", ts::UString::Join(ts::HFBand::GetAllBands(u"Japan")));
}

TSUNIT_DEFINE_TEST(Empty)
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"zozoland", u"UHF", report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(hf->empty());
    TSUNIT_EQUAL(0, hf->channelCount());
}

TSUNIT_DEFINE_TEST(Europe)
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"Europe", u"UHF", report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(u"UHF", hf->bandName());
    TSUNIT_EQUAL(49, hf->channelCount());
    TSUNIT_EQUAL(21, hf->firstChannel());
    TSUNIT_EQUAL(69, hf->lastChannel());

    TSUNIT_EQUAL(25, hf->nextChannel(24));
    TSUNIT_EQUAL(23, hf->previousChannel(24));
    TSUNIT_EQUAL(498000000, hf->frequency(24));
    TSUNIT_EQUAL(497666668, hf->frequency(24, -2));
    TSUNIT_EQUAL(498333332, hf->frequency(24, +2));
    TSUNIT_EQUAL(24, hf->channelNumber(498000000));
    TSUNIT_EQUAL(24, hf->channelNumber(497666668));
    TSUNIT_EQUAL(24, hf->channelNumber(498333332));
    TSUNIT_EQUAL(0, hf->offsetCount(498000000));
    TSUNIT_EQUAL(-2, hf->offsetCount(497666668));
    TSUNIT_EQUAL(int32_t(+2), hf->offsetCount(498333332));
    TSUNIT_ASSERT(!hf->inBand(200000000, false));
    TSUNIT_ASSERT(!hf->inBand(497666668, true));
    TSUNIT_ASSERT(hf->inBand(498000000, true));
    TSUNIT_ASSERT(hf->inBand(498333332, true));
    TSUNIT_ASSERT(hf->inBand(497666668, false));
    TSUNIT_ASSERT(hf->inBand(498000000, false));
    TSUNIT_ASSERT(hf->inBand(498333332, false));
    TSUNIT_EQUAL(8000000, hf->bandWidth(24));
    TSUNIT_EQUAL(166666, hf->offsetWidth(24));
    TSUNIT_EQUAL(-1, hf->firstOffset(24));
    TSUNIT_EQUAL(3, hf->lastOffset(24));

    TSUNIT_EQUAL(22, hf->nextChannel(21));
    TSUNIT_EQUAL(0, hf->previousChannel(21));

    TSUNIT_EQUAL(0, hf->nextChannel(69));
    TSUNIT_EQUAL(68, hf->previousChannel(69));
}

TSUNIT_DEFINE_TEST(USA)
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"USA", u"UHF", report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(u"UHF", hf->bandName());
    TSUNIT_EQUAL(23, hf->channelCount());
    TSUNIT_EQUAL(14, hf->firstChannel());
    TSUNIT_EQUAL(36, hf->lastChannel());

    TSUNIT_EQUAL(25, hf->nextChannel(24));
    TSUNIT_EQUAL(23, hf->previousChannel(24));
    TSUNIT_EQUAL(533000000, hf->frequency(24));
    TSUNIT_EQUAL(533000000, hf->frequency(24, -2));
    TSUNIT_EQUAL(533000000, hf->frequency(24, +2));
    TSUNIT_EQUAL(24, hf->channelNumber(533000000));
    TSUNIT_EQUAL(0, hf->offsetCount(533000000));
    TSUNIT_EQUAL(6000000, hf->bandWidth(24));
    TSUNIT_EQUAL(0, hf->offsetWidth(24));
    TSUNIT_EQUAL(0, hf->firstOffset(24));
    TSUNIT_EQUAL(0, hf->lastOffset(24));

    TSUNIT_EQUAL(15, hf->nextChannel(14));
    TSUNIT_EQUAL(0, hf->previousChannel(14));

    TSUNIT_EQUAL(0, hf->nextChannel(36));
    TSUNIT_EQUAL(35, hf->previousChannel(36));
}

TSUNIT_DEFINE_TEST(VHF)
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"USA", u"VHF", report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(u"VHF", hf->bandName());
    TSUNIT_EQUAL(13, hf->channelCount());
    TSUNIT_EQUAL(1, hf->firstChannel());
    TSUNIT_EQUAL(13, hf->lastChannel());

    TSUNIT_EQUAL(63000000, hf->frequency(3));
    TSUNIT_EQUAL(63000000, hf->frequency(3, -2));
    TSUNIT_EQUAL(63000000, hf->frequency(3, +2));
    TSUNIT_EQUAL(3, hf->channelNumber(63000000));
    TSUNIT_EQUAL(0, hf->offsetCount(63000000));
    TSUNIT_EQUAL(6000000, hf->bandWidth(3));
    TSUNIT_EQUAL(0, hf->offsetWidth(3));
    TSUNIT_EQUAL(0, hf->firstOffset(3));
    TSUNIT_EQUAL(0, hf->lastOffset(3));

    TSUNIT_EQUAL(2, hf->nextChannel(1));
    TSUNIT_EQUAL(0, hf->previousChannel(1));

    TSUNIT_EQUAL(5, hf->nextChannel(4));
    TSUNIT_EQUAL(3, hf->previousChannel(4));

    TSUNIT_EQUAL(6, hf->nextChannel(5));
    TSUNIT_EQUAL(4, hf->previousChannel(5));

    TSUNIT_EQUAL(0, hf->nextChannel(13));
    TSUNIT_EQUAL(12, hf->previousChannel(13));
}

TSUNIT_DEFINE_TEST(BS)
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"Japan", u"BS", report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(u"BS", hf->bandName());
    TSUNIT_EQUAL(24, hf->channelCount());
    TSUNIT_EQUAL(1, hf->firstChannel());
    TSUNIT_EQUAL(24, hf->lastChannel());

    TSUNIT_EQUAL(11765840000, hf->frequency(3));
    TSUNIT_EQUAL(3, hf->channelNumber(11765840000));
    TSUNIT_EQUAL(19180000, hf->bandWidth(3));
    TSUNIT_EQUAL(ts::POL_RIGHT, hf->polarization(17));
    TSUNIT_EQUAL(ts::POL_LEFT, hf->polarization(12));
}

TSUNIT_DEFINE_TEST(CS)
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"Japan", u"CS", report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(u"CS", hf->bandName());
    TSUNIT_EQUAL(24, hf->channelCount());
    TSUNIT_EQUAL(1, hf->firstChannel());
    TSUNIT_EQUAL(24, hf->lastChannel());

    TSUNIT_EQUAL(12311000000, hf->frequency(3));
    TSUNIT_EQUAL(3, hf->channelNumber(12311000000));
    TSUNIT_EQUAL(20000000, hf->bandWidth(3));
    TSUNIT_EQUAL(ts::POL_LEFT, hf->polarization(17));
    TSUNIT_EQUAL(ts::POL_RIGHT, hf->polarization(12));
}
