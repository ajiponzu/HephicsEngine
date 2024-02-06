#pragma once

#include "vulkan_interface/Interface.hpp"

namespace hephics_helper
{
	class GPUBuffer : public vk_interface::component::Buffer
	{
	protected:

	public:
		GPUBuffer() = default;
		GPUBuffer(const std::shared_ptr<vk_interface::Instance>& gpu_instance,
			const size_t& buffer_size, const vk::BufferUsageFlags& usage_flags)
		{
			Initialize(gpu_instance, buffer_size, usage_flags);
		}
		~GPUBuffer() {}

		void Initialize(const std::shared_ptr<vk_interface::Instance>& gpu_instance,
			const size_t& buffer_size, const vk::BufferUsageFlags& usage_flags);
	};

	class UniformBuffer : public vk_interface::component::Buffer
	{
	protected:

	public:
		UniformBuffer() = default;
		UniformBuffer(const std::shared_ptr<vk_interface::Instance>& gpu_instance, const size_t& buffer_size)
		{
			Initialize(gpu_instance, buffer_size);
		}
		~UniformBuffer() {}

		void Initialize(const std::shared_ptr<vk_interface::Instance>& gpu_instance, const size_t& buffer_size);
	};

	class StagingBuffer : public vk_interface::component::Buffer
	{
	protected:

	public:
		StagingBuffer() = default;
		StagingBuffer(const std::shared_ptr<vk_interface::Instance>& gpu_instance, const size_t& buffer_size)
		{
			Initialize(gpu_instance, buffer_size);
		}
		~StagingBuffer() {}

		void Initialize(const std::shared_ptr<vk_interface::Instance>& gpu_instance, const size_t& buffer_size);
	};

	namespace vk_init
	{
#ifdef _DEBUG
		std::vector<const char*> get_validation_layers();

		bool check_validation_layer_support();

		VKAPI_ATTR VkBool32 VKAPI_CALL
			debug_utils_messenger_callback(::VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
				::VkDebugUtilsMessageTypeFlagsEXT message_types,
				::VkDebugUtilsMessengerCallbackDataEXT const* p_callback_data,
				void* p_user_data);
#endif

		std::vector<const char*> get_device_extensions();

		std::vector<const char*> get_required_extensions();

		bool check_device_extension_support(const vk::PhysicalDevice& physical_device);

		vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats,
			const vk::Format& format, const vk::ColorSpaceKHR& color_space);

		vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes,
			const vk::PresentModeKHR& present_mode);

		vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* const ptr_window);

		vk_interface::component::QueueFamilyIndices find_queue_families(
			const vk::PhysicalDevice& physical_device, const vk::UniqueSurfaceKHR& vk_surface
		);

		vk_interface::component::SwapChainSupportDetails query_swap_chain_support(
			const vk::PhysicalDevice& physical_device, const vk::UniqueSurfaceKHR& vk_surface
		);

		vk::Format find_supported_format(
			const vk::PhysicalDevice& physical_device,
			const std::vector<vk::Format>& candidates,
			const vk::ImageTiling& tilling, const vk::FormatFeatureFlags& features);

		vk::Format find_depth_format(const vk::PhysicalDevice& physical_device);

		uint32_t find_memory_type(
			const vk::PhysicalDevice& physical_device,
			const uint32_t& memory_type_filter,
			const vk::MemoryPropertyFlags& memory_prop_flags);

		vk::SampleCountFlagBits get_multi_sample_count(const vk::PhysicalDevice& physical_device);
	};

	namespace simple_create_info
	{
#ifdef _DEBUG
		vk::DebugUtilsMessageSeverityFlagsEXT get_severity_flags();

		vk::DebugUtilsMessageTypeFlagsEXT get_message_type_flags();
#endif

		vk::ApplicationInfo get_application_info(const std::string& window_title,
			const std::string& engine_name = "No Engine");

		vk::ImageViewCreateInfo get_swap_chain_image_view_info();

		vk::ImageViewCreateInfo get_swap_chain_depth_image_view_info(const vk::Format& format);

		vk::ImageViewCreateInfo get_swap_chain_color_image_view_info(const vk::Format& format);

		std::vector<vk::AttachmentDescription> get_renderpass_attachment_descriptions(
			const vk::SampleCountFlagBits& sample_count, const vk::Format& color_format, const vk::Format& depth_format);

		vk::SubpassDependency get_renderpass_dependency();

		vk::BufferCreateInfo get_gpu_buffer_info(const std::shared_ptr<vk_interface::Instance>& gpu_instance,
			const size_t& buffer_size, const vk::BufferUsageFlags& usage_flags);

		vk::ImageCreateInfo get_texture_image_info(
			const std::shared_ptr<vk_interface::Instance>& gpu_instance, const vk::Extent2D& extent);

		vk::ImageViewCreateInfo get_texture_image_view_info(const vk::UniqueImage& image);

		vk::SamplerCreateInfo get_texture_sampler_info(
			const std::shared_ptr<vk_interface::Instance>& gpu_instance);
	};
};