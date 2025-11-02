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
# EKLE: native_app_glue kütüphanesini NDK'dan bul
find_library(native_app_glue_lib android_native_app_glue)


# Paylaşımlı kütüphaneyi oluşturma
add_library(vulkan-engine SHARED
    main.cpp
)

# Gerekli kütüphaneleri ve app glue'yu bağlama
target_link_libraries(vulkan-engine
    ${native_app_glue_lib} # NDK'dan doğru şekilde bağla
    ${log-lib}
    ${android-lib}
    ${EGL_lib}
    ${GLESv2_lib}
    ${Vulkan_lib}
)

# Kütüphane çıkış yolunu ayarlama
set_target_properties(vulkan-engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ""
)
