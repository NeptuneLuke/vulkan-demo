#define GLFW_INCLUDE_VULKAN // implicitly #include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // EXIT_FAILURE | EXIT_SUCCESS
#include <cstring> // strcmp
#include <cstdint> // uint32_t
#include <optional>
#include <algorithm> // std::clamp
#include <limits> // std::numeric_limits
#include <vector>
#include <set>


/* ----------------------------------------------------------------- */
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

// to get all required extensions (for now the VK_KHR_swapchain)
const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef _DEBUG
const bool ENABLE_VALIDATION_LAYERS = true;
#else
const bool ENABLE_VALIDATION_LAYERS = false;
#endif

struct QueueFamilyIndices {

    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

// Just checking if a swapchain is available is not sufficient, it may not be
// compatible with our window surface. Also we need to query for some details about
// the swapchain.
struct SwapchainSupportDetails {

    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};


VkResult create_debug_messenger(
    VkInstance vk_instance,
    const VkDebugUtilsMessengerCreateInfoEXT* ptr_debug_messenger_create_info,
    const VkAllocationCallbacks* ptr_allocator,
    VkDebugUtilsMessengerEXT* ptr_vk_debug_messenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(vk_instance, ptr_debug_messenger_create_info, ptr_allocator, ptr_vk_debug_messenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_messenger(
    VkInstance vk_instance,
    VkDebugUtilsMessengerEXT vk_debug_messenger,
    const VkAllocationCallbacks* ptr_allocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(vk_instance, vk_debug_messenger, ptr_allocator);
    }
}
/* ----------------------------------------------------------------- */


class VulkanDemo {

public:

    void run() {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

private:

    /* ----------------------------------------------------------------- */
    VkInstance vulkan_instance;
    VkSurfaceKHR vulkan_surface;

    VkPhysicalDevice vulkan_physical_device = VK_NULL_HANDLE; // Implicitly destroyed when vulkan_instance is destroyed
    VkDevice vulkan_logical_device;

    // Implicitly destroyed when vulkan_logical_device is destroyed.
    VkQueue vulkan_graphics_queue;
    VkQueue vulkan_present_queue;

    VkSwapchainKHR vulkan_swapchain;

    // Implicitly destroyed when vulkan_swapchain is destroyed
    std::vector<VkImage> vulkan_swapchain_images;
    std::vector<VkImageView> vulkan_swapchain_images_views;
    VkFormat vulkan_swapchain_image_format;
    VkExtent2D vulkan_swapchain_extent;

    VkDebugUtilsMessengerEXT debug_messenger;

    GLFWwindow* window;
    /* ----------------------------------------------------------------- */


    /* ----------------------------------------------------------------- */
    void init_window() {

        glfwInit(); // Initializes the GLFW lib

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Specify to use VULKAN (by explicitly not using OpenGL)

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable resizing window (temporary)

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan demo", nullptr, nullptr);
    }

    void init_vulkan() {

        create_vulkan_instance();

        setup_debug_messenger();
        
        create_vulkan_surface();
        
        select_physical_device();
        create_vulkan_logical_device();
        
        create_vulkan_swapchain();
        create_swapchain_image_views();
    }

    void main_loop() {

        // Checks for events until the window is closed
        while (!glfwWindowShouldClose(window)) {

            glfwPollEvents(); // Check for events
        }
    }

    void cleanup() {

        for (auto img_view : vulkan_swapchain_images_views) {
            vkDestroyImageView(vulkan_logical_device, img_view, nullptr);
        }

        vkDestroySwapchainKHR(vulkan_logical_device, vulkan_swapchain, nullptr);

        vkDestroyDevice(vulkan_logical_device, nullptr);

        if (ENABLE_VALIDATION_LAYERS) {
            destroy_debug_messenger(vulkan_instance, debug_messenger, nullptr);
        }

        vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, nullptr);

        vkDestroyInstance(vulkan_instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate(); // Shutdowns the GLFW lib
    }
    /* ----------------------------------------------------------------- */


    /* ----------------------------------------------------------------- */
    void create_vulkan_instance() {

        std::cout << "Creating Vulkan Instance... \n\n";

        /* [START] Get the extensions and validation layers - Not optional! */

        std::cout << "\t Getting validation layers... \n\n";

        // Check validation layers
        if (ENABLE_VALIDATION_LAYERS && !check_validation_layers_support()) {
            throw std::runtime_error("\t Validation layers requested but not available! \n");
        }

        // Get the application info
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.apiVersion = VK_API_VERSION_1_3; // Vulkan API version
        app_info.pApplicationName = "Hello Triangle";
        app_info.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);

        // Get instance
        VkInstanceCreateInfo instance_create_info{};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &app_info;

        // Get required GLFW extensions
        std::vector<const char*> glfw_extensions = get_required_extensions();
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
        instance_create_info.ppEnabledExtensionNames = glfw_extensions.data();

        // Get debugger
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};

        // Get validation layers
        if (ENABLE_VALIDATION_LAYERS) {
            instance_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

            populate_debug_messenger_create_info(debug_messenger_create_info);
            instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_create_info;
        }
        else {
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.pNext = nullptr;
        }

        // Get extensions
        std::cout << "\t Getting extensions... \n\n";

        // Get all available extensions for our system
        uint32_t available_extensions_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(available_extensions_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, available_extensions.data());

        #ifdef _DEBUG
            std::cout << "\t Available extensions: " << available_extensions_count << ".\n";
            std::cout << "\t Listing all extensions: \n";
            for (const auto& ext : available_extensions) {
                std::cout << "\t\t " << ext.extensionName << " | v." << ext.specVersion << " \n";
            }
            std::cout << "\n";
        #endif

        #ifdef _DEBUG
            std::cout << "\t Extensions obtained by GLFW: " << glfw_extensions.size() << ".\n";
            std::cout << "\t Listing all extensions: \n";
            for (const auto& ext : glfw_extensions) {
                std::cout << "\t\t " << ext << " \n";
            }
            std::cout << "\n";
        #endif
        /* [END] Get the extensions and validation layers - Not optional! */

        // Check if the instance is properly created
        if (vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create the Vulkan Instance! \n");
        }

        std::cout << "\n" << "Vulkan Instance created. \n\n";
    }

    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& debug_messenger_create_info) {

        debug_messenger_create_info = {};
        debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_messenger_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_messenger_create_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_messenger_create_info.pfnUserCallback = debug_CALLBACK_FUNC;
    }

    void setup_debug_messenger() {

        std::cout << "Creating Vulkan Debugger... \n";

        if (!ENABLE_VALIDATION_LAYERS) return;

        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
        populate_debug_messenger_create_info(debug_messenger_create_info);

        if (create_debug_messenger(vulkan_instance, &debug_messenger_create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up the Debug messenger! \n");
        }

        std::cout << "Vulkan Debugger created. \n\n";
    }

    void create_vulkan_surface() {

        std::cout << "Creating Vulkan Surface (Win32)... \n";

        // Another way to do it (native Vulkan platform)
        // GLFW is used only to get the handle of Win32 window
        /*
        #define VK_USE_PLATFORM_WIN32_KHR
        #define GLFW_INCLUDE_VULKAN
        #include <GLFW/glfw3.h>
        #define GLFW_EXPOSE_NATIVE_WIN32
        #include <GLFW/glfw3native.h>

        VkWin32SurfaceCreateInfoKHR win32_surface_create_info{};
        win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        win32_surface_create_info.hwnd = glfwGetWin32Window(window);
        win32_surface_create_info.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(
            vulkan_instance,
            &win32_surface_create_info,
            nullptr,
            &vulkan_surface) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create Vulkan Surface (Win32)! \n");
        }
        */

        if (glfwCreateWindowSurface(vulkan_instance, window, nullptr, &vulkan_surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan Surface (Win32)! \n");
        }

        std::cout << "Vulkan Surface (Win32) created. \n\n";
    }

    void select_physical_device() {

        // Listing physical devices
        std::cout << "Selecting Vulkan Physical devices (GPUs)... \n\n";

        // Enumerating all physical devices
        uint32_t devices_count = 0;
        vkEnumeratePhysicalDevices(vulkan_instance, &devices_count, nullptr);

        if (devices_count == 0) {
            throw std::runtime_error("Failed to find Vulkan Physical devices (GPUs) with Vulkan support!");
        }

        // Getting all physical devices
        std::vector<VkPhysicalDevice> devices(devices_count);
        vkEnumeratePhysicalDevices(vulkan_instance, &devices_count, devices.data());

        // Getting all physical devices info
        VkPhysicalDeviceProperties device_properties;
        VkPhysicalDeviceMemoryProperties device_memory;

        #ifdef _DEBUG
            std::cout << "\t Available physical devices (GPUs): " << devices_count << ".\n";
            std::cout << "\t Listing all physical devices: \n";
            for (const auto& device : devices) {
                vkGetPhysicalDeviceProperties(device, &device_properties);
                std::cout << "\t\t " << device_properties.deviceName << " \n";
            }
            std::cout << "\n";
        #endif

        // Check if they are suitable for the operations we want to perform
        for (const auto& vulkan_logical_device : devices) {
            if (is_device_suitable(vulkan_logical_device)) {
                vulkan_physical_device = vulkan_logical_device;
                break;
            }
        }

        if (vulkan_physical_device == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable Physical device (GPU)!");
        }

        vkGetPhysicalDeviceMemoryProperties(vulkan_physical_device, &device_memory);
        #ifdef _DEBUG
            std::cout << "\t Selected Physical device: \n";
            std::cout << "\t\t Name: " << device_properties.deviceName << ". \n";
            std::cout << "\t\t ID: " << device_properties.deviceID << ". \n";
            std::cout << "\t\t Type: ";
            switch (device_properties.deviceType) {

            default:

            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                std::cout << "Unknown." << " \n\n";
                break;

            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                std::cout << "Integrated." << " \n\n";
                break;

            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                std::cout << "Dedicated." << " \n\n";
                break;

            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                std::cout << "Virtual." << " \n\n";
                break;

            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                std::cout << "CPU." << " \n\n";
                break;
            }

            std::cout << "\t\t Driver version: "
                << VK_API_VERSION_MAJOR(device_properties.driverVersion) << "."
                << VK_API_VERSION_MINOR(device_properties.driverVersion) << "."
                << VK_API_VERSION_PATCH(device_properties.driverVersion) << ". \n";
            std::cout << "\t\t Vulkan API version: "
                << VK_API_VERSION_MAJOR(device_properties.apiVersion) << "."
                << VK_API_VERSION_MINOR(device_properties.apiVersion) << "."
                << VK_API_VERSION_PATCH(device_properties.apiVersion) << ". \n\n";

            for (uint32_t j = 0; j < device_memory.memoryHeapCount; j++) {

                float_t memory_size_gib = (((float_t)device_memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (device_memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    std::cout << "\t\t Local GPU memory: " << memory_size_gib << " GiB." << " \n";
                }
                else {
                    std::cout << "\t\t Shared System memory: " << memory_size_gib << " GiB." << " \n";
                }
            }
            std::cout << "\n";
        #endif

        std::cout << "Vulkan Physical device (GPU) found. \n\n";
    }

    void create_vulkan_logical_device() {

        std::cout << "Creating Vulkan Logical device... \n\n";

        // Specify the queues to be created. To be more specific,
        // we will set the number of queues we want for a single queue family.
        QueueFamilyIndices indices = find_queue_families(vulkan_physical_device);

        // Now i can create a set of all unique queue families that are necessary
        // for the required queues.
        std::vector<VkDeviceQueueCreateInfo> queue_families_create_info;
        std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

        // We don't really need more than one per family, because you can create all
        // of the command buffers on multiple threads and then submit them all at once
        // on the main thread with a single call.
        // We can also assign properties to queues to directly managing the scheduling of
        // command buffer execution. It is required even with one single queue.
        float queue_priority = 1.0f;
        for (uint32_t queue_family_index : unique_queue_families) {

            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queue_family_index;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queue_priority;

            queue_families_create_info.push_back(queueCreateInfo);
        }

        // Specify the device features needed, that we actually already queried up for
        // with vkGetPhysicalDeviceFeatures
        VkPhysicalDeviceFeatures device_features{}; // Right now we don't need anything special

        // Filling the main structure
        VkDeviceCreateInfo logical_device_create_info{};
        logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_families_create_info.size());
        logical_device_create_info.pQueueCreateInfos = queue_families_create_info.data();
        logical_device_create_info.pEnabledFeatures = &device_features;

        // We enable validation layers specific to the device for retrocompatibility purposes.
        // Note that now we have explicitly set the VK_KHR_swapchain extension in the function
        // check_extensions_support().
        logical_device_create_info.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
        logical_device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
        if (ENABLE_VALIDATION_LAYERS) {
            logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            logical_device_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }
        else {
            logical_device_create_info.enabledLayerCount = 0;
        }

        if (vkCreateDevice(vulkan_physical_device, &logical_device_create_info, nullptr, &vulkan_logical_device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan Logical Device! \n");
        }

        // Retrieve queue handles for each queue family.
        // If the queue families are the same, then we only need to pass its index once.
        vkGetDeviceQueue(vulkan_logical_device, indices.graphics_family.value(), 0, &vulkan_graphics_queue);
        vkGetDeviceQueue(vulkan_logical_device, indices.present_family.value(), 0, &vulkan_present_queue);

        std::cout << "Vulkan Logical device created. \n\n";
    }

    bool is_device_suitable(VkPhysicalDevice device) {

        /*
        VkPhysicalDeviceProperties device_properties;
        VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceProperties(device, &device_properties);
        vkGetPhysicalDeviceFeatures(device, &device_features);

        // Only dedicated graphic cards  that support geometry shaders are defined as suitable
        return device_properties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            && device_features.geometryShader;
        */

        // Select all suitable devices as devices that support VK_QUEUE_GRAPHICS_BIT
        // and support Presentation (Present Queue Family)
        QueueFamilyIndices indices = find_queue_families(device);

        // Get the extensions supported by the device (for now only Swapchain is required)
        bool extensions_supported = check_extensions_support(device);

        // Get the swapchain details
        bool swapchain_adequate = false;
        if (extensions_supported) {

            // We query for swapchain support only after veryifing that
            // the extension VK_KHR_swapchain is available for our device.
            SwapchainSupportDetails swapchain_support = query_swapchain_support(device);
            swapchain_adequate =
                !swapchain_support.formats.empty() &&
                !swapchain_support.present_modes.empty();
        }

        return indices.is_complete() && extensions_supported && swapchain_adequate;
    }

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {

        // Assign index to queue families that could be found
        std::cout << "\t Looking for Vulkan Queue Families... \n\n";

        QueueFamilyIndices family_indices;

        uint32_t queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families.data());

        // We need to find at least one queue family that supports
        // both VK_QUEUE_GRAPHICS_BIT and present family.
        // Not every device in the system necesseraly supports window system integration.
        // So we need to find a queue family that supports presenting to the surface created.
        // It is possible that queue families supporting drawing commands and the ones supporting
        // presentation do not overlap.
        int index = 0;
        for (const auto& queue_family : queue_families) {

            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                family_indices.graphics_family = index;
            }

            VkBool32 present_queue_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, index, vulkan_surface, &present_queue_support);

            if (present_queue_support) {
                family_indices.present_family = index;
            }

            if (family_indices.is_complete()) {
                break;
            }

            index++;
        }

        #ifdef _DEBUG
            std::cout << "\t\t Available Vulkan Queue Families: " << queue_families_count << ".\n";
            std::cout << "\t\t Listing all queue families: \n";
            for (const auto& queue_family : queue_families) {
                std::cout << "\t\t\t " << queue_family.queueFlags << " \n";
            }
            std::cout << "\n";
        #endif

        std::cout << "\t Vulkan Queue Families found. \n\n";

        return family_indices;
    }

    std::vector<const char*> get_required_extensions() {

        uint32_t glfw_extensions_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extensions_count);

        if (ENABLE_VALIDATION_LAYERS) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Debug messenger extension
        }

        return extensions;
    }

    bool check_validation_layers_support() {

        // Just checking if the validation layers are available

        uint32_t layers_count = 0;
        vkEnumerateInstanceLayerProperties(&layers_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layers_count);
        vkEnumerateInstanceLayerProperties(&layers_count, available_layers.data());

        for (const char* layer_name : VALIDATION_LAYERS) {

            bool layer_found = false;

            for (const auto& layer_properties : available_layers) {

                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                return false; // Vulkan specific error: VK_ERROR_LAYER_NOT_PRESENT
            }
        }

        return true;
    }

    bool check_extensions_support(VkPhysicalDevice device) {

        // Just checking if the extensions are available for our device

        // Technically, the availability of a presentation queue
        // implies that the swapchain extension (VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        // is supported. However it's still good to be explicit.

        uint32_t extensions_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extensions_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, available_extensions.data());

        // Unconfirmed required extensions
        std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

        // We can also use a nested loop like in check_validation_layers_support.
        for (const auto& ext : available_extensions) {
            required_extensions.erase(ext.extensionName);
        }

        return required_extensions.empty();
    }

    void create_vulkan_swapchain() {

        std::cout << "Creating Vulkan Swapchain... \n\n";

        SwapchainSupportDetails swapchain_support = query_swapchain_support(vulkan_physical_device);

        // Setting up swapchain properties
        VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats);
        VkPresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.present_modes);
        VkExtent2D swap_extent = choose_swapchain_extent(swapchain_support.capabilities);

        // Decide how many minimum images we want to have in the swapchain.
        // Sticking to the minimum means that sometimes it may occur that we have to wait
        // on the driver to complete internal operations before we can acquire
        // another image to render to. It is therefore recommended to request
        // at least one more image than the minimum.
        uint32_t images_in_swapchain_count = swapchain_support.capabilities.minImageCount + 1;

        // Make sure to not exceed the maximum number of images 
        if (swapchain_support.capabilities.maxImageCount > 0 &&
            images_in_swapchain_count > swapchain_support.capabilities.maxImageCount) {

            images_in_swapchain_count = swapchain_support.capabilities.maxImageCount;
        }

        // Fill the Swapchain struct
        VkSwapchainCreateInfoKHR swapchain_create_info{};
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.surface = vulkan_surface;
        swapchain_create_info.minImageCount = images_in_swapchain_count;
        swapchain_create_info.imageFormat = surface_format.format;
        swapchain_create_info.imageColorSpace = surface_format.colorSpace;
        swapchain_create_info.imageExtent = swap_extent;

        // The amount of layers each image consists of (always 1 unless developing stereoscopic 3D app)
        swapchain_create_info.imageArrayLayers = 1;

        // What kind of operation we will use the images in the swapchain for.
        // We are going to render them directly, which means they are used as color attachment.
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Select which type of sharing mode the images in the swapchain will use
        // with the queue families.
        QueueFamilyIndices family_indices = find_queue_families(vulkan_physical_device);
        uint32_t queue_family_indices[] = {
            family_indices.graphics_family.value(),
            family_indices.present_family.value()
        };

        // If the graphics and present queue are not the same queue family
        if (family_indices.graphics_family != family_indices.present_family) {
            swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchain_create_info.queueFamilyIndexCount = 2; // family and present
            swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
        }
        else {
            // If the graphics and presentation queue are the same queue family
            // which in most hardware is the case, the we should stick to exclusive mode
            swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchain_create_info.queueFamilyIndexCount = 0;
            swapchain_create_info.pQueueFamilyIndices = nullptr;
        }

        // We can specify that a certain transform should be applied to images in the swapchain
        // (if supported -> supportedTransform in capabilities), like a 90 degree clockwise rotation.
        // If you do not want any transformation, simply specify the current one.
        swapchain_create_info.preTransform = swapchain_support.capabilities.currentTransform;

        // Specifies the if the alpha channel should be used for blending with other windows
        // in the window system. Almost always you will want to ignore it and set it
        // as the following.
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        swapchain_create_info.presentMode = present_mode;

        // We don't care about color of pixels that are obscured
        // (for example because another window is in front of them).
        // Which results in better performances.
        swapchain_create_info.clipped = VK_TRUE;

        // Swapchain can become invalid or unoptimized ar runtime, 
        // for example when the window is resized. In that case the swapchain
        // must be recreated from scratch and a copy of the old one must be specified.
        // For now we will assume to create only one swapchain.
        swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(
            vulkan_logical_device,
            &swapchain_create_info,
            nullptr,
            &vulkan_swapchain) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create Vulkan Swapchain! \n");
        }

        // First query the final number of images, then resize and then call it again to
        // retrieve the handles. This is done because we only specified the minimum
        // number of images in the swapchain, so the implementation is allowed to
        // create a swapchain with more images.
        vkGetSwapchainImagesKHR(vulkan_logical_device, vulkan_swapchain, &images_in_swapchain_count, nullptr);
        vulkan_swapchain_images.resize(images_in_swapchain_count);
        vkGetSwapchainImagesKHR(vulkan_logical_device, vulkan_swapchain, &images_in_swapchain_count, vulkan_swapchain_images.data());

        // Written just because will need it in the future.
        vulkan_swapchain_image_format = surface_format.format;
        vulkan_swapchain_extent = swap_extent;

        std::cout << "Vulkan Swapchain created. \n\n";
    }

    SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device) {

        SwapchainSupportDetails details;

        // Get Surface details (capabilities)
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkan_surface, &details.capabilities);

        // Get formats details
        uint32_t formats_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan_surface, &formats_count, nullptr);

        if (formats_count != 0) {
            details.formats.resize(formats_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan_surface, &formats_count, details.formats.data());
        }

        // Get present modes details
        uint32_t present_modes_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan_surface, &present_modes_count, nullptr);

        if (present_modes_count != 0) {
            details.present_modes.resize(present_modes_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan_surface, &present_modes_count, details.present_modes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {

        // We are choosing the best possible swapchain surface format.
        // The typical combination is:
        // format = RGBA 8 bit (32 bits per pixel).
        // color space = SRGB (more accurate perceived colors)
        // SRGB is pretty much the color space standard for images and textures.

        for (const auto& available_format : available_formats) {

            // This is the best possible (if available) format.
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {

                return available_format;
            }
        }

        // If we don't get the best one we wanted, we could rank the available
        // formats and choose the (second) best, but in most cases it's okay
        // to just settle with the first specified format.
        return available_formats[0];
    }

    VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {

        // Since we are on a computer and not a mobile device, 
        // the best present mode is probably VK_PRESENT_MODE_MAILBOX_KHR.
        // Even though it may be possible that it's not available.
        // The only present mode guaranteed to be available is
        // VK_PRESENT_MODE_FIFO_KHR, so we need to query even for
        // present modes.

        for (const auto& available_present_mode : available_present_modes) {

            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {

                return available_present_mode;
            }
        }

        // As we said, if VK_PRESENT_MODE_MAILBOX_KHR is not present
        // just return the guaranteed one.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilities) {

        // The swap extent is the resolution of the swapchain images
        // and it's almost always equal to the resolution of the window
        // that we are drawing to (in pixels).
        // The range of the possible resolutions is defined
        // in the VkSurfaceCapabilitiesKHR struct.

        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {

            // This is because GLFW uses two units: pixels and screen coordinates.
            // But Vulkan works only with pixels, so the swapchain extent must be
            // specified in pixels.
            // We use glfwGetFramebufferSize() to query the resolution of the window
            // in pixel before matching it against the min/max image extent.

            int w, h;
            glfwGetFramebufferSize(window, &w, &h);

            VkExtent2D actual_extent = {
                static_cast<uint32_t>(w),
                static_cast<uint32_t>(h)
            };

            // Clamp to bound the values of width and height
            // between the allowed min/max extents that are supported
            // by the implementation.
            actual_extent.width = std::clamp(
                actual_extent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);

            actual_extent.height = std::clamp(
                actual_extent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actual_extent;
        }
    }

    void create_swapchain_image_views() {
        
        std::cout << "Creating Vulkan Image views for Vulkan Swapchain images... \n";

        // Resize to fit all the image views we will create
        vulkan_swapchain_images_views.resize(vulkan_swapchain_images.size());

        // Create an image view for every image
        for (size_t i = 0; i < vulkan_swapchain_images.size(); i++) {
            
            VkImageViewCreateInfo image_view_create_info{};
            image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.image = vulkan_swapchain_images[i];

            // How the image should be interpreted
            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_create_info.format = vulkan_swapchain_image_format;

            // You can map color channels to swizzle them around.
            // You can also use values between 0,1.
            // We will stick to default mapping.
            image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
            // Set what the image's purpose is and which part of the image
            // should be accessed. As we said, our images will be used as color targest
            // without any mimpapping levels or multiple layers.
            image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_create_info.subresourceRange.baseMipLevel = 0;
            image_view_create_info.subresourceRange.levelCount = 1;
            image_view_create_info.subresourceRange.baseArrayLayer = 0;
            image_view_create_info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(
                vulkan_logical_device,
                &image_view_create_info,
                nullptr,
                &vulkan_swapchain_images_views[i]) != VK_SUCCESS) {
            
                throw std::runtime_error("Failed to create Vulkan Image views for the Vulkan Swapchain images! \n");
            }
        }

        std::cout << "Vulkan Image views created. \n";
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_CALLBACK_FUNC(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* ptr_callback_data,
        void* ptr_user_data) {

        std::cerr << "\t Validation layer: " << ptr_callback_data->pMessage << std::endl;

        return VK_FALSE;
    }
};


int main() {

    VulkanDemo demo;

    try {
        demo.run();
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}