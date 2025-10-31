# Minimum CMake sürümü
cmake_minimum_required(VERSION 3.22.1)

# NDK/CMake C_ARCHIVE hatasını düzeltmek için zorunlu C derleyici ayarları:
# C derleyicisini C++ derleyicisiyle çalışmaya zorlayarak eksik değişken hatasını atlar.
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <LINK_FLAGS> <OBJECTS> <LIBRARIES> -o <TARGET> <LINKER_FLAGS>")

# Proje adını tanımlama ve C++ standardını ayarlama
project("vulkan-engine" LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# NDK yerel kütüphanelerini bulma
find_library(log-lib log)
find_library(android-lib android)

# Android native app glue'yu projenin kaynak kodu olarak ekle
add_library(app_glue_src STATIC
    /sources/android/native_app_glue/android_native_app_glue.c
)

# C kaynak dosyası için derleme bayrağını zorunlu olarak C olarak ayarla
set_target_properties(app_glue_src PROPERTIES COMPILE_FLAGS -xc)
target_include_directories(app_glue_src PUBLIC
    /sources/android/native_app_glue
)

# Paylaşımlı kütüphaneyi oluşturma
add_library(vulkan-engine SHARED
    src/main/cpp/main.cpp
)

# Gerekli kütüphaneleri ve app glue'yu bağlama
target_link_libraries(vulkan-engine
    app_glue_src
    lib
    lib
    EGL
    GLESv2
    Vulkan
)

# Kütüphane çıkış yolunu ayarlama
set_target_properties(vulkan-engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ""
)
