# 1. Minimum CMake sürümünü belirtin
cmake_minimum_required(VERSION 3.22.1)

# 2. C++ kütüphanesini tanımlayın ve kaynak dosyalarını ekleyin
# 'main.cpp' dosyanızın burada listelendiğinden emin olun
add_library(
    vulkan-engine
    SHARED
    main.cpp
)

# 3. NDK include yolunu belirten 'target_include_directories' komutu KALDIRILDI.
# Bağlama (target_link_libraries) ile otomatik bulunması bekleniyor.

# 4. Kütüphaneye bağlantı kurulacak diğer yerel kütüphaneleri ekleyin
target_link_libraries(
    vulkan-engine
    android
    EGL
    GLESv3
    vulkan
    log
    android_native_app_glue # Bu kütüphane bağlama için gereklidir
)
