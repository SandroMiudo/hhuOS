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

#include "lib/interface.h"
#include "lib/util/system/System.h"
#include "lib/util/memory/HeapMemoryManager.h"
#include "lib/util/memory/Constants.h"

void *allocateMemory(uint32_t size, uint32_t alignment) {
    auto *manager = reinterpret_cast<Util::Memory::HeapMemoryManager*>(Util::Memory::USER_SPACE_MEMORY_MANAGER_ADDRESS);
    return manager->allocateMemory(size, alignment);
}

void* reallocateMemory(void *pointer, uint32_t size, uint32_t alignment) {
    auto *manager = reinterpret_cast<Util::Memory::HeapMemoryManager*>(Util::Memory::USER_SPACE_MEMORY_MANAGER_ADDRESS);
    return manager->reallocateMemory(pointer, size, alignment);
}

void freeMemory(void *pointer, uint32_t alignment) {
    auto *manager = reinterpret_cast<Util::Memory::HeapMemoryManager*>(Util::Memory::USER_SPACE_MEMORY_MANAGER_ADDRESS);
    manager->freeMemory(pointer, alignment);
}

bool isSystemInitialized() {
    return true;
}

void* mapIO(uint32_t physicalAddress, uint32_t size) {
    void *mappedAddress;
    Util::System::call(Util::System::MAP_IO, 3, physicalAddress, size, &mappedAddress);
    return mappedAddress;
}

void unmap(uint32_t virtualStartAddress, uint32_t virtualEndAddress, uint32_t breakCount) {
    Util::System::call(Util::System::UNMAP, 3, virtualStartAddress, virtualEndAddress, breakCount);
}

bool createFile(const Util::Memory::String &path, Util::File::Type type) {
    return Util::System::call(Util::System::CREATE_FILE, 2, static_cast<const char*>(path), type) == Util::System::OK;
}

bool deleteFile(const Util::Memory::String &path) {
    return Util::System::call(Util::System::DELETE_FILE, 1, static_cast<const char*>(path)) == Util::System::OK;
}

int32_t openFile(const Util::Memory::String &path) {
    int32_t fileDescriptor;
    auto result = Util::System::call(Util::System::OPEN_FILE, 2, static_cast<const char*>(path), &fileDescriptor);
    return result == Util::System::OK ? fileDescriptor : -1;
}

void closeFile(int32_t fileDescriptor) {
    Util::System::call(Util::System::CLOSE_FILE, 1, fileDescriptor);
}

Util::File::Type getFileType(int32_t fileDescriptor) {
    Util::File::Type type;
    Util::System::call(Util::System::FILE_TYPE, 2, fileDescriptor, &type);
    return type;
}

uint32_t getFileLength(int32_t fileDescriptor) {
    uint64_t length;
    Util::System::call(Util::System::FILE_LENGTH, 2, fileDescriptor, &length);
    return length;
}

Util::Data::Array<Util::Memory::String> getFileChildren(int32_t fileDescriptor) {
    char **children;
    uint32_t count;
    Util::System::call(Util::System::FILE_CHILDREN, 3, fileDescriptor, &children, &count);

    auto ret = Util::Data::Array<Util::Memory::String>(count);
    for (uint32_t i = 0; i < count; i++) {
        ret[i] = children[i];
        delete children[i];
    }

    delete children;
    return ret;
}

uint64_t readFile(int32_t fileDescriptor, uint8_t *targetBuffer, uint64_t pos, uint64_t length) {
    uint64_t read;
    Util::System::call(Util::System::READ_FILE, 5, fileDescriptor, targetBuffer, pos, length, &read);
    return read;
}

uint64_t writeFile(int32_t fileDescriptor, const uint8_t *sourceBuffer, uint64_t pos, uint64_t length) {
    uint64_t written;
    Util::System::call(Util::System::WRITE_FILE, 5, fileDescriptor, sourceBuffer, pos, length, &written);
    return written;
}

bool changeDirectory(const Util::Memory::String &path) {
    auto result = Util::System::call(Util::System::CHANGE_DIRECTORY, 1, static_cast<const char*>(path));
    return result == Util::System::OK;
}

Util::File::File getCurrentWorkingDirectory() {
    char *path;
    Util::System::call(Util::System::GET_CURRENT_WORKING_DIRECTORY, 1, &path);
    return Util::File::File(path);
}

Util::Async::Process executeBinary(const Util::File::File &binaryFile, const Util::File::File &outputFile, const Util::Memory::String &command, const Util::Data::Array<Util::Memory::String> &arguments) {
    uint32_t processId;
    Util::System::call(Util::System::EXECUTE_BINARY, 5, &binaryFile, &outputFile, &command, &arguments, &processId);
    return Util::Async::Process(processId);
}

Util::Async::Process getCurrentProcess() {
    uint32_t processId;
    Util::System::call(Util::System::GET_CURRENT_PROCESS, 1, &processId);
    return Util::Async::Process(processId);
}

Util::Async::Thread getCurrentThread() {
    uint32_t threadId;
    Util::System::call(Util::System::GET_CURRENT_PROCESS, 1, &threadId);
    return Util::Async::Thread(threadId);
}

void joinProcess(uint32_t id) {
    Util::System::call(Util::System::JOIN_PROCESS, 1, id);
}

void sleep(const Util::Time::Timestamp &time) {
    Util::System::call(Util::System::SLEEP, 1, &time);
}

Util::Time::Timestamp getSystemTime() {
    Util::Time::Timestamp systemTime;
    Util::System::call(Util::System::GET_SYSTEM_TIME, 1, &systemTime);
    return systemTime;
}

Util::Time::Date getCurrentDate() {
    Util::Time::Date date;
    Util::System::call(Util::System::GET_CURRENT_DATE, 1, &date);
    return date;
}

void setDate(const Util::Time::Date &date) {
    Util::System::call(Util::System::SET_DATE, 1, &date);
}

bool shutdown(Util::Machine::ShutdownType type) {
    auto result = Util::System::call(Util::System::SHUTDOWN, 1, type);
    if (result == Util::System::OK) {
        Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, "Shutdown system call returned successful!");
    }

    // If this code is reached, the shutdown was not successful
    return false;
}

void throwError(Util::Exception::Error error, const char *message) {
    Util::System::out << Util::Exception::getExceptionName(error) << ": " << message << Util::Stream::PrintWriter::endl << Util::Stream::PrintWriter::flush;
    Util::System::call(Util::System::EXIT_PROCESS, 1, -1);
    while (true) {}
}