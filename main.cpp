#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // EXIT_FAILURE | EXIT_SUCCESS


class VulkanDemo {

public:
	
	void run() {
		init_vulkan();
		main_loop();
		cleanup();
	}

private:

	void init_vulkan() {
	
	}

	void main_loop() {
	
	}

	void cleanup() {
	
	}

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