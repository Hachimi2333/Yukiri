#include "RenderDevice.hpp"
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <cstring>
#include <iostream>

namespace Yukiri::Detail
{
namespace
{
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user)
{
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		++static_cast<RenderDevice*>(user)->m_ValidationErrors;
	std::cerr << "[Vulkan] " << data->pMessage << '\n';
	return VK_FALSE;
}
}

Result<std::unique_ptr<RenderDevice>> RenderDevice::Create(SDL_Window* window, bool validation)
{
	auto device = std::unique_ptr<RenderDevice>(new RenderDevice());
	auto result = device->Initialize(window, validation);
	if (!result)
		return std::unexpected(result.error());
	return device;
}

Result<void> RenderDevice::Initialize(SDL_Window* window, bool validation)
{
	if (volkInitialize() != VK_SUCCESS || volkGetInstanceVersion() < VK_API_VERSION_1_4)
		return std::unexpected(Error{ErrorCode::VulkanUnavailable, "A Vulkan 1.4 loader and driver are required."});
	std::uint32_t extensionCount = 0;
	const auto* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	if (!sdlExtensions)
		return std::unexpected(Error{ErrorCode::GraphicsInitialization, SDL_GetError()});
	std::vector<const char*> extensions(sdlExtensions, sdlExtensions + extensionCount);
	std::vector<const char*> layers;
#if defined(YUKIRI_ENABLE_VALIDATION)
	std::uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	if (validation && std::ranges::any_of(availableLayers, [](const auto& layer)
		{ return std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0; }))
	{
		layers.push_back("VK_LAYER_KHRONOS_validation");
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	else if (validation)
		std::cerr << "[Yukiri] Validation layer unavailable; continuing without validation.\n";
#else
	(void)validation;
#endif
	auto application = VkInfo<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
	application.pApplicationName = "Yukiri";
	application.pEngineName = "Yukiri";
	application.apiVersion = VK_API_VERSION_1_4;
	auto debug = VkInfo<VkDebugUtilsMessengerCreateInfoEXT>(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
	debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug.pfnUserCallback = DebugCallback;
	debug.pUserData = this;
	auto instanceInfo = VkInfo<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
	instanceInfo.pApplicationInfo = &application;
	instanceInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	instanceInfo.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
	instanceInfo.ppEnabledLayerNames = layers.data();
	instanceInfo.pNext = layers.empty() ? nullptr : &debug;
	auto result = vkCreateInstance(&instanceInfo, nullptr, &m_Instance);
	if (result != VK_SUCCESS) return GraphicsError("vkCreateInstance", result);
	volkLoadInstance(m_Instance);
	if (!layers.empty())
	{
		result = vkCreateDebugUtilsMessengerEXT(m_Instance, &debug, nullptr, &m_Messenger);
		if (result != VK_SUCCESS) return GraphicsError("vkCreateDebugUtilsMessengerEXT", result);
	}
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (!SDL_Vulkan_CreateSurface(window, m_Instance, nullptr, &surface))
		return std::unexpected(Error{ErrorCode::GraphicsInitialization, SDL_GetError()});
	std::uint32_t count = 0;
	result = vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
	if (result != VK_SUCCESS) { vkDestroySurfaceKHR(m_Instance, surface, nullptr); return GraphicsError("Enumerate devices", result); }
	std::vector<VkPhysicalDevice> devices(count);
	result = vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());
	if (result != VK_SUCCESS) { vkDestroySurfaceKHR(m_Instance, surface, nullptr); return GraphicsError("Enumerate devices", result); }
	int bestScore = -1;
	for (auto physical : devices)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physical, &properties);
		if (properties.apiVersion < VK_API_VERSION_1_4) continue;
		auto features13 = VkInfo<VkPhysicalDeviceVulkan13Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
		auto features = VkInfo<VkPhysicalDeviceFeatures2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
		features.pNext = &features13;
		vkGetPhysicalDeviceFeatures2(physical, &features);
		if (!features13.dynamicRendering || !features13.synchronization2) continue;
		std::uint32_t queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueCount, nullptr);
		std::vector<VkQueueFamilyProperties> queues(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueCount, queues.data());
		for (std::uint32_t i = 0; i < queueCount; ++i)
		{
			VkBool32 present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &present);
			const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 100 : 50;
			if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present && score > bestScore)
			{
				m_Physical = physical;
				m_QueueFamily = i;
				bestScore = score;
			}
		}
	}
	vkDestroySurfaceKHR(m_Instance, surface, nullptr);
	if (!m_Physical)
		return std::unexpected(Error{ErrorCode::UnsupportedDevice, "No present-capable Vulkan 1.4 graphics device was found."});
	const float priority = 1;
	auto queueInfo = VkInfo<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
	queueInfo.queueFamilyIndex = m_QueueFamily;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &priority;
	auto features13 = VkInfo<VkPhysicalDeviceVulkan13Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;
	const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	auto deviceInfo = VkInfo<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
	deviceInfo.pNext = &features13;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensions;
	result = vkCreateDevice(m_Physical, &deviceInfo, nullptr, &m_Device);
	if (result != VK_SUCCESS) return GraphicsError("vkCreateDevice", result);
	volkLoadDevice(m_Device);
	vkGetDeviceQueue(m_Device, m_QueueFamily, 0, &m_Queue);
	VmaVulkanFunctions functions{};
	functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.instance = m_Instance;
	allocatorInfo.physicalDevice = m_Physical;
	allocatorInfo.device = m_Device;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
	allocatorInfo.pVulkanFunctions = &functions;
	result = vmaCreateAllocator(&allocatorInfo, &m_Allocator);
	if (result != VK_SUCCESS) return GraphicsError("vmaCreateAllocator", result);
	VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
	auto layoutInfo = VkInfo<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;
	result = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_SetLayout);
	if (result != VK_SUCCESS) return GraphicsError("vkCreateDescriptorSetLayout", result);
	VkPushConstantRange pushRange{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2};
	auto pipelineInfo = VkInfo<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_SetLayout;
	pipelineInfo.pushConstantRangeCount = 1;
	pipelineInfo.pPushConstantRanges = &pushRange;
	result = vkCreatePipelineLayout(m_Device, &pipelineInfo, nullptr, &m_PipelineLayout);
	if (result != VK_SUCCESS) return GraphicsError("vkCreatePipelineLayout", result);
	auto samplerInfo = VkInfo<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	result = vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler);
	if (result != VK_SUCCESS) return GraphicsError("vkCreateSampler", result);
	auto white = CreateTexture(1, 1);
	if (!white) return std::unexpected(white.error());
	m_Textures[0]->m_Pixels[0] = 255;
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_Physical, &properties);
	std::cout << "Yukiri Vulkan 1.4 device: " << properties.deviceName << '\n';
	return {};
}

RenderDevice::~RenderDevice()
{
	if (m_Device)
	{
		vkDeviceWaitIdle(m_Device);
		m_Textures.clear();
		if (m_Sampler) vkDestroySampler(m_Device, m_Sampler, nullptr);
		if (m_PipelineLayout) vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		for (auto pool : m_DescriptorPools) vkDestroyDescriptorPool(m_Device, pool, nullptr);
		if (m_SetLayout) vkDestroyDescriptorSetLayout(m_Device, m_SetLayout, nullptr);
		if (m_Allocator) vmaDestroyAllocator(m_Allocator);
		vkDestroyDevice(m_Device, nullptr);
	}
	if (m_Messenger) vkDestroyDebugUtilsMessengerEXT(m_Instance, m_Messenger, nullptr);
	if (m_Instance) vkDestroyInstance(m_Instance, nullptr);
}
}
