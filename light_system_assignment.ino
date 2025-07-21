void setup() {
 Required Libraries for ESP32 Web Server
#include <WiFi.h> // For ESP32 Wi-Fi functionalities
#include <AsyncTCP.h> // Asynchronous TCP library, a dependency for ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // Asynchronous Web Server library for ESP32

// --- Wi-Fi Configuration ---
// Define the SSID (network name) for your ESP32's Access Point
const char* ssid = "fridah";
// Define the password for your ESP32's Access Point. IMPORTANT: Change this to a strong, unique password!
const char* password = "giramia123"; // <--- CHANGE THIS PASSWORD!

// --- LED Pin Definitions (Based on Schematic) ---
// These GPIO pins control the BASE of the NPN transistors that switch the LEDs.
const int LED1_CTRL_PIN = 18;  // GPIO18 → Q1 base
const int LED2_CTRL_PIN = 19;  // GPIO19 → Q2 base
const int LED3_CTRL_PIN = 21;  // GPIO21 → Q3 base

// --- LDR Pin Definition (Based on Schematic) ---
const int LDR_PIN = 34; // Connected to GPIO 34 for analog reading

// --- Automatic Control Settings ---
// Threshold for determining night/day based on LDR reading.
// Values range from 0 (darkest) to 4095 (brightest) for a 12-bit ADC.
// You will likely need to CALIBRATE this value for your specific LDR and environment.
// Lower value = darker for "night" detection.
const int NIGHT_THRESHOLD = 800; // Example: Below 800 is considered night. ADJUST THIS!

// Delay between automatic light checks (in milliseconds)
const long AUTO_CHECK_INTERVAL = 10000; // Check every 10 seconds

// --- Web Server Object ---
// Create an instance of the AsyncWebServer on port 80 (standard HTTP port)
AsyncWebServer server(80);

// --- LED State Variables ---
// Boolean variables to keep track of the current state of each LED (true for ON, false for OFF)
bool led1State = false;
bool led2State = false;
bool led3State = false;

// --- Automatic Mode State Variable ---
bool autoModeEnabled = false;

// --- Timing Variable for Automatic Control ---
unsigned long lastAutoCheckMillis = 0;

// --- Helper function to set LED state (turns transistor ON/OFF) ---
void setLED(int pin, bool state) {
  // Since we are driving NPN transistors in common-emitter configuration:
  // HIGH on base turns transistor ON -> LED ON
  // LOW on base turns transistor OFF -> LED OFF
  digitalWrite(pin, state ? HIGH : LOW);
}

// --- HTML Content for the Web Dashboard ---
String getDashboardHtml() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Light Control | GIRAMIA FRIDAH</title>
    <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600&family=Montserrat:wght@700&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #8A2BE2;  /* Vibrant purple */
            --secondary: #4B0082; /* Dark indigo */
            --accent: #FF69B4;   /* Hot pink */
            --light: #F8F0FF;    /* Light lavender */
            --dark: #2E0854;    /* Deep purple */
            --success: #9C27B0;  /* Purple success */
            --danger: #E91E63;  /* Pink danger */
            --text: #333333;
        }
        
        body {
            font-family: 'Poppins', sans-serif;
            background: linear-gradient(135deg, #F8F0FF 0%, #E6E6FA 100%);
            margin: 0;
            padding: 20px;
            color: var(--text);
            min-height: 100vh;
        }
        
        .dashboard-container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 15px 30px rgba(138, 43, 226, 0.2);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, var(--primary) 0%, var(--secondary) 100%);
            color: white;
            padding: 20px 25px 25px;
            text-align: center;
            position: relative;
        }
        
        .header h1 {
            font-family: 'Montserrat', sans-serif;
            margin: 0;
            font-size: 2rem;
            letter-spacing: 1px;
        }
        
        .header h2 {
            margin: 10px 0 0;
            font-weight: 400;
            opacity: 0.9;
            font-size: 1.1rem;
        }
        
        .student-info {
            background: rgba(255,255,255,0.15);
            padding: 15px;
            margin-top: 20px;
            border-radius: 12px;
            backdrop-filter: blur(5px);
            text-align: center;
            font-size: 1.1rem;
            font-weight: 500;
        }
        
        .student-info span {
            display: block;
            margin: 5px 0;
        }
        
        .student-name {
            font-size: 1.2rem;
            font-weight: 600;
        }
        
        .content {
            padding: 25px;
        }
        
        .info-card {
            background: var(--light);
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 25px;
            display: flex;
            align-items: center;
        }
        
        .wifi-icon {
            font-size: 2rem;
            color: var(--primary);
            margin-right: 20px;
        }
        
        .wifi-details h3 {
            margin: 0 0 5px;
            color: var(--secondary);
        }
        
        .wifi-details p {
            margin: 0;
            color: var(--dark);
            opacity: 0.8;
            font-size: 0.9rem;
        }
        
        .control-section {
            margin-bottom: 30px;
        }
        
        .section-title {
            font-family: 'Montserrat', sans-serif;
            color: var(--secondary);
            margin-bottom: 15px;
            display: flex;
            align-items: center;
        }
        
        .section-title i {
            margin-right: 10px;
            color: var(--accent);
        }
        
        .btn-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin-top: 20px;
        }
        
        .btn {
            border: none;
            border-radius: 12px;
            padding: 15px;
            font-family: 'Poppins', sans-serif;
            font-weight: 500;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            background: white;
            box-shadow: 0 5px 15px rgba(138, 43, 226, 0.1);
            position: relative;
            overflow: hidden;
        }
        
        .btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 4px;
            background: var(--primary);
        }
        
        .btn i {
            font-size: 1.8rem;
            margin-bottom: 10px;
            color: var(--primary);
        }
        
        .btn:hover {
            transform: translateY(-3px);
            box-shadow: 0 8px 20px rgba(138, 43, 226, 0.2);
        }
        
        .btn.on {
            background: var(--primary);
            color: white;
        }
        
        .btn.on i {
            color: white;
        }
        
        .btn.on::before {
            background: var(--accent);
        }
        
        .status-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            display: inline-block;
            margin-left: 8px;
        }
        
        .status-indicator.on {
            background: var(--success);
            box-shadow: 0 0 10px var(--success);
        }
        
        .status-indicator.off {
            background: var(--danger);
            box-shadow: 0 0 10px var(--danger);
        }
        
        .sensor-display {
            background: white;
            border-radius: 15px;
            padding: 20px;
            text-align: center;
            margin-top: 20px;
            box-shadow: 0 5px 15px rgba(138, 43, 226, 0.1);
        }
        
        .sensor-value {
            font-size: 2rem;
            font-weight: 600;
            color: var(--primary);
            margin: 10px 0;
        }
        
        .threshold {
            color: var(--dark);
            opacity: 0.7;
            font-size: 0.9rem;
        }
        
        .footer {
            text-align: center;
            margin-top: 30px;
            color: var(--dark);
            opacity: 0.7;
            font-size: 0.85rem;
        }
        
        @media (max-width: 600px) {
            .header h1 {
                font-size: 1.5rem;
            }
            
            .student-info {
                font-size: 1rem;
                padding: 10px;
            }
            
            .student-name {
                font-size: 1.1rem;
            }
            
            .btn-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
</head>
<body>
    <div class="dashboard-container">
        <div class="header">
            <h1>SMART LIGHT CONTROL</h1>
            <h2>ESP32 Web Dashboard</h2>
            
            <div class="student-info">
                <span class="student-name">GIRAMIA FRIDAH</span>
                <span>24/U/2374/GIW/Ps</span>
            </div>
        </div>
        
        <div class="content">
            <div class="info-card">
                <div class="wifi-icon">
                    <i class="fas fa-wifi"></i>
                </div>
                <div class="wifi-details">
                    <h3 id="wifiSSID">ESP32 Access Point</h3>
                    <p id="wifiStatus">IP: Loading...</p>
                </div>
            </div>
            
            <div class="control-section">
                <h2 class="section-title"><i class="fas fa-robot"></i> AUTOMATIC CONTROL</h2>
                <button id="autoModeButton" class="btn" onclick="toggleAutoMode()">
                    <i class="fas fa-magic"></i>
                    <span>Auto Mode</span>
                    <span id="autoModeStatus" class="status-indicator off"></span>
                </button>
                
                <div class="sensor-display">
                    <div>Light Sensor Reading</div>
                    <div class="sensor-value" id="ldrValue">---</div>
                    <div class="threshold">Threshold: <span id="thresholdValue">800</span></div>
                </div>
            </div>
            
            <div class="control-section">
                <h2 class="section-title"><i class="fas fa-lightbulb"></i> MANUAL CONTROL</h2>
                <div class="btn-grid">
                    <button id="led1Button" class="btn" onclick="toggleLED(1)">
                        <i class="fas fa-lightbulb"></i>
                        <span>LED 1</span>
                        <span id="led1Status" class="status-indicator off"></span>
                    </button>
                    
                    <button id="led2Button" class="btn" onclick="toggleLED(2)">
                        <i class="fas fa-lightbulb"></i>
                        <span>LED 2</span>
                        <span id="led2Status" class="status-indicator off"></span>
                    </button>
                    
                    <button id="led3Button" class="btn" onclick="toggleLED(3)">
                        <i class="fas fa-lightbulb"></i>
                        <span>LED 3</span>
                        <span id="led3Status" class="status-indicator off"></span>
                    </button>
                </div>
            </div>
        </div>
        
        <div class="footer">
            © <span id="currentYear"></span> Smart Light Control System | TEAM_A Project
        </div>
    </div>

    <script>
        // Set current year
        document.getElementById('currentYear').textContent = new Date().getFullYear();
        
        // Function to update WiFi information
        function updateWifiInfo(ssid, ip) {
            const wifiSSID = document.getElementById('wifiSSID');
            const wifiStatus = document.getElementById('wifiStatus');
            
            wifiSSID.textContent = ssid || "ESP32 Access Point";
            wifiStatus.textContent = ip ? `IP: ${ip}` : "Connecting...";
        }
        
        // Function to send a request to the ESP32 to toggle an LED
        async function toggleLED(ledNum) {
            try {
                const response = await fetch(`/led${ledNum}/toggle`);
                const data = await response.text();
                updateUI(ledNum, data.includes("ON"));
            } catch (error) {
                console.error('Error toggling LED:', error);
            }
        }

        // Function to toggle Automatic Mode
        async function toggleAutoMode() {
            try {
                const response = await fetch('/automode/toggle');
                const data = await response.json();
                updateAutoModeUI(data.autoModeEnabled);
            } catch (error) {
                console.error('Error toggling Auto Mode:', error);
            }
        }

        // Function to fetch the current status from ESP32
        async function updateAllStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                
                updateUI(1, data.led1);
                updateUI(2, data.led2);
                updateUI(3, data.led3);
                updateAutoModeUI(data.autoModeEnabled);
                document.getElementById('ldrValue').textContent = data.ldrValue;
                
                if (data.ipAddress) {
                    updateWifiInfo(data.wifiSSID, data.ipAddress);
                }

            } catch (error) {
                console.error('Error fetching system status:', error);
            }
        }

        // Helper function to update LED UI
        function updateUI(ledNum, state) {
            const button = document.getElementById(`led${ledNum}Button`);
            const statusIndicator = document.getElementById(`led${ledNum}Status`);

            if (state) {
                button.classList.add('on');
                statusIndicator.className = 'status-indicator on';
            } else {
                button.classList.remove('on');
                statusIndicator.className = 'status-indicator off';
            }
        }

        // Helper function to update Auto Mode UI
        function updateAutoModeUI(state) {
            const button = document.getElementById('autoModeButton');
            const statusIndicator = document.getElementById('autoModeStatus');

            if (state) {
                button.classList.add('on');
                button.innerHTML = '<i class="fas fa-magic"></i><span>Auto Mode: ON</span><span id="autoModeStatus" class="status-indicator on"></span>';
                statusIndicator.className = 'status-indicator on';
            } else {
                button.classList.remove('on');
                button.innerHTML = '<i class="fas fa-magic"></i><span>Auto Mode: OFF</span><span id="autoModeStatus" class="status-indicator off"></span>';
                statusIndicator.className = 'status-indicator off';
            }
        }

        // Initialize on load
        window.onload = function() {
            updateAllStatus();
            setInterval(updateAllStatus, 3000); // Refresh every 3 seconds
        };
    </script>
</body>
</html>
  )rawliteral";
  return html;
}

// --- Arduino Setup Function ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting ESP32 Intelligent Lighting System...");

  // Set LED control pins as OUTPUTs
  pinMode(LED1_CTRL_PIN, OUTPUT);
  pinMode(LED2_CTRL_PIN, OUTPUT);
  pinMode(LED3_CTRL_PIN, OUTPUT);
  
  // Set LDR pin as INPUT (implicitly done for analogRead, but good practice)
  pinMode(LDR_PIN, INPUT);

  // Initialize all LEDs to OFF state
  setLED(LED1_CTRL_PIN, LOW);
  setLED(LED2_CTRL_PIN, LOW);
  setLED(LED3_CTRL_PIN, LOW);
  led1State = false;
  led2State = false;
  led3State = false;

  // Start the ESP32 in Access Point (AP) mode
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point (AP) IP address: ");
  Serial.println(IP);
  Serial.print("Connect to Wi-Fi network: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.println("Then open a web browser and go to the IP address above.");

  // --- Web Server Route Handlers ---

  // Route for the root URL ("/") - serves the main HTML dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Client requested root URL '/'");
    request->send(200, "text/html", getDashboardHtml());
  });

  // Route to toggle LED 1
  server.on("/led1/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led1State = !led1State;
    setLED(LED1_CTRL_PIN, led1State);
    Serial.printf("LED 1 toggled to: %s\n", led1State ? "ON" : "OFF");
    request->send(200, "text/plain", led1State ? "LED1_ON" : "LED1_OFF");
  });

  // Route to toggle LED 2
  server.on("/led2/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led2State = !led2State;
    setLED(LED2_CTRL_PIN, led2State);
    Serial.printf("LED 2 toggled to: %s\n", led2State ? "ON" : "OFF");
    request->send(200, "text/plain", led2State ? "LED2_ON" : "LED2_OFF");
  });

  // Route to toggle LED 3
  server.on("/led3/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led3State = !led3State;
    setLED(LED3_CTRL_PIN, led3State);
    Serial.printf("LED 3 toggled to: %s\n", led3State ? "ON" : "OFF");
    request->send(200, "text/plain", led3State ? "LED3_ON" : "LED3_OFF");
  });

  // Route to toggle Automatic Mode
  server.on("/automode/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    autoModeEnabled = !autoModeEnabled;
    Serial.printf("Automatic Mode toggled to: %s\n", autoModeEnabled ? "ENABLED" : "DISABLED");
    String jsonResponse = "{ \"autoModeEnabled\": " + String(autoModeEnabled ? "true" : "false") + " }";
    request->send(200, "application/json", jsonResponse);
  });

  // Route to get the current status of all LEDs, Auto Mode, and LDR value as JSON
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Serial.println("Client requested system status '/status'"); // Uncomment for more verbose logging
    int ldrValue = analogRead(LDR_PIN); // Read LDR value
    
    String jsonResponse = "{";
    jsonResponse += "\"led1\":" + String(led1State ? "true" : "false") + ",";
    jsonResponse += "\"led2\":" + String(led2State ? "true" : "false") + ",";
    jsonResponse += "\"led3\":" + String(led3State ? "true" : "false") + ",";
    jsonResponse += "\"autoModeEnabled\":" + String(autoModeEnabled ? "true" : "false") + ",";
    jsonResponse += "\"ldrValue\":" + String(ldrValue);
    jsonResponse += "}";
    request->send(200, "application/json", jsonResponse);
  });

  // Start the web server
  server.begin();
  Serial.println("Web server started.");
}

// --- Arduino Loop Function ---
// This function runs repeatedly after setup()
void loop() {
  // Check for automatic light control if enabled
  if (autoModeEnabled) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastAutoCheckMillis >= AUTO_CHECK_INTERVAL) {
      lastAutoCheckMillis = currentMillis;

      int ldrValue = analogRead(LDR_PIN);
      Serial.printf("LDR Value: %d, Threshold: %d\n", ldrValue, NIGHT_THRESHOLD);

      if (ldrValue < NIGHT_THRESHOLD) { // It's dark (LDR value is low)
        Serial.println("It's NIGHT - Turning ALL LEDs ON automatically.");
        if (!led1State) { led1State = true; setLED(LED1_CTRL_PIN, HIGH); }
        if (!led2State) { led2State = true; setLED(LED2_CTRL_PIN, HIGH); }
        if (!led3State) { led3State = true; setLED(LED3_CTRL_PIN, HIGH); }
      } else { // It's bright (LDR value is high)
        Serial.println("It's DAY - Turning ALL LEDs OFF automatically.");
        if (led1State) { led1State = false; setLED(LED1_CTRL_PIN, LOW); }
        if (led2State) { led2State = false; setLED(LED2_CTRL_PIN, LOW); }
        if (led3State) { led3State = false; setLED(LED3_CTRL_PIN, LOW); }
      }
    }
  }
}