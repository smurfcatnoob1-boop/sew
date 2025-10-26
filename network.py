# network.py - İstemci Ağ Yöneticisi
import socket

class Network:
    def __init__(self):
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_ip = '127.0.0.1' 
        self.port = 5555
        self.addr = (self.server_ip, self.port)
        self.player_id = self.connect() 

    def connect(self):
        try:
            self.client.connect(self.addr)
            return int(self.client.recv(2048).decode())
        except Exception as e:
            print(f"HATA: Sunucuya bağlanılamadı. {e}")
            return -1

    def send(self, data):
        """Veriyi sunucuya gönderir ve rakibin verisini geri alır."""
        try:
            self.client.send(str.encode(data))
            return self.client.recv(2048).decode()
        except socket.error as e:
            # print(f"Ağ İletişim Hatası: {e}")
            return None
