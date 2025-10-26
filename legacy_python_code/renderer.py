# renderer.py - 3D Çizim, Model Yönetimi ve Grafik Ayarları
import pygame
from assets.models.model_config import MODELS, MAP_MODEL
# import moderngl  # Gerçek 3D motor kütüphanesi buraya eklenecekti

class GrafikAyarlari:
    """Grafik kalitesi ayarlarını ve sınırlamaları yönetir."""
    SEVIYELER = {
        "DUSUK": {"doku": 0.5, "shadows": False, "fps_limit": 30},
        "ORTA": {"doku": 1.0, "shadows": False, "fps_limit": 60},
        "YUKSEK": {"doku": 1.5, "shadows": True, "fps_limit": 60},
        "COK_YUKSEK": {"doku": 2.0, "shadows": True, "fps_limit": 120},
    }
    
    def __init__(self, baslangic_seviyesi="ORTA"):
        self.seviye_adi = baslangic_seviyesi
        self.ayarlar = self.SEVIYELER[baslangic_seviyesi]

    def set_seviye(self, yeni_seviye_adi, donanim_izin_veriyor=True):
        """Grafik seviyesini ayarlar ve donanım kontrolü yapar."""
        yeni_seviye_adi = yeni_seviye_adi.upper()
        if yeni_seviye_adi not in self.SEVIYELER:
            print(f"UYARI: Geçersiz grafik seviyesi: {yeni_seviye_adi}")
            return

        if not donanim_izin_veriyor and yeni_seviye_adi in ["YUKSEK", "COK_YUKSEK"]:
            print("CİHAZINIZ DESTEKLEMİYOR: Bu seviye oyunu çökertebilir. Ayar Değişmedi.")
            return False
            
        self.seviye_adi = yeni_seviye_adi
        self.ayarlar = self.SEVIYELER[yeni_seviye_adi]
        print(f"Grafik seviyesi ayarlandı: {self.seviye_adi} (Doku Çarpanı: {self.ayarlar['doku']})")
        return True
        
class Renderer:
    """3D Sahneyi Pygame penceresinde çizen taklit (mock) sınıf."""
    def __init__(self, ekran_boyutu, grafik_ayarlari):
        self.boyut = ekran_boyutu
        self.grafik_ayarlari = grafik_ayarlari
        self.model_cache = {}
        self.arka_plan_rengi = (10, 15, 20) # Hafif grimsi, boşluk gibi olmayan siyah atmosfer

        self._load_all_models()
        self._load_map()

    def _load_model(self, model_name):
        """GLTF/GLB dosyasını yükler (Bu kısım gerçek 3D kütüphanesi ile yazılır)."""
        # Şimdilik sadece model adını döndürüyoruz. Gerçekte burada GLTF verileri yüklenir.
        dosya_adi = MODELS.get(model_name)
        return {"file": f"assets/models/{dosya_adi}", "textures": self.grafik_ayarlari.ayarlar['doku']}

    def _load_all_models(self):
        """Tüm karakter modellerini önbelleğe alır."""
        for name in MODELS:
            self.model_cache[name] = self._load_model(name)
        print("Tüm karakter modelleri referansları yüklendi.")
        
    def _load_map(self):
        """Map modelini yükler (Poolrooms.glb)."""
        self.map_model = self._load_model(MAP_MODEL)
        print(f"Map yüklendi: {MAP_MODEL}")

    def render_sahne(self, ekran, karakterler):
        """Sahneyi ve karakterleri 3D olarak çizer (Taklit)."""
        ekran.fill(self.arka_plan_rengi) # Siyahımsı atmosfer

        # 1. Map'i çiz
        self.draw_3d_object(ekran, self.map_model, (0, 0, 0), is_map=True)

        # 2. Karakterleri Çiz
        for karakter in karakterler:
            model_ismi = karakter.get_current_skin_name()
            model_data = self.model_cache.get(model_ismi)
            
            # 3D Pozisyon: x, y (2D) ve Z (derinlik/yükseklik)
            pos_3d = (karakter.rect.x, karakter.rect.y, karakter.derinlik) 
            
            self.draw_3d_object(ekran, model_data, pos_3d, karakter.durum)

    def draw_3d_object(self, ekran, model_data, pos_3d, durum="IDLE", is_map=False):
        """3D çizim fonksiyonunun basitleştirilmiş Pygame taklidi."""
        x, y, z = pos_3d
        
        # Grafik seviyesine göre doku kalitesini simüle et
        doku_kalitesi = self.grafik_ayarlari.ayarlar['doku']
        
        # Basit 2D Çizim ile 3D Hissi Simülasyonu
        boyut_carpan = 50 * doku_kalitesi
        renk = (255, 255, 255) if not is_map else (50, 50, 100)
        
        # Hareket durumuna göre basit bir animasyon gösterimi
        offset = 0
        if durum == "RUN":
            offset = pygame.time.get_ticks() % 10 < 5
        
        # Gövde
        pygame.draw.rect(ekran, renk, (x - boyut_carpan/2, y - boyut_carpan * 1.5 + offset, boyut_carpan, boyut_carpan * 2), 0)
        
        # Doku kalitesi simülasyonu için kenarlık kalınlığı
        pygame.draw.rect(ekran, (255, 0, 0), (x - boyut_carpan/2, y - boyut_carpan * 1.5 + offset, boyut_carpan, boyut_carpan * 2), int(2 * doku_kalitesi))
        
        # Model adını ve doku kalitesini ekranda göster
        font = pygame.font.Font(None, 18)
        text = font.render(f"{model_data['file'].split('/')[-1]} (x{doku_kalitesi:.1f})", True, (255, 255, 255))
        ekran.blit(text, (x - 50, y - 20))
        
