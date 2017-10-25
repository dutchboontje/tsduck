//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for all sorts of demux from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Abstract base class for all sorts of demux from TS packets.
    //!
    //! The application sets a number of PID's to filter. What is extracted
    //! from those PID's and how they are reported to the application depend
    //! on the concrete demux class.
    //!
    class TSDUCKDLL AbstractDemux
    {
    public:
        //!
        //! The following method feeds the demux with a TS packet.
        //! @param [in] pkt A TS packet.
        //!
        virtual void feedPacket(const TSPacket& pkt) = 0;

        //!
        //! Replace the list of PID's to filter.
        //! The method resetPID() is invoked on each removed PID.
        //! @param [in] pid_filter The list of PID's to filter.
        //!
        virtual void setPIDFilter(const PIDSet& pid_filter);

        //!
        //! Add one PID to filter.
        //! @param [in] pid The new PID to filter.
        //!
        virtual void addPID(PID pid);

        //!
        //! Add several PID's to filter.
        //! @param [in] pids The list of new PID's to filter.
        //!
        virtual void addPIDs(const PIDSet& pids);

        //!
        //! Remove one PID to filter.
        //! The method resetPID() is invoked on @a pid.
        //! @param [in] pid The PID to no longer filter.
        //!
        virtual void removePID(PID pid);

        //!
        //! Get the current number of PID's being filtered.
        //! @return The current number of PID's being filtered.
        //!
        virtual size_t pidCount() const
        {
            return _pid_filter.count();
        }

        //!
        //! Reset the demux.
        //!
        //! Useful when the transport stream changes.
        //! The PID filter and the handlers are not modified.
        //!
        //! If invoked in an application-handler, the operation is delayed until
        //! the handler terminates. For subclass implementers, see beforeCallingHandler()
        //! and override immediateReset() instead of reset().
        //!
        virtual void reset();

        //!
        //! Reset the demuxing context for one single PID.
        //! Forget all previous partially demuxed data on this PID.
        //!
        //! If invoked in an application-handler, the operation is delayed until
        //! the handler terminates. For subclass implementers, see beforeCallingHandler()
        //! and override immediateResetPID() instead of resetPID().
        //!
        //! @param [in] pid The PID to reset.
        //!
        virtual void resetPID(PID pid);

        //!
        //! Destructor.
        //!
        virtual ~AbstractDemux();

    protected:
        //!
        //! Constructor.
        //! @param [in] pid_filter The initial set of PID's to demux.
        //!
        AbstractDemux(const PIDSet& pid_filter = NoPID);

        //!
        //! Helper for subclass, before invoking an application-defined handler.
        //!
        //! The idea is to protect the integrity of the demux during the execution
        //! of an application-defined handler. The handler is invoked in the middle
        //! of an operation but the handler may call reset() or resetPID(). Executing
        //! the reset in the middle of an operation may be problematic. By using
        //! beforeCallingHandler() and afterCallingHandler(), all reset operations
        //! in between are delayed after the execution of the handler.
        //!
        //! Example:
        //! @code
        //! beforeCallingHandler(pid);
        //! try {
        //!     _handler->handleEvent(*this, pid, ...);
        //! }
        //! catch (...) {
        //!     afterCallingHandler(false);
        //!     throw;
        //! }
        //! afterCallingHandler();
        //! @endcode
        //!
        void beforeCallingHandler(PID pid = PID_NULL);

        //!
        //! Helper for subclass, after invoking an application-defined handler.
        //! @param [in] executeDelayedOperations When true (the default), execute all pending reset operations.
        //! @return True if a delayed reset was executed.
        //! @see beforeCallingHandler()
        //!
        bool afterCallingHandler(bool executeDelayedOperations = true);

        //!
        //! Reset the demux immediately.
        //!
        virtual void immediateReset();

        //!
        //! Reset the demuxing context for one single PID immediately.
        //! @param [in] pid The PID to reset.
        //!
        virtual void immediateResetPID(PID pid);

        //!
        //! Current set of filtered PID's, directly accessible to subclasses.
        //!
        PIDSet _pid_filter;

    private:
        bool _in_handler;        // true when in the context of an application-defined handler
        PID  _pid_in_handler;    // PID which is currently processed by the handler
        bool _reset_pending;     // delayed reset()
        bool _pid_reset_pending; // delayed resetPID(_pid_in_handler)
    };
}
