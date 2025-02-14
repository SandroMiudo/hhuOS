#ifndef KERNEL_USB_DRIVER__INCLUDE
#define KERNEL_USB_DRIVER__INCLUDE

#include "../../../lib/util/base/String.h"

namespace Kernel::Usb::Driver{

class KernelUsbDriver{

public:

    explicit KernelUsbDriver(Util::String name);

    virtual int initialize() = 0;

    virtual int submit(uint8_t minor) = 0;

    virtual void create_usb_dev_node() = 0;

    char* getName();

private:

    Util::String driver_name;

};

};

#endif