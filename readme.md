Smart Wi-Fi Observer (ESP32 & Python)
This project is a network security system that uses an ESP32 development board to detect unknown or untrusted devices connecting to a local Wi-Fi network. When a new device is detected, it securely reports this information to a Python Flask server.
This system allows you to instantly notice unauthorized access attempts on your home or office network.
(You can add this simple diagram to your README to improve clarity.)
🚀 Key Features
Access Point Mode: The ESP32 creates its own Wi-Fi network (AP), allowing devices to connect directly to it.
Client Detection: It instantly detects the MAC addresses of all clients connected to its network.
Trusted Device Whitelist: It recognizes pre-defined "trusted" MAC addresses and does not generate alerts for these devices.
Secure Server Communication: When a new device is detected, it sends this information to a Python Flask server via a secure HTTP POST request.
Token-Based Authentication: All communication between the ESP32 and the server is protected with an Authorization: Bearer <token> header to prevent unauthorized access.
Detailed Logging: The server logs all successful and failed (unauthorized) connection attempts, including their IP addresses.
🛠️ How It Works
ESP32 Initialization: The ESP32 connects to your main router (for internet/server access) in Station mode and simultaneously broadcasts its own Wi-Fi network in Access Point mode.
Device Connection: Devices like phones and laptops connect to the Wi-Fi network created by the ESP32, named "ESP32_Observer_Net".
MAC Address Check: The ESP32 retrieves the MAC address of each connected device and compares it against a predefined trusted_macs list in the code.
Sending an Alert: If the connected device's MAC address is not on the trusted list and has not been reported before:
The ESP32 creates a JSON payload: {"event": "NEW_DEVICE_DETECTED", "mac": "XX:XX:XX:XX:XX:XX"}.
It sends this data via a POST request to the Python server, including a secret token in the Authorization header.
Server-Side Verification:
The Python server inspects the incoming request's Authorization header and the token within it.
If the token is valid, it accepts the request, logs the information, and returns a 200 OK response to the ESP32.
If the token is invalid or missing, it rejects the request, logs the unauthorized attempt, and returns a 401 Unauthorized error.
⚙️ Requirements
Hardware
ESP32 Development Board (e.g., NodeMCU-32S, WEMOS LOLIN D32)
Micro-USB Cable
Software
Arduino IDE
ESP32 Board Manager for Arduino IDE
Python 3.x
Flask Library (pip install Flask)
📋 Installation and Configuration
To run the project, you need to configure both the Python server and the ESP32.
1. Python Server Setup
Clone or download this repository to your computer.
Open a terminal, navigate to the project directory.
Install the required Flask library:
code
Bash
pip install Flask
Open flaskserver.py in a text editor and change the SECRET_TOKEN to a unique, hard-to-guess value:
code
Python
# Must be identical to the token in the ESP32 code!
SECRET_TOKEN = "ThisIsAVerySecretPassword123!"
2. ESP32 Setup
Open main.ino with the Arduino IDE.
Fill in the configuration section at the top of the code with your own details:
code
C++
// --- SECURITY TOKEN ---
// Must be EXACTLY the same as the token in the Python server.
const char* SECRET_TOKEN = "ThisIsAVerySecretPassword123!";

// --- ESP32 NETWORK SETTINGS ---
const char* ssid_ap = "ESP32_Observer_Net"; // Name of the network created by the ESP32
const char* password_ap = "securepassword";   // Password for this network (min 8 chars)

// --- HOME/OFFICE NETWORK SETTINGS (for connecting to the server) ---
const char* ssid_station = "YOUR_WIFI_SSID";     // Your main router's Wi-Fi name
const char* password_station = "YOUR_WIFI_PASSWORD"; // Your main router's password

// --- SERVER SETTINGS ---
// The local IP address of the computer running the Python server
const char* serverIp = "192.168.1.XX"; 
const int serverPort = 5000;

// --- TRUSTED DEVICES LIST ---
// MAC addresses of devices that won't trigger an alert
String trusted_macs[] = {
  "AA:BB:CC:11:22:33", // Example MAC 1 (Your phone)
  "DD:EE:FF:44:55:66", // Example MAC 2 (Your computer)
};
Select the correct ESP32 board and port from the Arduino IDE and upload the code.
▶️ Usage
Start the Server: In your terminal, navigate to the directory containing the Python server and run the following command:
code
Bash
python server.py
Verify that the server starts and displays the message * Running on http://0.0.0.0:5000.
Run the ESP32: Power up the ESP32 with the uploaded code. Open the Arduino IDE's Serial Monitor (at 115200 baud) to check if the ESP32 successfully connects to the networks and creates the AP.
Connect Devices: Connect the devices you want to monitor (phone, tablet, etc.) to the ESP32_Observer_Net Wi-Fi network.
Monitor the System:
Trusted Device: When a device from the trusted_macs list connects, its MAC address will appear in the Serial Monitor, but no alert will be sent to the server.
Foreign Device: When a new device not on the list connects, you will see a "NEW/UNTRUSTED DEVICE DETECTED" message in the Serial Monitor and details of the request being sent to the server.
In the Python terminal, you will see that the server has received and logged this alert.
⚠️ Important: If the ESP32 reports HTTP Request Error Code: -1, it likely means that the firewall on the computer running the Python server is blocking incoming connections on port 5000. You may need to create a firewall rule to allow incoming traffic for Python or on port 5000.
📁 Project Structure
code
Code
.
└── main.ino  (Arduino Code)
└── flaskserver.py              (Python Flask Server)
└── README.md

