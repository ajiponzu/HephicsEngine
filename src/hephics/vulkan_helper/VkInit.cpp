#include "../HephicsHelper.hpp"

/* static definition */
#ifdef _DEBUG
static const std::vector<const char*> g_validation_layers =
{
	"VK_LAYER_KHRONOS_validation"
};
#endif

static const std::vector<const char*> g_device_extensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef _DEBUG
std::vector<const char*> hephics_helper::vk_init::get_validation_layers()
{
	return g_validation_layers;
}

bool hephics_helper::vk_init::check_validation_layer_support()
{
	const auto& available_layers = vk::enumerateInstanceLayerProperties();

	for (const auto& layer_name : g_validation_layers)
	{
		bool layer_found = false;
		for (const auto& layer_properties : available_layers)
		{
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}
		if (!layer_found)
			return false;
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL hephics_helper::vk_init::debug_utils_messenger_callback(::VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, ::VkDebugUtilsMessageTypeFlagsEXT message_types, ::VkDebugUtilsMessengerCallbackDataEXT const* p_callback_data, void* p_user_data)
{
	std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;

	return VK_FALSE;
}
#endif

std::vector<const char*> hephics_helper::vk_init::get_device_extensions()
{
	return g_device_extensions;
}

std::vector<const char*> hephics_helper::vk_init::get_required_extensions()
{
	uint32_t glfw_extension_count = 0;
	const auto glfw_extensions = ::glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#ifdef _DEBUG
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	return extensions;
}

bool hephics_helper::vk_init::check_device_extension_support(const vk::PhysicalDevice& physical_device)
{
	const auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
	std::set<std::string> required_extensions(g_device_extensions.begin(), g_device_extensions.end());

	for (const auto& extension : available_extensions)
		required_extensions.erase(extension.extensionName);

#ifdef _DEBUG
	std::cout << "check_device: " << required_extensions.empty() << std::endl;
#endif

	return required_extensions.empty();
}

vk::SurfaceFormatKHR hephics_helper::vk_init::choose_swap_surface_format(
	const std::vector<vk::SurfaceFormatKHR>& available_formats, const vk::Format& format, const vk::ColorSpaceKHR& color_space)
{
	for (const auto& available_format : available_formats)
	{
		if (available_format.format == format && available_format.colorSpace == color_space)
			return available_format;
	}

	return available_formats.at(0);
}

vk::PresentModeKHR hephics_helper::vk_init::choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes,
	const vk::PresentModeKHR& present_mode)
{
	for (const auto& available_present_mode : available_present_modes)
	{
		if (available_present_mode == present_mode)
			return available_present_mode;
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D hephics_helper::vk_init::choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities,
	GLFWwindow* const ptr_window)
{
	int32_t width, height;
	::glfwGetFramebufferSize(ptr_window, &width, &height);

	vk::Extent2D actual_extent =
	{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
	};

	actual_extent.width = std::clamp(actual_extent.width,
		capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actual_extent.height = std::clamp(actual_extent.height,
		capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actual_extent;
}

vk_interface::component::QueueFamilyIndices hephics_helper::vk_init::find_queue_families(
	const vk::PhysicalDevice& physical_device, const vk::UniqueSurfaceKHR& vk_surface)
{
	vk_interface::component::QueueFamilyIndices indices;

	const auto queue_families = physical_device.getQueueFamilyProperties();

	uint32_t family_id = 0;
	for (const auto& queue_family : queue_families)
	{
		if ((queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
			&& (queue_family.queueFlags & vk::QueueFlagBits::eCompute))
			indices.graphics_and_compute_family = family_id;

		const auto present_support = physical_device.getSurfaceSupportKHR(family_id, vk_surface.get());
		if (present_support)
			indices.present_family = family_id;

		if (indices.is_complete())
			break;

		family_id++;
	}
	indices.family_array.at(0) = indices.graphics_and_compute_family.value();
	indices.family_array.at(1) = indices.present_family.value();

	return indices;
}

vk_interface::component::SwapChainSupportDetails hephics_helper::vk_init::query_swap_chain_support(
	const vk::PhysicalDevice& physical_device, const vk::UniqueSurfaceKHR& vk_surface)
{
	vk_interface::component::SwapChainSupportDetails details;
	details.capabilities = physical_device.getSurfaceCapabilitiesKHR(vk_surface.get());
	details.formats = physical_device.getSurfaceFormatsKHR(vk_surface.get());
	details.present_modes = physical_device.getSurfacePresentModesKHR(vk_surface.get());

	return details;
}

vk::Format hephics_helper::vk_init::find_supported_format(const vk::PhysicalDevice& physical_device,
	const std::vector<vk::Format>& candidates, const vk::ImageTiling& tilling, const vk::FormatFeatureFlags& features)
{
	for (const auto& format : candidates)
	{
		const auto props = physical_device.getFormatProperties(format);

		if (tilling == vk::ImageTiling::eLinear &&
			(props.linearTilingFeatures & features) == features)
			return format;
		else if (tilling == vk::ImageTiling::eOptimal &&
			(props.optimalTilingFeatures & features) == features)
			return format;
	}

	throw std::runtime_error("failed to find supported format!");
}

vk::Format hephics_helper::vk_init::find_depth_format(const vk::PhysicalDevice& physical_device)
{
	return find_supported_format(physical_device,
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

uint32_t hephics_helper::vk_init::find_memory_type(const vk::PhysicalDevice& physical_device,
	const uint32_t& memory_type_filter, const vk::MemoryPropertyFlags& memory_prop_flags)
{
	const auto memory_props = physical_device.getMemoryProperties();
	for (uint32_t memory_type_id = 0; memory_type_id < memory_props.memoryTypeCount; memory_type_id++)
	{
		if ((memory_type_filter & (1 << memory_type_id)) &&
			(memory_props.memoryTypes.at(memory_type_id).propertyFlags & memory_prop_flags)
			== memory_prop_flags)
			return memory_type_id;
	}

	return 0;
}

vk::SampleCountFlagBits hephics_helper::vk_init::get_multi_sample_count(const vk::PhysicalDevice& physical_device)
{
	const auto physical_device_props = physical_device.getProperties();
	const auto sample_count =
		physical_device_props.limits.framebufferColorSampleCounts & physical_device_props.limits.framebufferDepthSampleCounts;

	if (sample_count & vk::SampleCountFlagBits::e64)
		return vk::SampleCountFlagBits::e64;
	if (sample_count & vk::SampleCountFlagBits::e32)
		return vk::SampleCountFlagBits::e32;
	if (sample_count & vk::SampleCountFlagBits::e16)
		return vk::SampleCountFlagBits::e16;
	if (sample_count & vk::SampleCountFlagBits::e8)
		return vk::SampleCountFlagBits::e8;
	if (sample_count & vk::SampleCountFlagBits::e4)
		return vk::SampleCountFlagBits::e4;
	if (sample_count & vk::SampleCountFlagBits::e2)
		return vk::SampleCountFlagBits::e2;

	return vk::SampleCountFlagBits::e1;
}