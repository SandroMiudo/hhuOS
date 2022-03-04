/*
 * Copyright (C) 2018-2022 Heinrich-Heine-Universitaet Duesseldorf,
 * Institute of Computer Science, Department Operating Systems
 * Burak Akguel, Christian Gesse, Fabian Ruhland, Filip Krakowski, Michael Schoettner
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef __InterruptDispatcher_include__
#define __InterruptDispatcher_include__

#include <cstdint>
#include "InterruptHandler.h"
#include "kernel/process/ThreadState.h"
#include "kernel/service/Service.h"
#include "lib/util/data/ArrayList.h"
#include "lib/util/data/HashMap.h"

namespace Kernel {

/**
 * InterruptDispatcher - responsible for registering and dispatching interrupts to the
 * corresponding handlers.
 *
 * @author Michael Schoettner, Filip Krakowski, Fabian Ruhland, Burak Akguel, Christian Gesse
 * @date HHU, 2018
 */
class InterruptDispatcher {

public:

    enum {
        PAGEFAULT = 14,
        PIT = 32,
        KEYBOARD = 33,
        COM2 = 35,
        COM1 = 36,
        LPT2 = 37,
        FLOPPY = 38,
        LPT1 = 39,
        RTC = 40,
        FREE1 = 41,
        FREE2 = 42,
        FREE3 = 43,
        MOUSE = 44,
        FPU = 45,
        PRIMARY_ATA = 46,
        SECONDARY_ATA = 47,
        SYSTEM_CALL = 0x86
    };

    /**
     * Default constructor.
     */
    InterruptDispatcher() = default;

    InterruptDispatcher(const InterruptDispatcher &other) = delete;

    InterruptDispatcher& operator=(const InterruptDispatcher &other) = delete;

    ~InterruptDispatcher() = default;

    static InterruptDispatcher &getInstance() noexcept;

    /**
     * Register an interrupt handler to an interrupt number.
     *
     * @param slot Interrupt number for this handler
     * @param gate Pointer to the handler itself
     */
    void assign(uint8_t slot, InterruptHandler &gate);

    /**
     * Dispatched the interrupt to all registered interrupt handlers.
     *
     * @param frame The interrupt frame
     */
    void dispatch(InterruptFrame *frame);

    [[nodiscard]] uint32_t getInterruptDepth() const;

private:

    uint32_t interruptDepth = 0;

    /**
     * Get the interrupt handlers that are registered for a specific interrupt.
     *
     * @param slot Interrupt number
     * @return Pointer to a list of all registered handlers or nullptr if no handlers are registered
     */
    Util::Data::List<InterruptHandler*> *getHandlerForSlot(uint8_t slot);

    Util::Data::HashMap<uint8_t, Util::Data::ArrayList<InterruptHandler *> *> handler;

    void sendEoi(uint32_t slot);

};

}

#endif