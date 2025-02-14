#include "KernelAudioDriver.h"
#include "../../../service/MemoryService.h"
#include "../../../service/UsbService.h"
#include "../../../service/Service.h"
#include "lib/util/io/stream/QueueInputStream.h"
#include "lib/util/io/stream/QueueOutputStream.h"
#include "lib/util/collection/ArrayBlockingQueue.h"
#include "../../interface/UsbRegistry.h"
#include "../../node/audio/AudioNode.h"
#include "../../../log/Log.h"
#include "lib/util/base/Exception.h"
#include "lib/util/base/Address.h"
#include "lib/util/usb/io_control/AudioControl.h"
#include "lib/util/time/Timestamp.h"

extern "C" {
#include "../../../../device/usb/driver/audio/AudioDeviceDriver.h"
#include "../../../../device/usb/include/UsbGeneral.h"
#include "../../../../device/usb/dev/UsbDevice.h"
#include "../../../../device/usb/driver/UsbDriver.h"
#include "../../../../device/usb/events/listeners/audio/AudioListener.h"
#include "../../../../device/usb/events/listeners/EventListener.h"
#include "../../../../device/usb/include/UsbControllerInclude.h"
#include "../../../../device/usb/events/event/Event.h"
#include "../../../../device/usb/events/event/audio/AudioSampleEvent.h"
}

Kernel::Usb::Driver::KernelAudioDriver::KernelAudioDriver(Util::String name)
    : Kernel::Usb::Driver::KernelUsbDriver(name) {}

int Kernel::Usb::Driver::KernelAudioDriver::initialize() {
  int dev_found = 0;
  Kernel::MemoryService &m =
      Kernel::Service::getService<Kernel::MemoryService>();
  Kernel::UsbService &u = Kernel::Service::getService<Kernel::UsbService>();

  UsbDevice_ID usbDevs[] = {USB_INTERFACE_INFO(AUDIO, AUDIO_CONTROL, 0xFF),
                            USB_INTERFACE_INFO(AUDIO, AUDIO_STREAMING, 0xFF),
                            {}};

  AudioDriver *audio_driver =
      (AudioDriver*)m.allocateKernelMemory(sizeof(AudioDriver), 0);
  __STRUCT_INIT__(audio_driver, new_audio_driver, new_audio_driver, 
    this->getName(), usbDevs);

  this->driver = audio_driver;
  dev_found = u.add_driver((UsbDriver *)audio_driver);
  __IF_RET_NEG__(__IS_NEG_ONE__(dev_found));

  AudioListener* audio_listener = 
    (AudioListener*)m.allocateKernelMemory(sizeof(AudioListener), 0);
  __STRUCT_INIT__(audio_listener, new_listener, new_audio_listener);
  int id = u.register_listener((EventListener*)audio_listener);
  
  __IF_RET_NEG__(id < 0);

  ((UsbDriver*)audio_driver)->listener_id = id;
  audio_driver->configure_audio_device(driver);
  return __RET_S__;
}

void Kernel::Usb::Driver::KernelAudioDriver::clear_buffer(AudioDev* audio_dev, 
  Interface* as_streaming){
  void* top_lay_buff = __STRUCT_CALL__(driver, get_top_layer_buffer, as_streaming);
  Util::ArrayBlockingQueue<uint8_t>* audioBuffer = (Util::ArrayBlockingQueue<uint8_t>*)top_lay_buff;
  audioBuffer->clear();
}

bool Kernel::Usb::Driver::KernelAudioDriver::open_audio_stream(AudioDev* audio_dev, 
  Interface* as_streaming){
  Kernel::UsbService &u = Kernel::Service::getService<Kernel::UsbService>();
  if(__IS_NEG_ONE__(__STRUCT_CALL__(audio_dev->usb_dev, 
    __is_class_specific_interface_set,as_streaming->active_interface))){
    Alternate_Interface* alt_itf = __STRUCT_CALL__(audio_dev->usb_dev, 
      __get_alternate_interface_by_setting, as_streaming, 1);
    __TYPE_CAST__(ASInterface*, as_interface, alt_itf->class_specific);
    __STRUCT_CALL__(audio_dev->usb_dev, request_switch_alternate_setting,
      as_streaming, 1);
    __IF_RET_SELF__(audio_dev->usb_dev->error_while_transfering, false);
    u.reset_transfer(as_interface->qh_id);
    return true;
  }
  return false;
}

bool Kernel::Usb::Driver::KernelAudioDriver::close_audio_stream(AudioDev* audio_dev, 
  Interface* as_streaming){
  __IF_RET_SELF__(__IS_NEG_ONE__(__STRUCT_CALL__(driver, 
    __has_zero_bandwidth_setting, audio_dev->usb_dev, as_streaming)), false);
  __IF_RET_SELF__((__STRUCT_CALL__(driver, 
    __is_zero_bandwidth_active, audio_dev->usb_dev, as_streaming) == 1), false);
  __STRUCT_CALL__(audio_dev->usb_dev, request_switch_alternate_setting,
    as_streaming, 0);
  if(audio_dev->usb_dev->error_while_transfering){
    return false;
  }
  clear_buffer(audio_dev, as_streaming);
  __STRUCT_CALL__(driver, __clear_low_level_buffers, audio_dev);
  return true;
}

bool Kernel::Usb::Driver::KernelAudioDriver::remove_transfer(AudioDev* audio_dev,
  ASInterface* as_interface){
  Kernel::UsbService &u = Kernel::Service::getService<Kernel::UsbService>();
  return u.remove_transfer(as_interface->qh_id);
}

bool Kernel::Usb::Driver::KernelAudioDriver::add_transfer(AudioDev* audio_dev, Interface* itf, 
  ASInterface* as_interface){
  Kernel::UsbService &u = Kernel::Service::getService<Kernel::UsbService>();
  Kernel::MemoryService &mem = Kernel::Service::getService<Kernel::MemoryService>();
  UsbDev *dev = audio_dev->usb_dev;
  uint8_t terminal_typ = as_interface->terminal_type;
  __IF_RET_SELF__(terminal_typ == OUTPUT_TERMINAL, false); // micro currently not supported
  uint16_t buffer_size = (driver->__get_1ms_size(driver, as_interface));
  uint8_t* buffer_first  = (uint8_t*)mem.mapIO(1, true);
  uint8_t* buffer_second = (uint8_t*)mem.mapIO(1, true);
  as_interface->active_buffer = 0x01;
  as_interface->buffer_first = buffer_first;
  as_interface->buffer_second = buffer_second;
  as_interface->buffer_size = buffer_size;
  
  unsigned pipe;
  uint8_t endpoint = dev->__endpoint_number(dev, dev->__get_first_endpoint(dev,
      itf->active_interface));
  if(terminal_typ == INPUT_TERMINAL) pipe = usb_sndisopipe(endpoint);
  else if(terminal_typ == OUTPUT_TERMINAL) pipe = usb_rcvisopipe(endpoint);
  uint32_t qh_id = u.submit_iso_transfer(itf,
    pipe, PRIORITY_8, 1, as_interface->buffer_first, as_interface->buffer_size, audio_dev->callback);
  /*uint32_t qh_id = u.submit_iso_transfer_ext(audio_driver->dev[minor].audio_streaming_interfaces[i],
    dev->__get_first_endpoint(dev, audio_dev.audio_streaming_interfaces[i]->active_interface),
    PRIORITY_8, 1, as_interface->buffer_first, as_interface->buffer_size,
    audio_driver->dev[minor].callback); */
  as_interface->qh_id = qh_id;
  driver->sync_streaming_interface(driver, dev, itf);

  return true;
}

int Kernel::Usb::Driver::KernelAudioDriver::submit(uint8_t minor) {
  int num = driver->dev[minor].audio_streaming_interfaces_num;
  for(int i = 0; i < num; i++){
    ASInterface* as_interface = driver->__convert_to_class_specific_as_interface(driver, 
      driver->dev[minor].audio_streaming_interfaces[i]);
    __IF_CONTINUE__(__IS_NEG_ONE__(driver->__is_freq_set(driver, as_interface)));
    add_transfer(driver->dev + minor, driver->dev[minor].audio_streaming_interfaces[i],
      as_interface);
  }
  
  return __RET_S__;
}

Util::Io::QueueOutputStream* Kernel::Usb::Driver::KernelAudioDriver::input_terminal_routine(
  Interface* interface, uint32_t frame_size){
  Util::ArrayBlockingQueue<uint8_t>* audioBuffer =
          new Util::ArrayBlockingQueue<uint8_t>(frame_size);
  Util::Io::QueueOutputStream* outputStream = new Util::Io::QueueOutputStream(*audioBuffer);
  driver_stream_write_register_buffer((UsbDriver*)driver, 
    interface, audioBuffer, &audio_output_event_callback);

  return outputStream;
}

Util::Io::QueueInputStream* Kernel::Usb::Driver::KernelAudioDriver::output_terminal_routine(
  Interface* interface, uint32_t frame_size){
  Util::ArrayBlockingQueue<uint8_t>* audioBuffer =
      new Util::ArrayBlockingQueue<uint8_t>(frame_size);
  Util::Io::QueueInputStream* inputStream = new Util::Io::QueueInputStream(*audioBuffer);
  driver_stream_write_register_buffer((UsbDriver*)driver,
    interface, audioBuffer, &audio_input_event_callback);

  return inputStream;
}

void* Kernel::Usb::Driver::KernelAudioDriver::terminal_routine(Interface* interface,
  uint8_t* type){
  ASInterface* as_interface = driver->__convert_to_class_specific_as_interface(
    driver, interface);
  uint32_t frame_size = driver->__get_frame_size(driver, as_interface);
  if(as_interface->terminal_type == INPUT_TERMINAL) {
    *type = INPUT_TERMINAL;
    return input_terminal_routine(interface, frame_size);
  }
  *type = OUTPUT_TERMINAL;
  return output_terminal_routine(interface, frame_size);
}

Kernel::Usb::AudioNode* Kernel::Usb::Driver::KernelAudioDriver::single_terminal_routine(
  Interface* terminal, Util::String name, uint8_t minor){
  Util::Io::QueueInputStream* input = 0;
  Util::Io::QueueOutputStream* output = 0;
  uint8_t type;
  void* stream = terminal_routine(terminal, &type);
  assign_stream(&input, &output, stream, type);
  if(input == 0) return new Kernel::Usb::AudioNode(name, this, output, minor);
  return new Kernel::Usb::AudioNode(name, this, input, minor);
}

void Kernel::Usb::Driver::KernelAudioDriver::assign_stream(Util::Io::QueueInputStream** input,
  Util::Io::QueueOutputStream** output, void* stream, uint8_t type){
  if(type == INPUT_TERMINAL){
    *output = (Util::Io::QueueOutputStream*)stream;
  }
  else if(type == OUTPUT_TERMINAL){
    *input = (Util::Io::QueueInputStream*)stream;
  }
}

Kernel::Usb::AudioNode* Kernel::Usb::Driver::KernelAudioDriver::mult_terminal_routine(
  Interface** audio_streaming_itf, Util::String name, uint8_t minor){
  Interface* terminal_A = audio_streaming_itf[0];
  Interface* terminal_B = audio_streaming_itf[1];
  Util::Io::QueueInputStream*  input  = 0;
  Util::Io::QueueOutputStream* output = 0;
  uint8_t type_A, type_B;
  void* stream_A = terminal_routine(terminal_A, &type_A);
  void* stream_B = terminal_routine(terminal_B, &type_B);
  assign_stream(&input, &output, stream_A, type_A);
  assign_stream(&input, &output, stream_B, type_B);
  
  return new Kernel::Usb::AudioNode(name, this, input, output, minor);
}

void Kernel::Usb::Driver::KernelAudioDriver::create_usb_dev_node() {
  AudioDriver* driver = this->driver;
  uint8_t current_audio_node_num = 0;
  __FOR_RANGE__(i, int, 0, MAX_DEVICES_PER_USB_DRIVER){
    __IF_CONTINUE__(driver->audio_map[i] == 0);
    if(__NOT_NEG_ONE__(this->submit(i))) {
      AudioNode* audio_node;      
      uint8_t streaming_interface_count = driver->dev[i].audio_streaming_interfaces_num;
      __IF_CONTINUE__(__IS_ZERO__(streaming_interface_count));
      Util::String node_name = Util::String::format("audio%u", 
        current_audio_node_num++);
      if(streaming_interface_count == 0x01){
        audio_node = single_terminal_routine(driver->dev[i].audio_streaming_interfaces[0],
          node_name, i);
      }
      else if(streaming_interface_count == 0x02){
        audio_node = mult_terminal_routine(driver->dev[i].audio_streaming_interfaces,
          node_name, i);
      }
      // switch to zero band width setting when available
      for(int k = 0; k < streaming_interface_count; k++){
        close_audio_stream(driver->dev + i, driver->dev[i].audio_streaming_interfaces[k]);
      }
      audio_node->add_file_node();
      LOG_INFO("Succesful added audio node : minor %u -> associated "
                             "with 0x%x (%s driver)...",
                             audio_node->get_minor(), this, this->getName());
    }
  }
}

void audio_output_event_callback(uint8_t* map_io_buffer, uint16_t len, void* buffer){
  Util::ArrayBlockingQueue<uint8_t>* audio_buffer = ((Util::ArrayBlockingQueue<uint8_t>*)buffer);
  Util::Address address = Util::Address<uint32_t>(map_io_buffer);
  if(audio_buffer->isEmpty()){
    address.setRange(0, len);
    return;
  }
  __FOR_RANGE__(i, int, 0, len){
    __IF_ELSE__(audio_buffer->isEmpty(), map_io_buffer[i]= 0, 
      map_io_buffer[i]=audio_buffer->poll());
  }
}

void audio_input_event_callback(uint8_t* map_io_buffer, uint16_t len, void* buffer){
  Util::ArrayBlockingQueue<uint8_t>* audio_buffer = (Util::ArrayBlockingQueue<uint8_t>*)buffer;
  __FOR_RANGE__(i, int, 0, len){
    audio_buffer->add(map_io_buffer[i]);
  }
}

Interface* Kernel::Usb::Driver::KernelAudioDriver::request_common_routine(
    const Util::Array<uint32_t>& parameters, AudioDev* audio_dev){
  if(parameters[0] != OUT_TERMINAL_SELECT && parameters[0] != IN_TERMINAL_SELECT){
    Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT);
  }
  Interface* as_interface;
  if(parameters[0] == OUT_TERMINAL_SELECT){
    as_interface = driver->__get_as_interface_by_terminal(driver, audio_dev,
      OUTPUT_TERMINAL);
  }
  else if(parameters[0] == IN_TERMINAL_SELECT){
    as_interface = driver->__get_as_interface_by_terminal(driver, audio_dev,
      INPUT_TERMINAL); 
  }
  return as_interface;
}

bool Kernel::Usb::Driver::KernelAudioDriver::direct_requests(const Util::Array<uint32_t>& parameters,
  AudioDev* audio_dev, int8_t (*direct_req)(AudioDriver* driver, AudioDev* audio_dev, 
    Interface* as_interface)){
  if(parameters.length() != 0x01){
    Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, 
      "expecting uint32_t [IN or OUT]");
  }
  Interface* as_interface = request_common_routine(parameters, audio_dev);
  if(__IS_NULL__(as_interface)) return false;
  if(__IS_NEG_ONE__(direct_req(driver, audio_dev, as_interface))) return false;
  return true;
}

bool Kernel::Usb::Driver::KernelAudioDriver::set_sound_requests(const Util::Array<uint32_t>& parameters,
  AudioDev* audio_dev, int8_t (*set_call)(AudioDriver* driver, int16_t wVolume, AudioDev* audio_dev, 
    Interface* as_interface)){
  if(parameters.length() != 0x02){
    Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, 
      "expecting uint32_t [IN or OUT] and uint32_t [target buffer]");
  }
  if(parameters[0] != OUT_TERMINAL_SELECT && parameters[0] != IN_TERMINAL_SELECT){
    Util::Exception::throwException(Util::Exception::INVALID_ARGUMENT);
  }
  Interface* as_interface = request_common_routine(parameters, audio_dev);
  if(__IS_NULL__(as_interface)) return false;
  int16_t sound_value = (int16_t)parameters[1];
  return __IF_EXT__(__IS_NEG_ONE__(set_call(driver, sound_value, audio_dev, as_interface)), false, true);
}

bool Kernel::Usb::Driver::KernelAudioDriver::get_sound_requests(const Util::Array<uint32_t>& parameters,
  AudioDev* audio_dev, uint32_t (*get_sound_call)(AudioDriver* driver, AudioDev* audio_dev)){
  if(parameters.length() != 0x02){
    Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, 
      "expecting uint32_t [IN or OUT] and uint32_t [target buffer]");
  }
  Interface* as_interface = request_common_routine(parameters, audio_dev);
  if(__IS_NULL__(as_interface)) return false;
  uint32_t* __user_buff = (uint32_t*)(uintptr_t)parameters[1];
  *__user_buff = get_sound_call(driver, audio_dev);
  return true;
}

bool Kernel::Usb::Driver::KernelAudioDriver::get_requests(const Util::Array<uint32_t>& parameters,
  AudioDev* audio_dev, uint32_t (*get_call)(AudioDriver* driver, ASInterface* as_interface)){
  if(parameters.length() != 0x02){
    Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, 
      "expecting uint32_t [IN or OUT] and uint32_t [target buffer]");
  }
  Interface* as_interface = request_common_routine(parameters, audio_dev);
  if(__IS_NULL__(as_interface)) return false;
  
  uint32_t* __user_buff = (uint32_t*)(uintptr_t)parameters[1];
  *__user_buff = get_call(driver, driver->__get_class_specific_as_interface(driver,
    audio_dev->usb_dev, as_interface));
  return true;
}

bool Kernel::Usb::Driver::KernelAudioDriver::set_frequency(const Util::Array<uint32_t>& parameters,
  AudioDev* audio_dev){
  if(parameters.length() != 0x02){
    Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, 
      "expecting uint32_t [IN or OUT] and uint32_t [frequency]");
  }
  Interface* as_interface = request_common_routine(parameters, audio_dev);
  if(__IS_NULL__(as_interface)) return false;
  SampleFrequency sample_frequency = __SAMPLE_FREQUENCY_OF__(parameters[1]);
  int8_t status = __IF_EXT__(__IS_NEG_ONE__(driver->set_sampling_frequency(driver, audio_dev, as_interface,
    sample_frequency)), false, true);
  __IF_COND__(__IS_NEG_ONE__(status)) return false;
  ASInterface* as = __STRUCT_CALL__(driver, 
    __get_class_specific_as_interface, audio_dev->usb_dev, as_interface);
  if(!remove_transfer(audio_dev, as)) return false;
  return add_transfer(audio_dev, as_interface, as);
}

bool Kernel::Usb::Driver::KernelAudioDriver::audio_stream(const Util::Array<uint32_t>& parameters,
  AudioDev* audio_dev, uint32_t type){
  if(parameters.length() != 0x01){
    Util::Exception::throwException(Util::Exception::ILLEGAL_STATE, 
      "expecting uint32_t [IN or OUT]");
  }
  Interface* as_interface = request_common_routine(parameters, audio_dev);
  if(__IS_NULL__(as_interface)) return false;
  if(type == OPEN){
    return open_audio_stream(audio_dev, as_interface);
  }
  else if(type == CLOSE){
    return close_audio_stream(audio_dev, as_interface);
  }
  return false;
}

bool Kernel::Usb::Driver::KernelAudioDriver::control(uint32_t request, 
  const Util::Array<uint32_t>& parameters, uint8_t minor){
  AudioDev* audio_dev = driver->__get_audio_dev(driver, minor);
  __IF_COND__(__IS_NULL__(audio_dev)) return false;
  switch(request){
    case GET_FREQ : 
      return get_requests(parameters, audio_dev, driver->__get_sample_frequency);
    case GET_FRAME_SIZE : 
      return get_requests(parameters, audio_dev, driver->__get_frame_size);
    case GET_SUB_FRAME_SIZE :
      return get_requests(parameters, audio_dev, driver->__get_sub_frame_size);
    case GET_BIT_DEPTH :
      return get_requests(parameters, audio_dev, driver->__get_bit_depth);
    case GET_TOTAL_FREQ :
      return get_requests(parameters, audio_dev, driver->__get_total_supported_frequencies);
    case GET_NUM_CHANNELS :
      return get_requests(parameters, audio_dev, driver->__get_num_channels);
    case GET_SOUND :
      return get_sound_requests(parameters, audio_dev, driver->__get_volume);
    case GET_SOUND_MAX :
      return get_sound_requests(parameters, audio_dev, driver->__get_max_volume);
    case GET_SOUND_MIN :
      return get_sound_requests(parameters, audio_dev, driver->__get_min_volume);
    /*case SET_FREQ :
      // strange bug, when using this control 
      return set_frequency(parameters, audio_dev); */
    case SET_SOUND :
      return set_sound_requests(parameters, audio_dev, driver->set_sound_value);
    case SET_SOUND_MAX :
      return set_sound_requests(parameters, audio_dev, driver->set_max_sound_value);
    case SET_SOUND_MIN : 
      return set_sound_requests(parameters, audio_dev, driver->set_min_sound_value);
    case MUTE :
      return direct_requests(parameters, audio_dev, driver->mute);
    case UNMUTE :
      return direct_requests(parameters, audio_dev, driver->unmute);
    case OPEN :
      return audio_stream(parameters, audio_dev, OPEN);
    case CLOSE :
      return audio_stream(parameters, audio_dev, CLOSE);
    default: return false;
  }

  return false;
}