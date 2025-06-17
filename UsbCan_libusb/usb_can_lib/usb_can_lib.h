// usb_can_lib.h
#ifndef USB_CAN_LIB_H
#define USB_CAN_LIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

    // Error codes
#define USB_CAN_SUCCESS         0
#define USB_CAN_ERROR_INIT     -1
#define USB_CAN_ERROR_DEVICE   -2
#define USB_CAN_ERROR_CONFIG   -3
#define USB_CAN_ERROR_PARAM    -4
#define USB_CAN_ERROR_TIMEOUT  -5
#define USB_CAN_ERROR_IO       -6

// CAN frame flags
#define CAN_EFF_FLAG    0x80000000U  // Extended frame format (29-bit ID)
#define CAN_RTR_FLAG    0x40000000U  // Remote transmission request
#define CAN_ERR_FLAG    0x20000000U  // Error message frame

// OBD-II PIDs (common ones)
#define OBD_PID_SUPPORTED_PIDS_01_20    0x00
#define OBD_PID_MONITOR_STATUS          0x01
#define OBD_PID_FREEZE_DTC              0x02
#define OBD_PID_FUEL_SYSTEM_STATUS      0x03
#define OBD_PID_ENGINE_LOAD             0x04
#define OBD_PID_COOLANT_TEMP            0x05
#define OBD_PID_SHORT_TERM_FUEL_TRIM_1  0x06
#define OBD_PID_LONG_TERM_FUEL_TRIM_1   0x07
#define OBD_PID_SHORT_TERM_FUEL_TRIM_2  0x08
#define OBD_PID_LONG_TERM_FUEL_TRIM_2   0x09
#define OBD_PID_FUEL_PRESSURE           0x0A
#define OBD_PID_INTAKE_MAP              0x0B
#define OBD_PID_ENGINE_RPM              0x0C
#define OBD_PID_VEHICLE_SPEED           0x0D
#define OBD_PID_TIMING_ADVANCE          0x0E
#define OBD_PID_INTAKE_TEMP             0x0F
#define OBD_PID_MAF_FLOW                0x10
#define OBD_PID_THROTTLE_POS            0x11

// CAN frame structure for external use
    typedef struct {
        uint32_t can_id;        // CAN ID with flags
        uint8_t  length;        // Data length (0-8)
        uint8_t  data[8];       // Data bytes
        uint32_t timestamp_us;  // Timestamp in microseconds
    } can_frame_t;

    // Device information structure
    typedef struct {
        uint32_t hardware_version;
        uint32_t software_version;
        uint8_t  interface_count;
    } device_info_t;

    /**
     * Initialize the USB CAN library
     * Must be called before any other functions
     *
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_init(void);

    /**
     * Cleanup and release all USB CAN resources
     * Should be called when done using the library
     */
    EXPORT void usb_can_cleanup(void);

    /**
     * Open and connect to the first available USB CAN device
     * Searches for gs_usb compatible devices (VID:0x1d50, PID:0x606f)
     *
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_open_device(void);

    /**
     * Configure the USB CAN device with default settings
     * Sets up USB configuration, claims interface, and configures host format
     *
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_configure_device(void);

    /**
     * Get device information (hardware/software versions)
     *
     * @param hw_version Pointer to store hardware version
     * @param sw_version Pointer to store software version
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_get_device_info(uint32_t* hw_version, uint32_t* sw_version);

    /**
     * Enable or disable device identification mode (LED blinking)
     *
     * @param enable 1 to enable identification, 0 to disable
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_identify(int enable);

    /**
     * Set the CAN bus bitrate
     *
     * @param bitrate Bitrate in bits per second (e.g., 500000 for 500kbps)
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_set_bitrate(uint32_t bitrate);

    /**
     * Start CAN communication
     *
     * @param enable_loopback 1 to enable loopback mode, 0 for normal operation
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_start(int enable_loopback);

    /**
     * Stop CAN communication
     *
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_stop(void);

    /**
     * Send a CAN frame
     *
     * @param can_id CAN identifier (11-bit or 29-bit depending on flags)
     * @param data Pointer to data bytes (max 8 bytes)
     * @param length Number of data bytes (0-8)
     * @return Number of bytes sent on success, negative error code on failure
     */
    EXPORT int usb_can_send_frame(uint32_t can_id, const uint8_t* data, uint8_t length);

    /**
     * Receive a CAN frame (blocking with timeout)
     *
     * @param can_id Pointer to store received CAN ID
     * @param data Pointer to buffer for received data (must be at least 8 bytes)
     * @param length Pointer to store received data length
     * @param timestamp Pointer to store timestamp in microseconds
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_receive_frame(uint32_t* can_id, uint8_t* data, uint8_t* length, uint32_t* timestamp);

    /**
     * Purge/flush the receive queue
     * Removes all pending frames from the receive buffer
     *
     * @return Number of frames purged, or negative error code on failure
     */
    EXPORT int usb_can_purge_rx_queue(void);

    /**
     * Read an OBD-II PID value
     * Sends OBD request and waits for response
     *
     * @param pid OBD-II PID to read (0x00-0xFF)
     * @param response_data Buffer to store response data (must be at least 8 bytes)
     * @param response_length Pointer to store response data length
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_read_obd_pid(uint8_t pid, uint8_t* response_data, uint8_t* response_length);

    /**
     * Sleep for specified number of milliseconds
     * Cross-platform sleep function
     *
     * @param milliseconds Time to sleep in milliseconds
     */
    EXPORT void usb_can_sleep(int milliseconds);

    /**
     * Set receive timeout for blocking operations
     *
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_set_timeout(int timeout_ms);

    /**
     * Get last error message
     *
     * @return Pointer to last error message string
     */
    EXPORT const char* usb_can_get_last_error(void);

    /**
     * Check if device is connected and ready
     *
     * @return 1 if connected, 0 if not connected
     */
    EXPORT int usb_can_is_connected(void);

    /**
     * Send raw CAN frame with all parameters
     *
     * @param frame Pointer to CAN frame structure
     * @return Number of bytes sent on success, negative error code on failure
     */
    EXPORT int usb_can_send_frame_ex(const can_frame_t* frame);

    /**
     * Receive raw CAN frame with all parameters
     *
     * @param frame Pointer to CAN frame structure to fill
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_receive_frame_ex(can_frame_t* frame);

    /**
     * Get device information in a single call
     *
     * @param info Pointer to device_info_t structure to fill
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_get_device_info_ex(device_info_t* info);

    // Convenience macros for common operations
#define USB_CAN_STANDARD_ID(id)     ((id) & 0x7FF)      // 11-bit standard ID
#define USB_CAN_EXTENDED_ID(id)     (((id) & 0x1FFFFFFF) | CAN_EFF_FLAG)  // 29-bit extended ID
#define USB_CAN_IS_EXTENDED(id)     ((id) & CAN_EFF_FLAG)
#define USB_CAN_IS_RTR(id)          ((id) & CAN_RTR_FLAG)
#define USB_CAN_IS_ERROR(id)        ((id) & CAN_ERR_FLAG)

// OBD-II specific functions

/**
 * Initialize OBD-II communication
 * Sets up CAN for OBD-II communication (500kbps, standard format)
 *
 * @return 0 on success, negative error code on failure
 */
    EXPORT int usb_can_obd_init(void);

    /**
     * Get supported OBD-II PIDs in a range
     *
     * @param base_pid Base PID (0x00, 0x20, 0x40, 0x60, 0x80, 0xA0)
     * @param supported_mask Pointer to 32-bit mask of supported PIDs
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_obd_get_supported_pids(uint8_t base_pid, uint32_t* supported_mask);

    /**
     * Read engine RPM
     *
     * @param rpm Pointer to store RPM value
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_obd_get_engine_rpm(float* rpm);

    /**
     * Read vehicle speed
     *
     * @param speed Pointer to store speed in km/h
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_obd_get_vehicle_speed(uint8_t* speed);

    /**
     * Read throttle position
     *
     * @param position Pointer to store throttle position (0-100%)
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_obd_get_throttle_position(uint8_t* position);

    /**
     * Read engine coolant temperature
     *
     * @param temperature Pointer to store temperature in Celsius
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_obd_get_coolant_temperature(int16_t* temperature);

    /**
     * Read intake air temperature
     *
     * @param temperature Pointer to store temperature in Celsius
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_obd_get_intake_temperature(int16_t* temperature);

    /**
     * Read calculated engine load
     *
     * @param load Pointer to store engine load (0-100%)
     * @return 0 on success, negative error code on failure
     */
    EXPORT int usb_can_obd_get_engine_load(uint8_t* load);

#ifdef __cplusplus
}
#endif

#endif // USB_CAN_LIB_H