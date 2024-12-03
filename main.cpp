// #include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // EXIT_FAILURE | EXIT_SUCCESS
#include <vector>


/* ----------------------------------------------------------------- */
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

#ifdef _DEBUG
	const bool ENABLE_VALIDATION_LAYERS = true;
#else
	const bool ENABLE_VALIDATION_LAYERS = false;
#endif


VkResult create_debug_messenger(
	VkInstance vk_instance,
	const VkDebugUtilsMessengerCreateInfoEXT* ptr_create_info,
	const VkAllocationCallbacks* ptr_allocator,
	VkDebugUtilsMessengerEXT* ptr_vk_debug_messenger) {
	
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
	
	if (func != nullptr) {
		return func(vk_instance, ptr_create_info, ptr_allocator, ptr_vk_debug_messenger);
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
	VkPhysicalDevice vulkan_physical_device = VK_NULL_HANDLE; // Implicitly destroyed when vulkan_instance is destroyed

	VkDebugUtilsMessengerEXT vulkan_debug_messenger;

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
		select_physical_device();
	}

	void main_loop() {
		
		// Checks for events until the window is closed
		while (!glfwWindowShouldClose(window)) {
			
			glfwPollEvents();  // Check for events
		}
	}

	void cleanup() {
		
		if (ENABLE_VALIDATION_LAYERS) {
			destroy_debug_messenger(vulkan_instance, vulkan_debug_messenger, nullptr);
		}

		vkDestroyInstance(vulkan_instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate(); // Shutdowns the GLFW lib 
	}
	/* ----------------------------------------------------------------- */


	/* ----------------------------------------------------------------- */
	void create_vulkan_instance() {

		std::cout << "Creating Vulkan Instance... \n\n";

		// Get the application info
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.apiVersion = VK_API_VERSION_1_3; // Vulkan API version
		app_info.pApplicationName = "Hello Triangle";
		app_info.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);


		/* [START] Get the extensions and validation layers - Not optional! */

		std::cout << "Getting validation layers... \n\n";
		
		// Validation layers
		if (ENABLE_VALIDATION_LAYERS && !check_validation_layers_support()) {
			throw std::runtime_error("Validation layers requested but not available! \n");
		}
		
		VkInstanceCreateInfo instance_create_info{};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo = &app_info;
		
		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
		if (ENABLE_VALIDATION_LAYERS) {
			instance_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			populateDebugMessengerCreateInfo(debug_messenger_create_info);
			instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_create_info;
		}
		else {
			instance_create_info.enabledLayerCount = 0;
			instance_create_info.pNext = nullptr;
		}

		#ifdef _DEBUG
			std::cout << "Available validation layers: " << VALIDATION_LAYERS.size() << ".\n";
			std::cout << "Listing all validation layers: \n";
			for (const auto& ext : VALIDATION_LAYERS) {
				std::cout << ext << " \n";
			}
			std::cout << "\n";
		#endif


		std::cout << "Getting extensions... \n\n";
		
		// {VULKAN} Get all available extensions for our system
		uint32_t available_extensions_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(available_extensions_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, available_extensions.data());
		
		#ifdef _DEBUG
			std::cout << "Available extensions: " << available_extensions_count << ".\n";
			std::cout << "Listing all extensions: \n";
			for (const auto& ext : available_extensions) {
				std::cout << ext.extensionName << " | v." << ext.specVersion << " \n";
			}
			std::cout << "\n";
		#endif

		// Get the extensions required. Note that GLFW has a built in
		// function to get the required extensions needed.
		// {GLFW} Extensions required by GLFW for the Vulkan Instance
		std::vector<const char*> glfw_extensions = get_required_extensions();
		instance_create_info.enabledExtensionCount = static_cast<uint32_t> (glfw_extensions.size());
		instance_create_info.ppEnabledExtensionNames = glfw_extensions.data();

		#ifdef _DEBUG
			std::cout << "Extensions obtained by GLFW: " << glfw_extensions.size() << ".\n";
			std::cout << "Listing all extensions: \n";
			for (const auto& ext : glfw_extensions) {
				std::cout << ext << " \n";
			}
			std::cout << "\n";
		#endif
		/* [END] Get the extensions and validation layers - Not optional! */

		// We have specified everything Vulkan needs to create an instance
		VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance);
		
		// Check if the instance is properly created
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create the Vulkan Instance! \n");
		}

		std::cout << "Vulkan Instance created. \n";
	}

	bool check_validation_layers_support() {
		
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

	std::vector<const char*> get_required_extensions() {
		
		uint32_t glfw_extensions_count = 0;
		const char** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extensions_count);

		if (ENABLE_VALIDATION_LAYERS) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Debug messenger extension
		}

		return extensions;
	}

	void select_physical_device() {
		
		// Listing physical devices
		uint32_t devices_count = 0;
		vkEnumeratePhysicalDevices(vulkan_instance, &devices_count, nullptr);

		if (devices_count == 0) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support! \n");
		}

		std::vector<VkPhysicalDevice> devices(devices_count);
		vkEnumeratePhysicalDevices(vulkan_instance, &devices_count, devices.data());

		// Check if they are suitable for the operations we want to perform
		for (const auto& device : devices) {
			if (is_device_suitable(device)) {
				vulkan_physical_device = device;
			}
		}

		if(vulkan_physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU! \n");
		}
	}

	bool is_device_suitable(VkPhysicalDevice device) {
		
		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		vkGetPhysicalDeviceFeatures(device, &device_features);
		
		// Only dedicated graphic cards  that support geometry shaders are defined as suitable
		return device_properties.deviceType == 
			VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU 
			&& device_features.geometryShader;
	}
	/* ----------------------------------------------------------------- */


	/* ----------------------------------------------------------------- */
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
		
		create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity = 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = 
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = debug_log_CALLBACK_FUNC;
	}

	void setup_debug_messenger() {

		if (!ENABLE_VALIDATION_LAYERS) return;

		VkDebugUtilsMessengerCreateInfoEXT create_info;
		populateDebugMessengerCreateInfo(create_info);

		if (create_debug_messenger(vulkan_instance, &create_info, nullptr, &vulkan_debug_messenger) != VK_SUCCESS) {
			throw std::runtime_error("Failed to set up the Debug Messenger! \n");
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_log_CALLBACK_FUNC(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* ptr_callback_data,
		void* ptr_user_data) {
		
		std::cerr << "Validation layer: " << ptr_callback_data->pMessage << std::endl;
		
		return VK_FALSE;
	}
	/* ----------------------------------------------------------------- */
};



int main() {
	
	VulkanDemo demo;

	try {
		demo.run();
	}
	catch (const std::exception &ex) {
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}