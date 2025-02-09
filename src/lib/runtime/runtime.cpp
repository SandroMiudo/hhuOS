#include <cstdint>

#include "lib/util/base/operators.h"
#include "lib/util/base/Constants.h"
#include "lib/util/base/FreeListMemoryManager.h"
#include "lib/util/base/System.h"

// Export functions
extern "C" {
void initMemoryManager(uint8_t *startAddress);
void _exit(int32_t);
}

void initMemoryManager(uint8_t *startAddress) {
    auto *memoryManager = new (&Util::System::getAddressSpaceHeader().memoryManager) Util::FreeListMemoryManager();
    memoryManager->initialize(startAddress, reinterpret_cast<uint8_t*>(Util::MAIN_STACK_START_ADDRESS - 1));
}

void _exit(int32_t exitCode) {
    Util::System::call(Util::System::EXIT_PROCESS, 1, exitCode);
}