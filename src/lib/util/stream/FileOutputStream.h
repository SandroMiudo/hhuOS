/*
 * Copyright (C) 2018-2021 Heinrich-Heine-Universitaet Duesseldorf,
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

#ifndef HHUOS_FILEOUTPUTSTREAM_H
#define HHUOS_FILEOUTPUTSTREAM_H

#include <lib/util/file/File.h>
#include "OutputStream.h"

namespace Util::Stream {

class FileOutputStream : public OutputStream {

public:

    explicit FileOutputStream(const File::File &file);

    FileOutputStream(const Memory::String &path);

    FileOutputStream(const FileOutputStream &copy) = delete;

    FileOutputStream &operator=(const FileOutputStream &copy) = delete;

    ~FileOutputStream() override;

    void write(uint8_t c) override;

    void write(const uint8_t *sourceBuffer, uint32_t offset, uint32_t length) override;

private:

    uint32_t pos = 0;
    File::Node *node;

};

}

#endif
