#include "KernelHubDriver.h"
#include "lib/util/base/String.h"
#include "../../../service/MemoryService.h"
#include "../../../service/UsbService.h"
#include "../../../service/Service.h"
#include "../KernelUsbDriver.h"

extern "C" {
#include "../../../../device/usb/driver/hub/HubDriver.h"
}

Kernel::Usb::Driver::KernelHubDriver::KernelHubDriver(Util::String name)
    : Kernel::Usb::Driver::KernelUsbDriver(name) {}

int Kernel::Usb::Driver::KernelHubDriver::initialize() {
  int dev_found = 0;
  Kernel::MemoryService &m =
      Kernel::Service::getService<Kernel::MemoryService>();
  Kernel::UsbService &u = Kernel::Service::getService<Kernel::UsbService>();

  UsbDevice_ID usbDevs[] = {USB_DEVICE_INFO(HUB, 0xFF, 0xFF),
                            USB_INTERFACE_INFO(HUB, 0xFF, 0xFF),
                            {}};

  HubDriver *hub_driver =
      (HubDriver *)m.allocateKernelMemory(sizeof(HubDriver), 0);
  __STRUCT_INIT__(hub_driver, new_hub_driver, new_hub_driver, this->getName(), usbDevs);

  this->driver = hub_driver;
  dev_found = u.add_driver((UsbDriver *)hub_driver);
  __IF_RET_NEG__(__IS_NEG_ONE__(dev_found));
  __IF_RET_NEG__(__IS_NEG_ONE__(__STRUCT_CALL__(hub_driver, configure_hub)));

  return __RET_S__;
}

int Kernel::Usb::Driver::KernelHubDriver::submit(uint8_t minor) { return -1; }

void Kernel::Usb::Driver::KernelHubDriver::create_usb_dev_node() {}
