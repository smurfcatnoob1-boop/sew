# Minimum CMake sürümü
cmake_minimum_required(VERSION 3.22.1)

# NDK/CMake C_ARCHIVE hatasını düzeltmek için zorunlu C derleyici ayarları:
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

# NATIVE APP GLUE: Kaynak dosyayı manuel olarak ayrı bir STATIC kütüphane olarak ekle (yol sorununun kesin çözümü)
add_library(app_glue_src STATIC
    /sources/android/native_app_glue/android_native_app_glue.c
)
# Header dosyasının yolunu app_glue_src target'ına ekle
target_include_directories(app_glue_src PUBLIC
    /sources/android/native_app_glue
)
# C kaynak dosyası için derleme bayrağını zorunlu olarak C olarak ayarla
set_target_properties(app_glue_src PROPERTIES COMPILE_FLAGS -xc)


# Paylaşımlı kütüphaneyi oluşturma
add_library(vulkan-engine SHARED
    main.cpp
)

# Ana kütüphaneye de header yolunu ekle (garanti olsun diye)
target_include_directories(vulkan-engine PUBLIC
    /sources/android/native_app_glue
)

# Gerekli kütüphaneleri ve app glue'yu bağlama
target_link_libraries(vulkan-engine
    app_glue_src # Manuel eklenen app glue kaynağını bağlıyoruz
    lib
    lib
    
    
    
)

# Kütüphane çıkış yolunu ayarlama
set_target_properties(vulkan-engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ""
)
