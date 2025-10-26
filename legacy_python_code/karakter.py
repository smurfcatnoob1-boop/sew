# karakter.py - Oyun Karakter Sınıfı (Gelişmiş)
import pygame
from assets.models.model_config import MODELS

class Karakter(pygame.sprite.Sprite):
    def __init__(self, start_x, start_y, kontrol_id, baslangic_skin_ismi):
        super().__init__()
        
        # Fiziksel Özellikler
        self.width = 50
        self.height = 100
        self.rect = pygame.Rect(start_x, start_y, self.width, self.height)
        self.hiz_x = 0
        self.hiz_y = 0
        self.yer_hizi = 5
        self.zip_gucu = -15
        self.derinlik = 0 # 3D çizim için Z koordinatı

        # Durum Değişkenleri
        self.kontrol_id = kontrol_id
        self.zeminde = False
        self.durum = "IDLE" 
        self.aktif_skin_ismi = baslangic_skin_ismi
        self.mesaj = "" # Sohbet mesajı

    def handle_input(self, tuslar):
        """Yerel oyuncu için klavye girdi işleme (WASD)."""
        if self.kontrol_id != 0: 
            return 
        
        self.hiz_x = 0
        if tuslar[pygame.K_a]:
            self.hiz_x = -self.yer_hizi
            self.durum = "RUN"
        if tuslar[pygame.K_d]:
            self.hiz_x = self.yer_hizi
            self.durum = "RUN"
        if self.hiz_x == 0:
            self.durum = "IDLE"
            
        if tuslar[pygame.K_w] and self.zeminde:
            self.hiz_y = self.zip_gucu
            self.zeminde = False
            self.durum = "JUMP"
            
    def update(self, yercekimi, zemin_seviyesi, genislik):
        """Fizik ve pozisyon güncelleme."""
        self.hiz_y += yercekimi
        if self.hiz_y > 10: self.hiz_y = 10

        self.rect.x += self.hiz_x
        self.rect.y += self.hiz_y
        
        if self.rect.bottom >= zemin_seviyesi:
            self.rect.bottom = zemin_seviyesi
            self.hiz_y = 0
            self.zeminde = True
        
        # Ekran sınırları
        self.rect.x = max(0, min(self.rect.x, genislik - self.width))
        
    def get_data(self):
        """Ağ üzerinden göndermek için pozisyon verisini hazırlar."""
        # Format: x,y,durum,skin_id,mesaj
        return f"{self.rect.x},{self.rect.y},{self.durum},{self.aktif_skin_ismi},{self.mesaj}"

    def update_from_data(self, data):
        """Ağdan gelen rakip pozisyon verisini uygular."""
        try:
            x, y, durum, skin, mesaj = data.split(',')
            self.rect.x = int(x)
            self.rect.y = int(y)
            self.durum = durum
            self.aktif_skin_ismi = skin
            self.mesaj = mesaj
        except Exception as e:
            # print(f"HATA: Rakip veri formatı bozuk: {data}. {e}")
            pass

    def get_current_skin_name(self):
        return self.aktif_skin_ismi

class KarakterErkek(Karakter):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, baslangic_skin_ismi="Kaan", **kwargs)
        self.cinsiyet = "erkek"

class KarakterKiz(Karakter):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, baslangic_skin_ismi="Kiz", **kwargs)
        self.cinsiyet = "kız"
        self.soyunuk_mu = False

    def soyun_action(self, aktif=True):
        """Kız1.gltf modelini aktive eden fonksiyon."""
        self.soyunuk_mu = aktif
        if aktif:
            self.aktif_skin_ismi = "Kiz1"
            print("SKIN DEĞİŞTİ: Kız -> Kız1.gltf")
        else:
            self.aktif_skin_ismi = "Kiz"
            print("SKIN DEĞİŞTİ: Kız1 -> Kız.gltf")
