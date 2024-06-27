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

#include "NullNode.h"

#include "filesystem/memory/MemoryNode.h"

namespace Filesystem::Memory {

NullNode::NullNode(const Util::String &name) : MemoryNode(name) {}

Util::Io::File::Type NullNode::getType() {
    return Util::Io::File::CHARACTER;
}

uint64_t NullNode::writeData(const uint8_t *sourceBuffer, uint64_t pos, uint64_t numBytes) {
    return numBytes;
}

}