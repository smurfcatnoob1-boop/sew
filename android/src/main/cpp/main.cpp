#include <android/native_activity.h>
#include "android_native_app_glue.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/log.h>
#include <vector>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <string>

// GLES Uyumlu Kütüphaneler (FALLBACK İÇİN)
#include <EGL/egl.h> 
#include <GLES3/gl3.h>

#define LOG_TAG "HybridEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// **************************** UZANTILAR VE TEMEL YAPILAR ****************************

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;
    bool isComplete() {
        return graphicsFamily != -1 && presentFamily != -1;
    }
};

struct Engine {
    struct android_app* app;
    bool running = false;
    bool is_vulkan = false; // true ise Vulkan, false ise GLES

    // Vulkan Çekirdek Nesneleri
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    // Vulkan Swapchain ve Çizim Nesneleri
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    // Vulkan Senkronizasyon
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    // GLES Nesneleri (Fallback için)
    EGLDisplay glesDisplay = EGL_NO_DISPLAY;
    EGLSurface glesSurface = EGL_NO_SURFACE;
    EGLContext glesContext = EGL_NO_CONTEXT;
};

// ********************************** VULKAN UTILITY FONKSİYONLARI **********************************

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

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    return VK_PRESENT_MODE_FIFO_KHR; // En güvenli mod
}

// KRİTİK: GRALLOC BUG'I İÇİN BOYUTU ZORLA 1x1 YAPMA VEYA PENCERE BOYUTUNU KULLANMA
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, ANativeWindow* window) {
    if (capabilities.currentExtent.width != (uint32_t)-1) {
        return capabilities.currentExtent;
    } 
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(1),
        static_cast<uint32_t>(1)
    };
    LOGI("Swap Chain Kapsamı zorla 1x1 Yapıldı (Gralloc Hilesi).");
    return actualExtent;
}

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

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) { indices.graphicsFamily = i; }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) { indices.presentFamily = i; }
        if (indices.isComplete()) { break; }
        i++;
    }
    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, QueueFamilyIndices& indices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, deviceProperties); 
    if (deviceProperties.apiVersion < VK_API_VERSION_1_1) { return false; }
    indices = findQueueFamilies(device, surface);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    return extensionsSupported && swapChainAdequate && indices.isComplete();
}

// **************************** VULKAN BAŞLATMA ZİNCİRİ **********************************

bool createInstanceAndSurface(Engine* engine) {
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
    if (vkCreateInstance(&createInfo, nullptr, &engine->vkInstance) != VK_SUCCESS) { LOGE("Vulkan Instance oluşturulamadı!"); return false; }

    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = engine->app->window;

    if (vkCreateAndroidSurfaceKHR(engine->vkInstance, &surfaceCreateInfo, nullptr, &engine->surface) != VK_SUCCESS) { LOGE("Vulkan Surface oluşturulamadı!"); return false; }
    LOGI("Vulkan Instance ve Surface Başarıyla Oluşturuldu.");
    return true;
}

bool pickPhysicalDevice(Engine* engine) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(engine->vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0) { LOGE("Vulkan uyumlu cihaz bulunamadı!"); return false; }
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
    LOGE("Uygun Vulkan 1.1 cihaz bulunamadı!");
    return false;
}

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
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    if (vkCreateDevice(engine->physicalDevice, &createInfo, nullptr, &engine->device) != VK_SUCCESS) {
        LOGE("Logical Device oluşturulamadı!"); return false;
    }
    vkGetDeviceQueue(engine->device, indices.graphicsFamily, 0, &engine->graphicsQueue);
    vkGetDeviceQueue(engine->device, indices.presentFamily, 0, &engine->presentQueue);
    LOGI("Logical Device ve Queues Başarıyla Oluşturuldu.");
    return true;
}

bool createSwapChain(Engine* engine) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(engine->physicalDevice, engine->surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, engine->app->window);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount; 
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = engine->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
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

    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; 
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(engine->device, &createInfo, nullptr, &engine->swapchain) != VK_SUCCESS) {
        LOGE("KRİTİK HATA: Swap Chain oluşturulamadı! Vulkan başlatma başarısız."); 
        return false;
    }

    vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &imageCount, nullptr);
    engine->swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(engine->device, engine->swapchain, &imageCount, engine->swapchainImages.data());

    engine->swapchainImageFormat = surfaceFormat.format;
    engine->swapchainExtent = extent;

    LOGI("Swap Chain (%dx%d) Başarıyla Oluşturuldu.", extent.width, extent.height);
    return true;
}

bool createImageViews(Engine* engine) {
    engine->swapchainImageViews.resize(engine->swapchainImages.size());
    for (size_t i = 0; i < engine->swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = engine->swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = engine->swapchainImageFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(engine->device, &createInfo, nullptr, &engine->swapchainImageViews[i]) != VK_SUCCESS) {
            LOGE("Image View oluşturulamadı!"); return false;
        }
    }
    return true;
}

bool createRenderPass(Engine* engine) {
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
        LOGE("Render Pass oluşturulamadı!"); return false;
    }
    return true;
}

bool createFramebuffers(Engine* engine) {
    engine->swapchainFramebuffers.resize(engine->swapchainImageViews.size());
    for (size_t i = 0; i < engine->swapchainImageViews.size(); i++) {
        VkImageView attachments[] = { engine->swapchainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = engine->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = engine->swapchainExtent.width;
        framebufferInfo.height = engine->swapchainExtent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(engine->device, &framebufferInfo, nullptr, &engine->swapchainFramebuffers[i]) != VK_SUCCESS) {
            LOGE("Framebuffer oluşturulamadı!"); return false;
        }
    }
    return true;
}

bool createCommandObjects(Engine* engine) {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(engine->physicalDevice, engine->surface);
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(engine->device, &poolInfo, nullptr, &engine->commandPool) != VK_SUCCESS) { LOGE("Command Pool oluşturulamadı!"); return false; }
    engine->commandBuffers.resize(engine->swapchainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = engine->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)engine->commandBuffers.size();
    if (vkAllocateCommandBuffers(engine->device, &allocInfo, engine->commandBuffers.data()) != VK_SUCCESS) { LOGE("Command Buffers oluşturulamadı!"); return false; }
    return true;
}

bool createSyncObjects(Engine* engine) {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 
    if (vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &engine->imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &engine->renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(engine->device, &fenceInfo, nullptr, &engine->inFlightFence) != VK_SUCCESS) {
        LOGE("Senkronizasyon nesneleri oluşturulamadı!"); return false;
    }
    return true;
}

// **************************** GLES FALLBACK ****************************

bool init_gles_fallback(Engine* engine) {
    if (engine->app->window == NULL) { LOGE("GLES: Pencere başlatılmadı!"); return false; }
    
    engine->glesDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (engine->glesDisplay == EGL_NO_DISPLAY || eglInitialize(engine->glesDisplay, 0, 0) != EGL_TRUE) { LOGE("GLES: Display/Başlatma Başarısız!"); return false; }

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

    LOGI("GLES: OpenGL ES 3.0 Başarıyla Başlatıldı. Fallback Devrede.");
    return true;
}


// **************************** ÇİZİM KOMUTLARI (Mavi Ekran Testi) ****************************

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Engine* engine) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) { LOGE("Command Buffer başlatılamadı!"); return; }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = engine->renderPass;
    renderPassInfo.framebuffer = engine->swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = engine->swapchainExtent;

    // Mavi Clear Color (R:0.1, G:0.3, B:0.5)
    VkClearValue clearColor = {{{0.1f, 0.3f, 0.5f, 1.0f}}}; 
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // Buraya asıl PBR/RTX çizim komutlarınız gelecek
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) { LOGE("Command Buffer sonlandırılamadı!"); }
}

void draw_vulkan_frame(Engine* engine) {
    vkWaitForFences(engine->device, 1, &engine->inFlightFence, VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(engine->device, engine->swapchain, UINT64_MAX, engine->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { return; } 

    vkResetFences(engine->device, 1, &engine->inFlightFence);
    vkResetCommandBuffer(engine->commandBuffers[imageIndex], 0);

    recordCommandBuffer(engine->commandBuffers[imageIndex], imageIndex, engine);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {engine->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &engine->commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {engine->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(engine->graphicsQueue, 1, &submitInfo, engine->inFlightFence) != VK_SUCCESS) { LOGE("Çizim komutu gönderilemedi!"); }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {engine->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    vkQueuePresentKHR(engine->presentQueue, &presentInfo);
}

void draw_gles_frame(Engine* engine) {
    if (engine->glesDisplay != EGL_NO_DISPLAY) {
        // Mavi ekran çizimi (Vulkan ile aynı renk)
        glClearColor(0.1f, 0.3f, 0.5f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        eglSwapBuffers(engine->glesDisplay, engine->glesSurface);
    }
}

// **************************** VULKAN VE GENEL TEMİZLEME ****************************

void cleanup_vulkan(Engine* engine) {
    if (engine->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(engine->device);
        if (engine->renderFinishedSemaphore) { vkDestroySemaphore(engine->device, engine->renderFinishedSemaphore, nullptr); }
        if (engine->imageAvailableSemaphore) { vkDestroySemaphore(engine->device, engine->imageAvailableSemaphore, nullptr); }
        if (engine->inFlightFence) { vkDestroyFence(engine->device, engine->inFlightFence, nullptr); }
        for (auto framebuffer : engine->swapchainFramebuffers) { vkDestroyFramebuffer(engine->device, framebuffer, nullptr); }
        if (engine->renderPass) { vkDestroyRenderPass(engine->device, engine->renderPass, nullptr); }
        for (auto imageView : engine->swapchainImageViews) { vkDestroyImageView(engine->device, imageView, nullptr); }
        if (engine->commandPool) { vkDestroyCommandPool(engine->device, engine->commandPool, nullptr); }
        if (engine->swapchain) { vkDestroySwapchainKHR(engine->device, engine->swapchain, nullptr); }
        vkDestroyDevice(engine->device, nullptr);
    }
    if (engine->surface) { vkDestroySurfaceKHR(engine->vkInstance, engine->surface, nullptr); }
    if (engine->vkInstance) { vkDestroyInstance(engine->vkInstance, nullptr); }
    
    *engine = { .app = engine->app }; 
    LOGI("VULKAN: Temizleme Başarılı.");
}

bool init_vulkan_full(Engine* engine) {
    if (engine->app->window == NULL) { return false; }

    // KRİTİK GRALLOC HİLESİ: Pencere boyutunu zorla 1x1 yapıyoruz.
    ANativeWindow_setBuffersGeometry(engine->app->window, 1, 1, WINDOW_FORMAT_RGBX_8888);

    if (!createInstanceAndSurface(engine)) return false;
    if (!pickPhysicalDevice(engine)) return false;
    if (!createLogicalDevice(engine)) return false;
    
    if (!createSwapChain(engine)) return false; 
    
    if (!createImageViews(engine)) return false;
    if (!createRenderPass(engine)) return false;
    if (!createFramebuffers(engine)) return false;
    if (!createCommandObjects(engine)) return false;
    if (!createSyncObjects(engine)) return false;

    engine->is_vulkan = true;
    LOGI("VULKAN: Tüm Motor Temelleri Hazır. PBR/RTX Kodunuzu Entegre Etmeye Hazır.");
    return true;
}

// **************************** ANA UYGULAMA DÖNGÜSÜ ****************************

void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != NULL) {
                if (init_vulkan_full(engine)) {
                    engine->running = true;
                    LOGI("BAŞLATMA: Vulkan Motoru Başarıyla Seçildi.");
                    return;
                }

                LOGE("Vulkan başlatma başarısız oldu (muhtemelen Gralloc hatası). GLES Fallback deneniyor...");
                cleanup_vulkan(engine); 

                if (init_gles_fallback(engine)) {
                    engine->running = true;
                    LOGI("BAŞLATMA: GLES Fallback Motoru Seçildi.");
                    return;
                }
                
                LOGE("KRİTİK HATA: Ne Vulkan ne de GLES başlatılabildi.");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            engine->running = false;
            // engine bir pointer olduğu için burada -> kullanımı doğru.
            if (engine->is_vulkan) { 
                cleanup_vulkan(engine); 
            } else if (engine->glesDisplay != EGL_NO_DISPLAY) {
                // GLES Temizliği
                eglMakeCurrent(engine->glesDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                eglDestroyContext(engine->glesDisplay, engine->glesContext);
                eglDestroySurface(engine->glesDisplay, engine->glesSurface);
                eglTerminate(engine->glesDisplay);
                *engine = { .app = engine->app };
            }
            break;
        case APP_CMD_GAINED_FOCUS:
        case APP_CMD_LOST_FOCUS:
            break;
    }
}

void android_main(struct android_app* state) {
    // BURADA 'engine' bir struct'tır.
    struct Engine engine = {0}; 
    state->userData = &engine;
    engine.app = state;
    state->onAppCmd = engine_handle_cmd;

    LOGI("Hibrit Vulkan/GLES Motoru Başlatıldı.");

    int ident; int events; struct android_poll_source* source;
    while (1) {
        if (ALooper_pollAll(engine.running ? 0 : -1, &ident, &events, (void**)&source) >= 0) {
            if (source != NULL) { source->process(state, source); }
        }
        if (state->destroyRequested != 0) break;

        if (engine.running) { 
            // Çizim kısmı: engine bir struct olduğu için . kullanılır, pointer gerektiğinde &engine gönderilir.
            if (engine.is_vulkan) {
                draw_vulkan_frame(&engine);
            } else if (engine.glesDisplay != EGL_NO_DISPLAY) { // Hata düzeltildi: engine-> yerine engine.
                draw_gles_frame(&engine);
            }
        }
    }
}
