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

#include "lib/util/file/File.h"
#include "Filesystem.h"

namespace Filesystem {

bool Filesystem::mountVirtualDriver(const Util::Memory::String &targetPath, VirtualDriver *driver) {
    Util::Memory::String parsedPath = Util::File::File::getCanonicalPath(targetPath) + Util::File::File::SEPARATOR;
    Node *targetNode = getNode(parsedPath);

    if (targetNode == nullptr) {
        if (mountPoints.size() != 0) {
            return false;
        }
    }

    delete targetNode;
    lock.acquire();

    if (mountPoints.containsKey(parsedPath)) {
        return lock.releaseAndReturn(false);
    }

    mountPoints.put(parsedPath, driver);
    return lock.releaseAndReturn(true);
}

Memory::MemoryDriver &Filesystem::getVirtualDriver(const Util::Memory::String &path) {
    Util::Memory::String parsedPath = Util::File::File::getCanonicalPath(path) + Util::File::File::SEPARATOR;
    auto *driver = mountPoints.get(parsedPath);

    return *reinterpret_cast<Memory::MemoryDriver*>(driver);
}

bool Filesystem::unmount(const Util::Memory::String &path) {
    Util::Memory::String parsedPath = Util::File::File::getCanonicalPath(path) + Util::File::File::SEPARATOR;
    Node *targetNode = getNode(parsedPath);

    if (targetNode == nullptr) {
        if (path != "/") {
            return false;
        }
    }

    delete targetNode;
    lock.acquire();

    for(const Util::Memory::String &key : mountPoints.keySet()) {
        if(key.beginsWith(parsedPath)) {
            if(key != parsedPath) {
                return lock.releaseAndReturn(false);
            }
        }
    }

    if(mountPoints.containsKey(parsedPath)) {
        delete mountPoints.remove(parsedPath);
        return lock.releaseAndReturn(true);
    }

    return lock.releaseAndReturn(false);
}

Node* Filesystem::getNode(const Util::Memory::String &path) {
    Util::Memory::String parsedPath = Util::File::File::getCanonicalPath(path);
    lock.acquire();

    Driver *driver = getMountedDriver(parsedPath);
    if (driver == nullptr) {
        return lock.releaseAndReturn(nullptr);
    }

    Node *ret = driver->getNode(parsedPath);
    return lock.releaseAndReturn(ret);
}

bool Filesystem::createFile(const Util::Memory::String &path) {
    Util::Memory::String parsedPath = Util::File::File::getCanonicalPath(path);
    lock.acquire();

    Driver *driver = getMountedDriver(parsedPath);
    if (driver == nullptr) {
        return lock.releaseAndReturn(false);
    }

    bool ret = driver->createNode(parsedPath, Util::File::REGULAR);
    return lock.releaseAndReturn(ret);
}

bool Filesystem::createDirectory(const Util::Memory::String &path) {
    Util::Memory::String parsedPath = Util::File::File::getCanonicalPath(path);
    lock.acquire();

    Driver *driver = getMountedDriver(parsedPath);
    if (driver == nullptr) {
        return lock.releaseAndReturn(false);
    }

    bool ret = driver->createNode(parsedPath, Util::File::DIRECTORY);
    return lock.releaseAndReturn(ret);
}

bool Filesystem::deleteFile(const Util::Memory::String &path) {
    Util::Memory::String parsedPath = Util::File::File::getCanonicalPath(path);
    lock.acquire();

    for (const Util::Memory::String &key : mountPoints.keySet()) {
        if (key.beginsWith(parsedPath)) {
            lock.release();
            return false;
        }
    }

    Driver *driver = getMountedDriver(parsedPath);
    if (driver == nullptr) {
        return lock.releaseAndReturn(false);
    }

    bool ret = driver->deleteNode(parsedPath);
    return lock.releaseAndReturn<bool>(ret);
}

Driver* Filesystem::getMountedDriver(Util::Memory::String &path) {
    if (!path.endsWith(Util::File::File::SEPARATOR)) {
        path += Util::File::File::SEPARATOR;
    }

    Util::Memory::String ret;

    for (const Util::Memory::String &currentString: mountPoints.keySet()) {
        if (path.beginsWith(currentString)) {
            if (currentString.length() > ret.length()) {
                ret = currentString;
            }
        }
    }

    if (ret.isEmpty()) {
        return nullptr;
    }

    path = path.substring(ret.length(), path.length() - 1);
    return mountPoints.get(ret);
}

}