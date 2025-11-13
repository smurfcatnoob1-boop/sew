#include <android/native_activity.h>
#include "android_native_app_glue.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/log.h>
#include <vector>
#include <set>
#include <algorithm>
#include <string>

#define LOG_TAG "VulkanEngineMinimal"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Kullanılacak sadece Swapchain uzantısını yine de tanımlıyoruz
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
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // Diğer Temel Nesneler
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
};

// Basit yardımcı yapılar (Değişmedi)
struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;
    bool isComplete() { return graphicsFamily != -1 && presentFamily != -1; }
};

// Cihazın Gerekli Uzantıları Destekleyip Desteklemediğini Kontrol Eder (Aynı)
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

// Queue Family'leri Bulma (Aynı)
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

// Cihazın Uygunluğunu Kontrol Eder (Sadece uzantılar ve queue'lar)
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, QueueFamilyIndices& indices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties); 

    if (deviceProperties.apiVersion < VK_API_VERSION_1_1) return false;

    indices = findQueueFamilies(device, surface);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    
    // SwapChain desteği kontrolü kaldırıldı, sadece uzantının varlığı yeter
    return extensionsSupported && indices.isComplete();
}

// ********************************** BAŞLATMA FONKSİYONLARI **********************************

// Vulkan Instance ve Surface Oluşturma (Aynı)
bool createInstanceAndSurface(Engine* engine) {
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

    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = engine->app->window;

    // Bu, GLES Hack'inden sonra bile hataya neden olabilecek kritik Vulkan Surface oluşumu.
    if (vkCreateAndroidSurfaceKHR(engine->vkInstance, &surfaceCreateInfo, nullptr, &engine->surface) != VK_SUCCESS) {
        LOGE("Vulkan Surface oluşturulamadı!");
        return false;
    }

    LOGI("Vulkan Instance ve Surface Başarıyla Oluşturuldu.");
    return true;
}

// Fiziksel Cihaz Seçimi (Aynı)
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
            LOGI("Seçilen Fiziksel Cihaz: %s", deviceProperties.deviceName);
            return true;
        }
    }

    LOGE("Gerekli Vulkan özelliklerini destekleyen uygun cihaz bulunamadı!");
    return false;
}

// Logical Device Oluşturma (Aynı)
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

// Vulkan Motorunun Temiz Başlatılması (Swapchain ve RenderPass devre dışı!)
bool init_vulkan_minimal(Engine* engine) {
    if (engine->app->window == NULL) {
        LOGE("Pencere başlatılmadı!");
        return false;
    }

    if (!createInstanceAndSurface(engine)) return false;
    if (!pickPhysicalDevice(engine)) return false;
    if (!createLogicalDevice(engine)) return false;

    LOGI("Vulkan Temelleri (Instance/Device) Başlatıldı. SwapChain Atlandı.");
    return true;
}

// Uygulama Komutlarını İşleyen Fonksiyon
void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != NULL) {
                if (init_vulkan_minimal(engine)) { 
                    engine->running = true;
                }
            }
            break;
        case APP_CMD_TERM_WINDOW:
            engine->running = false;
            // Vulkan temizleme (term_vulkan_minimal) buraya gelecek
            break;
    }
}

// Ana Android Giriş Noktası
void android_main(struct android_app* state) {
    struct Engine engine = {0};
    state->userData = &engine;
    engine.app = state;
    state->onAppCmd = engine_handle_cmd;

    LOGI("Vulkan Minimal Engine Başlatıldı.");

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

        // Uygulama çalışıyorsa, sonsuz döngüde kalır ve çökmez.
        if (engine.running) { 
            // Çizim yapılmayacak, sadece döngüde kalacak.
        }
    }
}
