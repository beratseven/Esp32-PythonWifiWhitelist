from flask import Flask, request, jsonify
import logging # Loglama için kütüphaneyi ekliyoruz

app = Flask(__name__)

# Loglama ayarları
# Basit bir loglama için print() de kullanılabilir, ama bu daha profesyonel.
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# --- GÜVENLİK AYARLARI ---
# ESP32'deki token ile aynı olmalı!
SECRET_TOKEN = "123"

# --- Sunucu Ayarları ---
SERVER_IP = '0.0.0.0'
SERVER_PORT = 5000

@app.route('/alert', methods=['POST'])
def receive_alert():
    # 1. Authorization başlığını kontrol et
    auth_header = request.headers.get('Authorization')
    
    # Başlık yoksa veya format yanlışsa reddet
    if not auth_header or not auth_header.startswith('Bearer '):
        logging.warning(f"Yetkisiz erisim denemesi (Eksik/Hatali Baslik) - IP: {request.remote_addr}")
        return jsonify({"status": "error", "message": "Yetkilendirme basligi eksik veya hatali"}), 401 # 401 Unauthorized

    # 2. Token'ı kontrol et
    received_token = auth_header.split(' ')[1] # "Bearer <token>"'dan token'ı al
    if received_token != SECRET_TOKEN:
        logging.warning(f"Yetkisiz erisim denemesi (Gecersiz Token) - IP: {request.remote_addr}")
        return jsonify({"status": "error", "message": "Gecersiz token"}), 401 # 401 Unauthorized
    
    # --- Yetkilendirme başarılıysa devam et ---
    logging.info(f"Yetkili istek alindi - IP: {request.remote_addr}")

    if request.method == 'POST':
        data = request.json
        if data:
            # Gelen veriyi logla
            event = data.get('event', 'Bilinmiyor')
            mac = data.get('mac', 'Bilinmiyor')
            logging.info(f"Olay: {event}, MAC: {mac}")
            
            # Konsola da yazdıralım
            print(f"\n--- ESP32'den Uyari Alindi ---")
            print(f"Olay: {event}")
            print(f"MAC Adresi: {mac}")
            print(f"-----------------------------")
            
            return jsonify({"status": "success", "message": "Uyari basariyla alindi"}), 200
        else:
            logging.error("Gelen istekte JSON verisi bulunamadi.")
            return jsonify({"status": "error", "message": "Gecersiz JSON verisi"}), 400

if __name__ == '__main__':
    print(f"Guvenli Python Flask Sunucusu baslatiliyor...")
    print(f"Sunucu IP ve Port: http://{SERVER_IP}:{SERVER_PORT}")
    app.run(host=SERVER_IP, port=SERVER_PORT, debug=False)