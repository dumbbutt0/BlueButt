# BlueButt
BlueButt allows you to interact with Bluetooth devices by scanning for nearby devices, connecting to A2DP (Advanced Audio Distribution Profile) services, and flooding a target device with multiple A2DP connection attempts.

## Features

- **Device Scanning**: Scans for nearby Bluetooth devices and displays their MAC addresses and names.
- **A2DP Connection**: Establishes a connection with a target device using the A2DP service.
- **Flooding Mode**: Simultaneously floods the target device with multiple A2DP connection attempts using multithreading.
- **Sniper Mode**: Continuously attempts to connect to a specified device until successful, then floods it with A2DP connections.
  
## Requirements

- **Linux** system with Bluetooth capabilities.
- **BlueZ** Bluetooth stack installed on your system (common on most Linux distributions).
- **g++** for compiling the C++ code.
- **libbluetooth** library for Bluetooth operations.

### Install Dependencies

Run the following commands to install the necessary dependencies:

```bash
sudo apt-get update
sudo apt-get install libbluetooth-dev
sudo apt-get install bluez
```

### Compile the Program

To compile the program, use the following command:

```bash
g++ bluebutt.cpp -o bluebutt  -lbluetooth -lpthread
```

This will create an executable named `bluebutt`.

## Usage

The program offers multiple modes of operation depending on the provided command-line arguments:

### 1. **Default Mode (Scan and Flood)**

When no arguments are provided, the program scans for nearby Bluetooth devices and prompts the user to input the target MAC address. After selecting the target, the program attempts to flood the device with A2DP connections.

**Command:**

```bash
sudo ./bluebutt
```

**Example Output:**

```
Scanning for devices...
Device found: AirPods (58:93:E8:09:78:32)
Device found: Headphones (04:B9:E3:69:B6:63)
Enter the target's MAC address: 58:93:E8:09:78:32
Flooding (58:93:E8:09:78:32) with 4 A2DP connections...
```
<!--![scanner](/find.png)-->

### 2. **Direct Target Mode**

You can provide the target MAC address directly as a command-line argument to skip the scanning phase and immediately flood the target device with A2DP connections.

**Command:**

```bash
sudo ./bluebutt <TARGET_MAC_ADDRESS>
```

**Example:**

```bash
sudo ./bluebutt 58:93:E8:09:78:32
```

**Output:**

```
Target MAC address: 58:93:E8:09:78:32
Flooding (58:93:E8:09:78:32) with 4 A2DP connections...
```

<!--![direct](/direct.png)-->

### 3. **Sniper Mode**

In sniper mode, the program continuously attempts to connect to a target device. Once a successful connection is established, the program immediately floods the device with multiple A2DP connections.

**Command:**

```bash
sudo ./bluebutt sniper <TARGET_MAC_ADDRESS>
```

**Example:**

```bash
sudo ./bluebutt sniper 58:93:E8:09:78:32
```

**Output:**

```
Entering sniper mode targeting 58:93:E8:09:78:32...
Attempt #1: Trying to connect to 58:93:E8:09:78:32
ERROR: Could not connect to A2DP on 58:93:E8:09:78:32
Connection failed. Retrying...
Attempt #2: Trying to connect to 58:93:E8:09:78:32
Successfully connected to A2DP on 58:93:E8:09:78:32
Connection successful! Initiating flood...
Flooding (58:93:E8:09:78:32) with 4 A2DP connections...
```

In sniper mode, the program will continue retrying until it successfully connects to the target device. You can quit the program anytime by pressing `Ctrl+C`.

 <!--![sniper](/snipe.png)-->
### Program Options

- **Number of Flood Connections**: The number of connections the program tries to establish is set by the `num_connections` variable (default is 4). You can modify this variable in the code if you wish to attempt more connections.
- **Retry Delay in Sniper Mode**: In sniper mode, the program waits for 500ms (`usleep(500000)`) between each connection attempt. This value can also be adjusted in the code.

## Example Scenarios

### Scenario 1: Default Mode (Scan and Flood)

1. Run the program with no arguments.
2. The program scans for Bluetooth devices in the vicinity.
3. Enter the MAC address of the device you want to flood.
4. The program will flood the specified device with A2DP connection attempts.

### Scenario 2: Sniper Mode (Automated Retry)

1. Run the program in sniper mode by providing the `sniper` argument followed by the target device's MAC address.
2. The program will continuously attempt to connect to the target device.
3. Once connected, the program will flood the device with A2DP connections.

## Troubleshooting

- **Could not connect to A2DP**: Ensure that the target device supports A2DP (Advanced Audio Distribution Profile) and is in a discoverable state. Some devices might have protective measures that limit connection attempts.
- **No devices found**: Ensure that your Bluetooth adapter is working correctly, and nearby devices are in discoverable mode.
- **Segmentation Fault**: Ensure that your system has all necessary Bluetooth libraries installed and that you are running the program with the appropriate permissions (`sudo`).

## Customization

- **Modify Connection Attempts**: You can adjust the `num_connections` variable in the code to control how many simultaneous connection attempts the program will make when flooding.
- **Adjust Retry Timing**: Modify the `usleep(500000)` statement to control the delay between connection attempts in sniper mode.

## Known Issues

- **Limited to Bluetooth Devices Supporting A2DP**: The program is specifically designed for devices that support the A2DP profile. If the device does not support A2DP, the program will not be able to connect or flood.
- **Adapter Compatibility**: Some Bluetooth adapters may have limitations on the number of simultaneous connections they can handle.

## License

This project is open-source under the MIT License.

## Contributions

Feel free to contribute to this project by submitting pull requests, reporting issues, or suggesting new features.
