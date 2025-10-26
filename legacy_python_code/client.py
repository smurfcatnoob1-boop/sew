# client.py - Sevgili Oyunu Multiplayer İstemci (3000 Satır Hedef)
import pygame
import sys
from network import Network
from karakter import KarakterErkek, KarakterKiz, Karakter
from renderer import Renderer, GrafikAyarlari

# Sabitler
GENISLIK = 1024
YUKSEKLIK = 768
FPS = 60
YER_CEKIMI = 0.8
ZEMIN_SEVIYESI = YUKSEKLIK - 100 

class ChatManager:
    """Oyun içi sohbet sistemini yönetir."""
    def __init__(self, max_lines=10):
        self.mesajlar = []
        self.max_lines = max_lines
        self.girdi = ""
        self.aktif = False
        self.font = pygame.font.Font(None, 24)

    def add_mesaj(self, kimden, metin):
        """Yeni mesajı listeye ekler."""
        if len(self.mesajlar) >= self.max_lines:
            self.mesajlar.pop(0) # Eskiyi sil
        self.mesajlar.append(f"[{kimden}]: {metin}")

    def handle_girdi(self, event):
        """Klavye olaylarını işler."""
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_t: # T tuşu ile chat'i aç/kapat
                self.aktif = not self.aktif
                return
            
            if not self.aktif:
                return

            if event.key == pygame.K_RETURN:
                if self.girdi:
                    # Gönderim burada gerçekleşecek (karakterin mesaj alanına yazılacak)
                    mesaj_gonder = self.girdi
                    self.girdi = ""
                    self.aktif = False
                    return mesaj_gonder
            
            elif event.key == pygame.K_BACKSPACE:
                self.girdi = self.girdi[:-1]
            
            else:
                self.girdi += event.unicode
        return None

    def draw(self, ekran):
        """Sohbet kutusunu ve mesajları çizer."""
        x, y, w, h = 50, YUKSEKLIK - 250, GENISLIK - 100, 200
        
        # Arka plan
        s = pygame.Surface((w, h), pygame.SRCALPHA)
        s.fill((0, 0, 0, 150)) # Yarı saydam siyah
        ekran.blit(s, (x, y))

        # Mesajları çiz
        line_y = y + 10
        for mesaj in self.mesajlar:
            text_yuzey = self.font.render(mesaj, True, (255, 255, 255))
            ekran.blit(text_yuzey, (x + 10, line_y))
            line_y += 20
        
        # Girdi kutusunu çiz
        if self.aktif:
            pygame.draw.rect(ekran, (200, 200, 200), (x, YUKSEKLIK - 50, w, 40), 0)
            girdi_text = self.font.render("> " + self.girdi, True, (0, 0, 0))
            ekran.blit(girdi_text, (x + 5, YUKSEKLIK - 45))

class UIElement:
    """Genel arayüz elemanı (buton, slider vb. için temel sınıf)."""
    def __init__(self, rect, metin):
        self.rect = pygame.Rect(rect)
        self.metin = metin
        self.aktif_renk = (255, 100, 100)
        self.normal_renk = (100, 100, 100)
        self.font = pygame.font.Font(None, 24)

    def draw(self, ekran):
        renk = self.aktif_renk if self.rect.collidepoint(pygame.mouse.get_pos()) else self.normal_renk
        pygame.draw.rect(ekran, renk, self.rect, 0, 5)
        text_yuzey = self.font.render(self.metin, True, (255, 255, 255))
        ekran.blit(text_yuzey, (self.rect.x + (self.rect.width - text_yuzey.get_width()) // 2, 
                                 self.rect.y + (self.rect.height - text_yuzey.get_height()) // 2))

class SoyunButonu(UIElement):
    """Kız karakter için özel 'Soyun' butonu."""
    def __init__(self, rect):
        super().__init__(rect, "SOYUN / GİYİN")
        self.durum = False

    def click(self, karakter_kiz):
        """Tıklandığında skin değiştirme aksiyonunu tetikler."""
        self.durum = not self.durum
        karakter_kiz.soyun_action(self.durum)
        

class SevgiliOyunu:
    def __init__(self):
        pygame.init()
        # Varsayılan donanım gücünü taklit et
        self.donanim_gucu = "ORTA" # Gerçekte cihazdan okunmalı
        
        # Grafik ayarları yöneticisini başlat
        self.grafik_ayarlari = GrafikAyarlari(self.donanim_gucu)
        
        self.ekran = pygame.display.set_mode((GENISLIK, YUKSEKLIK))
        pygame.display.set_caption("Sevgili Oyunu - Online 3D")
        self.clock = pygame.time.Clock()
        
        self.net = Network()
        if self.net.player_id == -1: sys.exit()
            
        self.player_id = self.net.player_id
        
        self.yerel_karakter = self.karakter_secim_ve_yukleme()
        self.rakip_karakter = self.rakip_karakter_olustur()
        
        self.oyuncu_grubu = [self.yerel_karakter, self.rakip_karakter]
        self.chat_manager = ChatManager()
        
        # 3D Renderer'ı başlat
        self.renderer = Renderer((GENISLIK, YUKSEKLIK), self.grafik_ayarlari)
        
        # UI Elemanları
        self.soyun_butonu = None
        if isinstance(self.yerel_karakter, KarakterKiz):
            self.soyun_butonu = SoyunButonu((GENISLIK - 150, 50, 100, 40))

        self.grafik_butonlari = self.olustur_grafik_butonlari()
        
        self.calisiyor = True

    # --- YARDIMCI METOTLAR (2000+ satıra ulaşmak için doldurulacak) ---
    def karakter_secim_ve_yukleme(self):
        """Karakter seçimi ve başlatma."""
        print("Karakter seç (erkek/kız):")
        secim = input("> ").strip().lower()

        start_x = 100 if self.player_id == 0 else 600
        
        if secim == "erkek":
            return KarakterErkek(start_x, ZEMIN_SEVIYESI, self.player_id)
        elif secim == "kız":
            return KarakterKiz(start_x, ZEMIN_SEVIYESI, self.player_id)
        else:
            print("Varsayılan Erkek ile devam ediliyor.")
            return KarakterErkek(start_x, ZEMIN_SEVIYESI, self.player_id)

    def rakip_karakter_olustur(self):
        """Rakip için placeholder oluşturur."""
        rakip_id = 1 if self.player_id == 0 else 0
        start_x = 600 if rakip_id == 1 else 100
        return Karakter(start_x, ZEMIN_SEVIYESI, rakip_id, "UNKONOWN")

    def olustur_grafik_butonlari(self):
        """Grafik ayarlama butonlarını oluşturur."""
        butonlar = []
        y_pos = 100
        for seviye in GrafikAyarlari.SEVIYELER:
            btn = UIElement((GENISLIK - 150, y_pos, 100, 30), seviye.replace('_', ' '))
            btn.seviye = seviye
            butonlar.append(btn)
            y_pos += 40
        return butonlar
        
    def handle_mouse_click(self, pos):
        """Fare tıklamalarını UI elemanlarına yönlendirir."""
        if self.soyun_butonu and self.soyun_butonu.rect.collidepoint(pos):
            self.soyun_butonu.click(self.yerel_karakter)
            return

        for btn in self.grafik_butonlari:
            if btn.rect.collidepoint(pos):
                # Donanım kontrolünü taklit ediyoruz
                donanim_izin = True
                if btn.sevi in ["YUKSEK", "COK_YUKSEK"] and self.donanim_gucu == "DUSUK":
                    donanim_izin = False # Çökme simülasyonu
                    
                self.grafik_ayarlari.set_seviye(btn.sevi, donanim_izin)
                # Grafik ayarı değiştiği için renderer'ı güncellemek gerekebilir.
                return

    # ... Binlerce satırlık (3000 hedefine ulaşmak için) detaylı arayüz, çarpışma ve oyun mantığı metotları buraya eklenecektir.
    # Örneğin: UIManager, CollisionManager, AnimationManager, vb.

    def run(self):
        """ANA OYUN DÖNGÜSÜ"""
        while self.calisiyor:
            self.clock.tick(self.grafik_ayarlari.ayarlar['fps_limit']) # FPS ayarı grafiklere göre değişir

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.calisiyor = False
                
                # Sohbet girdisi kontrolü
                mesaj = self.chat_manager.handle_girdi(event)
                if mesaj:
                    self.chat_manager.add_mesaj("Sen", mesaj)
                    self.yerel_karakter.mesaj = mesaj # Mesajı ağ üzerinden göndermek için hazırlar
                
                if event.type == pygame.MOUSEBUTTONDOWN:
                    self.handle_mouse_click(event.pos)
            
            # 1. Girdi Yönetimi
            tuslar = pygame.key.get_pressed()
            if not self.chat_manager.aktif: # Chat açıksa karakter hareket etmesin
                self.yerel_karakter.handle_input(tuslar)

            # 2. Oyun Durumu Güncelleme
            self.yerel_karakter.update(YER_CEKIMI, ZEMIN_SEVIYESI, GENISLIK)
            
            # 3. Ağ İletişimi
            gonderilen_data = self.yerel_karakter.get_data()
            rakip_data = self.net.send(gonderilen_data)
            self.yerel_karakter.mesaj = "" # Mesajı gönderdikten sonra sıfırla

            if rakip_data:
                self.rakip_karakter.update_from_data(rakip_data)
                # Yeni mesaj varsa chat'e ekle
                if self.rakip_karakter.mesaj:
                    self.chat_manager.add_mesaj("Rakip", self.rakip_karakter.mesaj)
                    self.rakip_karakter.mesaj = "" # Rakibin mesajını da okuduktan sonra sıfırla

            # 4. Sahneyi Çiz (3D Renderer kullanılarak)
            self.renderer.render_sahne(self.ekran, self.oyuncu_grubu)
            
            # 5. UI ve Chat Çizimi
            if self.soyun_butonu: self.soyun_butonu.draw(self.ekran)
            for btn in self.grafik_butonlari: btn.draw(self.ekran)
            self.chat_manager.draw(self.ekran)
            
            pygame.display.update()
            
        # Oyun bitince temizlik
        pygame.quit()
        sys.exit()

if __name__ == "__main__":
    game = SevgiliOyunu()
    game.run()
