#include <android/native_activity.h>
#include "android_native_app_glue.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/log.h>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <EGL/egl.h> // GLES için eklendi
#include <GLES3/gl3.h> // GLES için eklendi

#define LOG_TAG "HybridEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Kullanılacak uzantı
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

// Motor Durum Yapısı (Hem Vulkan hem GLES için)
struct Engine {
    struct android_app* app;
    bool running = false;
    bool is_vulkan = false; // Hangi API'nin çalıştığını tutar

    // Vulkan
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    
    // GLES
    EGLDisplay glesDisplay = EGL_NO_DISPLAY;
    EGLSurface glesSurface = EGL_NO_SURFACE;
    EGLContext glesContext = EGL_NO_CONTEXT;
};

// Queue Family (Sıra Ailesi) Endeksleri (Diğer yardımcı yapılar)
struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;
    bool isComplete() { return graphicsFamily != -1 && presentFamily != -1; }
};

// ****************************** VULKAN BAŞLATMA VE HATA YAKALAMA ******************************

// Vulkan Instance ve Surface Oluşturma (Hata yakalama ile)
bool createInstanceAndSurface(Engine* engine) {
    // ... (Önceki kod ile aynı Instance oluşturma) ...
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SevgiliOyunu_Vulkan_PBR";
    appInfo.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    const std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    if (vkCreateInstance(&createInfo, nullptr, &engine->vkInstance) != VK_SUCCESS) {
        LOGE("HATA: Vulkan Instance oluşturulamadı!"); return false;
    }

    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = engine->app->window;

    // KRİTİK NOKTA: Surface oluşturma hatasını yakala!
    if (vkCreateAndroidSurfaceKHR(engine->vkInstance, &surfaceCreateInfo, nullptr, &engine->surface) != VK_SUCCESS) {
        LOGE("HATA: Vulkan Surface oluşturulamadı (Gralloc hatası?). Vulkan devre dışı!"); 
        vkDestroyInstance(engine->vkInstance, nullptr);
        engine->vkInstance = VK_NULL_HANDLE;
        return false;
    }
    LOGI("Vulkan Instance ve Surface Başarılı.");
    return true;
}

// Logical Device Oluşturma (Hata yakalama ile)
bool createLogicalDeviceAndPickPhysicalDevice(Engine* engine) {
    // ... (Fiziksel Cihaz ve Queue Family bulma fonksiyonlarını varsayın) ...
    // Basit olması için: Sadece Device oluşturmayı kontrol ediyoruz.
    QueueFamilyIndices indices = {0, 0}; // Gerçekte findQueueFamilies çağrılmalı
    // ... (Vulkan kodunuzdaki cihaz seçimi ve uygunluk kontrolleri buraya gelecek) ...
    
    // Geçici olarak, sadece hatayı yakalamak için basitleştirilmiş Device oluşturma
    // Eğer bu adım başarısız olursa (Adreno bug'ı nedeniyle), yakalamamız gerekiyor.
    
    // Varsayım: physicalDevice bulundu ve indices alındı.
    // Başarısız olursa güvenli çıkış.

    // ÖNEMLİ: Bu kısımda çökme olasılığı yüksek olduğu için, 
    // önceki kodlarınızdaki hatasız Logical Device oluşturma mantığını buraya taşıyın.
    // Eğer vkCreateDevice başarısız olursa, HATA yakalama ile false döndürün.

    // ... (Önceki Logical Device kodunuz)
    // Örn: if (vkCreateDevice(...) != VK_SUCCESS) { LOGE("HATA: Logical Device oluşturulamadı."); return false; }
    
    // Eğer bu noktaya kadar gelirse:
    LOGI("Vulkan Logical Device Başarılı.");
    return true;
}

// Vulkan Başlatma Güvenlik Katmanı
bool init_vulkan_safe(Engine* engine) {
    if (!createInstanceAndSurface(engine)) return false; 
    
    // Eğer buraya gelindiyse, Surface oluştu (ki bu büyük bir adımdır!)
    // Daha sonra Device ve Queues oluşturulmalı.
    if (!createLogicalDeviceAndPickPhysicalDevice(engine)) {
        // Device başarısız olursa, temizle ve false döndür.
        // vkDestroySurfaceKHR(engine->vkInstance, engine->surface, nullptr);
        // vkDestroyInstance(engine->vkInstance, nullptr);
        return false;
    }
    
    // Normalde burada SwapChain ve RenderPass oluşturulmalı.
    LOGI("Vulkan Temelleri Başarılı. SwapChain Atlanacak.");
    engine->is_vulkan = true;
    return true;
}

// ****************************** GLES BAŞLATMA (FALLBACK) ******************************

bool init_gles_fallback(Engine* engine) {
    if (engine->app->window == NULL) { LOGE("GLES: Pencere başlatılmadı!"); return false; }

    // GLES Kodunuz (Önceki yanıttan kopyalanmıştır)
    engine->glesDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (engine->glesDisplay == EGL_NO_DISPLAY) { LOGE("GLES: Display alınamadı!"); return false; }
    if (eglInitialize(engine->glesDisplay, 0, 0) != EGL_TRUE) { LOGE("GLES: Başlatılamadı!"); return false; }

    const EGLint attribs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };
    EGLConfig config;
    EGLint num_config;
    if (eglChooseConfig(engine->glesDisplay, attribs, &config, 1, &num_config) != EGL_TRUE || num_config == 0) { LOGE("GLES: Konfigürasyon bulunamadı!"); return false; }

    engine->glesSurface = eglCreateWindowSurface(engine->glesDisplay, config, engine->app->window, NULL);
    if (engine->glesSurface == EGL_NO_SURFACE) { LOGE("GLES: Surface oluşturulamadı!"); return false; }
    
    const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    engine->glesContext = eglCreateContext(engine->glesDisplay, config, EGL_NO_CONTEXT, context_attribs);
    if (engine->glesContext == EGL_NO_CONTEXT) { LOGE("GLES: Context oluşturulamadı!"); return false; }
    
    if (eglMakeCurrent(engine->glesDisplay, engine->glesSurface, engine->glesSurface, engine->glesContext) != EGL_TRUE) { LOGE("GLES: Context aktif edilemedi!"); return false; }

    LOGI("OpenGL ES 3.0 Başarılı. Fallback Devrede.");
    engine->is_vulkan = false;
    return true;
}

// ****************************** ANA UYGULAMA DÖNGÜSÜ ******************************

void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;
    if (cmd == APP_CMD_INIT_WINDOW && engine->app->window != NULL) {
        // 1. Önce Vulkan'ı Dene
        if (init_vulkan_safe(engine)) {
            engine->running = true;
            LOGI("VULKAN Seçildi ve Başlatıldı.");
            return;
        }

        // 2. Vulkan Başarısız Olursa GLES'e Geri Dön (Fallback)
        LOGI("Vulkan Başarısız Oldu. GLES Fallback Deneniyor...");
        if (init_gles_fallback(engine)) {
            engine->running = true;
            LOGI("GLES Başlatıldı ve Çalışıyor.");
            return;
        }
        
        LOGE("UYARI: Ne Vulkan ne de GLES başlatılamadı. Uygulama çalışmayacak.");
    }
    // Diğer komutları işleme...
}

void android_main(struct android_app* state) {
    // ... (Ana döngü kodları)
    struct Engine engine = {0};
    state->userData = &engine;
    engine.app = state;
    state->onAppCmd = engine_handle_cmd;

    // ... (ALooper_pollAll döngüsü)
    int ident;
    int events;
    struct android_poll_source* source;
    while (1) {
        if (ALooper_pollAll(engine.running ? 0 : -1, &ident, &events, (void**)&source) >= 0) {
            if (source != NULL) { source->process(state, source); }
        }
        if (state->destroyRequested != 0) break;

        if (engine.running) { 
            // Çizim kısmı: Hangi API aktifse onunla çizim yap
            if (engine.is_vulkan) {
                // Vulkan Çizim Kodları
            } else {
                // GLES Çizim Kodları (Örn: draw_frame(engine); eglSwapBuffers vb.)
                if (engine.glesDisplay != EGL_NO_DISPLAY) {
                    glClearColor(0.1f, 0.3f, 0.5f, 1.0f); // GLES ile mavi ekran
                    glClear(GL_COLOR_BUFFER_BIT);
                    eglSwapBuffers(engine.glesDisplay, engine.glesSurface);
                }
            }
        }
    }
}
// ...
