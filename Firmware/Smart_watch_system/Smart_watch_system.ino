#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <IRremote.hpp>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// Cấu hình Bluetooth
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// Chân kết nối
#define IR_PIN 16
#define SOUND_PIN 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Khởi tạo hiển thị
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Bluetooth
static BLEAddress *pServerAddress;
static BLEClient* pClient;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static boolean doConnect = false;
static boolean connected = false;
bool irReceived = false;
bool gestureEnabled = false;
bool gesturePerformed = false;
bool gatewayReady = false;

// Trạng thái búng tay
bool fingerSnapped = false;
int snapCount = 0;
const int soundThreshold = 1000;
int lastSoundValue = 0;
unsigned long lastSnapTime = 0;
const unsigned long snapDebounce = 500;
const unsigned long snapWindow = 1500;

// Biến thời gian
unsigned long lastTime = 0;
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000;
unsigned long lastDebugTime = 0;
const unsigned long debugInterval = 5000;

// Biến cho hiệu ứng hiển thị
unsigned long lastBlinkTime = 0;
bool blinkState = false;
const unsigned long blinkInterval = 500;

// Biến trạng thái đèn
bool isLightOn = false;

// Biểu tượng Bluetooth (16x16)
const uint8_t bluetoothIcon[] PROGMEM = {
  0b00000000, 0b00000000, // Hàng 1: 00000000 00000000
  0b00000000, 0b00000000, // Hàng 2: 00000000 00000000
  0b00000001, 0b00000001, // Hàng 3: 00000001 00000001
  0b00000010, 0b00000010, // Hàng 4: 00000010 00000010
  0b00000100, 0b00000100, // Hàng 5: 00000100 00000100
  0b00001000, 0b00001000, // Hàng 6: 00001000 00001000
  0b00010000, 0b00010000, // Hàng 7: 00010000 00010000
  0b00100001, 0b10000001, // Hàng 8: 00100001 10000001
  0b00100001, 0b10000001, // Hàng 9: 00100001 10000001
  0b00010000, 0b00010000, // Hàng 10: 00010000 00010000
  0b00001000, 0b00001000, // Hàng 11: 00001000 00001000
  0b00000100, 0b00000100, // Hàng 12: 00000100 00000100
  0b00000010, 0b00000010, // Hàng 13: 00000010 00000010
  0b00000001, 0b00000001, // Hàng 14: 00000001 00000001
  0b00000000, 0b00000000, // Hàng 15: 00000000 00000000
  0b00000000, 0b00000000  // Hàng 16: 00000000 00000000
};

// Biểu tượng đèn sáng (16x16)
const uint8_t lightOnIcon[] PROGMEM = {
  0b00011111, 0b11111000,
  0b01111111, 0b11111110,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b01111111, 0b11111110,
  0b00111111, 0b11111100,
  0b00111111, 0b11111100,
  0b00011111, 0b11111000,
  0b00001111, 0b11110000,
  0b00001111, 0b11110000,
  0b00011111, 0b11111000,
  0b00111111, 0b11111100,
  0b00000000, 0b00000000
};

// Biểu tượng đèn tắt (16x16)
const uint8_t lightOffIcon[] PROGMEM = {
  0b00111111, 0b11111100,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b01100000, 0b00000110,
  0b00110000, 0b00001100,
  0b00110000, 0b00001100,
  0b00011000, 0b00011000,
  0b00001111, 0b11110000,
  0b00001111, 0b11110000,
  0b00011111, 0b11111000,
  0b00111111, 0b11111100,
  0b00000000, 0b00000000
};

const unsigned char epd_bitmap_ute__1_[] PROGMEM = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0x7f,
  0xff, 0xff, 0xc0, 0xff, 0xff, 0xfe, 0x3f, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xfe, 0xdf, 0xff, 0xff,
  0xc0, 0xff, 0xff, 0xfc, 0xdf, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xfc, 0xcf, 0xff, 0xff, 0xc0, 0xfe,
  0x1f, 0xfe, 0x8f, 0xfe, 0x1f, 0xc0, 0xfc, 0x3f, 0xfc, 0xcf, 0xfe, 0x0f, 0xc0, 0xfc, 0x3f, 0xff,
  0xff, 0xff, 0x0f, 0xc0, 0xf8, 0x7f, 0xff, 0xff, 0xff, 0x87, 0xc0, 0xf0, 0xff, 0xff, 0xff, 0xff,
  0xc3, 0xc0, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xc0, 0xe1, 0xff, 0xff, 0x3f, 0xff, 0xe1, 0xc0,
  0xe3, 0xff, 0xfe, 0x1f, 0xff, 0xf1, 0xc0, 0xe3, 0xff, 0xf8, 0x07, 0xff, 0xf1, 0xc0, 0xc3, 0xff,
  0xfb, 0x37, 0xff, 0xf0, 0xc0, 0xc7, 0xff, 0xf3, 0x37, 0xff, 0xf8, 0xc0, 0xc7, 0xdf, 0xf7, 0x33,
  0xfe, 0xf8, 0xc0, 0xc7, 0x7f, 0xf7, 0x37, 0xff, 0xb8, 0xc0, 0xc7, 0xcf, 0xfb, 0x37, 0xfc, 0xf8,
  0xc0, 0x87, 0x3f, 0xf9, 0x27, 0xff, 0x38, 0xc0, 0x87, 0xc7, 0xfc, 0x1f, 0xf8, 0xf8, 0x40, 0x87,
  0x3f, 0xff, 0x3f, 0xff, 0x38, 0x40, 0x87, 0xe7, 0xff, 0x3f, 0xf9, 0xf8, 0xc0, 0xc7, 0x1d, 0xff,
  0xff, 0xee, 0x38, 0xc0, 0xc6, 0xe3, 0xff, 0xff, 0xf1, 0xd8, 0xc0, 0xc7, 0x84, 0xff, 0xff, 0xc8,
  0x78, 0xc0, 0xc6, 0x00, 0x1f, 0xff, 0x00, 0x18, 0xc0, 0xc3, 0x80, 0x0f, 0xfc, 0x00, 0x70, 0xc0,
  0xe3, 0xf0, 0x07, 0xf8, 0x03, 0xf1, 0xc0, 0xe3, 0xfe, 0x07, 0xf8, 0x1f, 0xe1, 0xc0, 0xe1, 0xfe,
  0x07, 0xf8, 0x1f, 0xe3, 0xc0, 0xf0, 0xfc, 0x07, 0xf8, 0x0f, 0xc3, 0xc0, 0xf8, 0xf8, 0x07, 0xf8,
  0x07, 0xc7, 0xc0, 0xf8, 0x78, 0x07, 0xf8, 0x07, 0x87, 0xc0, 0xfc, 0x38, 0x0f, 0xfc, 0x07, 0x0f,
  0xc0, 0xfe, 0x18, 0x0f, 0xfc, 0x06, 0x1f, 0xc0, 0xfe, 0x08, 0x0f, 0xfc, 0x04, 0x1f, 0xc0, 0xff,
  0x06, 0x0f, 0xfc, 0x18, 0x3f, 0xc0, 0xff, 0x81, 0x8f, 0xfc, 0x60, 0x7f, 0xc0, 0xff, 0xe0, 0x7f,
  0xff, 0x81, 0xff, 0xc0, 0xff, 0xf0, 0x0f, 0xfc, 0x03, 0xff, 0xc0, 0xff, 0xfc, 0x00, 0x00, 0x0f,
  0xff, 0xc0, 0xff, 0xff, 0x00, 0x00, 0x3f, 0xff, 0xc0, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xc0,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x39, 0xe3, 0x8e, 0x33, 0xb0, 0x30,
  0x00, 0x39, 0x81, 0x8e, 0x33, 0xa0, 0x30, 0x00, 0x39, 0x99, 0x84, 0x33, 0xbc, 0xf3, 0xc0, 0x19,
  0x9f, 0x84, 0x33, 0xbc, 0xf3, 0xc0, 0x01, 0x1f, 0x85, 0x33, 0xbc, 0xf0, 0xc0, 0x01, 0x1f, 0x91,
  0x33, 0xbc, 0xf0, 0xc0, 0x39, 0x9d, 0x91, 0x33, 0xbc, 0xf3, 0xc0, 0x39, 0x9d, 0x93, 0x33, 0x3c,
  0xf3, 0xc0, 0x39, 0x81, 0x93, 0x30, 0x3c, 0xf0, 0x00, 0x39, 0xc3, 0xbb, 0x38, 0x7c, 0xf0, 0x00
};

// Array of all bitmaps for convenience
const int epd_bitmap_allArray_LEN = 1;
const unsigned char* epd_bitmap_allArray[1] = {
  epd_bitmap_ute__1_
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.println("Connected!");
    display.setTextSize(1);
    display.setCursor(10, 40);
    display.println("Waiting for IR...");
    display.drawBitmap(100, 10, bluetoothIcon, 16, 16, SSD1306_WHITE); // Thay connectedIcon bằng bluetoothIcon
    display.display();
    Serial.println("Kết nối thành công với Gateway!");
    if (pRemoteCharacteristic) {
      pRemoteCharacteristic->writeValue("request_status", strlen("request_status"));
      Serial.println("Đã gửi yêu cầu trạng thái đến Gateway.");
    }
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    gatewayReady = false;
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.println("Disconnected!");
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    Serial.println("Ngắt kết nối với Gateway!");
    irReceived = false;
    gestureEnabled = false;
    gesturePerformed = false;
  }
};

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String receivedData = String((char*)pData).substring(0, length);
  Serial.println("DEBUG: Nhận từ Gateway raw data: " + String((char*)pData) + ", Length: " + String(length));
  Serial.println("DEBUG: Nhận từ Gateway processed: " + receivedData);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Received:");
  display.setCursor(0, 16);
  display.println(receivedData);
  if (receivedData == "gateway_ready") {
    gatewayReady = true;
    display.setCursor(0, 32);
    display.println("Gateway Ready!");
    Serial.println("Gateway đã sẵn sàng!");
  } else if (receivedData == "ir_received") {
    irReceived = true;
    gestureEnabled = true;
    gesturePerformed = false;
    display.setCursor(0, 32);
    display.println("Ready for snap...");
    display.drawCircle(100, 0, 5, SSD1306_WHITE);
    Serial.println("IR đã được xác nhận từ Gateway, irReceived set to true");
  } else if (receivedData == "ir_not_received") {
    irReceived = false;
    gestureEnabled = false;
    gesturePerformed = false;
    display.setCursor(0, 32);
    display.println("IR Not Received!");
    Serial.println("IR chưa được nhận từ Gateway, irReceived set to false");
  } else if (receivedData.startsWith("light_status:")) {
    display.setCursor(0, 32);
    display.println("Light Status Updated!");
    Serial.println("Trạng thái đèn đã được cập nhật!");
  }
  display.display();
}

bool connectToServer() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to");
  display.setCursor(0, 16);
  display.println("Gateway...");
  display.drawRect(0, 32, 80, 8, SSD1306_WHITE);
  display.fillRect(0, 32, 40, 8, SSD1306_WHITE);
  display.display();
  Serial.println("Đang kết nối tới Gateway với MAC: " + pServerAddress->toString());

  if (!pClient->connect(*pServerAddress)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Connection");
    display.setCursor(0, 16);
    display.println("Failed!");
    display.setCursor(0, 32);
    display.println("Check Gateway");
    display.display();
    Serial.println("Kết nối thất bại! Kiểm tra MAC hoặc Gateway.");
    return false;
  }
  Serial.println("Đã gửi yêu cầu kết nối.");

  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Service not");
    display.setCursor(0, 16);
    display.println("found!");
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    Serial.println("Không tìm thấy dịch vụ UUID: " + String(SERVICE_UUID));
    pClient->disconnect();
    return false;
  }
  Serial.println("Đã tìm thấy dịch vụ.");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Characteristic");
    display.setCursor(0, 16);
    display.println("not found!");
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    Serial.println("Không tìm thấy đặc trưng UUID: " + String(CHARACTERISTIC_UUID));
    pClient->disconnect();
    return false;
  }
  Serial.println("Đã tìm thấy đặc trưng.");

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Notifications");
    display.setCursor(0, 16);
    display.println("Enabled!");
    display.drawBitmap(100, 0, bluetoothIcon, 16, 16, SSD1306_WHITE); // Thay connectedIcon bằng bluetoothIcon
    display.display();
    Serial.println("Đã đăng ký thông báo từ Gateway.");
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA = GPIO 21, SCL = GPIO 22
  Wire.setClock(100000); // Tốc độ 100kHz cho OLED
  Serial.println("Đã khởi tạo I2C (SDA: GPIO 21, SCL: GPIO 22).");

  // Khởi tạo OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Không tìm thấy OLED! Kiểm tra kết nối hoặc địa chỉ I2C.");
    while (1) delay(1000);
  }
  display.clearDisplay();

  // Hiển thị bitmap 'ute (1)' khi khởi động
  display.drawBitmap(39, 0, epd_bitmap_ute__1_, 50, 64, SSD1306_WHITE); // Căn giữa (x = 39, y = 0)
  display.display();
  delay(2000); // Hiển thị bitmap trong 2 giây

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Watch");
  display.setCursor(0, 16);
  display.println("Starting...");
  display.drawRect(0, 32, SCREEN_WIDTH, 8, SSD1306_WHITE);
  display.fillRect(0, 32, SCREEN_WIDTH / 2, 8, SSD1306_WHITE);
  display.display();
  Serial.println("Bắt đầu khởi tạo Đồng hồ...");
  delay(1000);

  // Khởi tạo IR Sender
  IrSender.begin(IR_PIN);
  Serial.println("Đã khởi tạo IR Sender.");

  // Khởi tạo cảm biến âm thanh
  pinMode(SOUND_PIN, INPUT);
  Serial.println("Đã cấu hình cảm biến âm thanh.");

  // Khởi tạo Bluetooth
  BLEDevice::init("ESP32_Watch");
  Serial.println("Đã khởi tạo Bluetooth với tên ESP32_Watch.");
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("BLE Init...");
  display.drawBitmap(100, 16, bluetoothIcon, 16, 16, SSD1306_WHITE); // Thay connectedIcon bằng bluetoothIcon
  display.display();

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10);
  Serial.println("Đã quét Bluetooth. Danh sách thiết bị:");
  BLEScanResults* foundDevices = pBLEScan->getResults();
  for (int i = 0; i < foundDevices->getCount(); i++) {
    String deviceInfo = String("Tên: ") + String(foundDevices->getDevice(i).getName().c_str()) +
                        String(", MAC: ") + String(foundDevices->getDevice(i).getAddress().toString().c_str());
    Serial.println(deviceInfo);
  }

  // Tìm Gateway dựa trên tên
  bool foundGateway = false;
  for (int i = 0; i < foundDevices->getCount(); i++) {
    if (foundDevices->getDevice(i).getName() == "ESP32_C6_Gateway") {
      pServerAddress = new BLEAddress(foundDevices->getDevice(i).getAddress());
      Serial.println("Đã tìm thấy Gateway với MAC: " + pServerAddress->toString());
      foundGateway = true;
      break;
    }
  }
  if (!foundGateway) {
    Serial.println("Không tìm thấy Gateway! Sử dụng MAC mặc định.");
    pServerAddress = new BLEAddress("40:4c:ca:40:29:9a");
  }
  pBLEScan->clearResults();

  doConnect = true;

  Serial.println("Khởi động xong, kiểm tra IR từ Gateway...");
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      doConnect = false;
      connected = true;
      Serial.println("Kết nối Bluetooth thành công!");
    } else {
      Serial.println("Kết nối thất bại. Thử lại sau 5 giây...");
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 10);
      display.println("Failed!");
      display.setTextSize(1);
      display.setCursor(10, 40);
      display.println("Retry in 5s...");
      display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      display.display();
      delay(5000);
      doConnect = true;
    }
  }

  if (!connected && !doConnect) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= reconnectInterval) {
      Serial.println("Mất kết nối, thử kết nối lại...");
      doConnect = true;
      lastReconnectAttempt = now;
    }
  }

  unsigned long currentTime = millis();
  if (currentTime - lastDebugTime >= debugInterval) {
    Serial.println("Trạng thái: connected=" + String(connected) + ", irReceived=" + String(irReceived) + ", gestureEnabled=" + String(gestureEnabled) + ", isLightOn=" + String(isLightOn));
    lastDebugTime = currentTime;
  }

  // Xử lý búng tay khi đã kết nối và nhận IR
  if (connected && irReceived && gestureEnabled) {
    // Phát hiện búng tay
    int soundValue = analogRead(SOUND_PIN);
    if (currentTime - lastDebugTime >= debugInterval) {
      Serial.println("Debug Sound - Sound Value: " + String(soundValue));
    }
    if (!fingerSnapped && soundValue > soundThreshold && abs(soundValue - lastSoundValue) > 500 && (millis() - lastSnapTime > snapDebounce)) {
      Serial.println("Phát hiện búng tay! Giá trị: " + String(soundValue) + ", Time since last: " + String(millis() - lastSnapTime));
      fingerSnapped = true;
      snapCount++;
      lastSnapTime = millis();

      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(10, 10);
      display.println("Snap!");
      display.setTextSize(1);
      display.setCursor(10, 40);
      display.println("Count: " + String(snapCount));
      display.drawBitmap(100, 10, bluetoothIcon, 16, 16, SSD1306_WHITE); // Thay connectedIcon bằng bluetoothIcon
      display.display();
    }
    lastSoundValue = soundValue;

    // Xử lý lệnh bật/tắt đèn
    if (fingerSnapped) {
      String lightCommand;
      // Chỉ xử lý nếu búng tay 1 lần và sau snapWindow
      if (snapCount == 1 && (currentTime - lastSnapTime > snapWindow)) {
        Serial.println("Processing snap after window: " + String(currentTime - lastSnapTime));
        if (!isLightOn) {
          // Bật đèn nếu đèn đang tắt
          lightCommand = "light_status: on";
          isLightOn = true;
          display.clearDisplay();
          // Hiệu ứng nhấp nháy cho chữ "Light ON"
          for (int i = 0; i < 3; i++) {
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(16, 10);
            display.println("Light ON");
            display.drawBitmap(56, 30, lightOnIcon, 16, 16, SSD1306_WHITE);
            display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 8, SSD1306_WHITE);
            display.display();
            delay(200);
            display.setTextColor(SSD1306_BLACK);
            display.setCursor(16, 10);
            display.println("Light ON");
            display.display();
            delay(200);
          }
          // Hiển thị trạng thái cuối cùng
          display.clearDisplay();
          display.setTextSize(2);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(16, 10);
          display.println("Light ON");
          display.drawBitmap(56, 30, lightOnIcon, 16, 16, SSD1306_WHITE);
          display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 8, SSD1306_WHITE);
          display.display();
        } else {
          // Tắt đèn nếu đèn đang bật
          lightCommand = "light_status: off";
          isLightOn = false;
          display.clearDisplay();
          // Hiệu ứng nhấp nháy cho chữ "Light OFF"
          for (int i = 0; i < 3; i++) {
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(16, 10);
            display.println("Light OFF");
            display.drawBitmap(56, 30, lightOffIcon, 16, 16, SSD1306_WHITE);
            display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 8, SSD1306_WHITE);
            display.display();
            delay(200);
            display.setTextColor(SSD1306_BLACK);
            display.setCursor(16, 10);
            display.println("Light OFF");
            display.display();
            delay(200);
          }
          // Hiển thị trạng thái cuối cùng
          display.clearDisplay();
          display.setTextSize(2);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(16, 10);
          display.println("Light OFF");
          display.drawBitmap(56, 30, lightOffIcon, 16, 16, SSD1306_WHITE);
          display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 8, SSD1306_WHITE);
          display.display();
        }
        // Reset trạng thái búng tay, giữ irReceived và gestureEnabled
        snapCount = 0;
        fingerSnapped = false;
        Serial.println("State after command: irReceived=" + String(irReceived) + ", gestureEnabled=" + String(gestureEnabled) + ", isLightOn=" + String(isLightOn));
      }

      // Gửi lệnh qua Bluetooth
      if (!lightCommand.isEmpty()) {
        pRemoteCharacteristic->writeValue(lightCommand.c_str(), lightCommand.length());
        Serial.println("Đã gửi " + lightCommand + " qua Bluetooth, SnapCount: " + String(snapCount));
        gesturePerformed = true;
      }
    }
  }

  // Hiệu ứng nhấp nháy cho trạng thái kết nối
  if (connected && irReceived) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= blinkInterval) {
      blinkState = !blinkState;
      lastBlinkTime = currentTime;
      display.drawBitmap(100, 0, bluetoothIcon, 16, 16, blinkState ? SSD1306_WHITE : SSD1306_BLACK); // Thay connectedIcon bằng bluetoothIcon
      display.display();
    }
  }

  delay(50);
}