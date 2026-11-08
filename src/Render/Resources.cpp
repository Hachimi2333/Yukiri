#include "RenderDevice.hpp"
#include <cstring>

namespace Yukiri::Detail
{
Result<Buffer> RenderDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
{
	Buffer buffer;
	buffer.m_Size = size;
	auto info = VkInfo<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
	info.size = size;
	info.usage = usage;
	VmaAllocationCreateInfo allocation{};
	allocation.usage = VMA_MEMORY_USAGE_AUTO;
	allocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	VmaAllocationInfo details{};
	const auto result = vmaCreateBuffer(m_Allocator, &info, &allocation, &buffer.m_Buffer, &buffer.m_Allocation, &details);
	if (result != VK_SUCCESS) return GraphicsError("vmaCreateBuffer", result);
	buffer.m_Mapped = details.pMappedData;
	return buffer;
}

void RenderDevice::DestroyBuffer(Buffer& buffer)
{
	if (buffer.m_Buffer) vmaDestroyBuffer(m_Allocator, buffer.m_Buffer, buffer.m_Allocation);
	buffer = {};
}

Result<std::uint32_t> RenderDevice::CreateTexture(std::uint32_t width, std::uint32_t height)
{
	auto texture = std::make_unique<Texture>();
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_Pixels.resize(static_cast<std::size_t>(width) * height);
	auto imageInfo = VkInfo<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8_UNORM;
	imageInfo.extent = {width, height, 1};
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VmaAllocationCreateInfo allocation{};
	allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	auto result = vmaCreateImage(m_Allocator, &imageInfo, &allocation, &texture->m_Image, &texture->m_Allocation, nullptr);
	if (result != VK_SUCCESS) return GraphicsError("vmaCreateImage", result);
	auto viewInfo = VkInfo<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
	viewInfo.image = texture->m_Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8_UNORM;
	viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	result = vkCreateImageView(m_Device, &viewInfo, nullptr, &texture->m_View);
	if (result != VK_SUCCESS)
	{
		vmaDestroyImage(m_Allocator, texture->m_Image, texture->m_Allocation);
		return GraphicsError("vkCreateImageView", result);
	}
	auto setInfo = VkInfo<VkDescriptorSetAllocateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
	setInfo.descriptorPool = m_DescriptorPool;
	setInfo.descriptorSetCount = 1;
	setInfo.pSetLayouts = &m_SetLayout;
	result = vkAllocateDescriptorSets(m_Device, &setInfo, &texture->m_Set);
	if (result != VK_SUCCESS)
	{
		vkDestroyImageView(m_Device, texture->m_View, nullptr);
		vmaDestroyImage(m_Allocator, texture->m_Image, texture->m_Allocation);
		return GraphicsError("vkAllocateDescriptorSets", result);
	}
	VkDescriptorImageInfo descriptor{m_Sampler, texture->m_View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	auto write = VkInfo<VkWriteDescriptorSet>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	write.dstSet = texture->m_Set;
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = &descriptor;
	vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
	const auto index = static_cast<std::uint32_t>(m_Textures.size());
	m_Textures.push_back(std::move(texture));
	return index;
}

Result<void> RenderDevice::Immediate(const std::function<void(VkCommandBuffer)>& record)
{
	auto allocation = VkInfo<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
	allocation.commandPool = m_UploadPool;
	allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocation.commandBufferCount = 1;
	VkCommandBuffer command = VK_NULL_HANDLE;
	auto result = vkAllocateCommandBuffers(m_Device, &allocation, &command);
	if (result != VK_SUCCESS) return GraphicsError("vkAllocateCommandBuffers", result);
	auto begin = VkInfo<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	result = vkBeginCommandBuffer(command, &begin);
	if (result == VK_SUCCESS)
	{
		record(command);
		result = vkEndCommandBuffer(command);
	}
	if (result == VK_SUCCESS)
	{
		auto commandInfo = VkInfo<VkCommandBufferSubmitInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO);
		commandInfo.commandBuffer = command;
		auto submit = VkInfo<VkSubmitInfo2>(VK_STRUCTURE_TYPE_SUBMIT_INFO_2);
		submit.commandBufferInfoCount = 1;
		submit.pCommandBufferInfos = &commandInfo;
		result = vkQueueSubmit2(m_Queue, 1, &submit, VK_NULL_HANDLE);
		if (result == VK_SUCCESS) result = vkQueueWaitIdle(m_Queue);
	}
	vkFreeCommandBuffers(m_Device, m_UploadPool, 1, &command);
	if (result != VK_SUCCESS) return GraphicsError("Immediate upload", result);
	return {};
}

Result<void> RenderDevice::FlushTextures()
{
	for (const auto& texture : m_Textures)
	{
		if (!texture->m_Dirty) continue;
		auto staging = CreateBuffer(texture->m_Pixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		if (!staging) return std::unexpected(staging.error());
		std::memcpy(staging->m_Mapped, texture->m_Pixels.data(), texture->m_Pixels.size());
		vmaFlushAllocation(m_Allocator, staging->m_Allocation, 0, VK_WHOLE_SIZE);
		auto uploaded = Immediate([&](VkCommandBuffer command)
		{
			ImageBarrier(command, texture->m_Image,
				texture->m_Initialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				texture->m_Initialized ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_2_NONE,
				texture->m_Initialized ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : 0,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
			VkBufferImageCopy copy{};
			copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			copy.imageExtent = {texture->m_Width, texture->m_Height, 1};
			vkCmdCopyBufferToImage(command, staging->m_Buffer, texture->m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
			ImageBarrier(command, texture->m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
		});
		DestroyBuffer(*staging);
		if (!uploaded) return uploaded;
		texture->m_Initialized = true;
		texture->m_Dirty = false;
	}
	return {};
}
}
