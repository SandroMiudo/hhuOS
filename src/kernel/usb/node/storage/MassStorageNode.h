#ifndef MASS_STORAGE_NODE__INCLUDE
#define MASS_STORAGE_NODE__INCLUDE

#include <cstdint>
#include "filesystem/memory/MemoryNode.h"
#include "../../driver/storage/KernelMassStorageDriver.h"
#include "lib/util/collection/Array.h"
#include "lib/util/base/String.h"
#include "../UsbMemoryNode.h"

namespace Kernel::Usb{

class MassStorageNode : public UsbMemoryNode{

public:
    explicit MassStorageNode(Kernel::Usb::Driver::KernelMassStorageDriver* msd_kernel_driver, uint8_t minor, Util::String node_name);

    uint64_t readData(uint8_t *targetBuffer, uint64_t start_lba, uint64_t blocks) override;

    uint64_t writeData(const uint8_t *sourceBuffer, uint64_t start_lba, uint64_t blocks) override;

    bool control(uint32_t request, const Util::Array<uint32_t>& parameters) override;

    int add_file_node() override;

private:

Driver::KernelMassStorageDriver* driver;

};

};

#endif