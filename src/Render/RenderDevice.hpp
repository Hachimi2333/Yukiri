#pragma once

#include "VulkanUtility.hpp"
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <functional>
#include <cstdint>
#include <memory>
#include <map>
#include <vector>
#include <tuple>
#include <Yukiri/Render/Image.hpp>
#include "ImageCache.hpp"

namespace Yukiri::Detail
{
struct Buffer
{
	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VmaAllocation m_Allocation = VK_NULL_HANDLE;
	void* m_Mapped = nullptr;
	VkDeviceSize m_Size = 0;
};

struct Texture
{
	~Texture();
	VkDevice m_Device = VK_NULL_HANDLE;
	VmaAllocator m_Allocator = VK_NULL_HANDLE;
	VkDescriptorPool m_Pool = VK_NULL_HANDLE;
	VkImage m_Image = VK_NULL_HANDLE;
	VmaAllocation m_Allocation = VK_NULL_HANDLE;
	VkImageView m_View = VK_NULL_HANDLE;
	VkDescriptorSet m_Set = VK_NULL_HANDLE;
	std::uint32_t m_Width = 0;
	std::uint32_t m_Height = 0;
	std::uint32_t m_BytesPerPixel = 1;
	std::vector<unsigned char> m_Pixels;
	bool m_Dirty = true;
	bool m_Initialized = false;
	bool m_DiscardPixelsAfterUpload = false;
	VkRect2D m_DirtyBounds{};
	void MarkDirty(int x, int y, std::uint32_t width, std::uint32_t height);
};

class RenderDevice final
{
public:
	static Result<std::unique_ptr<RenderDevice>> Create(SDL_Window* probeWindow, bool validation);
	~RenderDevice();
	RenderDevice(const RenderDevice&) = delete;
	RenderDevice& operator=(const RenderDevice&) = delete;
	Result<Buffer> CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
	void DestroyBuffer(Buffer& buffer);
	Result<std::uint32_t> CreateTexture(std::uint32_t width, std::uint32_t height, bool rgba = false);
	Result<void> RecordTextureUploads(VkCommandBuffer command, Buffer& staging, std::vector<std::shared_ptr<Texture>>& retained);
	Result<std::uint32_t> GetImageTexture(const Image& image, std::uint32_t width, std::uint32_t height);
	void CollectImageTextures();

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_Physical = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkQueue m_Queue = VK_NULL_HANDLE;
	std::uint32_t m_QueueFamily = 0;
	VmaAllocator m_Allocator = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorPool> m_DescriptorPools;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkSampler m_Sampler = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
	std::uint32_t m_ValidationErrors = 0;
	std::vector<std::shared_ptr<Texture>> m_Textures;
	std::map<std::pair<std::uintptr_t, int>, std::uint32_t> m_ImageCache;

private:
	RenderDevice() = default;
	Result<void> Initialize(SDL_Window* probeWindow, bool validation);
	Result<void> AllocateTextureSet(Texture& texture);
	ImageCache m_PublicImages;
	std::vector<std::uint32_t> m_FreeTextureSlots;
};
}
