#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <iostream>
#include <stdexcept> //error handling
#include <functional> //for lambda functions
#include <vector>
#include <cstring>
//Window size
const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG //if not debugging, don't use validation layers
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true; //if debugging, use validation layers 
#endif 


//The struct we created (VkDebugReportCallbackCreateInfoEXT createInfo) should be passed
//to the vkCreateDebugReportCallbacEXT function to create the vkDebugReportCallbackEXT object.
//Unfortunately, because this function is an extension function, it is not automatically loaded.
//We will have to look up its address ourselves using vkGetInstanceProcAddr.
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}


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
	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Don't create an OpenGL context with our glfwCreateWindow call
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //Disable window resize
		vulkanWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void createInstance() {
		//enable validation layers and check to see if checkValidationLayerSupport passes
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available"); //<If it doesn't pass
		}
		//If validation passes this is run with validation layers

		VkApplicationInfo applicationInfo = {}; //default struct
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = "Rendering a Triangle";
		applicationInfo.pEngineName = "No Engine";
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.apiVersion = VK_API_VERSION_1_0; //Which vulkan api version to use

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &applicationInfo;

		//Get our extensions and store them
		 std::vector<const char*>extensions = getRequiredExtensions();
		 //Specify how many extensions 
		 createInfo.enabledExtensionCount = extensions.size();
		 //Supply the extensions
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (enableValidationLayers) { //if validation layers is enabled, provide our VkInstanceCreateInfo insance 'createInfo' with..
			createInfo.enabledLayerCount = validationLayers.size(); //how many validation layers
			createInfo.ppEnabledLayerNames = validationLayers.data(); //and the validation layer names
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		//We have now specified everything we need in order to create a Vulkan instance

		//so, let's make one.
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("vkCreateInstance failed");
		}

	}

	//Returns a vector of extensions based on whether validation layers are enabled or not
	std::vector<const char*> getRequiredExtensions() {

		std::vector<const char*> extensions;

		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for (unsigned int i = 0; i < glfwExtensionCount; i++) {
			extensions.push_back(glfwExtensions[i]);
		}

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		return extensions;
	}

	//Check if all requested layers are available
	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


		//Check if all layers in our 'validationLayers' vector exist in the availableLayers
		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	//callback function
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags, //specifies the type of message (which can be a combination of bit flags).
		VkDebugReportObjectTypeEXT objType, //specifies the type of object that is the subject of the message. For example if obj is a VkPhysicalDevice the objType would be VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg, //pointer to the message itself
		void* userData) { //userData is a parameter to pass your own data to the callback

		std::cerr << "validation layer: " << msg << std::endl;

		return VK_FALSE;
	}

	void initVulkan() {
		createInstance();
		setupDebugCallBack();
	}

	void setupDebugCallBack() {
		if (!enableValidationLayers) return;

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		//flags field allows us to filter which types of messages we would like to recieve
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		//The pfnCallBack field specifies the pointer to the callback function. You can optimally pass a pointer to the pUserData field
		//which will be passed along to the callback function via the userData parameter
		createInfo.pfnCallback = debugCallback;

		if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
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