#include "WindowRenderer.hpp"
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <iostream>
#include <chrono>

namespace Yukiri::Detail
{
Result<std::unique_ptr<WindowRenderer>> WindowRenderer::Create(RenderDevice& device, SDL_Window* window)
{
	auto renderer = std::unique_ptr<WindowRenderer>(new WindowRenderer(device, window));
	auto initialized = renderer->Initialize();
	if (!initialized) return std::unexpected(initialized.error());
	return renderer;
}

Result<void> WindowRenderer::Initialize()
{
	if (!SDL_Vulkan_CreateSurface(m_Window, m_Device.m_Instance, nullptr, &m_Surface))
		return std::unexpected(Error{ErrorCode::GraphicsInitialization, SDL_GetError()});
	VkBool32 present = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_Device.m_Physical, m_Device.m_QueueFamily, m_Surface, &present);
	if (!present) return std::unexpected(Error{ErrorCode::UnsupportedDevice, "The shared graphics queue cannot present this window."});
	auto pool = VkInfo<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
	pool.queueFamilyIndex = m_Device.m_QueueFamily;
	pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	auto result = vkCreateCommandPool(m_Device.m_Device, &pool, nullptr, &m_Pool);
	if (result != VK_SUCCESS) return GraphicsError("Window command pool", result);
	for (auto& frame : m_Frames)
	{
		auto allocation = VkInfo<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
		allocation.commandPool = m_Pool;
		allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocation.commandBufferCount = 1;
		result = vkAllocateCommandBuffers(m_Device.m_Device, &allocation, &frame.m_Command);
		if (result != VK_SUCCESS) return GraphicsError("Window command buffer", result);
		auto semaphore = VkInfo<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
		result = vkCreateSemaphore(m_Device.m_Device, &semaphore, nullptr, &frame.m_Acquired);
		if (result != VK_SUCCESS) return GraphicsError("Acquire semaphore", result);
		auto fence = VkInfo<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
		fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		result = vkCreateFence(m_Device.m_Device, &fence, nullptr, &frame.m_Fence);
		if (result != VK_SUCCESS) return GraphicsError("Frame fence", result);
	}
	return Recreate();
}

void WindowRenderer::ReleaseSwapchain()
{
	for (auto view : m_Views) vkDestroyImageView(m_Device.m_Device, view, nullptr);
	for (auto semaphore : m_Presented) vkDestroySemaphore(m_Device.m_Device, semaphore, nullptr);
	m_Views.clear();
	m_Presented.clear();
	m_Images.clear();
	m_Initialized.clear();
	if (m_Pipeline) vkDestroyPipeline(m_Device.m_Device, m_Pipeline, nullptr);
	if (m_Swapchain) vkDestroySwapchainKHR(m_Device.m_Device, m_Swapchain, nullptr);
	m_Pipeline = VK_NULL_HANDLE;
	m_Swapchain = VK_NULL_HANDLE;
}

WindowRenderer::~WindowRenderer()
{
	if (SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "YUKIRI_TRACE_FRAMES"))
		std::cout << "[Yukiri] Presented frames: " << m_PresentCount << '\n';
	vkDeviceWaitIdle(m_Device.m_Device);
	ReleaseSwapchain();
	for (auto& frame : m_Frames)
	{
		m_Device.DestroyBuffer(frame.m_Vertices);
		m_Device.DestroyBuffer(frame.m_Uploads);
		if (frame.m_Acquired) vkDestroySemaphore(m_Device.m_Device, frame.m_Acquired, nullptr);
		if (frame.m_Fence) vkDestroyFence(m_Device.m_Device, frame.m_Fence, nullptr);
	}
	if (m_Pool) vkDestroyCommandPool(m_Device.m_Device, m_Pool, nullptr);
	if (m_Surface) vkDestroySurfaceKHR(m_Device.m_Instance, m_Surface, nullptr);
}

Result<void> WindowRenderer::Recreate()
{
	int width = 0;
	int height = 0;
	SDL_GetWindowSizeInPixels(m_Window, &width, &height);
	if (width <= 0 || height <= 0) return {};
	auto result = vkDeviceWaitIdle(m_Device.m_Device);
	if (result != VK_SUCCESS) return GraphicsError("Wait before swapchain recreation", result);
	ReleaseSwapchain();
	VkSurfaceCapabilitiesKHR capabilities{};
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.m_Physical, m_Surface, &capabilities);
	if (result != VK_SUCCESS) return GraphicsError("Surface capabilities", result);
	std::uint32_t count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device.m_Physical, m_Surface, &count, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device.m_Physical, m_Surface, &count, formats.data());
	auto found = std::ranges::find_if(formats, [](const auto& format)
	{
		return (format.format == VK_FORMAT_B8G8R8A8_SRGB || format.format == VK_FORMAT_R8G8B8A8_SRGB) &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	});
	if (found == formats.end())
		return std::unexpected(Error{ErrorCode::UnsupportedDevice, "An sRGB swapchain is required for linear UI blending."});
	m_Format = found->format;
	m_Extent = capabilities.currentExtent;
	if (m_Extent.width == std::numeric_limits<std::uint32_t>::max())
		m_Extent = {std::clamp(static_cast<std::uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp(static_cast<std::uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
	if (!m_Extent.width || !m_Extent.height) return {};
	count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount) count = std::min(count, capabilities.maxImageCount);
	auto swapchain = VkInfo<VkSwapchainCreateInfoKHR>(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
	swapchain.surface = m_Surface;
	swapchain.minImageCount = count;
	swapchain.imageFormat = m_Format;
	swapchain.imageColorSpace = found->colorSpace;
	swapchain.imageExtent = m_Extent;
	swapchain.imageArrayLayers = 1;
	swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain.preTransform = capabilities.currentTransform;
	swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchain.clipped = VK_TRUE;
	result = vkCreateSwapchainKHR(m_Device.m_Device, &swapchain, nullptr, &m_Swapchain);
	if (result != VK_SUCCESS) return GraphicsError("vkCreateSwapchainKHR", result);
	vkGetSwapchainImagesKHR(m_Device.m_Device, m_Swapchain, &count, nullptr);
	m_Images.resize(count);
	vkGetSwapchainImagesKHR(m_Device.m_Device, m_Swapchain, &count, m_Images.data());
	m_Initialized.resize(count, false);
	for (auto image : m_Images)
	{
		auto view = VkInfo<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
		view.image = image;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = m_Format;
		view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		VkImageView imageView = VK_NULL_HANDLE;
		result = vkCreateImageView(m_Device.m_Device, &view, nullptr, &imageView);
		if (result != VK_SUCCESS) return GraphicsError("Swapchain image view", result);
		m_Views.push_back(imageView);
		auto info = VkInfo<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
		VkSemaphore semaphore = VK_NULL_HANDLE;
		result = vkCreateSemaphore(m_Device.m_Device, &info, nullptr, &semaphore);
		if (result != VK_SUCCESS) return GraphicsError("Present semaphore", result);
		m_Presented.push_back(semaphore);
	}
	return CreatePipeline();
}

Result<bool> WindowRenderer::TryBeginFrame()
{
	m_NeedsRetry = false;
	int width = 0;
	int height = 0;
	SDL_GetWindowSizeInPixels(m_Window, &width, &height);
	if (width <= 0 || height <= 0 || (SDL_GetWindowFlags(m_Window) & SDL_WINDOW_MINIMIZED)) return false;
	if (!m_Swapchain || static_cast<std::uint32_t>(width) != m_Extent.width || static_cast<std::uint32_t>(height) != m_Extent.height)
	{
		auto recreated = Recreate();
		if (!recreated) return std::unexpected(recreated.error());
		if (!m_Swapchain) return false;
	}
	auto& frame = m_Frames[m_Frame];
	auto result = vkGetFenceStatus(m_Device.m_Device, frame.m_Fence);
	if (result == VK_NOT_READY) return false;
	if (result != VK_SUCCESS) return GraphicsError("Frame fence status", result);
	result = vkAcquireNextImageKHR(m_Device.m_Device, m_Swapchain, 0, frame.m_Acquired, VK_NULL_HANDLE, &m_AcquiredImage);
	if (result == VK_NOT_READY || result == VK_TIMEOUT) return false;
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		auto recreated = Recreate();
		if (!recreated) return std::unexpected(recreated.error());
		return false;
	}
	if (result == VK_SUBOPTIMAL_KHR) m_NeedsRetry = true;
	else if (result != VK_SUCCESS) return GraphicsError("Acquire swapchain image", result);
	return true;
}

Result<void> WindowRenderer::Draw(const DrawList& list, Size logicalSize, Color background)
{
	const auto start = std::chrono::steady_clock::now();
	const bool suboptimal = m_NeedsRetry;
	auto& frame = m_Frames[m_Frame];
	const auto imageIndex = m_AcquiredImage;
	const int width = static_cast<int>(m_Extent.width);
	const int height = static_cast<int>(m_Extent.height);
	VkResult result;
	const auto bytes = list.m_Vertices.size() * sizeof(Vertex);
	if (bytes > frame.m_Vertices.m_Size)
	{
		m_Device.DestroyBuffer(frame.m_Vertices);
		auto buffer = m_Device.CreateBuffer(std::max<VkDeviceSize>(bytes * 2, 65536), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		if (!buffer) return std::unexpected(buffer.error());
		frame.m_Vertices = *buffer;
	}
	if (bytes)
	{
		std::memcpy(frame.m_Vertices.m_Mapped, list.m_Vertices.data(), bytes);
		vmaFlushAllocation(m_Device.m_Allocator, frame.m_Vertices.m_Allocation, 0, VK_WHOLE_SIZE);
	}
	vkResetCommandBuffer(frame.m_Command, 0);
	auto begin = VkInfo<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	result = vkBeginCommandBuffer(frame.m_Command, &begin);
	if (result != VK_SUCCESS) return GraphicsError("Begin frame", result);
	auto uploads = m_Device.RecordTextureUploads(frame.m_Command, frame.m_Uploads);
	if (!uploads) return uploads;
	ImageBarrier(frame.m_Command, m_Images[imageIndex],
		m_Initialized[imageIndex] ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
	auto linear = [](float component) { return component <= 0.04045f ? component / 12.92f : std::pow((component + 0.055f) / 1.055f, 2.4f); };
	auto attachment = VkInfo<VkRenderingAttachmentInfo>(VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO);
	attachment.imageView = m_Views[imageIndex];
	attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clearValue.color = {{linear(background.m_R), linear(background.m_G), linear(background.m_B), background.m_A}};
	auto rendering = VkInfo<VkRenderingInfo>(VK_STRUCTURE_TYPE_RENDERING_INFO);
	rendering.renderArea.extent = m_Extent;
	rendering.layerCount = 1;
	rendering.colorAttachmentCount = 1;
	rendering.pColorAttachments = &attachment;
	vkCmdBeginRendering(frame.m_Command, &rendering);
	if (bytes)
	{
		vkCmdBindPipeline(frame.m_Command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(frame.m_Command, 0, 1, &frame.m_Vertices.m_Buffer, &offset);
		VkViewport viewport{0, 0, static_cast<float>(m_Extent.width), static_cast<float>(m_Extent.height), 0, 1};
		vkCmdSetViewport(frame.m_Command, 0, 1, &viewport);
		const float extent[] = {logicalSize.m_Width, logicalSize.m_Height};
		vkCmdPushConstants(frame.m_Command, m_Device.m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(extent), extent);
		const float scaleX = static_cast<float>(m_Extent.width) / logicalSize.m_Width;
		const float scaleY = static_cast<float>(m_Extent.height) / logicalSize.m_Height;
		for (const auto& batch : list.m_Batches)
		{
			const auto left = std::clamp(static_cast<int>(std::floor(batch.m_Clip.m_X * scaleX)), 0, width);
			const auto top = std::clamp(static_cast<int>(std::floor(batch.m_Clip.m_Y * scaleY)), 0, height);
			const auto right = std::clamp(static_cast<int>(std::ceil(batch.m_Clip.Right() * scaleX)), left, width);
			const auto bottom = std::clamp(static_cast<int>(std::ceil(batch.m_Clip.Bottom() * scaleY)), top, height);
			if (right <= left || bottom <= top) continue;
			VkRect2D scissor{{left, top}, {static_cast<std::uint32_t>(right - left), static_cast<std::uint32_t>(bottom - top)}};
			vkCmdSetScissor(frame.m_Command, 0, 1, &scissor);
			const auto set = m_Device.m_Textures.at(batch.m_Texture)->m_Set;
			vkCmdBindDescriptorSets(frame.m_Command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Device.m_PipelineLayout, 0, 1, &set, 0, nullptr);
			vkCmdDraw(frame.m_Command, batch.m_Count, 1, batch.m_First, 0);
		}
	}
	vkCmdEndRendering(frame.m_Command);
	ImageBarrier(frame.m_Command, m_Images[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, 0);
	result = vkEndCommandBuffer(frame.m_Command);
	if (result != VK_SUCCESS) return GraphicsError("End frame", result);
	auto wait = VkInfo<VkSemaphoreSubmitInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
	wait.semaphore = frame.m_Acquired;
	wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	auto signal = VkInfo<VkSemaphoreSubmitInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
	signal.semaphore = m_Presented[imageIndex];
	signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	auto command = VkInfo<VkCommandBufferSubmitInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO);
	command.commandBuffer = frame.m_Command;
	auto submit = VkInfo<VkSubmitInfo2>(VK_STRUCTURE_TYPE_SUBMIT_INFO_2);
	submit.waitSemaphoreInfoCount = submit.signalSemaphoreInfoCount = submit.commandBufferInfoCount = 1;
	submit.pWaitSemaphoreInfos = &wait;
	submit.pSignalSemaphoreInfos = &signal;
	submit.pCommandBufferInfos = &command;
	vkResetFences(m_Device.m_Device, 1, &frame.m_Fence);
	result = vkQueueSubmit2(m_Device.m_Queue, 1, &submit, frame.m_Fence);
	if (result != VK_SUCCESS) return GraphicsError("Submit frame", result);
	m_Initialized[imageIndex] = true;
	auto present = VkInfo<VkPresentInfoKHR>(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &m_Presented[imageIndex];
	present.swapchainCount = 1;
	present.pSwapchains = &m_Swapchain;
	present.pImageIndices = &imageIndex;
	const auto beforePresent = std::chrono::steady_clock::now();
	m_RecordMs = std::chrono::duration<double, std::milli>(beforePresent - start).count();
	result = vkQueuePresentKHR(m_Device.m_Queue, &present);
	m_PresentMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - beforePresent).count();
	if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) ++m_PresentCount;
	m_Frame = (m_Frame + 1) % m_Frames.size();
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || suboptimal)
	{
		m_NeedsRetry = true;
		return Recreate();
	}
	if (result != VK_SUCCESS) return GraphicsError("Present frame", result);
	return {};
}
}
