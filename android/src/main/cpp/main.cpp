#include <android/native_activity.h>
#include "android_native_app_glue.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/log.h>
#include <EGL/egl.h> // GLES Hack için eklendi
#include <GLES3/gl3.h> // GLES Hack için eklendi
#include <vector>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <string>

#define LOG_TAG "HybridEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Kullanılacak Cihaz ve Uzantılar
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// Vulkan Motoru Durum Yapısı
struct Engine {
    struct android_app* app;
    bool running = false;

    // Vulkan Çekirdek Nesneleri
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE; // Bu yüzey GLES tarafından stabilize edilecek

    // PBR ve Swapchain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    // Diğer Temel Nesneler
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
};

// Queue Family (Sıra Ailesi) Endeksleri
struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;
    bool isComplete() {
        return graphicsFamily != -1 && presentFamily != -1;
    }
};

// Swap Chain Destek Detayları
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// ********************************** UTILITY FONKSİYONLARI **********************************

// Swap Chain Yüzey Formatını Seçme (HATA ATLAMA ÇÖZÜMÜ)
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // 1. En güvenilir formatı arayın (B8G8R8A8_UNORM + SRGB)
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            LOGI("Seçilen SwapChain Formatı: VK_FORMAT_B8G8R8A8_UNORM (TERCİH EDİLEN)");
            return availableFormat;
        }
    }

    // 2. Format 56 hatasını atlatmak için listedeki bir sonraki formatı dene
    if (availableFormats.size() > 1) {
        LOGI("Seçilen SwapChain Formatı: Cihazın Sunduğu 2. Format (Hata Atlatma)");
        return availableFormats[1];
    }
    
    // 3. Son çare
    LOGI("Seçilen SwapChain Formatı: Cihazın Sunduğu İlk Format (Son Çare)");
    return availableFormats[0];
}

// Swap Chain Sunum Modunu Seçme
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    return VK_PRESENT_MODE_FIFO_KHR; // En güvenilir mod
}

// Swap Chain Kapsamını Seçme (Ekran Çözünürlüğü)
// KRİTİK DÜZELTME: Boyutu 1x1 olarak zorluyoruz.
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, ANativeWindow* window) {
    // Gralloc (4x4) boyut hatasını atlamak için 1x1'e zorlama. 
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(1),
        static_cast<uint32_t>(1)
    };
    LOGI("Swap Chain Kapsamı Zorla 1x1 Yapıldı (Gralloc Hatasını Atlamak İçin).");
    return actualExtent;
}

// Swap Chain Desteklerini Sorgulama (Diğer yardımcı fonksiyonlar aynı kalır)
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    // ... (Önceki kodunuz ile aynı)
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// Cihaz Uzantı Kontrolü (Aynı kalır)
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // ... (Önceki kodunuz ile aynı)
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

// Queue Family'leri Bulma (Aynı kalır)
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    // ... (Önceki kodunuz ile aynı)
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

// Cihaz Uygunluk Kontrolü (Aynı kalır)
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, QueueFamilyIndices& indices) {
    // ... (Önceki kodunuz ile aynı)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties); 

    if (deviceProperties.apiVersion < VK_API_VERSION_1_1) {
        return false;
    }

    indices = findQueueFamilies(device, surface);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return extensionsSupported && swapChainAdequate && indices.isComplete();
}

// ********************************** KRİTİK GLES HACK FONKSİYONU **********************************

// GLES ile yüzeyi stabilize edip hemen temizler (Vulkan'ı kurtarır)
void gles_surface_stabilization_hack(ANativeWindow* window) {
    LOGI("GLES Yüzey Stabilizasyon Hack'i Başlatılıyor...");

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) { LOGE("EGL Display alınamadı!"); return; }
    
    eglInitialize(display, 0, 0);

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_config;
    if (eglChooseConfig(display, attribs, &config, 1, &num_config) != EGL_TRUE || num_config == 0) { 
        LOGE("EGL Konfigürasyonu seçilemedi!"); 
        eglTerminate(display);
        return; 
    }

    // 1. Surface Oluşturma (Bu adım Adreno sürücüsünü hazırlar)
    EGLSurface surface = eglCreateWindowSurface(display, config, window, NULL);
    if (surface == EGL_NO_SURFACE) { LOGE("EGL Surface oluşturulamadı!"); eglTerminate(display); return; }
    
    // 2. Context Oluşturma
    const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) { 
        LOGE("EGL Context oluşturulamadı!"); 
        eglDestroySurface(display, surface); 
        eglTerminate(display); 
        return; 
    }
    
    // 3. Context'i aktif et ve GLES çağrısı yap (Sürücüyü kesin uyandırmak için)
    eglMakeCurrent(display, surface, surface, context);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
    glClear(GL_COLOR_BUFFER_BIT);

    // 4. Temizleme (Vulkan'a devretmek için her şeyi yok et)
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    LOGI("GLES Hack Başarılı. Yüzey Vulkan için temizlendi ve hazır.");
}

// ********************************** BAŞLATMA FONKSİYONLARI **********************************

// Fiziksel Cihaz Seçimi (Aynı kalır)
bool pickPhysicalDevice(Engine* engine) {
    // ... (Önceki kodunuz ile aynı)
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(engine->vkInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        LOGE("Vulkan uyumlu cihaz bulunamadı!");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(engine->vkInstance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        QueueFamilyIndices indices;
        if (isDeviceSuitable(device, engine->surface, indices)) {
            engine->physicalDevice = device;
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            LOGI("Seçilen Fiziksel Cihaz: %s (Vulkan API v%d.%d.%d)",
                 deviceProperties.deviceName,
                 VK_VERSION_MAJOR(deviceProperties.apiVersion),
                 VK_VERSION_MINOR(deviceProperties.apiVersion),
                 VK_VERSION_PATCH(deviceProperties.apiVersion));
            return true;
        }
    }

    LOGE("Gerekli Vulkan 1.1 ve uzantılarını destekleyen uygun cihaz bulunamadı!");
    return false;
}

// Logical Device Oluşturma (Aynı kalır)
bool createLogicalDevice(Engine* engine) {
    // ... (Önceki kodunuz ile aynı)
    QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(engine->physicalDevice, &createInfo, nullptr, &engine->device) != VK_SUCCESS) {
        LOGE("Logical Device oluşturulamadı!");
        return false;
    }

    vkGetDeviceQueue(engine->device, indices.graphicsFamily, 0, &engine->graphicsQueue);
    vkGetDeviceQueue(engine->device, indices.presentFamily, 0, &engine->presentQueue);

    LOGI("Logical Device ve Queues Başarıyla Oluşturuldu.");
    return true;
}

// Swap Chain Oluşturma (Önceki tüm hileler dahil)
bool createSwapChain(Engine* engine) {
    // ... (Önceki kodunuz ile aynı)
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(engine->physicalDevice, engine->surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, engine->app->window);

    // KRİTİK DÜZELTME: Minimum imaj sayısını minimum + 1 olarak zorluyoruz.
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; 
    
    // Max imaj sayısını aşmamaya dikkat et
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = engine->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    // Adreno/Gralloc uyumluluğu için TRANSFER_DST_BIT eklendi.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);
    uint32_t queueFamilyIndices[] = {(uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // UYUMSUZLUK DÜZELTMESİ: Dönüşümü IDENTITY olarak sabitliyoruz.
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; 
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(engine->device, &createInfo, nullptr, &engine->swapchain) != VK_SUCCESS) {
        LOGE("Swap Chain oluşturulamadı!");
        return false;
    }

    vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &imageCount, nullptr);
    engine->swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &imageCount, engine->swapchainImages.data());

    engine->swapchainImageFormat = surfaceFormat.format;
    engine->swapchainExtent = extent;

    LOGI("Swap Chain (%dx%d, Images:%d) Başarıyla Oluşturuldu.", extent.width, extent.height, imageCount);
    return true;
}

// Render Pass Oluşturma (Aynı kalır)
bool createRenderPass(Engine* engine) {
    // ... (Önceki kodunuz ile aynı)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = engine->swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    if (vkCreateRenderPass(engine->device, &renderPassInfo, nullptr, &engine->renderPass) != VK_SUCCESS) {
        LOGE("Render Pass oluşturulamadı!");
        return false;
    }

    LOGI("Render Pass Başarıyla Oluşturuldu.");
    return true;
}

// Vulkan Instance ve Surface Oluşturma (Aynı kalır)
bool createInstanceAndSurface(Engine* engine) {
    // ... (Önceki kodunuz ile aynı)
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SevgiliOyunu_Vulkan_PBR";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "CustomVulkanEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
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

// Vulkan Motorunun Temiz Başlatılması
bool init_vulkan(Engine* engine) {
    if (engine->app->window == NULL) {
        LOGE("Pencere başlatılmadı!");
        return false;
    }

    if (!createInstanceAndSurface(engine)) return false;

    // ********* KRİTİK GLES HACK ÇAĞRISI *********
    gles_surface_stabilization_hack(engine->app->window);
    // ********************************************

    // KRİTİK AYAR: Swap Chain oluşturulmadan önce pencere boyutunu zorla 1x1 yapıyoruz.
    int32_t set_buffers_result = ANativeWindow_setBuffersGeometry(
        engine->app->window, 
        1,  // width
        1,  // height
        WINDOW_FORMAT_RGBX_8888 
    );

    if (set_buffers_result != 0) {
        LOGE("ANativeWindow_setBuffersGeometry (1x1) başarısız oldu: %d", set_buffers_result);
    } else {
        LOGI("ANativeWindow_setBuffersGeometry (1x1) başarılı.");
    }


    if (!pickPhysicalDevice(engine)) return false;
    if (!createLogicalDevice(engine)) return false;
    if (!createSwapChain(engine)) return false;
    if (!createRenderPass(engine)) return false;

    LOGI("Vulkan Engine Temelleri Hazır. PBR/RTX için bir sonraki aşamaya geçiliyor.");
    return true;
}

// Uygulama Komutlarını İşleyen Fonksiyon (Aynı kalır)
void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != NULL) {
                if (init_vulkan(engine)) { // Vulkan'ı başlat
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

// Ana Android Giriş Noktası (Aynı kalır)
void android_main(struct android_app* state) {
    struct Engine engine = {0};
    state->userData = &engine;
    engine.app = state;
    state->onAppCmd = engine_handle_cmd;

    LOGI("Hibrit Vulkan/GLES Native Engine Başlatıldı.");

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
            // VULKAN Çizim (vkQueueSubmit, vkQueuePresentKHR) buraya gelecek.
            // Şimdilik boş kalacak, sadece çalışıp çalışmadığını göreceğiz.
        }
    }
}

// Android sisteminin aradığı zorunlu Native Activity başlangıç fonksiyonu.
