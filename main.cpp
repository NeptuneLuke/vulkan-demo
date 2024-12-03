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
	}

	void main_loop() {
		
		// Checks for events until the window is closed
		while (!glfwWindowShouldClose(window)) {
			
			glfwPollEvents();  // Check for events
		}
	}

	void cleanup() {
		
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
		VkInstanceCreateInfo instance_create_info{};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo = &app_info;

		std::cout << "Getting validation layers... \n\n";
		// Validation layers
		if (ENABLE_VALIDATION_LAYERS && !check_validation_layers_support()) {
			throw std::runtime_error("Validation layers requested but not available. \n");
		}
		
		if (ENABLE_VALIDATION_LAYERS) {
			instance_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
			#ifdef _DEBUG
				std::cout << "Available validation layers: " << VALIDATION_LAYERS.size() << ".\n";
				std::cout << "Listing all validation layers: \n";
				for (const auto& ext : VALIDATION_LAYERS) {
					std::cout << ext << " \n";
				}
				std::cout << "\n\n";
			#endif
		}
		else {
			instance_create_info.enabledLayerCount = 0;
		}

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
		uint32_t glfw_extensions_count = 0;
		const char** glfw_extensions_1;
		glfw_extensions_1 = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
		instance_create_info.enabledExtensionCount = glfw_extensions_count;
		instance_create_info.ppEnabledExtensionNames = glfw_extensions_1;

		#ifdef _DEBUG
			std::cout << "Extensions obtained by GLFW: " << glfw_extensions_count << ".\n";
			std::cout << "Listing all extensions: \n";
			for (uint32_t i = 0; i < glfw_extensions_count; i++) {
				std::cout << glfw_extensions_1[i] << " \n";
			}
			std::cout << "\n";
		#endif
		/* [END] Get the extensions and validation layers - Not optional! */

		// We have specified everything Vulkan needs to create an instance
		VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance);
		
		// Check if the instance is properly created
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create the Vulkan Instance!");
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