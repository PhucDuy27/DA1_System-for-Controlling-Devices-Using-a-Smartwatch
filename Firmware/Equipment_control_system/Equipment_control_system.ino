#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

// Cấu hình Wi-Fi
const char* WIFI_SSID = "Dii";
const char* WIFI_PASSWORD = "duydeptrai";

// Cấu hình Firebase
const char* FIREBASE_HOST = "https://smartwatchcontrol-d3aa6-default-rtdb.firebaseio.com/";
const char* FIREBASE_AUTH = "RvD8RRNOf81mHc3rAcC1gW2mhyYjMJpvQalhIDg0";

// Cấu hình chân
#define IR_PIN D5    // GPIO 5: Chân nhận tín hiệu hồng ngoại
#define RELAY_PIN D1 // GPIO 1: Chân điều khiển relay

IRrecv irrecv(IR_PIN);
decode_results results;

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
String lastLightStatus = "off";
bool irReceived = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Bắt đầu khởi tạo ESP8266...");

  // Kết nối Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối Wi-Fi!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());

  // Cấu hình Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Đã khởi tạo Firebase!");

  if (Firebase.getString(firebaseData, "/Light_Status")) {
    Serial.println("Kết nối Firebase thành công, giá trị ban đầu: " + firebaseData.stringData());
  } else {
    Serial.println("Kết nối Firebase thất bại: " + firebaseData.errorReason());
  }

  // Đặt trạng thái ban đầu trên Firebase
  if (Firebase.setString(firebaseData, "/IR_Status", "Chưa nhận tín hiệu")) {
    Serial.println("Đã đặt /IR_Status: Chưa nhận tín hiệu");
  }
  if (Firebase.setString(firebaseData, "/Light_Status", "off")) {
    Serial.println("Đã đặt /Light_Status: off");
  }

  // Khởi tạo IR Receiver
  irrecv.enableIRIn();
  Serial.println("Đã khởi tạo IR Receiver.");

  // Khởi tạo Relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Đã khởi tạo Relay.");
}

void loop() {
  // Nhận bất kỳ tín hiệu IR nào
  if (!irReceived && irrecv.decode(&results)) {
    Serial.print("Đã nhận tín hiệu IR - Protocol: ");
    Serial.print(resultToHumanReadableBasic(&results));
    Serial.print(", Giá trị: 0x");
    Serial.println(results.value, HEX);

    if (Firebase.ready()) {
      if (Firebase.setString(firebaseData, "/IR_Status", "Đã nhận tín hiệu")) {
        Serial.println("Đã cập nhật /IR_Status: Đã nhận tín hiệu");
        irReceived = true; // Chỉ cập nhật IR một lần cho đến khi reset
      } else {
        Serial.println("Lỗi cập nhật /IR_Status: " + firebaseData.errorReason());
      }
    }
    irrecv.resume(); // Tiếp tục nhận tín hiệu IR
  }

  // Đọc trạng thái đèn từ Firebase
  if (Firebase.ready()) {
    if (Firebase.getString(firebaseData, "/Light_Status")) {
      String lightStatus = firebaseData.stringData();
      if (lightStatus != lastLightStatus) {
        Serial.println("Nhận từ Firebase: Light_Status = " + lightStatus);
        if (lightStatus == "on") {
          digitalWrite(RELAY_PIN, HIGH); // Bật relay (bật đèn)
          Serial.println("Đã bật đèn");
        } else if (lightStatus == "off") {
          digitalWrite(RELAY_PIN, LOW);  // Tắt relay (tắt đèn)
          Serial.println("Đã tắt đèn");
        }
        lastLightStatus = lightStatus;
      }
    } else {
      Serial.println("Lỗi đọc /Light_Status: " + firebaseData.errorReason());
    }
  } else {
    Serial.println("Firebase không sẵn sàng, kiểm tra kết nối...");
  }

  delay(1000);
}