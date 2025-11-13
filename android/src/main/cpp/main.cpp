

#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h> // Yeni ekledik
#include <jni.h>

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

// Hata ayıklama ve bilgi için kolaylık sağlamak amacıyla log macro'ları
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "HybridEngine", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "HybridEngine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "HybridEngine", __VA_ARGS__))

// Vulkan Motorumuzun Ana Yapısı
struct Engine {
    struct android_app* app;
    bool animating;

    // Vulkan nesneleri
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    
    // Swap Chain nesneleri
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // Render Pipeline nesneleri (Şimdilik yorum satırında)
    // VkRenderPass renderPass;
    // VkPipelineLayout pipelineLayout;
    // VkPipeline graphicsPipeline;
    // std::vector<VkFramebuffer> swapChainFramebuffers;
    
    // Komut tamponları
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

// -----------------------------------------------------------------------------
// VULKAN BAŞLATMA FONKSİYONLARI
// -----------------------------------------------------------------------------

/**
 * Vulkan Instance'ı oluşturur.
 */
bool createInstance(Engine* engine) {
    LOGI("Vulkan: Instance oluşturuluyor...");
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan PBR Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "HybridEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Gerekli uzantıları al (VK_KHR_android_surface ve VK_KHR_surface zorunlu)
    std::vector<const char*> instanceExtensions = {
        "VK_KHR_surface",
        "VK_KHR_android_surface"
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    
    // Geçerlilik katmanlarını devre dışı bırakıyoruz (Daha sonra eklenebilir)
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &engine->instance) != VK_SUCCESS) {
        LOGE("VULKAN HATA: Instance oluşturulamadı!");
        return false;
    }
    LOGI("Vulkan: Instance başarıyla oluşturuldu.");
    return true;
}

/**
 * Android Native Window için Vulkan Surface'ı oluşturur.
 */
bool createSurface(Engine* engine) {
    LOGI("Vulkan: Surface oluşturuluyor...");
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = engine->app->window;

    if (vkCreateAndroidSurfaceKHR(engine->instance, &createInfo, nullptr, &engine->surface) != VK_SUCCESS) {
        LOGE("VULKAN HATA: Vulkan Surface (Pencere Yüzeyi) oluşturulamadı!");
        return false;
    }
    LOGI("Vulkan: Surface başarıyla oluşturuldu.");
    return true;
}

/**
 * Fiziksel Cihazı (GPU) seçer.
 */
bool pickPhysicalDevice(Engine* engine) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(engine->instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        LOGE("VULKAN HATA: Vulkan destekleyen cihaz (GPU) bulunamadı!");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(engine->instance, &deviceCount, devices.data());

    // Basitçe ilk bulduğumuz cihazı seçiyoruz
    engine->physicalDevice = devices[0];
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(engine->physicalDevice, &deviceProperties);
    LOGI("Vulkan: Seçilen Cihaz: %s", deviceProperties.deviceName);
    return true;
}

/**
 * Mantıksal Cihazı ve Grafik Kuyruğunu oluşturur.
 */
bool createLogicalDevice(Engine* engine) {
    // Grafik Kuyruğu Ailesini Bul
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(engine->physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(engine->physicalDevice, &queueFamilyCount, queueFamilies.data());

    uint32_t graphicsQueueFamilyIndex = (uint32_t)-1;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        // Hem grafik hem de yüzey desteği olan kuyruğu bul
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(engine->physicalDevice, i, engine->surface, &presentSupport);

        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
            graphicsQueueFamilyIndex = i;
            break;
        }
    }

    if (graphicsQueueFamilyIndex == (uint32_t)-1) {
        LOGE("VULKAN HATA: Grafik ve Sunum (Present) desteği olan Kuyruk Ailesi bulunamadı!");
        return false;
    }

    // Mantıksal Cihazı Oluştur
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{}; // Özellikleri etkinleştirmiyoruz (şimdilik)

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Gerekli cihaz uzantıları (Swap Chain zorunlu)
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Cihazı oluştur
    if (vkCreateDevice(engine->physicalDevice, &createInfo, nullptr, &engine->device) != VK_SUCCESS) {
        LOGE("VULKAN HATA: Mantıksal Cihaz oluşturulamadı!");
        return false;
    }

    // Grafik Kuyruğunu al
    vkGetDeviceQueue(engine->device, graphicsQueueFamilyIndex, 0, &engine->graphicsQueue);
    LOGI("Vulkan: Mantıksal Cihaz ve Grafik Kuyruğu başarıyla oluşturuldu.");
    return true;
}

/**
 * Swap Chain'i oluşturur. (Çizim yapılabilmesi için gereken resim serisi)
 */
bool createSwapChain(Engine* engine) {
    // 1. Yüzey Özelliklerini (Surface Capabilities) sorgula
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine->physicalDevice, engine->surface, &capabilities);

    // 2. Yüzey Formatını (Surface Format) seç
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physicalDevice, engine->surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physicalDevice, engine->surface, &formatCount, availableFormats.data());

    VkSurfaceFormatKHR surfaceFormat = availableFormats[0]; // Basitçe ilk formatı seç
    engine->swapChainImageFormat = surfaceFormat.format;

    // 3. Takas Uzantısını (Swap Extent - Çözünürlük) ayarla
    engine->swapChainExtent = capabilities.currentExtent;
    if (capabilities.currentExtent.width == (uint32_t)-1) {
        // Eğer çözünürlük tanımsızsa, pencerenin boyutlarını kullan
        engine->swapChainExtent.width = ANativeWindow_getWidth(engine->app->window);
        engine->swapChainExtent.height = ANativeWindow_getHeight(engine->app->window);
    }
    
    // 4. Gerekli Resim Sayısını belirle (Minimum + 1)
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // 5. Swap Chain Oluşturma Bilgilerini ayarla
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = engine->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = engine->swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Doğrudan renklendirme için
    
    // Kuyruk Paylaşım Modu (Tek kuyruk kullandığımız için Exclusive)
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Diğer pencerelerle karışım olmasın
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // V-Sync
    createInfo.clipped = VK_TRUE; // Çizilmeyen piksellerle uğraşma
    createInfo.oldSwapchain = VK_NULL_HANDLE; // Yeniden oluşturulmadığı için boş

    // Swap Chain'i oluştur
    if (vkCreateSwapchainKHR(engine->device, &createInfo, nullptr, &engine->swapChain) != VK_SUCCESS) {
        LOGE("VULKAN HATA: Swap Chain oluşturulamadı!");
        return false;
    }

    // Swap Chain resimlerini al
    vkGetSwapchainImagesKHR(engine->device, engine->swapChain, &imageCount, nullptr);
    engine->swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(engine->device, engine->swapChain, &imageCount, engine->swapChainImages.data());
    
    LOGI("Vulkan: Swap Chain başarıyla oluşturuldu. Resim Sayısı: %d", imageCount);
    return true;
}

/**
 * Swap Chain Resimleri için Görüntü Görünümlerini (Image Views) oluşturur.
 */
bool createImageViews(Engine* engine) {
    engine->swapChainImageViews.resize(engine->swapChainImages.size());

    for (size_t i = 0; i < engine->swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = engine->swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = engine->swapChainImageFormat;
        
        // Renk bileşen eşleştirmesi
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Resim alt kaynağı aralığı
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(engine->device, &createInfo, nullptr, &engine->swapChainImageViews[i]) != VK_SUCCESS) {
            LOGE("VULKAN HATA: %zu. Image View oluşturulamadı!", i);
            return false;
        }
    }
    LOGI("Vulkan: Tüm Image View'lar başarıyla oluşturuldu.");
    return true;
}

// -----------------------------------------------------------------------------
// TEMİZLEME FONKSİYONLARI
// -----------------------------------------------------------------------------

/**
 * Vulkan nesnelerini temizler.
 */
void engine_term_display(Engine* engine) {
    engine->animating = false;

    if (engine->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(engine->device); // Tüm işlemlerin bitmesini bekle

        // Image View'ları temizle
        for (auto imageView : engine->swapChainImageViews) {
            vkDestroyImageView(engine->device, imageView, nullptr);
        }
        engine->swapChainImageViews.clear();

        // Swap Chain'i temizle
        if (engine->swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(engine->device, engine->swapChain, nullptr);
            engine->swapChain = VK_NULL_HANDLE;
        }

        // Mantıksal Cihazı temizle
        vkDestroyDevice(engine->device, nullptr);
        engine->device = VK_NULL_HANDLE;
    }

    // Surface'ı temizle
    if (engine->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(engine->instance, engine->surface, nullptr);
        engine->surface = VK_NULL_HANDLE;
    }

    // Instance'ı temizle
    if (engine->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(engine->instance, nullptr);
        engine->instance = VK_NULL_HANDLE;
    }
    
    LOGI("Vulkan: Display nesneleri temizlendi.");
}


// -----------------------------------------------------------------------------
// ANA MOTOR FONKSİYONLARI
// -----------------------------------------------------------------------------

/**
 * Uygulama ilk kez başlatıldığında veya pencere değiştiğinde (ekran döndürme vb.) çağrılır.
 */
void engine_init_display(Engine* engine) {
    LOGI("Engine: Display başlatılıyor...");

    // Vulkan'ı başlat
    if (!createInstance(engine)) goto ERROR_EXIT;
    if (!createSurface(engine)) goto ERROR_EXIT;
    if (!pickPhysicalDevice(engine)) goto ERROR_EXIT;
    if (!createLogicalDevice(engine)) goto ERROR_EXIT;
    if (!createSwapChain(engine)) goto ERROR_EXIT;
    if (!createImageViews(engine)) goto ERROR_EXIT;

    // Eğer buraya geldiysek, Vulkan başarılı
    LOGI("Vulkan Başarıyla Başlatıldı!");
    engine->animating = true;
    return;

ERROR_EXIT:
    // Hata durumunda tüm kaynakları temizle
    LOGE("KRİTİK HATA: Vulkan başlatma başarısız. Geri Dönüş (Fallback) yapılamadı.");
    engine_term_display(engine);
    // Bu noktada GLES'e geçmek için bir kod olmalıydı, ancak şimdilik uygulamayı bırakıyoruz.
    engine->app->destroyRequested = 1; 
}


/**
 * Her kare çizildiğinde çağrılır.
 * Şimdilik sadece başarılı bir şekilde başlatıldığımızı göstermek için boş.
 */
void engine_draw_frame(Engine* engine) {
    if (!engine->animating) {
        return;
    }

    // Vulkan çizim komutları buraya gelecek
    // Örneğin: Komut tamponunu kaydet, çizimi yap, sun (present)

    // Şimdilik sadece bir kare çizim komutunun başarılı olduğunu varsayalım:
    // Çizim komutları ve Swapchain Present kodu buraya gelecek.
}

/**
 * Sensör, dokunmatik ve pencere gibi komutları işler.
 */
void engine_handle_cmd(android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // Pencere oluşturuldu/yeniden oluşturuldu
            if (app->window != NULL) {
                
                // --- KRİTİK ÇÖKME DÜZELTMESİ BAŞLANGIÇ ---
                // Adreno/Qualcomm cihazlarda Vulkan/EGL başlatma çökmesini (Unknown Format)
                // önlemek için pencere yüzeyinin formatını açıkça RGBA_8888 olarak ayarlıyoruz.
                ANativeWindow_setBuffersGeometry(
                    app->window,
                    ANativeWindow_getWidth(app->window),
                    ANativeWindow_getHeight(app->window),
                    WINDOW_FORMAT_RGBA_8888
                );
                // --- KRİTİK ÇÖKME DÜZELTMESİ SONUÇ ---

                engine->app->window = app->window;
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // Pencere yok ediliyor (Uygulama arka plana geçti veya kapatıldı)
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // Uygulama odak kazandı, animasyonu başlat
            engine->animating = true;
            break;
        case APP_CMD_LOST_FOCUS:
            // Uygulama odak kaybetti, animasyonu durdur
            engine->animating = false;
            engine_draw_frame(engine); // Son bir kez çizim yap
            break;
        case APP_CMD_DESTROY:
            // Uygulama sonlandırılıyor
            engine_term_display(engine);
            LOGI("Engine: Uygulama sonlandırıldı.");
            break;
    }
}

/**
 * Dokunmatik/Giriş olaylarını işler.
 */
int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    // Şimdilik dokunma olaylarını ignore ediyoruz.
    return 0;
}

/**
 * Android ana döngüsü (Main Loop)
 */
void android_main(struct android_app* state) {
    Engine engine{};
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;
    engine.animating = false;

    LOGI("Engine: Main Loop başladı.");

    // Ana olay döngüsü
    while (state->destroyRequested == 0) {
        int ident;
        int events;
        struct android_poll_source* source;

        // Bir sonraki olayı bekle
        while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
            
            // Bir olay geldi, onu işle
            if (source != NULL) {
                source->process(state, source);
            }

            // Uygulama kapatılmak istendiyse döngüyü kır
            if (state->destroyRequested != 0) {
                break;
            }
        }

        if (engine.animating) {
            engine_draw_frame(&engine);
        }
    }
    
    LOGI("Engine: Main Loop sona erdi.");
}
