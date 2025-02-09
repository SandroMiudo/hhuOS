#include "UsbService.h"
#include "../../device/usb/controller/UsbRunnable.h"
#include "../../filesystem/VirtualDriver.h"
#include "../../filesystem/memory/MemoryDirectoryNode.h"
#include "../../filesystem/memory/MemoryDriver.h"
#include "../../filesystem/memory/MemoryFileNode.h"
#include "../../filesystem/memory/MemoryNode.h"
#include "../../lib/util/async/Thread.h"
#include "../../lib/util/base/String.h"
#include "../../lib/util/io/file/File.h"
#include "../../lib/util/io/stream/FileOutputStream.h"
#include "../service/Service.h"
#include "FilesystemService.h"
#include "MemoryService.h"
#include "UsbService_C.h"

Kernel::UsbService::UsbService() {
  Kernel::MemoryService &m =
      Kernel::Service::getService<Kernel::MemoryService>();
  usb_service_c =
      (UsbService_C *)m.allocateKernelMemory(sizeof(UsbService_C), 0);

  __STRUCT_INIT__(usb_service_c, new_service, new_usb_service);
}

int Kernel::UsbService::add_driver(UsbDriver *driver) {
  return __STRUCT_CALL__(usb_service_c, add_driver_c, driver); 
}

int Kernel::UsbService::remove_driver(UsbDriver *driver) {
  return __STRUCT_CALL__(usb_service_c, remove_driver_c, driver);
}

void Kernel::UsbService::submit_bulk_transfer(Interface *interface,
                                              unsigned int pipe, uint8_t prio,
                                              void *data, unsigned int len,
                                              callback_function callback) {
  __STRUCT_CALL__(usb_service_c, submit_bulk_transfer_c, interface, pipe, prio, data,
    len, callback);
}

void Kernel::UsbService::submit_interrupt_transfer(
    Interface *interface, unsigned int pipe, uint8_t prio, uint16_t interval,
    void *data, unsigned int len, callback_function callback) {
  __STRUCT_CALL__(usb_service_c, submit_interrupt_transfer_c, interface, pipe,
    prio, interval, data, len, callback);
}

uint32_t Kernel::UsbService::submit_iso_transfer(Interface* interface, unsigned int pipe, uint8_t prio, 
    uint16_t interval, void* data, unsigned int len, callback_function callback){
  return __STRUCT_CALL__(usb_service_c, submit_iso_transfer_c, interface, pipe, prio,
    interval, data, len, callback);
}

uint32_t Kernel::UsbService::submit_iso_transfer_ext(Interface* interface, Endpoint* endpoint, uint8_t prio, 
              uint16_t interval, void* data, unsigned int len, callback_function callback){
    return __STRUCT_CALL__(usb_service_c, submit_iso_ext_transfer_c,
      interface, endpoint, prio, interval, data, len, callback);
}

void Kernel::UsbService::submit_control_transfer(Interface *interface,
                                                 unsigned int pipe,
                                                 uint8_t prio, void *data,
                                                 uint8_t *setup,
                                                 callback_function callback) {
  __STRUCT_CALL__(usb_service_c, submit_control_transfer_c, interface, pipe,
    prio, data, setup, callback);
}

int Kernel::UsbService::remove_transfer(uint32_t transfer_id){
  return __STRUCT_CALL__(usb_service_c, remove_transfer_c, transfer_id);
}

int Kernel::UsbService::reset_transfer(uint32_t transfer_id){
  return __STRUCT_CALL__(usb_service_c, reset_transfer_c, transfer_id);
}

int Kernel::UsbService::register_callback(uint16_t register_type,
                                          event_callback event_c, void* buffer) {
  return usb_service_c->register_callback_c(usb_service_c, register_type,
                                            event_c, buffer);
}

int Kernel::UsbService::deregister_callback(uint16_t register_type,
                                            event_callback event_c) {
  return usb_service_c->deregister_callback_c(usb_service_c, register_type,
                                              event_c);
}

int Kernel::UsbService::register_listener(EventListener *listener) {
  return usb_service_c->register_listener_c(usb_service_c, listener);
}

int Kernel::UsbService::deregister_listener(int id) {
  return usb_service_c->deregister_listener_c(usb_service_c, id);
}

// fix : some values are somehow not set correct -> maybe not set at enumeration
// time ?
void Kernel::UsbService::create_usb_fs() {
  Kernel::FilesystemService &fs =
      Kernel::Service::getService<Kernel::FilesystemService>();
  list_element *l_e_controller = usb_service_c->head.l_e;

  Filesystem::Memory::MemoryDriver *m_driver =
      new Filesystem::Memory::MemoryDriver();

  Util::String temp = "/system/usbfs";
  unsigned int con_n = 0;
  fs.createDirectory(temp);
  fs.getFilesystem().mountVirtualDriver(temp, m_driver);

  while (l_e_controller != (void *)0) {
    unsigned int dev_n = 0;
    unsigned int driver_n = 0;
    UsbController *controller =
        usb_service_c->get_controller(usb_service_c, l_e_controller);

    temp = Util::String::format("controller%d", con_n++);
    Filesystem::Memory::MemoryDirectoryNode *controller_node =
        new Filesystem::Memory::MemoryDirectoryNode(temp);
    m_driver->addNode("/", controller_node);
    Filesystem::Memory::MemoryDirectoryNode *driver_node =
        new Filesystem::Memory::MemoryDirectoryNode("drivers");
    Filesystem::Memory::MemoryDirectoryNode *device_node =
        new Filesystem::Memory::MemoryDirectoryNode("devices");
    controller_node->addChild(driver_node);
    controller_node->addChild(device_node);

    list_element *l_e_dev = controller->head_dev.l_e;
    list_element *l_e_driver = controller->head_driver.l_e;

    while (l_e_dev != (void *)0) {
      UsbDev *dev = usb_service_c->get_dev(usb_service_c, l_e_dev);
      handle_fs_device(dev, dev_n, device_node);

      l_e_dev = l_e_dev->l_e;
      dev_n++;
    }

    while (l_e_driver != (void *)0) {
      UsbDriver *driver = usb_service_c->get_driver(usb_service_c, l_e_driver);
      handle_fs_driver(driver, driver_n, driver_node, controller);

      l_e_driver = l_e_driver->l_e;
      driver_n++;
    }

    l_e_controller = l_e_controller->l_e;
  }
}

void Kernel::UsbService::handle_fs_interface(
    UsbDev *dev, Filesystem::Memory::MemoryDirectoryNode *dev_node) {
  int num_interfaces = dev->active_config->config_desc.bNumInterfaces;
  Interface **interfaces = dev->active_config->interfaces;

  Util::String itf_temp;
  Util::String temp;

  for (int i = 0; i < num_interfaces; i++) {
    // config.interface
    Interface *itf = interfaces[i];
    uint8_t config_value = dev->active_config->config_desc.bConfigurationValue;
    uint8_t itf_value =
        itf->active_interface->alternate_interface_desc.bInterfaceNumber;
    Util::String i_dev = Util::String::format("%u.%u", config_value, itf_value);

    Filesystem::Memory::MemoryDirectoryNode *itf_node =
        new Filesystem::Memory::MemoryDirectoryNode(i_dev);

    dev_node->addChild(itf_node);

    temp = Util::String::format("%u", itf->active);
    write_to_node(itf_node, temp, "active");

    temp = Util::String("driver");
    if ((UsbDriver *)itf->driver == (void *)0) {
      itf_temp = "";
    } else
      itf_temp = Util::String(((UsbDriver *)itf->driver)->name);
    write_to_node(itf_node, itf_temp, temp);

    temp = Util::String("interface-description");
    itf_temp = Util::String(itf->interface_description);
    write_to_node(itf_node, itf_temp, temp);

    InterfaceDescriptor i_desc =
        itf->active_interface->alternate_interface_desc;

    temp = Util::String::format("%u", i_desc.bInterfaceNumber);
    write_to_node(itf_node, temp, "bInterfaceNumber");

    temp = Util::String::format("%u", i_desc.bAlternateSetting);
    write_to_node(itf_node, temp, "bAlternateSetting");

    temp = Util::String::format("%u", i_desc.bNumEndpoints);
    write_to_node(itf_node, temp, "bNumEndpoints");

    temp = Util::String::format("%u", i_desc.bInterfaceClass);
    write_to_node(itf_node, temp, "bInterfaceClass");

    temp = Util::String::format("%u", i_desc.bInterfaceSubClass);
    write_to_node(itf_node, temp, "bInterfaceSubClass");

    temp = Util::String::format("%u", i_desc.bInterfaceProtocol);
    write_to_node(itf_node, temp, "bInterfaceProtocol");
  }
}

void Kernel::UsbService::handle_fs_driver(
    UsbDriver *driver, uint8_t driver_n,
    Filesystem::Memory::MemoryDirectoryNode *driver_node,
    UsbController *controller) {
  Util::String driver_temp;
  list_element *l_e_dev;
  uint8_t dev_count = 0;
  Util::String temp = Util::String("name");
  driver_temp = Util::String(driver->name);

  Util::String c_driver = Util::String::format("driver%u", driver_n);
  Filesystem::Memory::MemoryDirectoryNode *dr_node =
      new Filesystem::Memory::MemoryDirectoryNode(c_driver);

  driver_node->addChild(dr_node);

  write_to_node(dr_node, driver_temp, temp);

  l_e_dev = driver->head.l_e;

  while (l_e_dev != (void *)0) {
    UsbDev *dev = usb_service_c->get_dev(usb_service_c, l_e_dev);

    Util::String dev_info = Util::String(dev->manufacturer);

    temp = Util::String::format("device%u", dev_count++);
    write_to_node(dr_node, dev_info, temp);

    l_e_dev = l_e_dev->l_e;
  }
}

void Kernel::UsbService::handle_fs_down_stream(
    UsbDev *dev, Filesystem::Memory::MemoryDirectoryNode *dev_node) {
  if (dev->max_down_stream == 0)
    return;

  for (int i = 0; i < dev->max_down_stream; i++) {
    Util::String usb_port = Util::String::format("usb_port%d", i);
    Filesystem::Memory::MemoryDirectoryNode *hub_node =
        new Filesystem::Memory::MemoryDirectoryNode(usb_port);
    dev_node->addChild(hub_node);
    if (i < dev->downstream_count) {
      handle_fs_device(dev->downstream_devs[i], i, hub_node);
    }
  }
}

void Kernel::UsbService::handle_fs_device(
    UsbDev *dev, uint8_t dev_n,
    Filesystem::Memory::MemoryDirectoryNode *device_node) {
  Util::String dev_temp;
  Util::String temp;

  Filesystem::Memory::MemoryDirectoryNode *dev_node =
      new Filesystem::Memory::MemoryDirectoryNode(
          Util::String::format("dev%u", dev_n));
  device_node->addChild(dev_node);

  temp = Util::String::format("%u", dev->level);
  write_to_node(dev_node, temp, "level");

  temp = Util::String::format("%u", dev->rootport);
  write_to_node(dev_node, temp, "root");

  temp = Util::String::format("%u", dev->dev_num);
  write_to_node(dev_node, temp, "dev_num");

  temp = Util::String::format("%u", dev->removable);
  write_to_node(dev_node, temp, "removable");

  temp = Util::String::format("%u", dev->speed);
  write_to_node(dev_node, temp, "speed");

  temp = Util::String::format("%u", dev->port);
  write_to_node(dev_node, temp, "port");

  temp = Util::String::format("%u", dev->max_packet_size);
  write_to_node(dev_node, temp, "max-packet-size");

  temp = Util::String::format("0x%u", dev->lang_id);
  write_to_node(dev_node, temp, "lang_id");

  temp = Util::String("manufacturer");
  dev_temp = Util::String(dev->manufacturer);
  write_to_node(dev_node, dev_temp, temp);

  temp = Util::String("product");
  dev_temp = Util::String(dev->product);
  write_to_node(dev_node, dev_temp, temp);

  temp = Util::String("serial-number");
  dev_temp = Util::String(dev->serial_number);
  write_to_node(dev_node, dev_temp, temp);

  DeviceDescriptor d_desc = dev->device_desc;

  temp = Util::String::format("0x%u", d_desc.bcdUSB);
  write_to_node(dev_node, temp, "bcdUSB");

  temp = Util::String::format("%u", d_desc.bDeviceClass);
  write_to_node(dev_node, temp, "bDeviceClass");

  temp = Util::String::format("%u", d_desc.bDeviceSubClass);
  write_to_node(dev_node, temp, "bDeviceSubClass");

  temp = Util::String::format("%u", d_desc.bDeviceProtocol);
  write_to_node(dev_node, temp, "bDeviceProtocol");

  temp = Util::String::format("0x%u", d_desc.idVendor);
  write_to_node(dev_node, temp, "idVendor");

  temp = Util::String::format("0x%u", d_desc.idProduct);
  write_to_node(dev_node, temp, "idProduct");

  temp = Util::String::format("0x%u", d_desc.bcdDevice);
  write_to_node(dev_node, temp, "bcdDevice");

  temp = Util::String::format("%u", d_desc.bNumConfigurations);
  write_to_node(dev_node, temp, "bNumConfigurations");

  handle_fs_down_stream(dev, dev_node);

  handle_fs_interface(dev, dev_node);
}

void Kernel::UsbService::write_to_node(
    Filesystem::Memory::MemoryDirectoryNode *dev_node, Util::String &s,
    Util::String name) {
  Filesystem::Memory::MemoryFileNode *m_node =
      new Filesystem::Memory::MemoryFileNode(name);
  dev_node->addChild(m_node);

  Util::String i_s = s + "\n";

  m_node->writeData((uint8_t *)i_s, 0, i_s.length());
}