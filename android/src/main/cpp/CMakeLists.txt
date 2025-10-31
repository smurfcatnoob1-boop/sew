# Minimum CMake sürümü
cmake_minimum_required(VERSION 3.22.1)

# NDK/CMake C_ARCHIVE hatasını düzeltmek için zorunlu C derleyici ayarları:
# Bu, önceki derleme sorununu çözen kısımdır.
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <LINK_FLAGS> <OBJECTS> <LIBRARIES> -o <TARGET> <LINKER_FLAGS>")

# Proje adını tanımlama ve C++ standardını ayarlama
project("vulkan-engine" LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# NDK yerel kütüphanelerini bulma
find_library(log-lib log)
find_library(android-lib android)
find_library(EGL_lib EGL)
find_library(GLESv2_lib GLESv2)
find_library(Vulkan_lib Vulkan)

# Paylaşımlı kütüphaneyi oluşturma
# HATA DÜZELTME 1: main.cpp artık sadece "main.cpp" olarak referans veriliyor,
# çünkü CMakeLists.txt ile aynı dizinde bulunuyor.
add_library(vulkan-engine SHARED
    main.cpp
)

# Gerekli kütüphaneleri bağlama
# HATA DÜZELTME 2: android_native_app_glue.c dosyasını manuel olarak eklemek yerine,
# NDK'nın sağladığı kararlı android_native_app_glue hedefini kullanıyoruz.
target_link_libraries(vulkan-engine
    android_native_app_glue # NDK Native App Glue
    lib
    lib
    
    
    
)

# Kütüphane çıkış yolunu ayarlama
set_target_properties(vulkan-engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ""
)
