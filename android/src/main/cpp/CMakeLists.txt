# CMake Versiyonu
cmake_minimum_required(VERSION 3.22.1)

# Proje Adı
project(vulkan-engine C CXX)

# Paylaşımlı kütüphaneyi oluşturma
add_library(vulkan-engine SHARED
target_include_directories(vulkan-engine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)
    main.cpp
)

# NDK Modüllerini Bağlama (Sadece NDK isimlerini kullan)
target_link_libraries(vulkan-engine
    # KRİTİK: Native App Glue'yu NDK tarafından yönetilmeye zorlar
    android_native_app_glue 
    
    # Zorunlu Linkleme Bayrakları (Symbol stripping'i önlemek için)
    -Wl,--whole-archive
    -Wl,--no-whole-archive

    # Gerekli NDK ve Vulkan kütüphaneleri
    android
    log
    vulkan 
)
