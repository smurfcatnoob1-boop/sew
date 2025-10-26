[app]

# (1) Başlık ve Paket Bilgileri
title = Sevgili Oyunu - Online 3D Chat
package.name = com.smurfcatnoob1boop.sevgili
package.domain = com.smurfcatnoob1boop
# Uygulamanın versiyonu. İlk versiyon 1.0
version = 1.0

# (2) Ana Dosya ve Kaynaklar
# Uygulamanın çalışacağı ana dosya
main.py = client.py

# Uygulama kaynaklarının olduğu dizin
source.dir = .

# Dahil edilecek dosya uzantıları
source.include_exts = py, png, jpg, kv, gltf, glb

# (3) Gerekli Kütüphaneler (Bu çok önemli!)
# Pygame'i Kivy'nin SDL2 altyapısıyla çalıştırmak için gerekli
requirements = python3, kivy==2.3.0, pygame, sdl2_ttf, sdl2_mixer, openssl, requests, six

# (4) Android Ayarları
# Minimum Android SDK versiyonu (Çoğu cihaz için uygundur)
android.minapi = 21

# Multiplayer için internet izni kesinlikle gerekli
android.permissions = INTERNET

# Uygulama oryantasyonu
android.orientation = landscape

# Başlatıcı ikonları (Şimdilik standart ikonlar kullanılacak)
# icon.filename = %(source.dir)s/data/icon.png
# icon.filename.android = %(source.dir)s/data/icon.png

# (5) Derleme Ayarları
# Derleme hedefi (android debug veya android release)
# Bu ayar build.yml'deki 'buildozer android debug' komutunu etkilemez
# Bu dosyayı buildozer'a tanıtmak için bu satır gereklidir:
target.build_dir = .buildozer

[buildozer]
log_level = 2
