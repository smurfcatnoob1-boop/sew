#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h> 
#include <android/log.h>
#include <vector>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <string> // HATA ÇÖZÜMÜ 1: std::string'i tanımlamak için string kütüphanesini dahil et.

#define LOG_TAG "VulkanEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Kullanılacak Cihaz ve Ray Tracing Uzantıları
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    // RTX/PBR için gelecekte eklenecek uzantılar buraya gelecek:
    // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, 
    // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME 
};

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

// Swap Chain Yüzey Formatını Seçme
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    // HATA ÇÖZÜMÜ 2: Döngüden sonra, eğer uygun format bulunamazsa, 
    //  listesinin ilk elemanını döndürüyoruz. (availableFormats[0])
    return availableFormats[0]; 
}

// Swap Chain Sunum Modunu Seçme
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { // En düşük gecikme için Mailbox (Üçlü Tamponlama)
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR; // V-Sync (Varsayılan)
}

// Swap Chain Kapsamını Seçme (Ekran Çözünürlüğü)
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, ANativeWindow* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int32_t width = ANativeWindow_getWidth(window);
        int32_t height = ANativeWindow_getHeight(window);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // HATA ÇÖZÜMÜ 3: std::clamp yerine std::max ve std::min kullanarak 
        // daha eski C++ standartları (NDK) ile uyumluluğu sağlıyoruz.
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

// Swap Chain Desteklerini Sorgulama
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
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

// Cihazın Gerekli Uzantıları Destekleyip Desteklemediğini Kontrol Eder
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    
    // HATA ÇÖZÜMÜ 4: requiredExtensions.erase(extensionCount) yerine,
    // Mevcut uzantıları kontrol edip gereksinim setinden siliyoruz.
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

// Queue Family'leri Bulma
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
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


// Cihazın Vulkan 1.2 Desteğini Kontrol Eder (RTX için zorunlu)
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, QueueFamilyIndices& indices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Vulkan 1.2 kontrolü
    if (deviceProperties.apiVersion < VK_API_VERSION_1_2) {
        return false;
    }
    
    // Uzantı kontrolü için Queue Family indexlerini buluyoruz.
    indices = findQueueFamilies(device, surface); 
    
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    
    // Vulkan 1.2, Gerekli uzantılar (Swapchain) ve uygun queue family'ler.
    return extensionsSupported && swapChainAdequate && indices.isComplete();
}

// ********************************** BAŞLATMA FONKSİYONLARI **********************************

// Fiziksel Cihaz Seçimi
bool pickPhysicalDevice(Engine* engine) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(engine->vkInstance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        LOGE("Vulkan uyumlu cihaz bulunamadı!");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(engine->vkInstance, &deviceCount, devices.data());
    
    for (const auto& device : devices) {
        QueueFamilyIndices indices; // findQueueFamilies isDeviceSuitable içinde çağrılıyor
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
    
    LOGE("Gerekli Vulkan 1.2 ve uzantılarını destekleyen uygun cihaz bulunamadı!");
    return false;
}

// Logical Device Oluşturma (GPU'ya emir göndermek için)
bool createLogicalDevice(Engine* engine) {
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
    // Burada Ray Tracing ve PBR için gerekli fiziksel özellikleri etkinleştirebiliriz.
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    // Cihaz uzantılarını etkinleştirme (Swapchain zorunlu)
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (vkCreateDevice(engine->physicalDevice, &createInfo, nullptr, &engine->device) != VK_SUCCESS) {
        LOGE("Logical Device oluşturulamadı!");
        return false;
    }
    
    // Queue (Sıra) tutacak değişkenleri tanımlama
    vkGetDeviceQueue(engine->device, indices.graphicsFamily, 0, &engine->graphicsQueue);
    vkGetDeviceQueue(engine->device, indices.presentFamily, 0, &engine->presentQueue);
    
    LOGI("Logical Device ve Queues Başarıyla Oluşturuldu.");
    return true;
}

// Swap Chain Oluşturma (Görüntüyü Ekrana Taşıyan Mekanizma)
bool createSwapChain(Engine* engine) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(engine->physicalDevice, engine->surface);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, engine->app->window);
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

    // Queue Ailesi Paylaşım Modu
    QueueFamilyIndices indices = findQueueFamilies(engine->physicalDevice, engine->surface);
    uint32_t queueFamilyIndices[] = {(uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Aynı anda birden fazla sıra ailesi
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // Yeniden oluşturma için kullanılacak

    if (vkCreateSwapchainKHR(engine->device, &createInfo, nullptr, &engine->swapchain) != VK_SUCCESS) {
        LOGE("Swap Chain oluşturulamadı!");
        return false;
    }

    // Swap Chain Görüntülerini ve View'lerini Alma
    vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &imageCount, nullptr);
    engine->swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &imageCount, engine->swapchainImages.data());

    engine->swapchainImageFormat = surfaceFormat.format;
    engine->swapchainExtent = extent;
    
    LOGI("Swap Chain (%dx%d) Başarıyla Oluşturuldu.", extent.width, extent.height);
    return true;
}

// Render Pass Oluşturma (Çizim İşleminin İskeleti)
bool createRenderPass(Engine* engine) {
    // 1. Renk Yüzeyinin Tanımı (PBR Sonucu Buraya Yazılacak)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = engine->swapchainImageFormat; // Swap chain formatıyla aynı
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Her frame'i temizle
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Sonucu kaydet
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Ekrana sunum için hazırla

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 2. Subpass (Alt Geçiş) Tanımı
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // 3. Render Pass Oluşturma
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

// 1. Vulkan Instance ve Surface Oluşturma
bool createInstanceAndSurface(Engine* engine) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SevgiliOyunu_Vulkan_PBR";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "CustomVulkanEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2; 

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
    
    if (!pickPhysicalDevice(engine)) return false; 
    
    if (!createLogicalDevice(engine)) return false; // Logical Device
    if (!createSwapChain(engine)) return false;     // Swap Chain
    if (!createRenderPass(engine)) return false;    // Render Pass
    
    LOGI("Vulkan Engine Temelleri Hazır. PBR/RTX için bir sonraki aşamaya geçiliyor.");
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
