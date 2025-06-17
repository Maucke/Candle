using System;
using System.Runtime.InteropServices;
using System.Threading;

namespace UsbCanLibrary
{
    /// <summary>
    /// C# wrapper for USB CAN library
    /// </summary>
    public class UsbCanDevice : IDisposable
    {
        // DLL imports
        private const string DLL_NAME = "usb_can_lib.dll"; // Windows
        // For Linux/Mac, use "libusb_can_lib.so" or "libusb_can_lib.dylib"

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_init();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void usb_can_cleanup();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_open_device();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_configure_device();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_get_device_info(out uint hw_version, out uint sw_version);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_identify(int enable);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_set_bitrate(uint bitrate);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_start(int enable_loopback);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_stop();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_send_frame(uint can_id, byte[] data, byte length);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_receive_frame(out uint can_id, byte[] data, out byte length, out uint timestamp);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_purge_rx_queue();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int usb_can_read_obd_pid(byte pid, byte[] response_data, out byte response_length);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void usb_can_sleep(int milliseconds);

        // Properties
        public bool IsConnected { get; private set; }
        public uint HardwareVersion { get; private set; }
        public uint SoftwareVersion { get; private set; }

        private bool _disposed = false;

        /// <summary>
        /// Initialize the USB CAN device
        /// </summary>
        public UsbCanDevice()
        {
            int result = usb_can_init();
            if (result < 0)
            {
                throw new InvalidOperationException($"Failed to initialize USB library: {result}");
            }
        }

        /// <summary>
        /// Connect to the USB CAN device
        /// </summary>
        /// <returns>True if successful</returns>
        public bool Connect()
        {
            if (IsConnected) return true;

            // Open device
            int result = usb_can_open_device();
            if (result < 0)
            {
                return false;
            }

            // Configure device
            result = usb_can_configure_device();
            if (result < 0)
            {
                return false;
            }

            // Get device info
            result = usb_can_get_device_info(out uint hw_version, out uint sw_version);
            if (result < 0)
            {
                return false;
            }

            HardwareVersion = hw_version;
            SoftwareVersion = sw_version;
            IsConnected = true;

            return true;
        }

        /// <summary>
        /// Disconnect from the USB CAN device
        /// </summary>
        public void Disconnect()
        {
            if (IsConnected)
            {
                usb_can_stop();
                IsConnected = false;
            }
        }

        /// <summary>
        /// Enable or disable device identification (LED blinking)
        /// </summary>
        /// <param name="enable">True to enable, false to disable</param>
        /// <returns>True if successful</returns>
        public bool SetIdentify(bool enable)
        {
            if (!IsConnected) return false;
            return usb_can_identify(enable ? 1 : 0) >= 0;
        }

        /// <summary>
        /// Set the CAN bitrate
        /// </summary>
        /// <param name="bitrate">Bitrate in bps (e.g., 500000 for 500kbps)</param>
        /// <returns>True if successful</returns>
        public bool SetBitrate(uint bitrate)
        {
            if (!IsConnected) return false;
            return usb_can_set_bitrate(bitrate) >= 0;
        }

        /// <summary>
        /// Start CAN communication
        /// </summary>
        /// <param name="enableLoopback">Enable loopback mode for testing</param>
        /// <returns>True if successful</returns>
        public bool StartCan(bool enableLoopback = false)
        {
            if (!IsConnected) return false;
            return usb_can_start(enableLoopback ? 1 : 0) >= 0;
        }

        /// <summary>
        /// Stop CAN communication
        /// </summary>
        /// <returns>True if successful</returns>
        public bool StopCan()
        {
            if (!IsConnected) return false;
            return usb_can_stop() >= 0;
        }

        /// <summary>
        /// Send a CAN frame
        /// </summary>
        /// <param name="canId">CAN ID</param>
        /// <param name="data">Data bytes (max 8 bytes)</param>
        /// <returns>True if successful</returns>
        public bool SendFrame(uint canId, byte[] data)
        {
            if (!IsConnected || data == null || data.Length > 8) return false;
            return usb_can_send_frame(canId, data, (byte)data.Length) >= 0;
        }

        /// <summary>
        /// Receive a CAN frame
        /// </summary>
        /// <param name="canId">Received CAN ID</param>
        /// <param name="data">Received data</param>
        /// <param name="timestamp">Timestamp in microseconds</param>
        /// <returns>True if frame received successfully</returns>
        public bool ReceiveFrame(out uint canId, out byte[] data, out uint timestamp)
        {
            canId = 0;
            data = null;
            timestamp = 0;

            if (!IsConnected) return false;

            byte[] buffer = new byte[8];
            byte length;
            int result = usb_can_receive_frame(out canId, buffer, out length, out timestamp);

            if (result >= 0)
            {
                data = new byte[length];
                Array.Copy(buffer, data, length);
                return true;
            }

            return false;
        }

        /// <summary>
        /// Purge the receive queue
        /// </summary>
        /// <returns>Number of frames purged</returns>
        public int PurgeReceiveQueue()
        {
            if (!IsConnected) return -1;
            return usb_can_purge_rx_queue();
        }

        /// <summary>
        /// Read an OBD-II PID value
        /// </summary>
        /// <param name="pid">PID to read</param>
        /// <returns>Response data or null if failed</returns>
        public byte[] ReadObdPid(byte pid)
        {
            if (!IsConnected) return null;

            byte[] responseData = new byte[8];
            byte responseLength;
            int result = usb_can_read_obd_pid(pid, responseData, out responseLength);

            if (result >= 0)
            {
                byte[] response = new byte[responseLength];
                Array.Copy(responseData, response, responseLength);
                return response;
            }

            return null;
        }

        /// <summary>
        /// Sleep for specified milliseconds
        /// </summary>
        /// <param name="milliseconds">Time to sleep</param>
        public static void Sleep(int milliseconds)
        {
            usb_can_sleep(milliseconds);
        }

        #region OBD-II Helper Methods

        /// <summary>
        /// Get engine RPM
        /// </summary>
        /// <returns>RPM value or -1 if failed</returns>
        public float GetEngineRpm()
        {
            var data = ReadObdPid(0x0C);
            if (data != null && data.Length >= 2)
            {
                return ((data[0] << 8) + data[1]) / 4.0f;
            }
            return -1;
        }

        /// <summary>
        /// Get vehicle speed in km/h
        /// </summary>
        /// <returns>Speed in km/h or -1 if failed</returns>
        public int GetVehicleSpeed()
        {
            var data = ReadObdPid(0x0D);
            if (data != null && data.Length >= 1)
            {
                return data[0];
            }
            return -1;
        }

        /// <summary>
        /// Get throttle position percentage
        /// </summary>
        /// <returns>Throttle position 0-100% or -1 if failed</returns>
        public int GetThrottlePosition()
        {
            var data = ReadObdPid(0x11);
            if (data != null && data.Length >= 1)
            {
                return (data[0] * 100) / 255;
            }
            return -1;
        }

        /// <summary>
        /// Get supported PIDs (0x00-0x1F)
        /// </summary>
        /// <returns>Array of supported PID numbers</returns>
        public int[] GetSupportedPids()
        {
            var supportedPids = new System.Collections.Generic.List<int>();

            for (int baseId = 0; baseId < 96; baseId += 32)
            {
                var data = ReadObdPid((byte)baseId);
                if (data != null && data.Length >= 4)
                {
                    for (int i = 0; i < 32; i++)
                    {
                        if ((data[i / 8] & (0x80 >> (i & 7))) != 0)
                        {
                            supportedPids.Add(baseId + i);
                        }
                    }
                }
            }

            return supportedPids.ToArray();
        }

        #endregion

        #region IDisposable Implementation

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    Disconnect();
                }

                usb_can_cleanup();
                _disposed = true;
            }
        }

        ~UsbCanDevice()
        {
            Dispose(false);
        }

        #endregion
    }
}