using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace UsbCanLibrary
{

    /// <summary>
    /// Example usage class
    /// </summary>
    public class Program
    {
        public static void Main(string[] args)
        {
            using (var canDevice = new UsbCanDevice())
            {
                try
                {
                    // Connect to device
                    if (!canDevice.Connect())
                    {
                        Console.WriteLine("Failed to connect to USB CAN device");
                        return;
                    }

                    Console.WriteLine($"Connected to device");
                    Console.WriteLine($"Hardware Version: {canDevice.HardwareVersion}");
                    Console.WriteLine($"Software Version: {canDevice.SoftwareVersion}");

                    // Identify device (blink LEDs for 3 seconds)
                    Console.WriteLine("Identifying device...");
                    canDevice.SetIdentify(true);
                    Thread.Sleep(3000);
                    canDevice.SetIdentify(false);

                    // Set bitrate and start CAN
                    if (!canDevice.SetBitrate(500000)) // 500kbps
                    {
                        Console.WriteLine("Failed to set bitrate");
                        return;
                    }

                    if (!canDevice.StartCan(true)) // Enable loopback for testing
                    {
                        Console.WriteLine("Failed to start CAN");
                        return;
                    }

                    // Purge receive queue
                    int purged = canDevice.PurgeReceiveQueue();
                    Console.WriteLine($"Purged {purged} frames from receive queue");

                    // Get supported PIDs
                    Console.WriteLine("Getting supported PIDs...");
                    var supportedPids = canDevice.GetSupportedPids();
                    Console.WriteLine($"Supported PIDs: {string.Join(", ", supportedPids)}");

                    // Read OBD data continuously
                    Console.WriteLine("Reading OBD data (press Ctrl+C to stop)...");
                    while (true)
                    {
                        // Read engine RPM
                        float rpm = canDevice.GetEngineRpm();
                        if (rpm >= 0)
                        {
                            Console.WriteLine($"Engine RPM: {rpm:F1}");
                        }

                        // Read vehicle speed
                        int speed = canDevice.GetVehicleSpeed();
                        if (speed >= 0)
                        {
                            Console.WriteLine($"Vehicle Speed: {speed} km/h");
                        }

                        // Read throttle position
                        int throttle = canDevice.GetThrottlePosition();
                        if (throttle >= 0)
                        {
                            Console.WriteLine($"Throttle Position: {throttle}%");
                        }

                        Console.WriteLine();
                        Thread.Sleep(1000);
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error: {ex.Message}");
                }
                finally
                {
                    canDevice.StopCan();
                }
            }
        }
    }
}
