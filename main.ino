#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_wifi.h" // ESP-IDF Wi-Fi kütüphanesi, istemci listesi için gerekli
const char* SECRET_TOKEN = "123";
// ----- BURAYI KENDİ BİLGİLERİNİZLE DOLDURUN -----

// DEĞİŞTİ: Artık ESP32 kendi ağını oluşturacak. 
// Cihazlarınızı bu ağa bağlayacaksınız.
const char* ssid_ap = "ESP32_Gozlemci_Agi";    // ESP32'nin oluşturacağı Wi-Fi ağının adı
const char* password_ap = "12345678"; // ESP32'nin ağ şifresi (en az 8 karakter)

// DEĞİŞTİ: Sunucuya bağlanmak için ana modeminize de bağlanmanız gerekebilir.
// Bu yüzden orijinal Wi-Fi bilgilerinizi de tutuyoruz.
const char* ssid_station = "***";   // Evdeki ana Wi-Fi ağınızın adı
const char* password_station = "*******"; // Evdeki ana Wi-Fi şifreniz

const char* serverIp = "192.168.1.6"; // Python sunucunuzun çalıştığı bilgisayarın IP adresi
const int serverPort = 5000;

// Ağınızdaki güvenilir cihazların MAC adresleri.
// Bu cihazlar ESP32'nin ağına bağlandığında uyarı verilmeyecek.
String trusted_macs[] = {
  "***********", // Örnek MAC 1 (Telefonunuz)
};
const int trusted_mac_count = sizeof(trusted_macs) / sizeof(trusted_macs[0]);

// Yeni bir cihaz tespit edildiğinde tekrar uyarı göndermemek için
// bilinen yeni cihazları tutan bir liste.
String known_new_devices[20]; 
int known_new_device_count = 0;
// ------------------------------------------------

// Tarama aralığı (milisaniye cinsinden). 10 saniyede bir kontrol edelim.
unsigned long scan_interval = 10000; 
unsigned long last_scan_time = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nAkilli Wi-Fi Gozlemcisi baslatiliyor...");
  
  // YENİ: ESP32'yi hem istemci (sunucuya bağlanmak için) hem de AP (cihazları izlemek için) modunda başlatıyoruz.
  setupWiFi_AP_STA();
}

void loop() {
  // Belirlenen aralıklarla ağı tara
  if (millis() - last_scan_time > scan_interval) {
    last_scan_time = millis();
    // DEĞİŞTİ: Fonksiyonun adı daha açıklayıcı oldu.
    scanForConnectedClients();
  }
}

// YENİ FONKSİYON: ESP32'yi hem AP hem Station modunda başlatır.
void setupWiFi_AP_STA() {
  WiFi.mode(WIFI_AP_STA);

  // Station Modu (İnternete/Sunucuya çıkmak için)
  Serial.print("Ana WiFi'ye baglaniliyor: ");
  Serial.println(ssid_station);
  WiFi.begin(ssid_station, password_station);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nAna WiFi baglantisi basarili!");
  Serial.print("ESP32 IP Adresi (Ana Agda): ");
  Serial.println(WiFi.localIP());

  // Access Point Modu (Cihazların bağlanması için)
  Serial.print("Access Point olusturuluyor: ");
  Serial.println(ssid_ap);
  WiFi.softAP(ssid_ap, password_ap);
  
  Serial.print("AP IP Adresi: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Gozlemci AP basariyla baslatildi. Cihazlarinizi bu aga baglayabilirsiniz.");
}


// TAMAMEN DEĞİŞEN FONKSİYON (YENİ ESP32 SÜRÜMLERİ İLE UYUMLU)
void scanForConnectedClients() {
  Serial.println("\nBagli istemciler taranıyor...");

  // ESP-IDF fonksiyonları ile bağlı istasyonların listesini al
  wifi_sta_list_t sta_list;

  // Listeyi temizle ve bağlı istasyonları al
  esp_wifi_ap_get_sta_list(&sta_list);
  
  int n = sta_list.num; // Bağlı cihaz sayısı doğrudan bu struct içinden gelir

  if (n == 0) {
    Serial.println("ESP32'ye bagli hicbir cihaz yok.");
  } else {
    Serial.print(n);
    Serial.println(" cihaz bagli.");

    // sta_list.sta bir dizidir ve bağlı her cihaz için bilgi içerir
    for (int i = 0; i < n; i++) {
      wifi_sta_info_t sta = sta_list.sta[i];
      
      // MAC adresini al ve okunabilir bir string'e formatla
      char mac_str[18];
      sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
              sta.mac[0], sta.mac[1], sta.mac[2],
              sta.mac[3], sta.mac[4], sta.mac[5]);
      String current_mac = String(mac_str);
      
      // Bu MAC adresi güvenilir listesinde mi?
      if (!isMacTrusted(current_mac)) {
        // Eğer güvenilir değilse, daha önce rapor edilmiş mi?
        if (!isAlreadyKnownNewDevice(current_mac)) {
          Serial.print("!!! YENI/GUVENILMEYEN CIHAZ TESPIT EDILDI: ");
          Serial.println(current_mac);
          
          // Sunucuya uyarı gönder
          sendAlert("NEW_DEVICE_DETECTED", current_mac);
          
          // Raporlananlar listesine ekle
          if (known_new_device_count < 20) {
            known_new_devices[known_new_device_count] = current_mac;
            known_new_device_count++;
          }
        }
      }
    }
  }
}

// BU FONKSİYONLAR AYNI KALIYOR
bool isMacTrusted(String mac) {
  for (int i = 0; i < trusted_mac_count; i++) {
    if (trusted_macs[i] == mac) {
      return true;
    }
  }
  return false;
}

bool isAlreadyKnownNewDevice(String mac) {
  for (int i = 0; i < known_new_device_count; i++) {
    if (known_new_devices[i] == mac) {
      return true;
    }
  }
  return false;
}

void sendAlert(String event, String mac) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String serverUrl = "http://" + String(serverIp) + ":" + String(serverPort) + "/alert";
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // YENİ EKLENEN SATIRLAR: Authorization başlığını ekliyoruz.
    // "Bearer" standardı, token tabanlı kimlik doğrulama için yaygın bir yöntemdir.
    String authHeader = "Bearer " + String(SECRET_TOKEN);
    http.addHeader("Authorization", authHeader);
    
    // JSON verisini oluştur
    String payload = "{\"event\": \"" + event + "\", \"mac\": \"" + mac + "\"}";
    
    Serial.print("Sunucuya gonderiliyor: ");
    Serial.println(payload);

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Yanit Kodu: ");
      Serial.println(httpResponseCode);
      Serial.print("Yanit: ");
      Serial.println(response);
    } else {
      Serial.print("HTTP Istegi Hata Kodu: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Uyari gonderilemedi, WiFi baglantisi yok.");
  }
}