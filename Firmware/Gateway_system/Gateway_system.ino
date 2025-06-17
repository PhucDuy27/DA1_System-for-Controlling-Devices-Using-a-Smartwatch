#include <WiFi.h>
#include <FirebaseESP32.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// Cấu hình Wi-Fi
const char WIFI_SSID[] PROGMEM = "Dii";
const char WIFI_PASSWORD[] PROGMEM = "duydeptrai";

// Cấu hình Firebase
const char FIREBASE_HOST[] PROGMEM = "https://smartwatchcontrol-d3aa6-default-rtdb.firebaseio.com/";
const char FIREBASE_AUTH[] PROGMEM = "RvD8RRNOf81mHc3rAcC1gW2mhyYjMJpvQalhIDg0";

// Cấu hình Bluetooth
const char SERVICE_UUID[] PROGMEM = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char CHARACTERISTIC_UUID[] PROGMEM = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
BLECharacteristic *pCharacteristic;
String lastIrStatus = "Chưa nhận tín hiệu";

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();
    Serial.println("Nhận từ Đồng hồ qua Bluetooth: " + value);
    if (value == "light_status: on" || value == "light_status: off") {
      String newLightStatus = (value == "light_status: on") ? "on" : "off";
      if (Firebase.ready()) {
        if (Firebase.setString(firebaseData, "/Light_Status", newLightStatus)) {
          Serial.println("Đã gửi Light_Status = " + newLightStatus + " lên Firebase");
        } else {
          Serial.println("Lỗi ghi /Light_Status: " + firebaseData.errorReason());
        }
      } else {
        Serial.println("Firebase không sẵn sàng, bỏ qua cập nhật Light_Status");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Bắt đầu khởi tạo Gateway...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối Wi-Fi!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Địa chỉ MAC của Wi-Fi: ");
  Serial.println(WiFi.macAddress());

  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Đã khởi tạo Firebase!");

  // Reset trạng thái IR khi khởi động
  if (Firebase.setString(firebaseData, "/IR_Status", "Chưa nhận tín hiệu")) {
    Serial.println("Đã đặt /IR_Status: Chưa nhận tín hiệu");
  } else {
    Serial.println("Lỗi đặt /IR_Status: " + firebaseData.errorReason());
  }

  BLEDevice::init("ESP32_C6_Gateway");
  Serial.println("Đã khởi tạo Bluetooth...");
  Serial.print("Địa chỉ MAC Bluetooth: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Đã khởi tạo Bluetooth Server!");
}

void loop() {
  if (Firebase.ready()) {
    if (Firebase.getString(firebaseData, "/IR_Status")) {
      String irData = firebaseData.stringData();
      Serial.println("Nhận từ Firebase: " + irData);
      if (irData != lastIrStatus && irData == "Đã nhận tín hiệu") {
        pCharacteristic->setValue("ir_received");
        pCharacteristic->notify();
        Serial.println("Đã gửi ir_received qua Bluetooth");
        lastIrStatus = irData;
      } else if (irData != lastIrStatus) {
        lastIrStatus = irData;
      }
    } else {
      Serial.println("Lỗi đọc Firebase: " + firebaseData.errorReason());
    }
  } else {
    Serial.println("Firebase không sẵn sàng, kiểm tra kết nối...");
  }
  delay(1000);
}