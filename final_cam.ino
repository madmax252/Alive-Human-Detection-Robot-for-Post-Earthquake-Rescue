// ===============================
// ESP32-CAM Web + Telegram + Sensor Trigger
// V3 (UI, Flash Toggle, Bug Fix, Sensor Trigger)
// ===============================

// 1. WiFi Credentials
const char* ssid = "M4 Pro";
const char* password = "mad234hav";

// 2. Telegram Bot Details
String BOTtoken = "your bot token";
String chat_id  = "your chat id";

// ===============================
// NEW: Sensor Trigger Settings
// ===============================
#define SENSOR_PIN 13                   // GPIO pin for the sensor (e.g., PIR motion sensor)
#define TRIGGER_COOLDOWN_MS 10000       // 10-second cooldown between sensor triggers

// 3. Include Libraries
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_http_server.h"

// ===============================
// Camera Pins (AI THINKER)
// ===============================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4 // Flash LED

// ===============================
// Embedded HTML page (unchanged)
// ===============================
const char* HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html class="h-full bg-gray-100">
<head>
  <title>ESP32 Camera Controls</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <script src="https://cdn.tailwindcss.com"></script>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; }
    #camera-view { max-width: 100%; height: auto; max-height: 70vh; background: #222; display:block; margin:0 auto; border-radius: 0.5rem; border: 1px solid #ddd; }
    .toast { position: fixed; right: 16px; bottom: 16px; background: rgba(17, 24, 39, 0.9); color: white; padding: 12px 16px; border-radius: 8px; display:none; box-shadow: 0 4px 12px rgba(0,0,0,0.15); }
    .btn { display:inline-flex; align-items:center; gap: 0.5rem; padding: 0.6rem 1rem; border-radius: 0.375rem; font-weight: 600; text-white; transition: background-color 0.2s; }
    .btn:hover { filter: brightness(1.1); }
    .btn-icon { width: 1.25rem; height: 1.25rem; }
    .control-grid { display: grid; grid-template-columns: 120px 1fr; gap: 0.75rem; align-items: center; margin-bottom: 0.75rem; }
    .label { font-weight: 500; }
    input[type='range'] { width: 100%; }
    select, input { border-radius: 0.375rem; border: 1px solid #D1D5DB; padding: 0.25rem 0.5rem; }
  </style>
</head>
<body class="h-full text-gray-800">
  <div class="w-full max-w-6xl mx-auto p-4">
    <h1 class="text-3xl font-bold text-center mb-4 text-gray-900">ESP32 Camera Control Panel</h1>

    <div class="grid grid-cols-1 lg:grid-cols-5 gap-6">
      
      <div class="lg:col-span-3 bg-white rounded-lg shadow-md p-4 border">
        <div class="mb-4">
          <img id="camera-view" src="https://placehold.co/800x600/222222/ffffff?text=Camera+Offline" alt="Camera">
        </div>

        <div class="flex flex-wrap gap-3">
          <button id="toggle-stream" class="btn bg-blue-600">
            <svg class="btn-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M5.636 5.636a9 9 0 1 0 12.728 0M12 3v9" /></svg>
            Start Stream
          </button>
          <button id="toggle-flash" class="btn bg-yellow-600">
            <svg id="flash-icon" class="btn-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="m3.75 13.5 10.5-11.25L12 10.5h8.25L9.75 21.75 12 13.5H3.75Z" /></svg>
            <span id="flash-text">Flash: Off</span>
          </button>
          <button id="capture-photo" class="btn bg-green-600">
            <svg class="btn-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M6.827 6.175A2.31 2.31 0 0 1 5.186 7.23c-.38.054-.757.112-1.134.174C2.999 7.58 2.25 8.507 2.25 9.574v8.358c0 1.067.75 1.994 1.802 2.169a.75.75 0 0 0 .701-.04 4.907 4.907 0 0 0 3.127 0 .75.75 0 0 0 .701.04 2.31 2.31 0 0 1 2.043 1.06c.38.061.757.12 1.134.174.969.15 1.916.036 2.787-.333.357-.146.724-.29 1.1-.434.652-.248 1.287-.583 1.885-.985A2.31 2.31 0 0 1 20.814 18l.173.013a2.31 2.31 0 0 1 2.043-1.06c.38-.061.757-.12 1.134-.174.969-.15 1.916-.036 2.787.333C23.443 17.4 24 16.47 24 15.402v-8.357c0-1.067-.75-1.994-1.802-2.169a.75.75 0 0 0-.701.04 4.907 4.907 0 0 0-3.127 0 .75.75 0 0 0-.701-.04 2.31 2.31 0 0 1-2.043-1.06c-.38-.061-.757-.12-1.134-.174A4.907 4.907 0 0 0 12 2.25Z" /></svg>
            Capture Photo
          </button>
          <button id="capture-telegram" class="btn bg-indigo-600">
            <svg class="btn-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M6 12 3.269 3.125A59.769 59.769 0 0 1 21.485 12 59.768 59.768 0 0 1 3.27 20.875L6 12Zm0 0h7.5" /></svg>
            Capture + Send
          </button>
        </div>
      </div>

      <div class="lg:col-span-2 bg-white rounded-lg shadow-md p-4 border">
        <h2 class="text-xl font-bold mb-3">Camera Settings</h2>
        
        <div class="control-grid">
          <div class="label">Resolution</div>
          <select id="framesize" class="w-full">
            <option value="FRAMESIZE_QQVGA">QQVGA (160x120)</option>
            <option value="FRAMESIZE_QVGA">QVGA (320x240)</option>
            <option value="FRAMESIZE_VGA">VGA (640x480)</option>
            <option value="FRAMESIZE_SVGA" selected>SVGA (800x600)</option>
            <option value="FRAMESIZE_XGA">XGA (1024x768)</option>
            <option value="FRAMESIZE_UXGA">UXGA (1600x1200)</option>
          </select>
        </div>

        <div class="control-grid">
          <div class="label">Brightness</div>
          <div class="flex items-center gap-2">
            <input id="brightness" type="range" min="-2" max="2" step="1" value="0">
            <span id="brightness-val" class="w-4 text-center">0</span>
          </div>
        </div>

        <div class="control-grid">
          <div class="label">Contrast</div>
          <div class="flex items-center gap-2">
            <input id="contrast" type="range" min="-2" max="2" step="1" value="0">
            <span id="contrast-val" class="w-4 text-center">0</span>
          </div>
        </div>

        <div class="control-grid">
          <div class="label">Saturation</div>
          <div class="flex items-center gap-2">
            <input id="saturation" type="range" min="-2" max="2" step="1" value="0">
            <span id="saturation-val" class="w-4 text-center">0</span>
          </div>
        </div>

        <div class="control-grid">
          <div class="label">Sharpness</div>
          <div class="flex items-center gap-2">
            <input id="sharpness" type="range" min="-2" max="2" step="1" value="0">
            <span id="sharpness-val" class="w-4 text-center">0</span>
          </div>
        </div>

        <div class="control-grid">
          <div class="label">Horizontal Flip</div>
          <select id="hmirror">
            <option value="0">Off</option>
            <option value="1">On</option>
          </select>
        </div>

        <div class="control-grid">
          <div class="label">Vertical Flip</div>
          <select id="vflip">
            <option value="0">Off</option>
            <option value="1">On</option>
          </select>
        </div>

        <div class="control-grid">
          <div class="label">White Balance</div>
          <select id="whitebal">
            <option value="1">Auto</option>
            <option value="0">Off</option>
          </select>
        </div>

        <div class="control-grid">
          <div class="label">Exposure Ctrl</div>
          <select id="exposure">
            <option value="1">Auto</option>
            <option value="0">Off</option>
          </select>
        </div>

        <div class="mt-4">
          <button id="apply-settings" class="btn bg-gray-700 w-full">Apply Settings</button>
          <span class="text-sm text-gray-500 mt-2 block">Settings affect stream and are used for capture.</span>
        </div>
      </div>
    </div>
  </div>

  <div id="toast" class="toast">Saved</div>

  <script>
    const streamBtn = document.getElementById('toggle-stream');
    const captureBtn = document.getElementById('capture-photo');
    const captureTelegramBtn = document.getElementById('capture-telegram');
    const flashBtn = document.getElementById('toggle-flash');
    const flashText = document.getElementById('flash-text');
    const cameraView = document.getElementById('camera-view');
    const applyBtn = document.getElementById('apply-settings');
    const toast = document.getElementById('toast');

    let isStreaming = false;
    const streamUrl = '/stream';
    const captureUrl = '/capture';
    const captureSendUrl = '/capture-send';
    const controlUrl = '/control';
    const flashUrl = '/toggle-flash';

    const controls = {
      framesize: document.getElementById('framesize'),
      brightness: document.getElementById('brightness'),
      contrast: document.getElementById('contrast'),
      saturation: document.getElementById('saturation'),
      sharpness: document.getElementById('sharpness'),
      hmirror: document.getElementById('hmirror'),
      vflip: document.getElementById('vflip'),
      whitebal: document.getElementById('whitebal'),
      exposure: document.getElementById('exposure'),
    };

    ['brightness', 'contrast', 'saturation', 'sharpness'].forEach(id => {
      const el = controls[id];
      const valEl = document.getElementById(id + '-val');
      valEl.textContent = el.value;
      el.addEventListener('input', () => valEl.textContent = el.value);
    });

    function showToast(msg){
      toast.textContent = msg;
      toast.style.display = 'block';
      setTimeout(()=> toast.style.display = 'none', 2500);
    }

    streamBtn.addEventListener('click', () => {
      if (isStreaming) {
        window.stop();
        isStreaming = false;
        streamBtn.innerHTML = '<svg class="btn-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M5.636 5.636a9 9 0 1 0 12.728 0M12 3v9" /></svg> Start Stream';
        cameraView.src = 'https://placehold.co/800x600/222222/ffffff?text=Camera+Offline';
      } else {
        cameraView.src = streamUrl + '?_t=' + Date.now();
        isStreaming = true;
        streamBtn.innerHTML = '<svg class="btn-icon" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" d="M5.636 5.636a9 9 0 1 0 12.728 0M12 3v9" /></svg> Stop Stream';
      }
    });

    async function applySettings() {
      const params = new URLSearchParams();
      for (const key in controls) {
        params.append(key, controls[key].value);
      }

      try {
        const r = await fetch(controlUrl + '?' + params.toString());
        if (r.ok) {
          showToast('Settings applied');
          if (isStreaming) {
            cameraView.src = streamUrl + '?_t=' + Date.now();
          }
        } else {
          showToast('Apply failed');
        }
      } catch (e) {
        showToast('Error applying settings');
      }
    }
    applyBtn.addEventListener('click', applySettings);

    async function captureAndShow(url, toastMsg) {
      if(isStreaming) streamBtn.click();
      try {
        await applySettings();
      } catch(e){}
      
      showToast('Capturing...');
      try {
        const resp = await fetch(url + '?_t=' + Date.now());
        if (!resp.ok) { showToast('Capture failed'); return; }
        const blob = await resp.blob();
        const imgUrl = URL.createObjectURL(blob);
        cameraView.src = imgUrl;
        showToast(toastMsg);
      } catch (e) {
        showToast('Error capturing');
      }
    }

    captureBtn.addEventListener('click', () => {
      captureAndShow(captureUrl, 'Captured!');
    });

    captureTelegramBtn.addEventListener('click', () => {
      captureAndShow(captureSendUrl, 'Captured & Sent!');
    });

    flashBtn.addEventListener('click', async () => {
      try {
        const r = await fetch(flashUrl);
        const data = await r.json();
        if (data.flash === 'on') {
          flashText.textContent = 'Flash: On';
          flashBtn.classList.remove('bg-yellow-600');
          flashBtn.classList.add('bg-yellow-400');
        } else {
          flashText.textContent = 'Flash: Off';
          flashBtn.classList.remove('bg-yellow-400');
          flashBtn.classList.add('bg-yellow-600');
        }
        showToast('Flash ' + data.flash);
      } catch (e) {
        showToast('Error toggling flash');
      }
    });

    window.addEventListener('load', () => {
      applySettings();
    });
  </script>
</body>
</html>
)rawliteral";

// ===============================
// Globals
// ===============================
static httpd_handle_t server = NULL;
static bool flashState = false; 
static unsigned long lastTriggerTime = 0; // NEW: for sensor cooldown

// ===============================
// Helper: map framesize string to framesize_t
// ===============================
framesize_t framesize_from_string(const char* s){
  if (strcmp(s, "FRAMESIZE_QQVGA") == 0) return FRAMESIZE_QQVGA;
  if (strcmp(s, "FRAMESIZE_QVGA") == 0) return FRAMESIZE_QVGA;
  if (strcmp(s, "FRAMESIZE_VGA") == 0) return FRAMESIZE_VGA;
  if (strcmp(s, "FRAMESIZE_SVGA") == 0) return FRAMESIZE_SVGA;
  if (strcmp(s, "FRAMESIZE_XGA") == 0) return FRAMESIZE_XGA;
  if (strcmp(s, "FRAMESIZE_UXGA") == 0) return FRAMESIZE_UXGA;
  return FRAMESIZE_SVGA;
}

// ===============================
// Telegram Send Function
// ===============================
bool sendPhotoToTelegram(camera_fb_t * fb) {
  WiFiClientSecure client;
  client.setInsecure();
  Serial.println("Connecting to Telegram...");

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Connection failed!");
    return false;
  }

  String head = "--boundary\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + chat_id + "\r\n--boundary\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--boundary--\r\n";

  uint32_t imageLen = fb->len;
  uint32_t totalLen = head.length() + imageLen + tail.length();

  String request = "POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1\r\n";
  request += "Host: api.telegram.org\r\n";
  request += "Content-Length: " + String(totalLen) + "\r\n";
  request += "Content-Type: multipart/form-data; boundary=boundary\r\n\r\n";

  client.print(request);
  client.print(head);
  client.write(fb->buf, fb->len);
  client.print(tail);

  String response = client.readString();
  Serial.println("Telegram response:");
  Serial.println(response);
  client.stop();
  return true;
}

// ===============================
// Web Handlers (All unchanged)
// ===============================
static esp_err_t root_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t control_handler(httpd_req_t *req){
  size_t len = httpd_req_get_url_query_len(req) + 1;
  char *buf = (char*)malloc(len);
  if(!buf) return ESP_FAIL;
  httpd_req_get_url_query_str(req, buf, len);
  char val[32];
  sensor_t * s = esp_camera_sensor_get();
  if (httpd_query_key_value(buf, "framesize", val, sizeof(val)) == ESP_OK){s->set_framesize(s, framesize_from_string(val));}
  if (httpd_query_key_value(buf, "brightness", val, sizeof(val)) == ESP_OK){s->set_brightness(s, atoi(val));}
  if (httpd_query_key_value(buf, "contrast", val, sizeof(val)) == ESP_OK){s->set_contrast(s, atoi(val));}
  if (httpd_query_key_value(buf, "saturation", val, sizeof(val)) == ESP_OK){s->set_saturation(s, atoi(val));}
  if (httpd_query_key_value(buf, "sharpness", val, sizeof(val)) == ESP_OK){s->set_sharpness(s, atoi(val));}
  if (httpd_query_key_value(buf, "hmirror", val, sizeof(val)) == ESP_OK){s->set_hmirror(s, atoi(val));}
  if (httpd_query_key_value(buf, "vflip", val, sizeof(val)) == ESP_OK){s->set_vflip(s, atoi(val));}
  if (httpd_query_key_value(buf, "whitebal", val, sizeof(val)) == ESP_OK){s->set_whitebal(s, atoi(val));}
  if (httpd_query_key_value(buf, "exposure", val, sizeof(val)) == ESP_OK){s->set_exposure_ctrl(s, atoi(val));}
  free(buf);
  httpd_resp_set_type(req, "application/json");
  const char* resp = "{\"status\":\"ok\"}";
  httpd_resp_send(req, resp, strlen(resp));
  return ESP_OK;
}

static esp_err_t flash_toggle_handler(httpd_req_t *req){
  flashState = !flashState;
  digitalWrite(LED_GPIO_NUM, flashState ? HIGH : LOW);
  Serial.printf("Flash Toggled -> %s\n", flashState ? "ON" : "OFF");
  httpd_resp_set_type(req, "application/json");
  char resp[64];
  sprintf(resp, "{\"status\":\"ok\", \"flash\":\"%s\"}", flashState ? "on" : "off");
  httpd_resp_send(req, resp, strlen(resp));
  return ESP_OK;
}

static esp_err_t capture_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

static esp_err_t capture_send_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  Serial.println("Sending photo to Telegram...");
  sendPhotoToTelegram(fb);
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len;
  uint8_t * _jpg_buf;
  char part_buf[64];
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  while(true){
    fb = esp_camera_fb_get();
    if (!fb) { res = ESP_FAIL; break; }
    if(fb->format != PIXFORMAT_JPEG){
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb); fb = NULL;
      if(!jpeg_converted){ res = ESP_FAIL; }
    } else { _jpg_buf_len = fb->len; _jpg_buf = fb->buf; }
    if(res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if(res == ESP_OK){ size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len); res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen); }
    if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    if(fb) { esp_camera_fb_return(fb); fb = NULL; } else if(_jpg_buf) { free(_jpg_buf); _jpg_buf = NULL; }
    if(res != ESP_OK) break;
  }
  return res;
}

// ===============================
// Start Server (unchanged)
// ===============================
void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t root_uri = { "/", HTTP_GET, root_handler, NULL };
  httpd_uri_t capture_uri = { "/capture", HTTP_GET, capture_handler, NULL };
  httpd_uri_t stream_uri = { "/stream", HTTP_GET, stream_handler, NULL };
  httpd_uri_t control_uri = { "/control", HTTP_GET, control_handler, NULL };
  httpd_uri_t flash_uri = { "/toggle-flash", HTTP_GET, flash_toggle_handler, NULL };
  httpd_uri_t capture_send_uri = { "/capture-send", HTTP_GET, capture_send_handler, NULL };
  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &root_uri);
    httpd_register_uri_handler(server, &capture_uri);
    httpd_register_uri_handler(server, &stream_uri);
    httpd_register_uri_handler(server, &control_uri);
    httpd_register_uri_handler(server, &flash_uri);
    httpd_register_uri_handler(server, &capture_send_uri);
  }
}

// ===============================
// NEW: Sensor Trigger Logic
// ===============================
void handleSensorTrigger() {
  // Check if enough time has passed since the last trigger
  if (millis() - lastTriggerTime > TRIGGER_COOLDOWN_MS) {
    // Read the sensor pin state
    if (digitalRead(SENSOR_PIN) == HIGH) {
      Serial.println("Sensor triggered! Capturing photo...");
      
      // Update the trigger time IMMEDIATELY to start cooldown
      lastTriggerTime = millis();
      
      // If flash is manually set to OFF, turn it on just for this photo
      bool temporaryFlash = false;
      if (!flashState) {
          temporaryFlash = true;
          digitalWrite(LED_GPIO_NUM, HIGH);
          delay(250); // Small delay for flash to illuminate before capture
      }
      
      // --- Capture and Send Logic ---
      camera_fb_t* fb = esp_camera_fb_get();
      if (fb) {
        sendPhotoToTelegram(fb);
        esp_camera_fb_return(fb); // IMPORTANT: release the frame buffer
      } else {
        Serial.println("Failed to capture photo from sensor trigger.");
      }

      // If we turned the flash on temporarily, turn it back off
      if (temporaryFlash) {
        digitalWrite(LED_GPIO_NUM, LOW);
      }
    }
  }
}

// ===============================
// Setup
// ===============================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Initialize flash pin
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);
  flashState = false;

  // *** NEW *** Initialize sensor pin
  pinMode(SENSOR_PIN, INPUT_PULLDOWN); // Use internal pull-down
  Serial.printf("Sensor trigger enabled on GPIO %d\n", SENSOR_PIN);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM; config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000; config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; config.jpeg_quality = 10; config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; config.jpeg_quality = 12; config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SVGA); s->set_brightness(s, 0); s->set_contrast(s, 0);
  s->set_saturation(s, 0); s->set_sharpness(s, 0); s->set_hmirror(s, 0); s->set_vflip(s, 0);
  s->set_whitebal(s, 1); s->set_exposure_ctrl(s, 1);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400); Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());

  startCameraServer();
  lastTriggerTime = -TRIGGER_COOLDOWN_MS; // Allow immediate trigger after boot
}

// ===============================
// Loop
// ===============================
void loop() {
  // *** NEW ***
  // This now handles the sensor check continuously
  // without blocking the web server.
  handleSensorTrigger();

  // A very small delay to prevent the loop from running too fast
  // and to be friendly to other tasks.
  delay(50);
}
