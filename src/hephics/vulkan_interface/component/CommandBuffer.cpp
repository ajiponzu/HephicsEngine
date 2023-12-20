#include "../Interface.hpp"

void vk_interface::component::CommandBuffer::BeginRecordingCommands(
	const vk::CommandBufferBeginInfo& begin_info)
{
	m_commandBuffer->begin(begin_info);
}

void vk_interface::component::CommandBuffer::EndRecordingCommands()
{
	m_commandBuffer->end();
}

void vk_interface::component::CommandBuffer::ResetCommands(const vk::CommandBufferResetFlags& reset_flag)
{
	m_commandBuffer->reset(reset_flag);
}

void vk_interface::component::CommandBuffer::TransitionImageCommandLayout(const std::shared_ptr<Image>& vk_image,
	const vk::Format& vk_format, const std::pair<vk::ImageLayout, vk::ImageLayout>& transition_layout_pair)
{
	const auto& [old_image_layout, new_image_layout] = transition_layout_pair;

	vk::ImageMemoryBarrier image_memory_barrier(vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eNone, old_image_layout, new_image_layout, 0, 0, vk_image->GetImage().get(),
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	vk::PipelineStageFlags src_stage_flags;
	vk::PipelineStageFlags dst_stage_flags;

	if (old_image_layout == vk::ImageLayout::eUndefined
		&& new_image_layout == vk::ImageLayout::eTransferDstOptimal)
	{
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		src_stage_flags = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage_flags = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (old_image_layout == vk::ImageLayout::eTransferDstOptimal
		&& new_image_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		src_stage_flags = vk::PipelineStageFlagBits::eTransfer;
		dst_stage_flags = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (old_image_layout == vk::ImageLayout::eUndefined
		&& new_image_layout == vk::ImageLayout::eStencilAttachmentOptimal)
	{
		image_memory_barrier.dstAccessMask =
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		src_stage_flags = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage_flags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else
		throw std::invalid_argument("unsupported layout transition!");

	if (new_image_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		image_memory_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (vk_format == vk::Format::eD32SfloatS8Uint || vk_format == vk::Format::eD24UnormS8Uint)
			image_memory_barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
	}
	else
		image_memory_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

	m_commandBuffer->pipelineBarrier(src_stage_flags, dst_stage_flags,
		{}, nullptr, nullptr, image_memory_barrier);
}

void vk_interface::component::CommandBuffer::CopyBuffer(const std::shared_ptr<Buffer>& src_buffer,
	const std::shared_ptr<Buffer>& dst_buffer, const size_t& device_size)
{
	vk::BufferCopy copy_region(0, 0, device_size);
	m_commandBuffer->copyBuffer(src_buffer->GetBuffer().get(), dst_buffer->GetBuffer().get(), { copy_region });
}

void vk_interface::component::CommandBuffer::CopyTexture(const std::shared_ptr<Buffer>& staging_buffer,
	const std::shared_ptr<Image>& texture_image, const vk::Extent2D& extent)
{
	vk::BufferImageCopy image_copy_region(0, 0, 0,
		vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
		vk::Offset3D(0, 0, 0), vk::Extent3D(extent, 1U));

	m_commandBuffer->copyBufferToImage(staging_buffer->GetBuffer().get(), texture_image->GetImage().get(),
		vk::ImageLayout::eTransferDstOptimal, { image_copy_region });
}

void vk_interface::component::CommandBuffer::SetCommandBuffer(std::vector<vk::UniqueCommandBuffer>&& command_buffers)
{
	if (command_buffers.empty())
		throw std::runtime_error("command_buffers: empty");

	m_commandBuffer = std::move(command_buffers.at(0));
}