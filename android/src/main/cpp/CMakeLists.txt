# 1. Minimum CMake sürümünü belirtin
cmake_minimum_required(VERSION 3.22.1)

# 2. C++ kütüphanesini tanımlayın ve kaynak dosyalarını ekleyin
add_library(
    vulkan-engine
    SHARED
    main.cpp
)

# 3. Kütüphaneye NDK'daki gerekli include yollarını ekleyin (Başarıya ulaştı)
target_include_directories(
    vulkan-engine
    PRIVATE
    $ENV{ANDROID_NDK_HOME}/sources/android/native_app_glue
)

# 4. Bağlayıcının (Linker) kütüphane dosyalarını arayacağı dizini ekleyin
# Burası, 'android_native_app_glue' kütüphanesini bulmak için kritik öneme sahiptir.
target_link_directories(
    vulkan-engine
    PRIVATE
    $ENV{ANDROID_NDK_HOME}/sources/android/native_app_glue
)

# 5. Kütüphaneye bağlantı kurulacak diğer yerel kütüphaneleri ekleyin
target_link_libraries(
    vulkan-engine
    android
    EGL
    GLESv3
    vulkan
    log
    android_native_app_glue 
)
