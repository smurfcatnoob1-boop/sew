# CMake Versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje Adı
project(vulkan-engine C CXX)

# Kütüphaneyi oluşturma
add_library(vulkan-engine
    SHARED
    android/src/main/cpp/main.cpp
)

# KRİTİK: Header Dosyası Yollarını Tanımlama
# NDK'nın standart yolu ve kullanıcı tarafından kaydedilen yolu da ekliyoruz.
target_include_directories(vulkan-engine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    ../../../ndk_includes/android # Kullanıcının .h dosyasını koyduğu yer (Önemli)
)

# Derleme bayrakları
target_compile_options(vulkan-engine PRIVATE
    -std=c++17
    -Wall
    -Wextra
)

# NDK Modüllerini Bağlama (app_glue, android, log, vulkan)
# android_native_app_glue modülünü bağlayarak .c dosyasını otomatik derlemesini sağlıyoruz.
target_link_libraries(vulkan-engine
    android_native_app_glue
    android
    log
    EGL
    GLESv3
    vulkan
)
