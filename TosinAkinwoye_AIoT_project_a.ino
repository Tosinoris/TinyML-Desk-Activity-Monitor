#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_http_server.h"

// ============== THINGSPEAK CONFIG ==============
const char* thingSpeakServer = "http://api.thingspeak.com/update";
const char* writeAPIKey = "#################";
unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 15000;

// ============== PIN DEFINITIONS ==============
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5
#define Y2_GPIO_NUM    11
#define Y3_GPIO_NUM    9
#define Y4_GPIO_NUM    8
#define Y5_GPIO_NUM    10
#define Y6_GPIO_NUM    12
#define Y7_GPIO_NUM    18
#define Y8_GPIO_NUM    17
#define Y9_GPIO_NUM    16
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

#define LED_PIN 21
#define LDR_PIN 1
#define MIC_PIN 2

// ============== WIFI CREDENTIALS ==============
const char *ssid = "EE-WFF2P2";
const char *password = "FgfaFMuJYpvhp6";

// ============== GLOBAL VARIABLES ==============
int lightLevel = 0;
int noiseLevel = 0;
String lightStatus = "Unknown";
String noiseStatus = "Unknown";
String systemStatus = "Initializing";
int occupiedCount = 0;
int emptyCount = 0;

httpd_handle_t camera_httpd = NULL;

// ============== SENSOR FUNCTIONS ==============
void readSensors() {
  lightLevel = analogRead(LDR_PIN);
  
  if (lightLevel < 1000) {
    lightStatus = "Dark";
  } else if (lightLevel < 2500) {
    lightStatus = "Dim";
  } else {
    lightStatus = "Bright";
  }
  
  int micMin = 4095;
  int micMax = 0;
  
  for (int i = 0; i < 500; i++) {
    int sample = analogRead(MIC_PIN);
    if (sample < micMin) micMin = sample;
    if (sample > micMax) micMax = sample;
    delayMicroseconds(200);
  }
  
  noiseLevel = micMax - micMin;
  
  if (noiseLevel < 30) {
    noiseStatus = "Quiet";
  } else if (noiseLevel < 150) {
    noiseStatus = "Moderate";
  } else {
    noiseStatus = "Loud";
  }
}

void printSensorData() {
  Serial.println("===== SENSOR READINGS =====");
  Serial.print("Light: ");
  Serial.print(lightLevel);
  Serial.print(" (");
  Serial.print(lightStatus);
  Serial.print(") | Noise: ");
  Serial.print(noiseLevel);
  Serial.print(" (");
  Serial.print(noiseStatus);
  Serial.println(")");
  Serial.print("Images - Occupied: ");
  Serial.print(occupiedCount);
  Serial.print(" | Empty: ");
  Serial.println(emptyCount);
  Serial.println("===========================\n");
}

void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  int lightStatusNum = (lightStatus == "Dark") ? 1 : (lightStatus == "Dim") ? 2 : 3;
  int noiseStatusNum = (noiseStatus == "Quiet") ? 1 : (noiseStatus == "Moderate") ? 2 : 3;
  
  String url = String(thingSpeakServer);
  url += "?api_key=" + String(writeAPIKey);
  url += "&field1=" + String(lightLevel);
  url += "&field2=" + String(noiseLevel);
  url += "&field3=" + String(lightStatusNum);
  url += "&field4=" + String(noiseStatusNum);
  
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == 200) {
    Serial.println("ThingSpeak: Data uploaded!");
  }
  http.end();
}

// ============== CAPTURE IMAGE HANDLER ==============
static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=capture.jpg");
  httpd_resp_send(req, (const char *)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
  return ESP_OK;
}

// ============== CAPTURE OCCUPIED HANDLER ==============
static esp_err_t capture_occupied_handler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  
  occupiedCount++;
  Serial.print("Captured OCCUPIED image #");
  Serial.println(occupiedCount);
  
  // Flash LED to confirm capture
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  
  char filename[50];
  sprintf(filename, "attachment; filename=occupied_%d.jpg", occupiedCount);
  
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", filename);
  httpd_resp_send(req, (const char *)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
  return ESP_OK;
}

// ============== CAPTURE EMPTY HANDLER ==============
static esp_err_t capture_empty_handler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  
  emptyCount++;
  Serial.print("Captured EMPTY image #");
  Serial.println(emptyCount);
  
  // Flash LED twice to confirm capture
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
  }
  
  char filename[50];
  sprintf(filename, "attachment; filename=empty_%d.jpg", emptyCount);
  
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", filename);
  httpd_resp_send(req, (const char *)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
  return ESP_OK;
}

// ============== STREAM HANDLER ==============
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      res = ESP_FAIL;
    } else {
      size_t hlen = snprintf(part_buf, 64, _STREAM_PART, fb->len);
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
      if (res == ESP_OK) res = httpd_resp_send_chunk(req, part_buf, hlen);
      if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
    if (res != ESP_OK) break;
  }
  return res;
}

// ============== WEB DASHBOARD ==============
static esp_err_t index_handler(httpd_req_t *req) {
  String html = R"rawliteral(
<!DOCTYPE html><html><head>
<title>Desk Activity Monitor - Image Capture</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#1a1a2e;color:#eee;}
.container{max-width:900px;margin:0 auto;}
h1{color:#00d4ff;text-align:center;margin-bottom:10px;}
h2{color:#00d4ff;margin-top:20px;}
.subtitle{text-align:center;color:#aaa;margin-bottom:20px;}
.card{background:#16213e;border-radius:10px;padding:20px;margin:15px 0;}
.stream-container{text-align:center;margin:20px 0;}
.stream-container img{max-width:100%;width:400px;border-radius:10px;border:2px solid #00d4ff;}
.btn-container{display:flex;gap:15px;justify-content:center;flex-wrap:wrap;margin:20px 0;}
.btn{padding:15px 30px;font-size:18px;border:none;border-radius:8px;cursor:pointer;font-weight:bold;transition:transform 0.2s;}
.btn:hover{transform:scale(1.05);}
.btn-occupied{background:#4ade80;color:#000;}
.btn-empty{background:#f87171;color:#000;}
.btn-preview{background:#00d4ff;color:#000;}
.sensor{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #0f3460;}
.label{color:#aaa;}
.value{font-weight:bold;color:#00d4ff;}
.count-box{display:inline-block;background:#0f3460;padding:10px 20px;border-radius:8px;margin:5px;}
.count-occupied{border-left:4px solid #4ade80;}
.count-empty{border-left:4px solid #f87171;}
.instructions{background:#0f3460;padding:15px;border-radius:8px;margin:15px 0;}
.instructions li{margin:8px 0;color:#ccc;}
</style>
</head><body>
<div class="container">
<h1>Desk Activity Monitor</h1>
<p class="subtitle">TinyML Training Data Collection</p>

<div class="card">
<h2>Live Camera Feed</h2>
<div class="stream-container">
<img src="/stream" id="stream">
</div>

<div class="btn-container">
<a href="/capture_occupied" class="btn btn-occupied" download>Capture OCCUPIED</a>
<a href="/capture_empty" class="btn btn-empty" download>Capture EMPTY</a>
</div>

<div style="text-align:center;margin-top:15px;">
<div class="count-box count-occupied">Occupied: <span id="occ">)rawliteral";

  html += String(occupiedCount);
  html += R"rawliteral(</span></div>
<div class="count-box count-empty">Empty: <span id="emp">)rawliteral";

  html += String(emptyCount);
  html += R"rawliteral(</span></div>
</div>
</div>

<div class="card">
<h2>Sensor Readings</h2>
<div class="sensor"><span class="label">Light Level:</span><span class="value">)rawliteral";
  html += String(lightLevel) + " (" + lightStatus + ")";
  html += R"rawliteral(</span></div>
<div class="sensor"><span class="label">Noise Level:</span><span class="value">)rawliteral";
  html += String(noiseLevel) + " (" + noiseStatus + ")";
  html += R"rawliteral(</span></div>
<div class="sensor"><span class="label">System Status:</span><span class="value">)rawliteral";
  html += systemStatus;
  html += R"rawliteral(</span></div>
</div>

<div class="card">
<h2>Instructions</h2>
<div class="instructions">
<ol>
<li><strong>For OCCUPIED images:</strong> Sit at your desk in various positions, then click "Capture OCCUPIED"</li>
<li><strong>For EMPTY images:</strong> Move away from desk, then click "Capture EMPTY"</li>
<li>Aim for 50-100 images of each class</li>
<li>Vary lighting conditions, angles, and positions</li>
<li>Images download automatically - save them in separate folders</li>
<li>LED flashes once for occupied, twice for empty</li>
</ol>
</div>
</div>

<div class="card">
<h2>API Endpoints</h2>
<div class="sensor"><span class="label">Live Stream:</span><span class="value">/stream</span></div>
<div class="sensor"><span class="label">Single Capture:</span><span class="value">/capture</span></div>
<div class="sensor"><span class="label">JSON Data:</span><span class="value">/data</span></div>
</div>

</div>
<script>
// Auto-refresh sensor data every 5 seconds
setTimeout(function(){location.reload();}, 10000);
</script>
</body></html>
)rawliteral";

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html.c_str(), html.length());
}

// ============== JSON DATA HANDLER ==============
static esp_err_t data_handler(httpd_req_t *req) {
  String json = "{";
  json += "\"lightLevel\":" + String(lightLevel) + ",";
  json += "\"lightStatus\":\"" + lightStatus + "\",";
  json += "\"noiseLevel\":" + String(noiseLevel) + ",";
  json += "\"noiseStatus\":\"" + noiseStatus + "\",";
  json += "\"occupiedCount\":" + String(occupiedCount) + ",";
  json += "\"emptyCount\":" + String(emptyCount) + ",";
  json += "\"systemStatus\":\"" + systemStatus + "\"";
  json += "}";
  
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json.c_str(), json.length());
}

// ============== START WEB SERVER ==============
void startWebServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.max_uri_handlers = 10;

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = index_handler};
    httpd_uri_t stream_uri = {.uri = "/stream", .method = HTTP_GET, .handler = stream_handler};
    httpd_uri_t capture_uri = {.uri = "/capture", .method = HTTP_GET, .handler = capture_handler};
    httpd_uri_t capture_occ_uri = {.uri = "/capture_occupied", .method = HTTP_GET, .handler = capture_occupied_handler};
    httpd_uri_t capture_empty_uri = {.uri = "/capture_empty", .method = HTTP_GET, .handler = capture_empty_handler};
    httpd_uri_t data_uri = {.uri = "/data", .method = HTTP_GET, .handler = data_handler};
    
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &stream_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &capture_occ_uri);
    httpd_register_uri_handler(camera_httpd, &capture_empty_uri);
    httpd_register_uri_handler(camera_httpd, &data_uri);
  }
}

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n===== DESK ACTIVITY MONITOR =====");
  Serial.println("Image Capture Mode for TinyML Training\n");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  analogReadResolution(12);
  
  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;  // 320x240 - good for TinyML
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized!");

  // WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.println("=====================================");
    Serial.print("Dashboard: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=====================================");
    startWebServer();
    systemStatus = "Ready";
  } else {
    Serial.println("\nWiFi failed!");
    systemStatus = "No WiFi";
  }
}

// ============== MAIN LOOP ==============
void loop() {
  static bool ledState = false;
  static unsigned long lastUpdate = 0;
  
  // Toggle LED
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? LOW : HIGH);
  
  // Read sensors
  readSensors();
  printSensorData();
  
  // ThingSpeak update
  unsigned long now = millis();
  if (now - lastUpdate >= thingSpeakInterval) {
    sendToThingSpeak();
    lastUpdate = now;
  }
  
  delay(2000);
}
