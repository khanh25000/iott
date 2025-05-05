#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ================== CẤU HÌNH WIFI ================== //
const char* ssid = "KTX MINH CHAU 19-24";        
const char* password = "Mc270579@";     
const char* mqtt_server = "192.168.1.4";

// ================== CẤU HÌNH CHÂN ================== //
#define DHTPIN D1          
#define DHTTYPE DHT11      
#define BUZZER_PIN D0      
#define GAS_SENSOR_PIN A0  

// ================== NGƯỠNG CẢNH BÁO ================== //
const int GAS_THRESHOLD = 300;    
const int TEMP_THRESHOLD = 40;    

// ================== KHỞI TẠO ================== //
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  dht.begin();

  connectWiFi();
  client.setServer(mqtt_server, 1883);
  client.setKeepAlive(60); // Thêm keepalive
}

void loop() {
  // Kiểm tra và kết nối lại WiFi nếu mất kết nối
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Đọc cảm biến
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int gasValue = analogRead(GAS_SENSOR_PIN);

  // Kiểm tra lỗi cảm biến
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Lỗi đọc DHT11!");
    return;
  }

  // Xử lý dữ liệu
  temperature = round(temperature);
  humidity = round(humidity);
  
  // Kiểm tra cảnh báo
  bool gasAlert = (gasValue > GAS_THRESHOLD);
  bool tempAlert = (temperature > TEMP_THRESHOLD);
  bool alert = gasAlert || tempAlert;
  digitalWrite(BUZZER_PIN, alert ? HIGH : LOW);

  // Xuất thông tin serial
  Serial.printf("Nhiệt độ: %d°C | Độ ẩm: %d%% | Gas: %d | Cảnh báo: %s\n", 
                (int)temperature, (int)humidity, gasValue, alert ? "CÓ" : "KHÔNG");

  // Gửi dữ liệu MQTT
  if (client.connected()) {
    client.publish("sensors/temperature", String(temperature).c_str());
    client.publish("sensors/humidity", String(humidity).c_str());
    client.publish("sensors/gas", String(gasValue).c_str());
    client.publish("sensors/alert", alert ? "1" : "0");
  }

  delay(2000);
}

// ================== HÀM KẾT NỐI WIFI ================== //
void connectWiFi() {
  WiFi.disconnect(true);  // Reset kết nối trước đó
  WiFi.mode(WIFI_STA);    // Chế độ station
  
  Serial.print("\nĐang kết nối WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nKết nối WiFi thành công!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ================== HÀM KẾT NỐI LẠI MQTT ================== //
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Đang kết nối MQTT...");
    
    if (client.connect("ESP8266_Gas_System")) {
      Serial.println("Thành công!");
    } else {
      Serial.print("Thất bại, rc=");
      Serial.print(client.state());
      Serial.println(" Thử lại sau 2 giây...");
      delay(2000);
    }
  }
}