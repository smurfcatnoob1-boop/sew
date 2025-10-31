#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan.h>
#include <android/log.h>
#include <vector>

#define LOG_TAG "VulkanEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Vulkan Motoru Durum Yapısı
struct Engine {
    struct android_app* app;
    bool running = false;

    // Vulkan Çekirdek Nesneleri
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    
    // PBR ve Ray Tracing için kritik: Pipeline ve Swapchain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    
    // Shader'lar (GLSL/SPIR-V) buraya yüklenecek
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE; 
};

// 1. Vulkan Instance ve Surface Oluşturma
bool createInstanceAndSurface(Engine* engine) {
    // Instance oluşturma kodları (Loglama uzantıları ile)
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SevgiliOyunu_Vulkan_PBR";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "CustomVulkanEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2; // Vulkan 1.2'yi hedefliyoruz

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Android için zorunlu uzantılar
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
    
    // Android yüzeyini oluşturma (Render hedefimiz)
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = engine->app->window;

    if (vkCreateAndroidSurfaceKHR(engine->vkInstance, &surfaceCreateInfo, nullptr, &engine->surface) != VK_SUCCESS) {
        LOGE("Vulkan Surface oluşturulamadı!");
        return false;
    }
    
    LOGI("Vulkan Instance ve Surface Başarıyla Oluşturuldu.");
    return true;
}

// 2. Grafik Pipeline'ını Oluşturma (Gelecekte PBR Shader'lar buraya gelecek)
bool createGraphicsPipeline(Engine* engine) {
    LOGI("Grafik Pipeline İskeleti Oluşturuluyor... (Shader'lar daha sonra eklenecek)");
    
    // Basit bir Layout oluşturma (Gerekli)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, nullptr, &engine->pipelineLayout) != VK_SUCCESS) {
        LOGE("Pipeline Layout oluşturulamadı!");
        return false;
    }

    // Gerçek Pipeline oluşturma adımları (Render Pass, Viewports, Shader Stages) buraya gelecek.

    LOGI("Vulkan Pipeline İskeleti Başarılı.");
    return true;
}


// Vulkan Motorunun Temiz Başlatılması
bool init_vulkan(Engine* engine) {
    if (engine->app->window == NULL) {
        LOGE("Pencere başlatılmadı!");
        return false;
    }

    if (!createInstanceAndSurface(engine)) return false;
    
    // Buraya: Physical Device Seçimi (Vulkan 1.2 ve Ray Tracing desteği kontrolü)
    // Buraya: Logical Device ve Queue Family Oluşturma
    
    // if (!createGraphicsPipeline(engine)) return false;
    
    LOGI("Vulkan Engine Hazır: Render döngüsüne geçiliyor.");
    return true;
}


// Uygulama Komutlarını İşleyen Fonksiyon
void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != NULL) {
                if (init_vulkan(engine)) {
                    engine->running = true;
                }
            }
            break;
        case APP_CMD_TERM_WINDOW:
            engine->running = false;
            // Vulkan temizleme kodları buraya gelecek
            break;
        // Diğer yaşam döngüsü komutları
    }
}

// Ana Android Giriş Noktası
void android_main(struct android_app* state) {
    app_dummy();
    struct Engine engine = {0};
    state->userData = &engine;
    engine.app = state;
    state->onAppCmd = engine_handle_cmd;
    
    LOGI("Vulkan Native Engine Başlatıldı.");

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
            // Vulkan Çizim (vkQueueSubmit, vkQueuePresentKHR) buraya gelecek.
        }
    }
}
