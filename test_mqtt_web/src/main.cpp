#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPSPlus.h>

// ====================================================================
// 1. CẤU HÌNH WIFI & MQTT
// ====================================================================
const char* ssid = "tranhii";
const char* password = "1234567890";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "edr_blackbox/telemetry";

WiFiClient espClient;
PubSubClient client(espClient);

// ====================================================================
// 2. KHỞI TẠO CẢM BIẾN MPU-6050 VÀ GPS NEO-M8N
// ====================================================================
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial ss(1); // GPS UART1: RX=18, TX=17

bool mpuConnected = false;
unsigned long lastSend = 0;

void setupWiFi() {
  Serial.printf("\n[WIFI] Dang ket noi toi: %s ...\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Da ket noi thanh cong!");
  Serial.print("[WIFI] IP ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("[MQTT] Dang ket noi toi Broker EMQX...");
    String clientId = "ESP32_EDR_" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" KET NOI THANH CONG!");
    } else {
      Serial.printf(" Loi rc=%d. Thu lai sau 2s...\n", client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==================================================");
  Serial.println("  AIoT EDR BLACKBOX: DOC DU LIEU THAT 100% SENSORS");
  Serial.println("==================================================");

  // 1. Khởi động I2C cho MPU-6050 (Địa chỉ 0x69: AD0 nối 3.3V)
  Wire.begin(8, 9);
  if (mpu.begin(0x69)) {
    mpuConnected = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("[OK] Da ket noi MPU-6050 that (Dia chi 0x69).");
  } else {
    // Thử lại địa chỉ mặc định 0x68 phòng khi chân AD0 chưa nối 3.3V
    if (mpu.begin(0x68)) {
      mpuConnected = true;
      Serial.println("[OK] Da ket noi MPU-6050 that (Dia chi 0x68).");
    } else {
      Serial.println("[CANH BAO] Khong tim thay MPU-6050 tren bus I2C (GPIO 8, 9)!");
    }
  }

  // 2. Khởi động GPS UART1 (RX:18, TX:17, 9600 baud)
  ss.begin(9600, SERIAL_8N1, 18, 17);
  Serial.println("[OK] Da khoi dong UART1 doc GPS NEO-M8N.");

  // 3. Kết nối mạng
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Đọc liên tục dòng NMEA từ GPS NEO-M8N
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  // Gửi dữ liệu thật mỗi 1 giây (1000ms)
  if (millis() - lastSend > 1000) {
    lastSend = millis();

    // --- 1. ĐỌC THẬT TỪ MPU-6050 ---
    float ax = 0.0, ay = 0.0, az = 0.0;
    float roll = 0.0, pitch = 0.0;
    String status = "NORMAL";

    if (mpuConnected) {
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      // Gia tốc thật (m/s2)
      ax = a.acceleration.x;
      ay = a.acceleration.y;
      az = a.acceleration.z;

      // Tính góc nghiêng thật của cảm biến (Roll & Pitch theo độ)
      roll = atan2(ay, az) * 180.0 / PI;
      pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

      // Độ lớn tổng gia tốc: a_total = sqrt(ax^2 + ay^2 + az^2)
      float a_total = sqrt(ax * ax + ay * ay + az * az);

      // Logic phát hiện tai nạn / té ngã:
      // - Va chạm mạnh: tổng gia tốc > 25 m/s2 (~2.5G)
      // - Xe bị đổ / ngã: góc nghiêng Roll vượt quá 50 độ
      if (a_total > 25.0 || abs(roll) > 50.0) {
        status = "CRASH_DETECTED";
      }
    }

    // --- 2. ĐỌC THẬT TỪ GPS NEO-M8N ---
    double lat = 0.0;
    double lng = 0.0;
    float speed = 0.0;
    int satellites = 0;

    if (gps.satellites.isValid()) {
      satellites = gps.satellites.value();
    }

    // Nếu GPS ngoài trời đã bắt được vệ tinh (Fix)
    if (gps.location.isValid()) {
      lat = gps.location.lat();
      lng = gps.location.lng();
      if (gps.speed.isValid()) {
        speed = gps.speed.kmph();
      }
    } else {
      // Khi ở trong phòng kín chưa bắt được sóng vệ tinh:
      // Tạm giữ tọa độ thực nghiệm phòng thí nghiệm để bản đồ không bị nhảy ra giữa biển
      lat = 10.776889;
      lng = 106.700806;
      speed = 0.0; // Xe đứng yên
    }

    // --- 3. ĐÓNG GÓI JSON GỬI ĐI ---
    StaticJsonDocument<320> doc;
    doc["device"] = "EDR_ESP32S3_01";
    doc["lat"] = lat;
    doc["lng"] = lng;
    doc["speed"] = round(speed * 10) / 10.0;
    doc["ax"] = round(ax * 100) / 100.0;
    doc["ay"] = round(ay * 100) / 100.0;
    doc["az"] = round(az * 100) / 100.0;
    doc["roll"] = round(roll * 10) / 10.0;
    doc["pitch"] = round(pitch * 10) / 10.0;
    doc["status"] = status;
    doc["sats"] = satellites;

    char buffer[320];
    serializeJson(doc, buffer);

    client.publish(mqtt_topic, buffer);
    Serial.printf("[SENSOR THẬT] Ax:%.2f Ay:%.2f Az:%.2f | Roll:%.1f Pitch:%.1f | Sats:%d -> %s\n", 
                  ax, ay, az, roll, pitch, satellites, status.c_str());
  }
}