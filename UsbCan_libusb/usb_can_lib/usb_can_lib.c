#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libusb.h"
#include "usb_can_lib.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
#define EXPORT
#include <unistd.h>
#define SLEEP(ms) usleep((ms) * 1000)
#endif

// USB constants
static const int USB_DIR_OUT = 0;
static const int USB_DIR_IN = 0x80;
static const int USB_TYPE_VENDOR = (0x02 << 5);
static const int USB_RECIP_INTERFACE = 0x01;

// gs_usb constants
enum gs_usb_breq {
    GS_USB_BREQ_HOST_FORMAT = 0,
    GS_USB_BREQ_BITTIMING,
    GS_USB_BREQ_MODE,
    GS_USB_BREQ_BERR,
    GS_USB_BREQ_BT_CONST,
    GS_USB_BREQ_DEVICE_CONFIG,
    GS_USB_BREQ_TIMESTAMP,
    GS_USB_BREQ_IDENTIFY,
};

static const uint32_t GS_CAN_CONFIG_BYTE_ORDER = 0x0000beef;

enum gs_can_mode {
    GS_CAN_MODE_RESET = 0,
    GS_CAN_MODE_START
};

static const int GS_CAN_MODE_HW_TIMESTAMP_FLAG = (1 << 4);
static const int GS_CAN_MODE_LOOP_BACK_FLAG = (1 << 1);

enum gs_can_identify_mode {
    GS_CAN_IDENTIFY_OFF = 0,
    GS_CAN_IDENTIFY_ON
};

#pragma pack(push, 1)
typedef struct {
    uint32_t byte_order;
} gs_host_config;

typedef struct {
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t icount;
    uint32_t sw_version;
    uint32_t hw_version;
} gs_device_config;

typedef struct {
    uint32_t mode;
} gs_identify_mode;

typedef struct {
    uint32_t feature;
    uint32_t fclk_can;
    uint32_t tseg1_min;
    uint32_t tseg1_max;
    uint32_t tseg2_min;
    uint32_t tseg2_max;
    uint32_t sjw_max;
    uint32_t brp_min;
    uint32_t brp_max;
    uint32_t brp_inc;
} gs_device_bt_const;

typedef struct {
    uint32_t prop_seg;
    uint32_t phase_seg1;
    uint32_t phase_seg2;
    uint32_t sjw;
    uint32_t brp;
} gs_device_bittiming;

typedef struct {
    uint32_t mode;
    uint32_t flags;
} gs_device_mode;

typedef struct {
    uint32_t echo_id;
    uint32_t can_id;
    uint8_t can_dlc;
    uint8_t channel;
    uint8_t flags;
    uint8_t reserved;
    uint8_t data[8];
    uint32_t timestamp_us;
} gs_host_frame;
#pragma pack(pop)

// Global variables
static libusb_context* usb_context = NULL;
static libusb_device_handle* device_handle = NULL;
static int is_initialized = 0;

// Helper functions
int control_in(libusb_device_handle* handle, uint8_t request, void* data, size_t size) {
    return libusb_control_transfer(handle,
        USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
        request, 0, 0,
        (unsigned char*)data, size, 1000);
}

int control_out(libusb_device_handle* handle, uint8_t request, const void* data, size_t size) {
    return libusb_control_transfer(handle,
        USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
        request, 0, 0,
        (unsigned char*)data, size, 1000);
}

int data_in(libusb_device_handle* handle, void* data, size_t size) {
    int transferred;
    int ret = libusb_bulk_transfer(handle, 0x81,
        (unsigned char*)data, size, &transferred, 1000);
    return ret >= 0 ? transferred : ret;
}

int data_out(libusb_device_handle* handle, const void* data, size_t size) {
    int transferred;
    int ret = libusb_bulk_transfer(handle, 0x02,
        (unsigned char*)data, size, &transferred, 1000);
    return ret >= 0 ? transferred : ret;
}

// Exported functions
EXPORT int usb_can_init(void) {
    if (is_initialized) {
        return 0; // Already initialized
    }

    int ret = libusb_init(&usb_context);
    if (ret < 0) {
        return ret;
    }

    is_initialized = 1;
    return 0;
}

EXPORT void usb_can_cleanup(void) {
    if (device_handle) {
        libusb_close(device_handle);
        device_handle = NULL;
    }

    if (usb_context) {
        libusb_exit(usb_context);
        usb_context = NULL;
    }

    is_initialized = 0;
}

EXPORT int usb_can_open_device(void) {
    if (!is_initialized) {
        return -1;
    }

    libusb_device** devs;
    ssize_t cnt = libusb_get_device_list(usb_context, &devs);
    if (cnt < 0) {
        return (int)cnt;
    }

    int found = 0;
    for (int i = 0; devs[i]; ++i) {
        struct libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(devs[i], &desc);
        if (ret < 0) continue;

        // Check for gs_usb device (OpenMoko vendor)
        if (desc.idVendor == 0x1d50 && desc.idProduct == 0x606f) {
            ret = libusb_open(devs[i], &device_handle);
            if (ret == LIBUSB_SUCCESS) {
                found = 1;
                break;
            }
        }
    }

    libusb_free_device_list(devs, 1);
    return found ? 0 : -1;
}

EXPORT int usb_can_configure_device(void) {
    if (!device_handle) {
        return -1;
    }

    // Get and set configuration
    struct libusb_config_descriptor* config;
    int ret = libusb_get_config_descriptor(libusb_get_device(device_handle), 0, &config);
    if (ret < 0) return ret;

    ret = libusb_set_configuration(device_handle, config->bConfigurationValue);
    libusb_free_config_descriptor(config);
    if (ret < 0) return ret;

    // Claim interface
    ret = libusb_claim_interface(device_handle, 0);
    if (ret < 0) return ret;

    ret = libusb_set_interface_alt_setting(device_handle, 0, 0);
    if (ret < 0) return ret;

    // Set host config
    gs_host_config host_config;
    host_config.byte_order = GS_CAN_CONFIG_BYTE_ORDER;
    ret = control_out(device_handle, GS_USB_BREQ_HOST_FORMAT, &host_config, sizeof(host_config));
    if (ret < 0) return ret;

    return 0;
}

EXPORT int usb_can_get_device_info(uint32_t* hw_version, uint32_t* sw_version) {
    if (!device_handle || !hw_version || !sw_version) {
        return -1;
    }

    gs_device_config device_config;
    int ret = control_in(device_handle, GS_USB_BREQ_DEVICE_CONFIG, &device_config, sizeof(device_config));
    if (ret < 0) return ret;

    *hw_version = device_config.hw_version;
    *sw_version = device_config.sw_version;
    return 0;
}

EXPORT int usb_can_identify(int enable) {
    if (!device_handle) {
        return -1;
    }

    gs_identify_mode identify_mode;
    identify_mode.mode = enable ? GS_CAN_IDENTIFY_ON : GS_CAN_IDENTIFY_OFF;
    return control_out(device_handle, GS_USB_BREQ_IDENTIFY, &identify_mode, sizeof(identify_mode));
}

EXPORT int usb_can_set_bitrate(uint32_t bitrate) {
    if (!device_handle) {
        return -1;
    }

    // Get bit timing constraints
    gs_device_bt_const bt_const;
    int ret = control_in(device_handle, GS_USB_BREQ_BT_CONST, &bt_const, sizeof(bt_const));
    if (ret < 0) return ret;

    // Calculate bit timing for given bitrate
    gs_device_bittiming bit_timing;
    bit_timing.prop_seg = 0;
    bit_timing.phase_seg1 = 13;
    bit_timing.phase_seg2 = 2;
    bit_timing.sjw = 1;
    bit_timing.brp = bt_const.fclk_can / (bitrate * 16);

    return control_out(device_handle, GS_USB_BREQ_BITTIMING, &bit_timing, sizeof(bit_timing));
}

EXPORT int usb_can_start(int enable_loopback) {
    if (!device_handle) {
        return -1;
    }

    gs_device_mode mode;
    mode.mode = GS_CAN_MODE_START;
    mode.flags = GS_CAN_MODE_HW_TIMESTAMP_FLAG;
    if (enable_loopback) {
        mode.flags |= GS_CAN_MODE_LOOP_BACK_FLAG;
    }

    return control_out(device_handle, GS_USB_BREQ_MODE, &mode, sizeof(mode));
}

EXPORT int usb_can_stop(void) {
    if (!device_handle) {
        return -1;
    }

    gs_device_mode mode;
    mode.mode = GS_CAN_MODE_RESET;
    mode.flags = 0;

    return control_out(device_handle, GS_USB_BREQ_MODE, &mode, sizeof(mode));
}

EXPORT int usb_can_send_frame(uint32_t can_id, const uint8_t* data, uint8_t length) {
    if (!device_handle || !data || length > 8) {
        return -1;
    }

    gs_host_frame frame = { 0 };
    frame.can_id = can_id;
    frame.can_dlc = length;
    memcpy(frame.data, data, length);

    return data_out(device_handle, &frame, sizeof(frame));
}

EXPORT int usb_can_receive_frame(uint32_t* can_id, uint8_t* data, uint8_t* length, uint32_t* timestamp) {
    if (!device_handle || !can_id || !data || !length || !timestamp) {
        return -1;
    }

    gs_host_frame frame = { 0 };
    int ret = data_in(device_handle, &frame, sizeof(frame));
    if (ret < 0) return ret;

    *can_id = frame.can_id;
    *length = frame.can_dlc;
    *timestamp = frame.timestamp_us;
    memcpy(data, frame.data, frame.can_dlc);

    return 0;
}

EXPORT int usb_can_purge_rx_queue(void) {
    if (!device_handle) {
        return -1;
    }

    gs_host_frame frame;
    int ret;
    int purged = 0;

    do {
        ret = data_in(device_handle, &frame, sizeof(frame));
        if (ret >= 0) purged++;
    } while (ret >= 0);

    return purged;
}

EXPORT int usb_can_read_obd_pid(uint8_t pid, uint8_t* response_data, uint8_t* response_length) {
    if (!device_handle || !response_data || !response_length) {
        return -1;
    }

    // Send OBD request
    uint8_t request_data[8] = { 2, 1, pid, 0x55, 0x55, 0x55, 0x55, 0x55 };
    int ret = usb_can_send_frame(0x7DF, request_data, 8);
    if (ret < 0) return ret;

    // Read echo
    uint32_t can_id, timestamp;
    uint8_t data[8], length;
    ret = usb_can_receive_frame(&can_id, data, &length, &timestamp);
    if (ret < 0) return ret;

    // Read response
    ret = usb_can_receive_frame(&can_id, data, &length, &timestamp);
    if (ret < 0) return ret;

    // Check if it's a valid OBD response
    if (length >= 3 && data[1] == 0x41 && data[2] == pid) {
        *response_length = length - 3;
        memcpy(response_data, &data[3], *response_length);
        return 0;
    }

    return -1;
}

EXPORT void usb_can_sleep(int milliseconds) {
    SLEEP(milliseconds);
}