# 1. Minimum CMake sürümünü belirtin
cmake_minimum_required(VERSION 3.22.1)

# NDK'daki native_app_glue kaynak dosyasının yolunu tanımlıyoruz
set(NATIVE_APP_GLUE_DIR $ENV{ANDROID_NDK_HOME}/sources/android/native_app_glue)
set(NATIVE_APP_GLUE_SOURCE ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)

# 2. C++ kütüphanesini tanımlayın ve kaynak dosyalarını ekleyin
# main.cpp ve NDK'nın native_app_glue kaynak dosyasını doğrudan ekliyoruz.
add_library(
    vulkan-engine
    SHARED
    main.cpp
    ${NATIVE_APP_GLUE_SOURCE}
)

# 3. Kütüphaneye NDK'daki gerekli include yollarını ekleyin
# Şimdi sadece native_app_glue'nun başlık dosyasının dizinini eklememiz yeterli
target_include_directories(
    vulkan-engine
    PRIVATE
    ${NATIVE_APP_GLUE_DIR}
    
    # --- YENİ EKLENEN KÜTÜPHANELERİN INCLUDE YOLLARI ---
    # Not: GLM, TinyGLTF ve ImGui'nin başlık (header) dosyalarının bulunduğu dizinler:
    ${CMAKE_SOURCE_DIR}/libraries/glm       # GLM (Vektör Matematiği)
    ${CMAKE_SOURCE_DIR}/libraries/tinygltf  # TinyGLTF (Model Yükleme)
    ${CMAKE_SOURCE_DIR}/libraries/imgui     # Dear ImGui (UI Çizimi)
    # Firebase C++ SDK'sının include yolu (Eğer gerekliyse buraya eklenecek)
)

# 4. Kütüphaneye bağlantı kurulacak diğer yerel kütüphaneleri ekleyin
# 'android_native_app_glue' kütüphanesi çıkarıldı, çünkü kaynak kodunu ekledik.
target_link_libraries(
    vulkan-engine
    android
    EGL
    GLESv3
    vulkan
    log
    
    # --- YENİ EKLENEN KÜTÜPHANELERİN BAĞLANTI LİSTESİ (LINKING) ---
    
    # 1. Firebase SDK'ları (Multiplayer ve Chat için Zorunlu)
    # Bu adların Android NDK'da Firebase kütüphanesine doğru bağlandığından emin olun:
    firebase_app
    firebase_firestore
    firebase_auth
    
    # 2. Model ve UI Kütüphaneleri
    # Eğer bu kütüphaneleri alt dizin (add_subdirectory) olarak eklemediyseniz
    # ve sadece başlık dosyaları (header-only) kullanıyorsanız bu satırlar gerekmeyebilir.
    # Kaynak kod dahil ettiyseniz, doğru target adını kullanın. Örnek:
    # tinygltf
    # imgui
)
