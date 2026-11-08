#pragma once

#include <Yukiri/Core/Error.hpp>
#include <volk.h>
#include <string_view>

namespace Yukiri::Detail
{
template<typename T>
T VkInfo(VkStructureType type)
{
	T value{};
	value.sType = type;
	return value;
}

inline std::unexpected<Error> GraphicsError(std::string_view operation, VkResult result)
{
	return std::unexpected(Error{ErrorCode::GraphicsOperation,
		std::string(operation) + " failed (VkResult " + std::to_string(static_cast<int>(result)) + ")."});
}

inline void ImageBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
	VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess,
	VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess)
{
	auto barrier = VkInfo<VkImageMemoryBarrier2>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2);
	barrier.srcStageMask = sourceStage;
	barrier.srcAccessMask = sourceAccess;
	barrier.dstStageMask = destinationStage;
	barrier.dstAccessMask = destinationAccess;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	auto dependency = VkInfo<VkDependencyInfo>(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(command, &dependency);
}
}
