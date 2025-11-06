cmake_minimum_required(VERSION 3.10.2)

# Hata 1 Çözümü: project() komutu eklendi
project(sevgili_oyunu) 

# ndk_includes klasörünü dahil etme yolu olarak ayarlar
set(NDK_CUSTOM_INCLUDES "${CMAKE_SOURCE_DIR}/ndk_includes")

# android_native_app_glue.h dosyasını artık direk NDK'da aramıyoruz.
# Bunun yerine, NDK içindeki libandroid_native_app_glue.a kütüphanesini buluyoruz.
# Bu kütüphaneyi NDK'nın kendisi bulsun diye path vermiyoruz.
find_library(
        ANDROID_NATIVE_APP_GLUE_LIBRARY
        android_native_app_glue)

find_library(
        VULKAN_LIBRARY
        vulkan)

add_library(sevgili_oyunu_native SHARED
            main.cpp)

# NDK_CUSTOM_INCLUDES yolunu target_include_directories'e ekleyerek 
# oradaki android_native_app_glue.h dosyasının kullanılmasını sağlıyoruz.
target_include_directories(sevgili_oyunu_native PUBLIC
                           ${NDK_CUSTOM_INCLUDES}
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${VULKAN_INCLUDE_DIR})

# ANDROID_NATIVE_APP_GLUE_LIBRARY değişkeni artık boş olmadığı için hata çözülüyor.
target_link_libraries(sevgili_oyunu_native
                      ${ANDROID_NATIVE_APP_GLUE_LIBRARY}
                      android
                      log
                      EGL
                      GLESv2
                      ${VULKAN_LIBRARY}
                      m)
