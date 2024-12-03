#define GLFW_INCLUDE_VULKAN // implicitly #include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // EXIT_FAILURE | EXIT_SUCCESS
#include <vector>
#include <optional>
#include <set>

// #include <cstring> in some system may be required for strcmp()


/* ----------------------------------------------------------------- */
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

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
        create_logical_device();
    }

    void main_loop() {

        // Checks for events until the window is closed
        while (!glfwWindowShouldClose(window)) {

            glfwPollEvents(); // Check for events
        }
    }

    void cleanup() {
        
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
        if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support()) {
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

    void create_logical_device() {
        
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
        // Note that we have not set device specific extensions for now.
        logical_device_create_info.enabledExtensionCount = 0;

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
        QueueFamilyIndices indices = find_queue_families(device);

        return indices.is_complete();
    }

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {
        
        // Assign index to queue families that could be found
        std::cout << "Looking for Vulkan Queue Families... \n\n";

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
            std::cout << "\t Available Vulkan Queue Families: " << queue_families_count << ".\n";
            std::cout << "\t Listing all queue families: \n";
            for (const auto& queue_family : queue_families) {
                std::cout << "\t\t " << queue_family.queueFlags << " \n";
            }
            std::cout << "\n";
        #endif

        std::cout << "Vulkan Queue Families found. \n\n";

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

    bool check_validation_layer_support() {
        
        uint32_t layers_count;
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