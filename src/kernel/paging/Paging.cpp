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

#include "kernel/multiboot/Multiboot.h"
#include "Paging.h"
#include "MemoryLayout.h"

namespace Kernel {

void Paging::Table::clear() {
    Util::Address<uint32_t>(this).setRange(0, sizeof(Paging::Table));
}

Paging::Entry &Paging::Table::operator[](uint32_t index) {
    return entries[index];
}

bool Paging::Entry::isUnused() {
    return address == 0 && flags == 0;
}

void Paging::Entry::set(uint32_t address, uint32_t flags) {
    this->flags = flags;
    this->address = address >> 12;
}

uint32_t Paging::Entry::getAddress() {
    return address << 12;
}

uint32_t Paging::Entry::getFlags() {
    return flags;
}

void Paging::loadDirectory(const Paging::Table &directory) {
    asm volatile(
            "mov %0, %%cr3"
            : :
            "r"(&directory)
            :
            );
}

}