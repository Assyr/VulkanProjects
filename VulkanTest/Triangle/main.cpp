#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <iostream>
#include <stdexcept> //error handling
#include <functional> //for lambda functions
//Window size
const int WIDTH = 800;
const int HEIGHT = 600;

template <typename T>
class VDeleter {
public:
	VDeleter() : VDeleter([](T _) {}) {}

	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [=](T obj) { deletef(obj, nullptr); };
	}

	VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
	}

	VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
	}

	~VDeleter() {
		cleanup();
	}

	T* operator &() {
		cleanup();
		return &object;
	}

	operator T() const {
		return object;
	}

private:
	T object{ VK_NULL_HANDLE };
	std::function<void(T)> deleter;

	void cleanup() {
		if (object != VK_NULL_HANDLE) {
			deleter(object);
		}
		object = VK_NULL_HANDLE;
	}
};

class TriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan(); //Set everything up before entering main loop
		mainLoop();
	}
private:
	GLFWwindow* vulkanWindow;
	VDeleter<VkInstance> instance{ vkDestroyInstance };

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Don't create an OpenGL context with our glfwCreateWindow call
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //Disable window resize
		vulkanWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void createInstance() {
		VkApplicationInfo applicationInfo = {}; //default struct
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = "Rendering a Triangle";
		applicationInfo.pEngineName = "No Engine";
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.apiVersion = VK_API_VERSION_1_0; //Which vulkan api version to use

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &applicationInfo;

		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); //Get the desired global extensions

		//Specify how many extensions 
		createInfo.enabledExtensionCount = glfwExtensionCount;
		//Supply the extensions
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		createInfo.enabledLayerCount = 0;

		//We have now specified everything we need in order to create a Vulkan instance

		//so, let's make one.
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("vkCreateInstance failed");
		}

	}

	void initVulkan() {
		createInstance();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(vulkanWindow)) {
			glfwPollEvents();
		}
	}
};

int main() {
	TriangleApplication triangleApp; //TriangleApplication instance

	try //Try to call TriangleApplication.run() function
	{
		triangleApp.run();
	}
	catch (const std::runtime_error& e) //If any errors are thrown, catch them and spit them out
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE; //Return 1 = exited with failure (crash)
	}

	return EXIT_SUCCESS; //return 0 - exited with success
}