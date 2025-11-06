# En azından NDK 25+ ile uyumlu bir CMake versiyonu.
cmake_minimum_required(VERSION 3.22.1)

# Projenizin adını belirtin
project(vulkan-engine CXX)

# Kütüphane kaynak dosyasını tanımlayın. (main.cpp'yi derlenecek hedef olarak ekler.)
add_library(vulkan-engine SHARED
    main.cpp
)

# ----------------------------------------------------
# KRİTİK ADIM: android_native_app_glue kütüphanesini bulma ve bağlama
# ----------------------------------------------------

# android_native_app_glue kütüphanesini NDK içinde bulur
find_library(
    NATIVE_APP_GLUE_LIB
    android_native_app_glue
)

# Kütüphanenin içeriğinin tamamen bağlanması için zorunlu bayraklar kullanılır.
target_link_libraries(vulkan-engine
    # 1. Kendi yerel kütüphaneniz
    ${NATIVE_APP_GLUE_LIB}
    
    # 2. Zorunlu linkleme bayrakları
    -Wl,--whole-archive
    ${NATIVE_APP_GLUE_LIB}
    -Wl,--no-whole-archive
    
    # 3. Diğer gerekli NDK ve Vulkan kütüphaneleri
    android  # Android sistem işlevleri için gereklidir (ör. logcat, native_activity)
    log      # Loglama (print) işlevleri için gereklidir (__android_log_print)
    
    # Sadece Vulkan kaldı!
    vulkan 
)
