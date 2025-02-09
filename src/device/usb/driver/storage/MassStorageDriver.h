#ifndef MASS_STORAGE_DRIVER__INCLUDE
#define MASS_STORAGE_DRIVER__INCLUDE

#include "../UsbDriver.h"
#include "stdint.h"
#include "CommandInterface.h"
#include "../../interfaces/LoggerInterface.h"
#include "../../interfaces/MapInterface.h"

#define MAX_TRANSFER_BYTES 0x00008000
#define MAX_BLOCKS 64
#define ALLOWED_CAPACITES 31
#define READ_FORMAT_CAPACITIES_THRESHHOLD 5 // atleast 5 iterations 
#define MAX_VOLUMES 16
// #define TEST_ON ::: this macro is only used for testing purpose inside the kernel 
// don't activate this macro if you don't know what you are doing !!!

#define __INIT_MSD_DRIVER__(name, driver_name, entry) \
    __ENTRY__(name, configure_device) = &configure_device; \
    __ENTRY__(name, send_inquiry) = &send_inquiry; \
    __ENTRY__(name, send_read_format_capacities) = &send_read_format_capacities; \
    __ENTRY__(name, parse_format_capacities) = &parse_format_capacities; \
    __ENTRY__(name, get_data) = &get_data; \
    __ENTRY__(name, send_data) = &send_data; \
    __ENTRY__(name, build_command) = &build_command; \
    __ENTRY__(name, send_command) = &send_command; \
    __ENTRY__(name, retrieve_status) = &retrieve_status; \
    __ENTRY__(name, check_csw_status) = &check_csw_status; \
    __ENTRY__(name, command_helper) = &command_helper; \
    __ENTRY__(name, send_request_sense) = &send_request_sense; \
    __ENTRY__(name, send_read_capacity__32_bit) = &send_read_capacity__32_bit; \
    __ENTRY__(name, send_read_capacity__64_bit) = &send_read_capacity__64_bit; \
    __ENTRY__(name, send_read) = &send_read; \
    __ENTRY__(name, send_write) = &send_write; \
    __ENTRY__(name, get_drive_size) = &get_drive_size; \
    __ENTRY__(name, get_block_size) = &get_block_size; \
    __ENTRY__(name, get_block_num) = &get_block_num; \
    __ENTRY__(name, test_mass_storage_writes) = &test_mass_storage_writes; \
    __ENTRY__(name, init_sense_description) = &init_sense_description; \
    __ENTRY__(name, is_valid_volume) = &is_valid_volume; \
    __ENTRY__(name, get_inquiry_data) = &get_inquiry_data; \
    __ENTRY__(name, get_capacity_descpritor) = &get_capacity_descpritor; \
    __ENTRY__(name, get_sense_data) = &get_sense_data; \
    __ENTRY__(name, get_capacity) = &get_capacity; \
    __ENTRY__(name, set_dev_properites) = &set_dev_properites; \
    __ENTRY__(name, msd_match_routine) = &msd_match_routine; \
    __ENTRY__(name, get_free_cbw) = &get_free_cbw; \
    __ENTRY__(name, get_free_csw) = &get_free_csw; \
    __ENTRY__(name, free_cbw) = &free_cbw; \
    __ENTRY__(name, free_csw) = &free_csw; \
    __ENTRY__(name, init_msd_maps) = &init_msd_maps; \
    __ENTRY__(name, read_msd) = &read_msd; \
    __ENTRY__(name, write_msd) = &write_msd; \
    __ENTRY__(name, clear_msd_map) = &clear_msd_map; \
    __ENTRY__(name, init_io_msd) = &init_io_msd; \
    __ENTRY__(name, get_free_msd_dev) = &get_free_msd_dev; \
    __ENTRY__(name, free_msd_dev) = &free_msd_dev; \
    __ENTRY__(name, match_msd_dev) = &match_msd_dev; \
    __ENTRY__(name, get_msd_dev_by_minor) = &get_msd_dev_by_minor; \
    __ENTRY__(name, build_write_command) = &build_write_command; \
    __ENTRY__(name, build_read_command) = &build_read_command; \
    __ENTRY__(name, calc_t_len) = &calc_t_len; \
    __ENTRY__(name, valid_block_number) = &valid_block_number; \
    __ENTRY__(name, parse_request_sense) = &parse_request_sense; \
    __ENTRY__(name, get_capacity_count) = &get_capacity_count; \
    __ENTRY__(name, reset_bulk_only)    = &reset_bulk_only; \
    __ENTRY__(name, get_max_logic_unit_numbers) = &get_max_logic_unit_numbers; \
    \
    __SUPER__(name, probe) = &probe_mass_storage; \
    __SUPER__(name, disconnect) = &disconnect_mass_storage; \
    __SUPER__(name, new_usb_driver) = &new_usb_driver; \
    \
    __CALL_SUPER__(name->super, new_usb_driver, driver_name, entry)

struct MassStorageDev{
    UsbDev* usb_dev;
    unsigned int bulk_out_endpoint_addr;
    unsigned int bulk_in_endpoint_addr;
    void* buffer; // just store MAX_TRANSFER_BYTES max not complete drive
    unsigned int buffer_size;
    uint8_t priority;
    Interface* interface;
    UsbDriver* usb_driver;

    uint8_t success_transfer;

    struct MassStorageVolume* mass_storage_volumes;
    uint8_t volumes; // 0 = 1, 1 = 2 ... 15 = 16 are assuming consecutively

    /*void (*callback)(UsbDev* dev, uint32_t status, void* data);
    void (*callback_cbw)(UsbDev* dev, uint32_t status, void* cbw);
    void (*callback_csw)(UsbDev* dev, uint32_t status, void* csw); */
};

struct MassStorageVolume{
    void (*new_storage_volume)(struct MassStorageVolume* volume, uint8_t volume_num);
    uint8_t version;
    uint8_t volume_number;

    InquiryCommandData inquiry;

    CapacityDescriptor capacity_desc[ALLOWED_CAPACITES];
    unsigned int found_capacities;

    uint32_t volume_size;
    uint32_t block_size;
    uint32_t block_num;

    ReadCapacity__32_Bit rc_32_bit; // filter via version
    ReadCapacity__64_Bit rc_64_bit;
};

typedef void (*msd_callback)(uint8_t* buffer, uint32_t len);

struct MassStorageDriver{
    struct UsbDriver super;
    struct MassStorageDev dev[MAX_DEVICES_PER_USB_DRIVER];
    uint8_t msd_map[MAX_DEVICES_PER_USB_DRIVER];

    void (*new_mass_storage_driver)(struct MassStorageDriver* driver, char* name, UsbDevice_ID* entry);

    int (*configure_device)(struct MassStorageDriver *driver);
    int (*send_inquiry)(struct MassStorageDriver *driver, struct MassStorageDev* msd_dev, CommandBlockWrapper *cbw,
                    CommandStatusWrapper *csw, InquiryCommandData *inquiry_data,
                    uint8_t volume, RequestSense* rs);
    int (*send_read_format_capacities)(struct MassStorageDriver *driver, struct MassStorageDev* msd_dev, CommandBlockWrapper *cbw,
                                    CommandStatusWrapper* csw, CapacityListHeader* clh,
                                    uint8_t volume, RequestSense* rs);
    void (*parse_format_capacities)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CapacityListHeader* clh, uint8_t volume);
    int (*get_data)(struct MassStorageDriver *driver, struct MassStorageDev* msd_dev, void *data, unsigned int len, uint8_t flags, callback_function callback);                       
    int (*send_data)(struct MassStorageDriver *driver, struct MassStorageDev* msd_dev, void *data, unsigned int len, uint8_t flags, callback_function callback);
    void (*build_command)(struct MassStorageDriver *driver, CommandBlockWrapper *cbw,
                    uint32_t tag, uint32_t transfer_length, uint8_t flags,
                    uint8_t lun, uint8_t command_len);
    int (*send_command)(struct MassStorageDriver *driver, struct MassStorageDev* msd_dev, CommandBlockWrapper *cbw,
                    callback_function callback, uint8_t flags);  
    int (*retrieve_status)(struct MassStorageDriver *driver, struct MassStorageDev* msd_dev, CommandStatusWrapper *csw, uint8_t flags, callback_function callback);
    uint8_t (*check_csw_status)(struct MassStorageDriver *driver, CommandStatusWrapper *csw);
    void (*command_helper)(struct MassStorageDriver *driver, uint8_t *command, uint8_t c1,
                        uint8_t c2, uint8_t c3, uint8_t c4, uint8_t c5, uint8_t c6,
                        uint8_t c7, uint8_t c8, uint8_t c9, uint8_t c10,
                        uint8_t c11, uint8_t c12, uint8_t c13, uint8_t c14,
                        uint8_t c15, uint8_t c16);
    int (*send_request_sense)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CommandBlockWrapper* cbw, CommandStatusWrapper* csw,
                       RequestSense* rs, uint8_t volume);
    int (*send_read_capacity__32_bit)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CommandBlockWrapper* cbw, CommandStatusWrapper* csw,
                                ReadCapacity__32_Bit* rc, uint8_t volume, RequestSense* rs);
    int (*send_read_capacity__64_bit)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CommandBlockWrapper* cbw, CommandStatusWrapper* csw,
                                ReadCapacity__64_Bit* rc, uint8_t volume, RequestSense* rs);                                                                          
    int (*send_read)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CommandBlockWrapper* cbw, CommandStatusWrapper* csw,
                uint8_t* buffer, uint32_t blocks, uint8_t volume, uint32_t lba_low, uint32_t lba_high,
                callback_function callback, uint8_t flags, RequestSense* rs);
    int (*send_write)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CommandBlockWrapper* cbw, CommandStatusWrapper* csw,
                uint8_t* buffer, uint32_t blocks, uint8_t volume, uint32_t lba_low, uint32_t lba_high,
                callback_function callback, uint8_t flags, RequestSense* rs);              
    uint32_t (*get_drive_size)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume);
    uint32_t (*get_block_size)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume);
    uint32_t (*get_block_num)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume);
    int (*test_mass_storage_writes)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CommandBlockWrapper* cbw, CommandStatusWrapper* csw, uint8_t volume, RequestSense* rs);
    void (*parse_request_sense)(struct MassStorageDriver* driver, RequestSense* rs);
    void (*init_sense_description)(struct MassStorageDriver* driver);
    int (*is_valid_volume)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume);
    uint32_t (*get_capacity_count)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume);

    int (*get_inquiry_data)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume, uint8_t param, 
                          uint8_t* target, uint8_t* len);
    int (*get_capacity_descpritor)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume, uint8_t capacity_num, uint8_t param,
                                 uint8_t* target, uint8_t* len);
    int (*get_sense_data)(struct MassStorageDriver* driver, uint8_t volume, uint8_t param,
                        uint8_t* target, uint8_t* len);
    int (*get_capacity)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume, uint8_t param, 
                      uint8_t* target, uint8_t* len);
    int (*get_max_logic_unit_numbers)(struct MassStorageDriver* driver, 
        UsbDev *dev, Interface *interface, uint8_t *data);
    int (*reset_bulk_only)(struct MassStorageDriver* driver, UsbDev *dev, 
        Interface *interface, callback_function callback);
    void (*set_dev_properites)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, UsbDev* dev, Interface* interface);
    void (*msd_match_routine)(struct MassStorageDriver* driver, MemoryService_C* mem_service, 
        UsbDev* dev, Endpoint* endpoint, struct MassStorageDev* msd_dev, uint8_t* select, 
        unsigned int* msd_endpoint);
    CommandBlockWrapper* (*get_free_cbw)(struct MassStorageDriver *driver);
    void (*free_cbw)(struct MassStorageDriver* driver, CommandBlockWrapper* cbw);
    CommandStatusWrapper* (*get_free_csw)(struct MassStorageDriver* driver);
    void (*free_csw)(struct MassStorageDriver* driver, CommandStatusWrapper* csw);

    struct MassStorageDev* (*get_msd_dev_by_minor)(struct MassStorageDriver* driver, uint8_t minor);

    char* description_sense[SENSE_STATUS_LEN];

    uint8_t* csw_map_io;
    uint8_t* cbw_map_io;

    uint8_t csw_map_io_bitmap[PAGE_SIZE / sizeof(CommandStatusWrapper)];
    uint8_t cbw_map_io_bitmap[PAGE_SIZE / sizeof(CommandBlockWrapper)];

    SuperMap* cbw_map;
    SuperMap* csw_map;
    SuperMap* data_map;
    SuperMap* stored_target_map;
    SuperMap* stored_len_map;
    SuperMap* stored_mem_buffer_map;

    void (*init_msd_maps)(struct MassStorageDriver* driver);
    uint64_t (*read_msd)(struct MassStorageDriver* driver, uint8_t* target, uint64_t start_lba, uint32_t blocks,
              uint16_t magic_number, uint8_t u_tag, uint8_t volume, uint8_t minor);
    int (*set_callback_msd)(struct MassStorageDriver* driver, msd_callback callback, 
                        uint16_t magic_number, uint8_t u_tag);
    int (*unset_callback_msd)(struct MassStorageDriver* driver, uint16_t magic_number, uint8_t u_tag);                        
    uint64_t (*write_msd)(struct MassStorageDriver* driver, uint8_t* source, uint64_t start_lba, uint32_t blocks,
              uint16_t magic_number, uint8_t u_tag, uint8_t volume, uint8_t minor);
    void (*clear_msd_map)(struct MassStorageDriver* driver, uint32_t id);

    uint32_t (*calc_t_len)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, uint8_t volume, uint32_t blocks);
    void (*build_read_command)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, 
                                CommandBlockWrapper* cbw, uint8_t volume, uint32_t blocks,
                                uint32_t lba_low, uint32_t lba_high, uint32_t t_len);
    void (*build_write_command)(struct MassStorageDriver* driver, struct MassStorageDev* msd, CommandBlockWrapper* cbw, uint8_t volume,
                        uint32_t blocks, uint32_t lba_low, uint32_t lba_high, uint32_t t_len);
    int (*init_io_msd)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev, CommandBlockWrapper* cbw, uint32_t blocks,
                        uint64_t start_lba, uint8_t* buffer, uint16_t magic_number, uint8_t u_tag, uint8_t volume, uint8_t read);
    struct MassStorageDev* (*get_free_msd_dev)(struct MassStorageDriver* driver);
    void (*free_msd_dev)(struct MassStorageDriver* driver, struct MassStorageDev* msd_dev);
    struct MassStorageDev* (*match_msd_dev)(struct MassStorageDriver* driver, UsbDev* dev);
    uint8_t (*valid_block_number)(struct MassStorageDriver* driver, uint32_t blocks);
    void (*copy_from_user)(uint8_t* __user, uint8_t* __kernel, size_t size);
    void (*copy_to_user)(uint8_t* __user, uint8_t* __kernel, size_t size);
};
    
typedef struct MassStorageDriver MassStorageDriver;
typedef struct MassStorageDev MassStorageDev;
typedef struct MassStorageVolume MassStorageVolume;

void new_mass_storage_driver(MassStorageDriver* driver, char* name, UsbDevice_ID* entry);

#endif  