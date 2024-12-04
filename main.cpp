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
#include <fstream>


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

    VkPipelineLayout vulkan_pipeline_layout;
    VkRenderPass vulkan_render_pass;

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

        create_render_pass();

        create_graphics_pipeline();
    }

    void main_loop() {

        // Checks for events until the window is closed
        while (!glfwWindowShouldClose(window)) {

            glfwPollEvents(); // Check for events
        }
    }

    void cleanup() {

        std::cout << "Destroying Vulkan Pipeline Layout... \n\n";
        vkDestroyPipelineLayout(vulkan_logical_device, vulkan_pipeline_layout, nullptr);

        std::cout << "Destroying Vulkan Render pass... \n\n";
        vkDestroyRenderPass(vulkan_logical_device, vulkan_render_pass, nullptr);

        std::cout << "Destroying Vulkan Image views... \n\n";
        for (auto img_view : vulkan_swapchain_images_views) {
            vkDestroyImageView(vulkan_logical_device, img_view, nullptr);
        }

        std::cout << "Destroying Vulkan Swapchain... \n\n";
        vkDestroySwapchainKHR(vulkan_logical_device, vulkan_swapchain, nullptr);

        std::cout << "Destroying Vulkan Logical device... \n\n";
        vkDestroyDevice(vulkan_logical_device, nullptr);

        if (ENABLE_VALIDATION_LAYERS) {
            std::cout << "Destroying Vulkan Debug messenger... \n\n";
            destroy_debug_messenger(vulkan_instance, debug_messenger, nullptr);
        }

        std::cout << "Destroying Vulkan Surface (Win32)... \n\n";
        vkDestroySurfaceKHR(vulkan_instance, vulkan_surface, nullptr);

        std::cout << "Destroying Vulkan Instance... \n\n";
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

        std::cout << "Vulkan Image views created. \n\n";
    }

    void create_render_pass() {
        
        std::cout << "Creating Vulkan Render pass... \n";

        // We need to specify the framebuffer attachments that will be used while
        // rendering. We need to specify how many color and depth buffers there will be.
        // How many samples to use for each of them and how their contents should be handled 
        // throughout the rendering operations. This informations are wrapped in a render pass object.
        // In our case we will have just a single color buffer attachment represented by one of the
        // images from the swapchain.
        VkAttachmentDescription color_attachment{};
        color_attachment.format = vulkan_swapchain_image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // we are not doing anything with multisampling
        
        // This are for color and depth data.
        // What to do before rendering, in our case clear the framebuffer 
        // (sets the window to black)
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // What to do after rendering, in our case render the triangle to screen (store it in the framebuffer)
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // This are for stencil data. We are not doing anything with stencils
        // so we set it in the following way:
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // Which layout the image will have before render pass begins.
        // We do not care what the previous layout the image was.
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // Which layout the image will have after render pass finishes.
        // We want the image to be ready for presentation using the swapchain
        // after rendering.
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        // A single render pass can consist of multiple subpasses. Subpasses are subsequent
        // rendering operations that depend on the contents of framebuffers in previous
        // passes, for example a sequence of post - processing effects that are applied one
        // after another. In our case we will stick to a single subpass.
        // Every subpass references one or more of the attachments that we’ve described.
        VkAttachmentReference color_attachment_reference{};
        
        // Specifies which attachment to reference by its
        // index in the attachment descriptions array.Our array consists of a single
        // VkAttachmentDescription, so its index is 0.
        color_attachment_reference.attachment = 0;
        
        // Specifies which layout we would like the attachment to have during a subpass that uses this
        // reference. We intend to use the attachment to function as a color buffer and
        // the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout will give us the best performance,
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_reference;

        VkRenderPassCreateInfo render_pass_create_info{};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.attachmentCount = 1;
        render_pass_create_info.pAttachments = &color_attachment;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &subpass;

        if (vkCreateRenderPass(
            vulkan_logical_device,
            &render_pass_create_info,
            nullptr,
            &vulkan_render_pass) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create render pass! \n");
        }

        std::cout << "Vulkan Render pass created. \n\n";
    }

    void create_graphics_pipeline() {
        
        std::cout << "Creating the Vulkan Pipeline Layout... \n\n";

        auto vert_shader_bytecode = read_file("vert.spv");
        auto frag_shader_bytecode = read_file("frag.spv");

        std::cout << "\t Vert shader file size: " << vert_shader_bytecode.size() << " bytes. \n";
        std::cout << "\t Frag shader file size: " << frag_shader_bytecode.size() << " bytes. \n\n";

        std::cout << "\t Creating the shader modules... \n";
        VkShaderModule vert_shader_module = create_shader_module(vert_shader_bytecode);
        VkShaderModule frag_shader_module = create_shader_module(frag_shader_bytecode);
        
        if (vert_shader_module == VK_NULL_HANDLE) {
            throw std::runtime_error("Vert shader not created! \n");
        }
        else if (frag_shader_module == VK_NULL_HANDLE) {
            throw std::runtime_error("Frag shader not created! \n");
        }
        else {
            std::cout << "\t Shader modules created. \n\n";
        }


        std::cout << "\t Creating the shader stages... \n";
        // To actually use the shaders we will need to assign them to a specific
        // pipeline stage.
        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module;
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
        frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module;
        frag_shader_stage_info.pName = "main";
        
        VkPipelineShaderStageCreateInfo shader_stages[] = { 
            vert_shader_stage_info,
            frag_shader_stage_info };
        std::cout << "\t Shader stages created.  \n\n";


        // While most of the pipeline state needs to be baked into the pipeline static state, a
        // limited amount of the state can actually be dynamic, changing it without recreating the
        // pipeline at draw time. If we want to use dynamic state:
        std::vector<VkDynamicState> dynamic_state;

        VkPipelineDynamicStateCreateInfo dynamic_states_create_info{};
        dynamic_states_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_states_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state.size());
        dynamic_states_create_info.pDynamicStates = dynamic_state.data();

        // The VkPipelineVertexInputStateCreateInfo structure describes the format
        // of the vertex data that will be passed to the vertex shader.
        // Because we’re hard coding the vertex data directly in the vertex shader, we’ll
        // fill in this structure to specify that there is no vertex data to load for now.
        // The pVertexBindingDescriptions and pVertexAttributeDescriptions
        // members point to an array of structs that describe the aforementioned details
        // for loading vertex data.
        VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
        vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_create_info.vertexBindingDescriptionCount = 0;
        vertex_input_create_info.pVertexBindingDescriptions = nullptr;
        vertex_input_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

        // The VkPipelineInputAssemblyStateCreateInfo struct describes two things:
        // what kind of geometry will be drawn from the vertices and if primitive restart
        // should be enabled.
        // Normally, the vertices are loaded from the vertex buffer by index in sequential
        // order, but with an element buffer you can specify the indices to use yourself.
        // This allows you to perform optimizations like reusing vertices.
        // We intend to draw triangles throughout this tutorial, so we’ll stick to the 
        // following data for the structure :
        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
        input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

        // A viewport describes the region of the framebuffer that the output
        // will be rendered to. This will almost always be (0, 0) to (width, height)
        // and in this tutorial will also be the case.
        // Remember that the size of the swapchain and its images may differ from the WIDTH
        // and HEIGHT of the window. The swapchain images will be used as framebuffers later on,
        // so we should stick to their size.
        // The minDepth and maxDepth values specify the range of depth values to use
        // the framebuffer.These values must be within the[0.0f, 1.0f] range, but
        // minDepth may be higher than maxDepth.If you aren’t doing anything special,
        // then you should stick to the standard values of 0.0f and 1.0f.
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) vulkan_swapchain_extent.width;
        viewport.height = (float) vulkan_swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // While viewports define the transformation from the image to the framebuffer,
        // scissor rectangles define in which regions pixels will actually be stored.
        // Any pixels outside the scissor rectangles will be discarded by the rasterizer.
        // They function like a filter rather than a transformation.
        // If we want to draw to the entire framebuffer we should specify a scissor
        // rectangle that covers it entirely.
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vulkan_swapchain_extent;

        // Most of the times viewport and scissor are set as dynamic state in the command buffer
        // rather than as a static part of the pipeline.
        dynamic_state = {
           VK_DYNAMIC_STATE_VIEWPORT,
           VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state.size());
        dynamic_state_create_info.pDynamicStates = dynamic_state.data();

        // Specify their count at pipeline creation time.
        // With dynamic state, the actual viewport(s) and scissor rectangle(s) will be set up at draw time.
        // Without dynamic state, the viewport and scissor rectangle need to be set in the
        // pipeline using the VkPipelineViewportStateCreateInfo struct.This makes
        // the viewport and scissor rectangle for this pipeline immutable.
        VkPipelineViewportStateCreateInfo viewport_state_create_info{};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.viewportCount = 1;
        // viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        // viewport_state_create_info.pScissors = &scissor;

        std::cout << "\t Creating Vulkan Rasterizer... \n";
        // The rasterizer takes the geometry that is shaped by the vertices from the
        // vertex shader and turns it into fragments to be colored by the fragment shader.
        VkPipelineRasterizationStateCreateInfo rasterizer_create_info{};
        rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer_create_info.depthClampEnable = VK_FALSE;

        // Geometry passes through the rasterizer stage, basically disabling any output
        // to the framebuffer.
        rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;

        // Determines how fragments are generated from geometry.
        // Fill the area of the polygon with fragments.
        rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer_create_info.lineWidth = 1.0f; // thickness of line in terms of number of fragments
        rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;

        // Specifies the vertex order for faces to be considered front-facing
        // and can be clockwise/counterclockwise.
        rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer_create_info.depthBiasEnable = VK_FALSE;
        std::cout << "\t Vulkan Rasterizer created. \n\n";


        // Multisampling is one of the ways to perform anti-aliasing. It works by combining
        // the fragment shader results of multiple polygons that rasterize to the same pixel.
        // This mainly occurs around edges, which is also where the most noticeable aliasing
        // artifacts occur.
        // For now we will keep it disabled, and revisit it later.
        VkPipelineMultisampleStateCreateInfo multisampling_create_info{};
        multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_create_info.sampleShadingEnable = VK_FALSE;
        multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // After a fragment shader has returned a color, it needs to be combined with the color
        // already present in the framebuffer.
        // There are two types of structs to configure color blending.
        // VkPipelineColorBlendAttachmentState contains the config per attached framebuffer
        // VkPipelineColorBlendStateCreateInfo contains the global color blending settings
        // We only have one framebuffer.
        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;
        
        // The second structure references the array of structures for all of the framebuffers
        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;

        // You can use uniform values in shaders, which are globals similar to dynamic
        // state variables that can be changed at drawing time to alter the behavior of
        // your shaders without having to recreate them.
        // These uniform values need to be specified during pipeline creation by creating a
        // VkPipelineLayout object.Even though we won’t be using them until a future
        // chapter, we are still required to create an empty pipeline layout.
        VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if (vkCreatePipelineLayout(
            vulkan_logical_device,
            &pipeline_layout_create_info,
            nullptr,
            &vulkan_pipeline_layout) != VK_SUCCESS) {

            throw std::runtime_error("Failed to create Vulkan Pipeline Layout! \n");
        }
       
        std::cout << "Vulkan Pipeline Layout created. \n\n";

        // We can destroy the shader modules as soon as the pipeline
        // is finished.
        std::cout << "Destroying shader stages... \n\n";
        vkDestroyShaderModule(vulkan_logical_device, vert_shader_module, nullptr);
        vkDestroyShaderModule(vulkan_logical_device, frag_shader_module, nullptr);
    }

    // Before we can pass the code to the pipeline,
    // we have to wrap it in a VkShaderModule object.
    VkShaderModule create_shader_module(const std::vector<char>& shader_code) {

        VkShaderModuleCreateInfo shader_module_create_info{};
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.codeSize = shader_code.size();
        // The size of the shader bytecode (a char specified in bytes) is different from the bytecode pointer
        // (a uint32_t) so we need to cast it with reinterpret_cast.
        // Technically with a cast like this we should also need to ensure data satisfies the 
        // alignment requirement of uint32_t, but the std::vector default allocator already ensures it.
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

        VkShaderModule shader_module;

        if (vkCreateShaderModule(vulkan_logical_device, &shader_module_create_info, nullptr,
            &shader_module) != VK_SUCCESS) {
            
            throw std::runtime_error("Failed to create the shader module! \n");
        }

        return shader_module;
    }

    static std::vector<char> read_file(const std::string& file_name) {
        
        // std::ios::ate = start reading at the end of the file
        // std::ios::binary = read the file as a binary file (avoid text transformations)
        // We start reading at the end of the file because so that we can
        // determine the size of the file and allocate a buffer.
        std::ifstream file(file_name, std::ios::ate | std::ios::binary);

        const std::string error_file = "Failed to open file: " + file_name + " \n";
        if (!file.is_open()) {
            throw std::runtime_error(error_file);
        }

        // Now i get the size of the file
        size_t file_size = (size_t)file.tellg();
        std::vector<char> buffer(file_size);

        // Now i go back to the top of the file and start reading
        // all of the bytes at once.
        file.seekg(0);
        file.read(buffer.data(), file_size);

        file.close();

        return buffer;
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