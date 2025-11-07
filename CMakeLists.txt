# CMake Versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje Adı
project(vulkan-engine C CXX)

# C dilini etkinleştir (native_app_glue bir C dosyasıdır)
enable_language(C)

# CMake'e NDK include yolunu direkt olarak veriyoruz
# Bu, <android/native_app_glue/android_native_app_glue.h> yolunu bulmasını sağlayacak
include_directories(
    ${ANDROID_NDK}/sources/android/native_app_glue 
    src/main/cpp
)

# Native App Glue Kaynak Dosyasını Değişken Olarak Tanımlama
set(NATIVE_APP_GLUE_SRC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Kaynak dosyalarını tanımlama
set(SOURCE_FILES 
    android/src/main/cpp/main.cpp
    ${NATIVE_APP_GLUE_SRC} # <<< KAYNAK DOSYASINI DİREKT EKLEDİK
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
    
    # native_app_glue kütüphanesini burada linklemeye gerek yok çünkü kaynağını direkt derledik,
    # ancak yine de bırakabiliriz, çoğu zaman zarar vermez.
)

