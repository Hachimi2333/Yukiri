#pragma once

#include "DrawList.hpp"
#include "RenderDevice.hpp"
#include <array>

namespace Yukiri::Detail
{
class WindowRenderer final
{
public:
	static Result<std::unique_ptr<WindowRenderer>> Create(RenderDevice& device, SDL_Window* window);
	~WindowRenderer();
	Result<bool> TryBeginFrame();
	Result<void> Draw(const DrawList& list, Size logicalSize, Color background);
	[[nodiscard]] bool NeedsRetry() const noexcept { return m_NeedsRetry; }
	[[nodiscard]] double GetRecordMilliseconds() const noexcept { return m_RecordMs; }
	[[nodiscard]] double GetPresentMilliseconds() const noexcept { return m_PresentMs; }

private:
	struct Frame
	{
		VkCommandBuffer m_Command = VK_NULL_HANDLE;
		VkSemaphore m_Acquired = VK_NULL_HANDLE;
		VkFence m_Fence = VK_NULL_HANDLE;
		Buffer m_Vertices;
		Buffer m_Uploads;
		std::vector<std::shared_ptr<Texture>> m_Textures;
	};
	WindowRenderer(RenderDevice& device, SDL_Window* window) : m_Device(device), m_Window(window) {}
	Result<void> Initialize();
	Result<void> Recreate();
	Result<void> CreatePipeline();
	void ReleaseSwapchain();
	RenderDevice& m_Device;
	SDL_Window* m_Window;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	VkExtent2D m_Extent{};
	VkFormat m_Format = VK_FORMAT_UNDEFINED;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkCommandPool m_Pool = VK_NULL_HANDLE;
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_Views;
	std::vector<VkSemaphore> m_Presented;
	std::vector<bool> m_Initialized;
	std::array<Frame, 2> m_Frames;
	std::size_t m_Frame = 0;
	std::uint32_t m_AcquiredImage = 0;
	bool m_NeedsRetry = false;
	std::uint64_t m_PresentCount = 0;
	double m_RecordMs = 0;
	double m_PresentMs = 0;
};
}
