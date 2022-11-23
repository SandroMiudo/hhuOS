/*
 * Copyright (C) 2018-2022 Heinrich-Heine-Universitaet Duesseldorf,
 * Institute of Computer Science, Department Operating Systems
 * Burak Akguel, Christian Gesse, Fabian Ruhland, Filip Krakowski, Hannes Feil, Michael Schoettner
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

#include "ArpModule.h"
#include "network/ethernet/EthernetHeader.h"
#include "lib/util/stream/ByteArrayOutputStream.h"
#include "network/ethernet/EthernetModule.h"
#include "lib/util/async/Thread.h"

namespace Network::Arp {

Kernel::Logger ArpModule::log = Kernel::Logger::get("Arp");

void ArpModule::readPacket(Util::Stream::InputStream &stream, Device::Network::NetworkDevice &device) {
    auto arpHeader = ArpHeader();
    arpHeader.read(stream);

    if (arpHeader.getHardwareAddressType() != ArpHeader::ETHERNET) {
        log.warn("Discarding packet because of unsupported hardware address type 0x%04x", arpHeader.getHardwareAddressType());
        return;
    }

    if (arpHeader.getProtocolAddressType() != ArpHeader::IP4) {
        log.warn("Discarding packet because of unsupported protocol address type 0x%04x", arpHeader.getProtocolAddressType());
        return;
    }

    auto sourceMacAddress = MacAddress();
    auto targetMacAddress = MacAddress();
    auto sourceIpAddress = Ip4::Ip4Address();
    auto targetIpAddress = Ip4::Ip4Address();

    sourceMacAddress.read(stream);
    sourceIpAddress.read(stream);
    targetMacAddress.read(stream);
    targetIpAddress.read(stream);

    switch (arpHeader.getOperation()) {
        case ArpHeader::REQUEST:
            handleRequest(sourceMacAddress, sourceIpAddress, targetIpAddress, device);
            break;
        case ArpHeader::REPLY:
            handleReply(sourceMacAddress, sourceIpAddress, targetMacAddress, targetIpAddress);
            break;
        default:
            log.warn("Discarding packet because of unsupported operation type 0x%04x", arpHeader.getOperation());
    }
}

bool ArpModule::resolveAddress(const Ip4::Ip4Address &protocolAddress, MacAddress &hardwareAddress, Device::Network::NetworkDevice &device) {
    for (uint32_t i = 0; i < MAX_REQUEST_RETRIES; i++) {
        lock.acquire();
        if (hasHardwareAddress(protocolAddress)) {
            hardwareAddress = getHardwareAddress(protocolAddress);
            return lock.releaseAndReturn(true);
        }
        lock.release();

        auto ethernetHeader = Ethernet::EthernetHeader();
        ethernetHeader.setDestinationAddress(MacAddress::createBroadcastAddress());
        ethernetHeader.setSourceAddress(device.getMacAddress());
        ethernetHeader.setEtherType(Ethernet::EthernetHeader::ARP);

        auto arpHeader = ArpHeader();
        arpHeader.setOperation(ArpHeader::REQUEST);

        auto packet = Util::Stream::ByteArrayOutputStream();
        ethernetHeader.write(packet);
        arpHeader.write(packet);

        device.getMacAddress().write(packet);
        Ip4::Ip4Address("127.0.0.1").write(packet); // TODO: Replace with real source IP address
        MacAddress().write(packet);
        protocolAddress.write(packet);

        Ethernet::EthernetModule::finalizePacket(packet);
        device.sendPacket(packet.getBuffer(), packet.getSize());

        Util::Async::Thread::sleep(Util::Time::Timestamp(1, 0));
    }

    return false;
}

void ArpModule::setEntry(const Ip4::Ip4Address &protocolAddress, const MacAddress &hardwareAddress) {
    lock.acquire();

    for (auto &entry : arpCache) {
        if (entry.getProtocolAddress() == protocolAddress) {
            entry.setHardwareAddress(hardwareAddress);
            lock.release();
            return;
        }
    }

    arpCache.add({protocolAddress, hardwareAddress});
    lock.release();
}

MacAddress ArpModule::getHardwareAddress(const Ip4::Ip4Address &protocolAddress) {
    lock.acquire();

    for (const auto &entry : arpCache) {
        if (entry.getProtocolAddress() == protocolAddress) {
            return lock.releaseAndReturn(entry.getHardwareAddress());
        }
    }

    lock.release();
    Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "ARP: Protocol address not found!");
}

bool ArpModule::hasHardwareAddress(const Ip4::Ip4Address &protocolAddress) {
    lock.acquire();

    for (const auto &entry : arpCache) {
        if (entry.getProtocolAddress() == protocolAddress) {
            return lock.releaseAndReturn(true);
        }
    }

    return lock.releaseAndReturn(false);
}

void ArpModule::handleRequest(const MacAddress &sourceHardwareAddress, const Ip4::Ip4Address &sourceProtocolAddress,
                              const Ip4::Ip4Address &targetProtocolAddress, Device::Network::NetworkDevice &device) {
    setEntry(sourceProtocolAddress, sourceHardwareAddress);

    lock.acquire();
    if (hasHardwareAddress(targetProtocolAddress)) {
        auto targetHardwareAddress = getHardwareAddress(targetProtocolAddress);
        lock.release();

        auto ethernetHeader = Ethernet::EthernetHeader();
        ethernetHeader.setDestinationAddress(targetHardwareAddress);
        ethernetHeader.setSourceAddress(device.getMacAddress());
        ethernetHeader.setEtherType(Ethernet::EthernetHeader::ARP);

        auto arpHeader = ArpHeader();
        arpHeader.setOperation(ArpHeader::REPLY);

        auto packet = Util::Stream::ByteArrayOutputStream();
        ethernetHeader.write(packet);
        arpHeader.write(packet);

        device.getMacAddress().write(packet);
        targetProtocolAddress.write(packet);
        sourceHardwareAddress.write(packet);
        sourceProtocolAddress.write(packet);

        Ethernet::EthernetModule::finalizePacket(packet);
        device.sendPacket(packet.getBuffer(), packet.getSize());
    } else {
        lock.release();
    }
}

void ArpModule::handleReply(const MacAddress &sourceHardwareAddress, const Ip4::Ip4Address &sourceProtocolAddress,
                            const MacAddress &targetHardwareAddress, const Ip4::Ip4Address &targetProtocolAddress) {
    setEntry(sourceProtocolAddress, sourceHardwareAddress);

    //Learn own addresses if not broadcast
    if (!targetHardwareAddress.isBroadcastAddress()) {
        setEntry(targetProtocolAddress, targetHardwareAddress);
    }
}

}