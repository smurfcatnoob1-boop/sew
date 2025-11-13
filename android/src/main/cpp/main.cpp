#include <android/native_activity.h>
#include "android_native_app_glue.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/log.h>
#include <vector>
#include <set>
#include <algorithm>
#include <string>

// ANativeWindow Buffer Kontrolü için AHardwareBuffer'ı dahil et (API 26+)
#include <android/hardware_buffer.h> 

#define LOG_TAG "VulkanFixEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct Engine {
    struct android_app* app;
    bool running = false;
    bool is_vulkan = false; 

    // Vulkan
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // GLES (Fallback'i basitleştirmek için kaldırıldı, ama mantık aynı)
};

// Basit yardımcı yapılar (Değişmedi)
struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;
    bool isComplete() { return graphicsFamily != -1 && presentFamily != -1; }
};

// ... Diğer yardımcı fonksiyonlar (checkDeviceExtensionSupport, findQueueFamilies, isDeviceSuitable) aynı kalacak ...

// Vulkan Instance ve Surface Oluşturma (ANATIVEWINDOW FIX İLE)
bool createInstanceAndSurface(Engine* engine) {
    // 1. Vulkan Instance Oluşturma (Aynı)
    // ...
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SevgiliOyunu_Vulkan_PBR";
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    const std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &engine->vkInstance) != VK_SUCCESS) {
        LOGE("Vulkan Instance oluşturulamadı!");
        return false;
    }
    LOGI("Vulkan Instance Başarılı.");


    // 2. KRİTİK ADIM: ANativeWindow Ayarlarını Zorlama (Adreno 4x4 Bug Fix)
    // Bu, Gralloc'un varsayılan formatları test etmesini engeller.
    
    // a) Pencerenin mevcut genişlik ve yüksekliğini al
    int32_t width = ANativeWindow_getWidth(engine->app->window);
    int32_t height = ANativeWindow_getHeight(engine->app->window);

    // b) Buffer Formatını zorla (AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM'a karşılık gelen 1)
    // Bu, Vulkan formatlarıyla karışmasın diye Android API formatı kullanılır.
    int32_t format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM; 
    
    // c) Buffer geometrisini ve formatını ayarla (Bu, 4x4 testini bypass etmeli)
    if (ANativeWindow_setBuffersGeometry(engine->app->window, width, height, format) != 0) {
        LOGE("HATA: ANativeWindow_setBuffersGeometry başarısız oldu!");
    } else {
        LOGI("ANativeWindow Geometry: %dx%d Format: %d ayarlandı.", width, height, format);
    }
    
    // d) Swap Chain için minimum 2 buffer sayısını zorla
    // Bazı Adreno sürücüleri varsayılan 1 buffer'a takılı kalır.
    if (ANativeWindow_setBufferCount(engine->app->window, 2) != 0) {
        LOGE("HATA: ANativeWindow_setBufferCount başarısız oldu!");
    } else {
        LOGI("ANativeWindow Buffer Count 2 olarak ayarlandı.");
    }

    // 3. Vulkan Surface Oluşturma
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = engine->app->window;

    // Hatayı yakala
    if (vkCreateAndroidSurfaceKHR(engine->vkInstance, &surfaceCreateInfo, nullptr, &engine->surface) != VK_SUCCESS) {
        LOGE("HATA: Vulkan Surface oluşturulamadı! (Surface yaratma çöküşü teyit edildi)");
        vkDestroyInstance(engine->vkInstance, nullptr);
        engine->vkInstance = VK_NULL_HANDLE;
        return false;
    }

    LOGI("Vulkan Instance ve Surface Başarıyla Oluşturuldu.");
    return true;
}

// Logical Device Oluşturma (Sadece Hata Teyidi için Basitleştirilmiş)
bool createLogicalDeviceAndPickPhysicalDevice(Engine* engine) {
    // ... (Önceki kodunuzdaki fiziksel cihaz seçimi, queue bulma ve device oluşturma mantığını buraya taşıyın) ...

    // Hata teyidi için basitleştirilmiş bir başarı mesajı.
    LOGI("Vulkan Logical Device Başarılı.");
    return true;
}

// Vulkan Motorunun Güvenli Başlatılması
bool init_vulkan_safe(Engine* engine) {
    if (engine->app->window == NULL) {
        LOGE("Pencere başlatılmadı!");
        return false;
    }

    if (!createInstanceAndSurface(engine)) return false;
    if (!createLogicalDeviceAndPickPhysicalDevice(engine)) return false; // Burası pickPhysicalDevice'ı da içermeli.

    LOGI("Vulkan Temelleri Başlatıldı ve Crash Hatası Atlatıldı.");
    engine->is_vulkan = true;
    return true;
}

// Uygulama Komutlarını İşleyen Fonksiyon
void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != NULL) {
                if (init_vulkan_safe(engine)) { 
                    // Eğer Vulkan başarılı olduysa, çalıştır
                    engine->running = true;
                    return;
                }
                
                // Vulkan başarısız olursa, GLES'e geri dönme mantığı buraya eklenebilir.
                // Şu an sadece Vulkan'ı atlatıp çökmemeyi hedefliyoruz.
                LOGE("Vulkan Fix denemesi başarısız. GLES fallback gereklidir.");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            engine->running = false;
            // Temizleme fonksiyonu (DestroyInstance vb.) buraya gelecek
            break;
    }
}

// Ana Android Giriş Noktası
void android_main(struct android_app* state) {
    struct Engine engine = {0};
    state->userData = &engine;
    engine.app = state;
    state->onAppCmd = engine_handle_cmd;

    LOGI("Vulkan Fix Engine Başlatılıyor.");

    int ident;
    int events;
    struct android_poll_source* source;

    while (1) {
        if (ALooper_pollAll(engine.running ? 0 : -1, &ident, &events, (void**)&source) >= 0) {
            if (source != NULL) {
                source->process(state, source);
            }
        }

        if (state->destroyRequested != 0) {
            break;
        }

        if (engine.running) { 
            // Uygulama ayakta kalırsa, bu döngüde çizim yapılabilir.
        }
    }
}
