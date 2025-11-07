# CMake Versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje Adı
project(vulkan-engine C CXX)

# C dilini etkinleştir (native_app_glue bir C dosyasıdır)
enable_language(C)

# NDK Yolu Tespiti (GitHub Actions için)
# ANDROID_NDK_HOME veya ANDROID_NDK değişkenlerinden birini kullan.
if (DEFINED ENV{ANDROID_NDK_HOME})
    set(NDK_PATH $ENV{ANDROID_NDK_HOME})
elseif (DEFINED ENV{ANDROID_NDK})
    set(NDK_PATH $ENV{ANDROID_NDK})
else()
    message(FATAL_ERROR "HATA: ANDROID_NDK veya ANDROID_NDK_HOME ortam değişkeni tanımlı değil!")
endif()

# KRİTİK: NDK include yolunu direkt olarak zorla ekle
include_directories(
    src/main/cpp
    # En olası iki NDK yolu
    ${NDK_PATH}/sources/android/native_app_glue 
    ${NDK_PATH}/sysroot/usr/include 
)

# Native App Glue Kaynak Dosyasını Değişken Olarak Tanımlama
set(NATIVE_APP_GLUE_SRC ${NDK_PATH}/sources/android/native_app_glue/android_native_app_glue.c)

# Kaynak dosyalarını tanımlama
set(SOURCE_FILES 
    android/src/main/cpp/main.cpp
    ${NATIVE_APP_GLUE_SRC} # <<< KAYNAK DOSYASINI DİREKT EKLEDİK (Bulunamazsa hata verir)
)

# Kütüphaneyi oluşturma
add_library(vulkan-engine
    SHARED
    ${SOURCE_FILES}
)

# Derleme bayrakları
target_compile_options(vulkan-engine PRIVATE 
    -std=c++17 
    -Wall 
    -Wextra
)

# Gerekli kütüphaneleri bağlama (Linking)
target_link_libraries(vulkan-engine
    # Vulkan için temel kütüphaneler
    android
    log
    EGL
    GLESv3
    vulkan
)

