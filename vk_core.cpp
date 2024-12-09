#include "vk_core.hpp"
#include "vk_debugger.hpp"
#include "vk_queue_family.hpp"
#include "vk_swapchain.hpp"

#include <set>


void create_vulkan_instance(VkInstance& vk_instance) {

    std::cout << "Creating Vulkan Instance... \n\n";

    /* [START] Get the extensions and validation layers - Not optional! */

    std::cout << "\t Getting validation layers... \n\n";

    // Check validation layers
    if (ENABLE_VALIDATION_LAYERS && !check_validation_layers_support()) {
        throw std::runtime_error("\t Validation layers requested but not available! \n");
    }

    // Set the application infos
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_3; // Vulkan API version
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);

    // Set instance infos
    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;

    // Get required GLFW extensions
    std::vector<const char*> glfw_extensions = get_required_extensions();
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
    instance_create_info.ppEnabledExtensionNames = glfw_extensions.data();

    // Get debugger infos
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};

    // Get validation layers and create debug messenger
    if (ENABLE_VALIDATION_LAYERS) {
        instance_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        build_debug_messenger(debug_messenger_create_info);
        instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_create_info;
    }
    else {
        instance_create_info.enabledLayerCount = 0;
        instance_create_info.pNext = nullptr;
    }

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
    if (vkCreateInstance(&instance_create_info, nullptr, &vk_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create the Vulkan Instance! \n");
    }

    std::cout << "\n" << "Vulkan Instance created. \n\n";
}


void create_vulkan_surface(VkSurfaceKHR& vk_surface, VkInstance vk_instance, GLFWwindow* window) {

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

    if (glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan Surface (Win32)! \n");
    }

    std::cout << "Vulkan Surface (Win32) created. \n\n";
}


void select_physical_device(VkPhysicalDevice& vk_phys_device, VkInstance& vk_instance, VkSurfaceKHR& vk_surface) {

    std::cout << "Selecting Vulkan Physical devices (GPUs)... \n\n";

    // Enumerating all physical devices
    uint32_t devices_count = 0;
    vkEnumeratePhysicalDevices(vk_instance, &devices_count, nullptr);

    if (devices_count == 0) {
        throw std::runtime_error("Failed to find Vulkan Physical devices (GPUs) with Vulkan support! \n");
    }

    // Getting all physical devices
    std::vector<VkPhysicalDevice> devices(devices_count);
    vkEnumeratePhysicalDevices(vk_instance, &devices_count, devices.data());

    print_all_devices(devices, devices_count);

    // Check if they are suitable for the operations we want to perform
    for (const auto& dev : devices) {
        
        if (is_device_suitable(vk_surface, dev)) {
            vk_phys_device = dev;
            break;
        }
        
    }

    if (vk_phys_device == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable Physical device (GPU)! \n");
    }

    print_device_properties(vk_phys_device);

    std::cout << "Vulkan Physical device (GPU) found. \n\n";
}


void create_vulkan_logical_device(
    VkDevice& vk_logic_device,
    VkSurfaceKHR vk_surface,
    VkPhysicalDevice vk_phys_device,
    VkQueue& vk_graphics_queue,
    VkQueue& vk_present_queue) {

    std::cout << "Creating Vulkan Logical device... \n\n";

    // Specify the queues to be created. To be more specific,
    // we will set the number of queues we want for a single queue family.
    QueueFamilyIndices indices = find_queue_families(vk_surface, vk_phys_device);

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

    // Filling the logical device infos
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

    if (vkCreateDevice(vk_phys_device, &logical_device_create_info, nullptr, &vk_logic_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan Logical Device! \n");
    }

    // Retrieve queue handles for each queue family.
    // If the queue families are the same, then we only need to pass its index once.
    vkGetDeviceQueue(vk_logic_device, indices.graphics_family.value(), 0, &vk_graphics_queue);
    vkGetDeviceQueue(vk_logic_device, indices.present_family.value(), 0, &vk_present_queue);

    std::cout << "Vulkan Logical device created. \n\n";
}


bool is_device_suitable(VkSurfaceKHR vk_surface, VkPhysicalDevice phys_device) {

    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(phys_device, &device_properties);
    vkGetPhysicalDeviceFeatures(phys_device, &device_features);

    // Only dedicated graphic cards  that support geometry shaders are defined as suitable
    return device_properties.deviceType ==
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        && device_features.geometryShader;


    // Select all suitable devices as devices that support VK_QUEUE_GRAPHICS_BIT
    // and support Presentation (Present Queue Family)
    QueueFamilyIndices indices = find_queue_families(vk_surface, phys_device);

    // Get the extensions supported by the device (for now only Swapchain is required)
    bool extensions_supported = check_extensions_support(phys_device);

    // Get the swapchain details
    bool swapchain_adequate = false;
    if (extensions_supported) {

        // We query for swapchain support only after veryifing that
        // the extension VK_KHR_swapchain is available for our device.
        SwapchainSupportDetails swapchain_support = query_swapchain_support(vk_surface, phys_device);
        swapchain_adequate =
            !swapchain_support.formats.empty() &&
            !swapchain_support.present_modes.empty();
    }

    return indices.is_complete() && extensions_supported && swapchain_adequate;
}


void print_all_devices(
    const std::vector<VkPhysicalDevice> physical_devices,
    uint32_t devices_count) {

    VkPhysicalDeviceProperties device_properties;
    std::cout << "\t Available physical devices (GPUs): " << devices_count << ".\n";
    std::cout << "\t Listing all physical devices: \n";
    for (const auto& device : physical_devices) {
        vkGetPhysicalDeviceProperties(device, &device_properties);
        std::cout << "\t\t " << device_properties.deviceName << " \n";
    }
    std::cout << "\n";

}


void print_device_properties(VkPhysicalDevice phys_device) {

    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceMemoryProperties device_memory;

    vkGetPhysicalDeviceProperties(phys_device, &device_properties);
    vkGetPhysicalDeviceMemoryProperties(phys_device, &device_memory);

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

}

// sposta in questo file create_logical_device()
// 
// attento a mettere & o * nei parametri di funzione!
// cerca online bene i puntatori in c++
// 
// 
