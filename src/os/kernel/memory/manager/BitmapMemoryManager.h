/*
* Copyright (C) 2018 Burak Akguel, Christian Gesse, Fabian Ruhland, Filip Krakowski, Michael Schoettner
* Heinrich-Heine University
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

#ifndef __BITMAPMEMORYMANAGER_H__
#define __BITMAPMEMORYMANAGER_H__

#include "MemoryManager.h"
#include <lib/String.h>

#define DEBUG_BMM 0

/**
 * BitmapMemoryManager - manages a given area of memory in 4kb blocks using
 * a bitmap mechanism.
 *
 * @author Burak Akguel, Christian Gesse, Filip Krakowski, Fabian Ruhland, Michael Schoettner
 * @date 2018
 */
class BitmapMemoryManager : public MemoryManager{

public:

    enum ManagerType {
        PAGE_FRAME_ALLOCATOR,
        PAGING_AREA_MANAGER,
        MISC
    };

protected:
	// type of this memory manager
    ManagerType managerType = MISC;
    // name of this memory manager for debugging purposes
    String name;
    // bitmap-array for free page frames
    uint32_t *freeBitmap;
    // length of bitmap-array
    uint32_t freeBitmapLength;
    // size of a single memory block
    uint32_t blockSize;
    // index at which to start searching for free blocks
    uint32_t bmpSearchOffset;
    // shall allocated memory be zeroed? -> needed if memory is allocated for PageTables
    bool zeroMemory = false;

public:
    /**
     * Constructor
     *
     * @param memoryStartAddress Start address of the memory area to manage
     * @param memoryEndAddress End address of the memory area to manage
     * @param name Name of this memory manager for debugging output
     * @param zeroMemory Indicates if new allocated memory should be zeroed
     */
    BitmapMemoryManager(uint32_t memoryStartAddress, uint32_t memoryEndAddress, uint32_t blockSize,
                        String name, bool zeroMemory, bool doUnmap);

    /**
     * Allocate a 4kb block of memory
     *
     * @return Start address of the alloctated memory
     */
    void * alloc(uint32_t size) override;

    /**
     * Free a 4kb memory block
     *
     * @param ptr Address of the memory block to free
     */
    void free(void *ptr) override;

    /**
     * Dump bitmap for debugging reasons
     */
    void dump();
};


#endif
