/*
 * Copyright (C) 2018-2024 Heinrich-Heine-Universitaet Duesseldorf,
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

#include "kernel/service/InterruptService.h"
#include "Pit.h"
#include "kernel/log/Log.h"
#include "device/system/FirmwareConfiguration.h"
#include "device/interrupt/InterruptRequest.h"
#include "kernel/interrupt/InterruptVector.h"
#include "kernel/service/Service.h"
#include "kernel/service/ProcessService.h"
#include "kernel/process/Scheduler.h"

namespace Kernel {
struct InterruptFrame;
}  // namespace Kernel

namespace Device {

Pit::Pit(const Util::Time::Timestamp &timerInterval, const Util::Time::Timestamp &yieldInterval) : timerInterval(timerInterval), yieldInterval(yieldInterval) {
    setInterruptRate(timerInterval);
}

void Pit::setInterruptRate(const Util::Time::Timestamp &interval) {
    auto divisor = interval.toNanoseconds() / NANOSECONDS_PER_TICK;
    if (divisor > UINT16_MAX) divisor = UINT16_MAX;

    setDivisor(divisor);
}

void Pit::setDivisor(uint16_t divisor) {
    timerInterval = Util::Time::Timestamp::ofNanoseconds(NANOSECONDS_PER_TICK * divisor);
    LOG_INFO("Setting PIT interval to [%ums] (Divisor: [%u])", static_cast<uint32_t>(timerInterval.toMilliseconds() < 1 ? 1 : timerInterval.toMilliseconds()), divisor);

    auto command = Command(OperatingMode::RATE_GENERATOR, AccessMode::LOW_BYTE_HIGH_BYTE);
    controlPort.writeByte(static_cast<uint8_t>(command)); // Select channel 0, Use low-/high byte access mode, Set operating mode to rate generator
    dataPort0.writeByte((uint8_t) (divisor & 0xff)); // Low byte
    dataPort0.writeByte((uint8_t) (divisor >> 8)); // High byte
}

void Pit::plugin() {
    auto &interruptService = Kernel::Service::getService<Kernel::InterruptService>();
    interruptService.assignInterrupt(Kernel::InterruptVector::PIT, *this);
    interruptService.allowHardwareInterrupt(Device::InterruptRequest::PIT);
}

void Pit::trigger(const Kernel::InterruptFrame &frame, Kernel::InterruptVector slot) {
    time += timerInterval;

    if (!Kernel::Service::getService<Kernel::InterruptService>().usesApic()) {
        timeSinceLastYield += timerInterval;
        if (timeSinceLastYield > yieldInterval) {
            timeSinceLastYield = Util::Time::Timestamp::ofMilliseconds(0);
            Kernel::Service::getService<Kernel::ProcessService>().getScheduler().yield();
        }
    }
}

Util::Time::Timestamp Pit::getTime() {
    return time;
}

void Pit::wait(const Util::Time::Timestamp &waitTime) {
    auto elapsedTime = Util::Time::Timestamp();
    auto lastTimerValue = readTimer();

    while (elapsedTime < waitTime) {
        auto timerValue = readTimer();
        auto ticks = lastTimerValue >= timerValue ? lastTimerValue - timerValue : ((timerInterval.toNanoseconds() / NANOSECONDS_PER_TICK) - timerValue + lastTimerValue);
        elapsedTime += Util::Time::Timestamp::ofNanoseconds(ticks * NANOSECONDS_PER_TICK);

        lastTimerValue = timerValue;
    }
}

bool Pit::isLocked() const {
    return readTimerLock.isLocked();
}

uint16_t Pit::readTimer() {
    const auto latchCountCommand = Command(OperatingMode::INTERRUPT_ON_TERMINAL_COUNT, AccessMode::LATCH_COUNT);

    // We need to make sure, that no other thread is accessing the PIT, while the timer value is read.
    // However, the scheduler relies on system time for updating its sleep list, potentially causing a deadlock.
    // Circumventing the deadlock using `tryAcquire()` works, but messes up the scheduler's timing, leading to threads sleeping much longer than they anticipated.
    // The best option so far is to just disable interrupts for a short amount of time, while reading the timer value.
    readTimerLock.acquire();
    controlPort.writeByte(static_cast<uint8_t>(latchCountCommand));
    uint16_t lowByte = dataPort0.readByte();
    uint16_t highByte = dataPort0.readByte();
    readTimerLock.release();

    return lowByte | (highByte << 8);
}

Pit::Command::Command(Pit::OperatingMode operatingMode, Pit::AccessMode accessMode) : bcdBinaryMode(BINARY), operatingMode(operatingMode), accessMode(accessMode), channel(CHANNEL_0) {}

Pit::Command::operator uint8_t() const {
    return *reinterpret_cast<const uint8_t*>(this);
}

}