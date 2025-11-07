# CMake Versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje Adı
project(vulkan-engine C CXX)

# Kütüphaneleri otomatik bulma denemesi
find_package(android_native_app_glue)

# NDK include yollarını otomatik olarak ayarla
if (android_native_app_glue_FOUND)
    # Native App Glue kütüphanesi bulunduysa, include yollarını ekle
    include_directories(${android_native_app_glue_INCLUDE_DIRS})
else()
    # Fallback: Kütüphane bulunamazsa NDK içerisindeki standart yolu dene
    include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
endif()

# Diğer gerekli include dizinlerini ekle
include_directories(
    src/main/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ndk_includes
)

# Kaynak dosyalarını tanımlama
set(SOURCE_FILES 
    android/src/main/cpp/main.cpp
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
    
    # Native App Glue Kütüphanesi (Otomatik bulma veya manuel ekleme)
    native_app_glue
)

# İsteğe bağlı: JNIEXPORT/JNICALL tanımlarını düzeltmek için
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-integrated-as")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-integrated-as")

