# server.py - Basit 2 Kişilik Oyun Sunucusu
import socket
import threading

SERVER_IP = '127.0.0.1'  # Yerel ağ testi için (Ağ üzerinde oynamak için burayı değiştirin!)
PORT = 5555
ADDR = (SERVER_IP, PORT)
# P1: x,y,durum,skin_id | P2: x,y,durum,skin_id
pozisyonlar = ["0,0,IDLE,Kaan", "0,0,IDLE,Kiz"] 

def handle_client(conn, addr, player_id):
    """Her oyuncu için ayrı bir thread'de çalışan fonksiyon."""
    print(f"[BAĞLANDI] Oyuncu {player_id + 1} ({addr}) bağlandı.")
    conn.send(str(player_id).encode()) 
    
    while True:
        try:
            data = conn.recv(2048).decode()
            if not data:
                break
            
            pozisyonlar[player_id] = data 
            rakip_id = 1 if player_id == 0 else 0
            rakip_pozisyon = pozisyonlar[rakip_id]
            conn.send(rakip_pozisyon.encode())
            
        except:
            break
            
    print(f"[BAĞLANTI KOPTU] Oyuncu {player_id + 1} ayrıldı.")
    # pozisyonlar[player_id] = "0,0,IDLE,DEAD" # Örnek: Rakibe öldü bilgisi göndermek
    conn.close()

def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        server.bind(ADDR)
    except socket.error as e:
        print(f"HATA: Sunucu başlatılamadı: {e}")
        return
        
    server.listen(2) 
    print(f"[BAŞLADI] Sunucu {SERVER_IP}:{PORT} adresinde dinliyor...")
    
    current_player = 0
    while current_player < 2:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr, current_player)).start()
        current_player += 1
        print(f"[AKTİF BAĞLANTI] {threading.active_count() - 1} oyuncu")
        
    print("[DOLU] 2 oyuncu bağlandı. Oyun Başlıyor...")

if __name__ == "__main__":
    start_server()
