cmake_minimum_required(VERSION 3.10.2)

# ndk_includes klasörünü dahil etme yolu olarak ayarlar
set(NDK_CUSTOM_INCLUDES "${CMAKE_SOURCE_DIR}/ndk_includes")

find_library(
        ANDROID_NATIVE_APP_GLUE_LIBRARY
        android_native_app_glue)

find_library(
        VULKAN_LIBRARY
        vulkan)

add_library(sevgili_oyunu_native SHARED
            main.cpp)

# NDK_CUSTOM_INCLUDES yolunu target_include_directories'e ekler
target_include_directories(sevgili_oyunu_native PUBLIC
                           ${NDK_CUSTOM_INCLUDES} 
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${VULKAN_INCLUDE_DIR})

target_link_libraries(sevgili_oyunu_native
                      ${ANDROID_NATIVE_APP_GLUE_LIBRARY}
                      android
                      log
                      EGL
                      GLESv2
                      ${VULKAN_LIBRARY}
                      m)
