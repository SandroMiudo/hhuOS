#include "MouseNode.h"
#include "../UsbNode.h"
#include "filesystem/memory/StreamNode.h"
#include "filesystem/memory/MemoryDriver.h"
#include "kernel/system/System.h"
#include "../UsbRegistry.h"
#include "../../../device/usb/include/UsbControllerInclude.h"
#include "kernel/service/FilesystemService.h"
#include "../../../device/usb/events/event/Event.h"
#include "../../../device/usb/events/event/hid/MouseEvent.h"
#include "filesystem/memory/StreamNode.h"
#include "../../../lib/util/io/stream/QueueInputStream.h"
#include "../../../lib/util/collection/ArrayBlockingQueue.h"
#include "../../../lib/util/io/stream/FilterInputStream.h"
#include "../../../lib/util/base/String.h"

Util::ArrayBlockingQueue<uint8_t> mouseBuffer = Util::ArrayBlockingQueue<uint8_t>(BUFFER_SIZE);
Util::Io::QueueInputStream mouseinputStream = Util::Io::QueueInputStream(mouseBuffer);

Kernel::Usb::MouseNode::MouseNode(uint8_t minor) : Kernel::Usb::UsbNode(&mouse_node_callback, minor), Util::Io::FilterInputStream(mouseinputStream) {}

int Kernel::Usb::MouseNode::add_file_node(Util::String node_name){
  int s = interface_register_callback(MOUSE_LISTENER, this->get_callback());

  if (s == -1){
    delete this;
    return -1;
  }
  
  auto *streamNode = new Filesystem::Memory::StreamNode(node_name, this);
  auto &filesystem =
      Kernel::System::getService<Kernel::FilesystemService>().getFilesystem();
  auto &driver = filesystem.getVirtualDriver("/device");
  bool success = driver.addNode("/", streamNode);

  if (!success) {
    delete streamNode;
    return -1;
  }

  return 1;
}

void mouse_node_callback(void* e){

}