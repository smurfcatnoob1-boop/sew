# En azından NDK 25+ ile uyumlu bir CMake versiyonu.
cmake_minimum_required(VERSION 3.22.1)

# Projenizin adını belirtin
project(vulkan-engine CXX)

# Kütüphane kaynak dosyasını tanımlayın.
add_library(vulkan-engine SHARED
    main.cpp
)

# ----------------------------------------------------
# KRİTİK DÜZELTME: Header dosyasının yolunu elle ekle!
# ----------------------------------------------------
target_include_directories(vulkan-engine PRIVATE
    # android_native_app_glue.h'nin bulunduğu tam yolu ekle
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# ----------------------------------------------------
# android_native_app_glue kütüphanesini bulma ve bağlama
# ----------------------------------------------------

find_library(
    NATIVE_APP_GLUE_LIB
    android_native_app_glue
)

# Kütüphanenin içeriğinin tamamen bağlanması için zorunlu bayraklar kullanılır.
target_link_libraries(vulkan-engine
    ${NATIVE_APP_GLUE_LIB}
    
    -Wl,--whole-archive
    ${NATIVE_APP_GLUE_LIB}
    -Wl,--no-whole-archive
    
    # Gerekli NDK ve Vulkan kütüphaneleri
    android
    log
    vulkan 
)
