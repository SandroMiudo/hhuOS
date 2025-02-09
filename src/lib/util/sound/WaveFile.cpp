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

#include "WaveFile.h"

#include "lib/util/base/Exception.h"

namespace Util {
namespace Io {
class File;
}  // namespace Io
}  // namespace Util

namespace Util::Sound {

WaveFile::WaveFile(const Io::File &file) : Io::FilterInputStream(stream), stream(file) {
    auto readBytes = read(reinterpret_cast<uint8_t*>(&riffChunk), 0, sizeof(RiffChunk));
    if (readBytes <= 0) {
        Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT, "WaveFile: No 'data' chunk found!");
    }
    String signature = String(riffChunk.dataChunk.dataSignature);
    if(signature != chunkIDData){
        uint32_t org_size = riffChunk.dataChunk.chunkSize + sizeof(DataChunk);
        uint8_t* buffer = new uint8_t[org_size];
        read(buffer, 0, org_size);
        riffChunk.dataChunk = *((DataChunk*)(buffer + riffChunk.dataChunk.chunkSize));
    }
}

uint32_t WaveFile::getDataSize() const {
    return riffChunk.dataChunk.chunkSize;
}

WaveFile::AudioFormat WaveFile::getAudioFormat() const {
    return riffChunk.formatChunk.audioFormat;
}

uint16_t WaveFile::getNumChannels() const {
    return riffChunk.formatChunk.numChannels;
}

uint32_t WaveFile::getSamplesPerSecond() const {
    return riffChunk.formatChunk.samplesPerSecond;
}

uint32_t WaveFile::getBytesPerSecond() const {
    return riffChunk.formatChunk.bytesPerSecond;
}

uint16_t WaveFile::getBitsPerSample() const {
    return riffChunk.formatChunk.bitsPerSample;
}

uint16_t WaveFile::getFrameSize() const {
    return riffChunk.formatChunk.frameSize;
}

uint32_t WaveFile::getSampleCount() const {
    return riffChunk.dataChunk.chunkSize / riffChunk.formatChunk.frameSize;
}

}