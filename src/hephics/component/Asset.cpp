#include "../Hephics.hpp"

template<> struct std::hash<hephics::asset::VertexData>
{
	size_t operator()(hephics::asset::VertexData const& vertex) const
	{
		const auto&& ret = ((std::hash<glm::vec3>()(vertex.pos) ^
			(std::hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
			(std::hash<glm::vec2>()(vertex.tex_coord) << 1);
		return ret;
	}
};

std::unordered_map<std::string, std::unordered_map<std::string, hephics::asset::AssetVariant>>
hephics::asset::AssetManager::s_assetDictionaries;

void hephics::asset::Texture::GenerateMipmaps(const std::shared_ptr<VkInstance>& gpu_instance, uint32_t width, uint32_t height)
{
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer("copy")->GetCommandBuffer();

	vk::Image image = m_ptrImage->GetImage().get();

	vk::ImageMemoryBarrier barrier{
		vk::AccessFlagBits::eNone, vk::AccessFlagBits::eNone,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined, 0, 0, image,
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	};

	int32_t mip_width = width;
	int32_t mip_height = height;

	for (uint32_t i = 1; i < m_miplevel; i++)
	{
		barrier.subresourceRange.setBaseMipLevel(i - 1);
		barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
		barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

		command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags(), nullptr, nullptr, barrier);

		vk::ImageBlit blit{
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1),
			{ vk::Offset3D(0, 0, 0), vk::Offset3D(mip_width, mip_height, 1)},
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1),
			{ vk::Offset3D(0, 0, 0), vk::Offset3D(std::max(1, mip_width / 2), std::max(1, mip_height / 2), 1)}
		};

		command_buffer->blitImage(
			image, vk::ImageLayout::eTransferSrcOptimal,
			image, vk::ImageLayout::eTransferDstOptimal,
			blit, vk::Filter::eLinear
		);

		barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
		barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags(), nullptr, nullptr, barrier);

		if (mip_width > 1)
			mip_width /= 2;
		if (mip_height > 1)
			mip_height /= 2;
	}

	barrier.subresourceRange.setBaseMipLevel(m_miplevel - 1);
	barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
	barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

	command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlags(), nullptr, nullptr, barrier);
}

hephics::asset::Texture::Texture(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& path, const std::string& cv_mat_key)
{
	hephics::asset::AssetManager::RegistCvMat(path, cv_mat_key);
	const auto& cv_mat = hephics::asset::AssetManager::GetCvMat(cv_mat_key);
	const auto& cv_mat_size = cv_mat->size();
	m_miplevel = static_cast<uint32_t>(std::floor(std::log2(std::max(cv_mat_size.width, cv_mat_size.height)))) + 1U;

	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& queue_family_array = gpu_instance->GetQueueFamilyIndices().get_families_array();

	m_ptrImage = std::make_shared<vk_interface::component::Image>();
	auto image_create_info = hephics_helper::simple_create_info::get_texture_image_info(
		gpu_instance,
		vk::Extent2D{
			static_cast<uint32_t>(cv_mat_size.width),
			static_cast<uint32_t>(cv_mat_size.height)
		});
	image_create_info.setMipLevels(m_miplevel);
	m_ptrImage->SetImage(logical_device, image_create_info);

	const auto memory_requirements = logical_device->getImageMemoryRequirements(m_ptrImage->GetImage().get());
	const auto memory_type_idx =
		gpu_instance->FindMemoryType(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	vk::MemoryAllocateInfo alloc_info(memory_requirements.size, memory_type_idx);
	m_ptrImage->SetMemory(logical_device, alloc_info);

	m_ptrImage->BindMemory(logical_device);

	auto view_create_info = hephics_helper::simple_create_info::get_texture_image_view_info(m_ptrImage->GetImage());
	view_create_info.subresourceRange.setLevelCount(m_miplevel);
	m_ptrImage->SetImageView(logical_device, view_create_info);

	SetSampler(logical_device,
		hephics_helper::simple_create_info::get_texture_sampler_info(gpu_instance));
}

hephics::asset::Texture::Texture(const std::shared_ptr<VkInstance>& gpu_instance, const std::shared_ptr<cv::Mat>& cv_mat)
{
	const auto& cv_mat_size = cv_mat->size();
	m_miplevel = static_cast<uint32_t>(std::floor(std::log2(std::max(cv_mat_size.width, cv_mat_size.height)))) + 1U;

	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& queue_family_array = gpu_instance->GetQueueFamilyIndices().get_families_array();

	m_ptrImage = std::make_shared<vk_interface::component::Image>();
	auto image_create_info = hephics_helper::simple_create_info::get_texture_image_info(
		gpu_instance,
		vk::Extent2D{
			static_cast<uint32_t>(cv_mat_size.width),
			static_cast<uint32_t>(cv_mat_size.height)
		});
	image_create_info.setMipLevels(m_miplevel);
	m_ptrImage->SetImage(logical_device, image_create_info);

	const auto memory_requirements = logical_device->getImageMemoryRequirements(m_ptrImage->GetImage().get());
	const auto memory_type_idx =
		gpu_instance->FindMemoryType(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	vk::MemoryAllocateInfo alloc_info(memory_requirements.size, memory_type_idx);
	m_ptrImage->SetMemory(logical_device, alloc_info);

	m_ptrImage->BindMemory(logical_device);

	auto view_create_info = hephics_helper::simple_create_info::get_texture_image_view_info(m_ptrImage->GetImage());
	view_create_info.subresourceRange.setLevelCount(m_miplevel);
	m_ptrImage->SetImageView(logical_device, view_create_info);

	SetSampler(logical_device,
		hephics_helper::simple_create_info::get_texture_sampler_info(gpu_instance));
}

void hephics::asset::Texture::SetSampler(const vk::UniqueDevice& logical_device,
	const vk::SamplerCreateInfo& create_info)
{
	vk::SamplerCreateInfo new_create_info = create_info;
	if (m_miplevel > 1)
	{
		new_create_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
		//new_create_info.setMinLod(static_cast<float_t>(m_miplevel / 2));
		new_create_info.setMinLod(0.0f);
		new_create_info.setMaxLod(static_cast<float_t>(m_miplevel)); // miplevel higher => lower resolution image
		new_create_info.setMipLodBias(0.0f);
	}
	m_sampler = logical_device->createSamplerUnique(new_create_info);
}

void hephics::asset::Texture::CopyTexture(const std::shared_ptr<VkInstance>& gpu_instance, const std::shared_ptr<cv::Mat>& cv_mat)
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer("copy");

	const auto cv_mat_size = cv_mat->size();
	const auto buffer_size = cv_mat->total() * cv_mat->elemSize();

	auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, buffer_size);
	auto staging_map_address = staging_buffer->Mapping(logical_device);
	std::memcpy(staging_map_address, cv_mat->data, buffer_size);
	staging_buffer->Unmapping(logical_device);

	command_buffer->TransitionImageCommandLayout(m_ptrImage, vk::Format::eR8G8B8A8Srgb,
		{ vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal }, m_miplevel);
	command_buffer->CopyTexture(staging_buffer, m_ptrImage,
		vk::Extent2D{ static_cast<uint32_t>(cv_mat_size.width), static_cast<uint32_t>(cv_mat_size.height) });
	if (m_miplevel > 1)
		GenerateMipmaps(gpu_instance, cv_mat_size.width, cv_mat_size.height);
	else
		command_buffer->TransitionImageCommandLayout(m_ptrImage, vk::Format::eR8G8B8A8Srgb,
			{ vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal }, m_miplevel);

	auto& staging_buffers = hephics::Scene::GetStagingBuffers();
	staging_buffers.emplace_back(std::move(staging_buffer));
}

void hephics::asset::Asset3D::CopyVertexBuffer(const std::shared_ptr<VkInstance>& gpu_instance) const
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer("copy");

	const auto& buffer_size = m_ptrVertexBuffer->GetSize();
	auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, buffer_size);
	auto staging_map_address = staging_buffer->Mapping(logical_device);
	std::memcpy(staging_map_address, m_vertices.data(), buffer_size);
	staging_buffer->Unmapping(logical_device);

	command_buffer->CopyBuffer(staging_buffer, m_ptrVertexBuffer, buffer_size);

	auto& staging_buffers = hephics::Scene::GetStagingBuffers();
	staging_buffers.emplace_back(std::move(staging_buffer));
}

void hephics::asset::Asset3D::CopyIndexBuffer(const std::shared_ptr<VkInstance>& gpu_instance) const
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer("copy");

	const auto& buffer_size = m_ptrIndexBuffer->GetSize();
	auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, buffer_size);
	auto staging_map_address = staging_buffer->Mapping(logical_device);
	std::memcpy(staging_map_address, m_indices.data(), buffer_size);
	staging_buffer->Unmapping(logical_device);

	command_buffer->CopyBuffer(staging_buffer, m_ptrIndexBuffer, buffer_size);

	auto& staging_buffers = hephics::Scene::GetStagingBuffers();
	staging_buffers.emplace_back(std::move(staging_buffer));
}

hephics::asset::Texture3D::Texture3D(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices)
{
	m_vertices = vertices;
	m_indices = indices;
	const auto vertex_size = sizeof(VertexData) * m_vertices.size();
	const auto index_size = sizeof(uint32_t) * m_indices.size();

	m_ptrVertexBuffer =
		std::make_shared<hephics_helper::GPUBuffer>(gpu_instance,
			vertex_size, vk::BufferUsageFlagBits::eVertexBuffer);
	m_ptrIndexBuffer =
		std::make_shared<hephics_helper::GPUBuffer>(gpu_instance,
			index_size, vk::BufferUsageFlagBits::eIndexBuffer);
}

hephics::asset::Object3D::Object3D(const std::shared_ptr<VkInstance>& gpu_instance, const std::string& path)
{
	m_ptrAttribute = std::make_shared<tinyobj::attrib_t>();

	std::string warn, err;
	if (!tinyobj::LoadObj(m_ptrAttribute.get(), &m_shapes, &m_materials, &warn, &err, path.c_str()))
		throw std::runtime_error(warn + err);

	std::unordered_map<VertexData, uint32_t> unique_vertices{};

	for (const auto& shape : m_shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			VertexData vertex{};

			vertex.pos =
			{
				m_ptrAttribute->vertices.at(3 * index.vertex_index + 0),
				m_ptrAttribute->vertices.at(3 * index.vertex_index + 1),
				m_ptrAttribute->vertices.at(3 * index.vertex_index + 2)
			};

			vertex.tex_coord =
			{
				m_ptrAttribute->texcoords.at(2 * index.texcoord_index + 0),
				1.0f - m_ptrAttribute->texcoords.at(2 * index.texcoord_index + 1)
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (unique_vertices.count(vertex) == 0)
			{
				unique_vertices.emplace(vertex, static_cast<uint32_t>(m_vertices.size()));
				m_vertices.emplace_back(vertex);
			}
			m_indices.emplace_back(unique_vertices.at(vertex));
		}
	}

	const auto vertex_size = sizeof(VertexData) * m_vertices.size();
	const auto index_size = sizeof(uint32_t) * m_indices.size();

	m_ptrVertexBuffer =
		std::make_shared<hephics_helper::GPUBuffer>(gpu_instance,
			vertex_size, vk::BufferUsageFlagBits::eVertexBuffer);
	m_ptrIndexBuffer =
		std::make_shared<hephics_helper::GPUBuffer>(gpu_instance,
			index_size, vk::BufferUsageFlagBits::eIndexBuffer);
}

void hephics::asset::AssetManager::RegistCvMat(const std::string& asset_path, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("cv_mat"))
		s_assetDictionaries["cv_mat"] = {};

	if (s_assetDictionaries.at("cv_mat").contains(asset_key))
		return;

	auto img = cv::imread(std::format("assets/img/{}", asset_path));
	cv::cvtColor(img, img, cv::COLOR_BGR2RGBA);
	s_assetDictionaries.at("cv_mat").emplace(asset_key, std::make_shared<cv::Mat>(img));
}

void hephics::asset::AssetManager::RegistCvMat(const std::string& asset_key, const cv::Mat& cv_mat)
{
	if (!s_assetDictionaries.contains("cv_mat"))
		s_assetDictionaries["cv_mat"] = {};

	if (s_assetDictionaries.at("cv_mat").contains(asset_key))
		return;

	s_assetDictionaries.at("cv_mat").emplace(asset_key, std::make_shared<cv::Mat>(cv_mat));
}

void hephics::asset::AssetManager::RegistObject3D(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& asset_path, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("object_3d"))
		s_assetDictionaries["object_3d"] = {};

	if (s_assetDictionaries.at("object_3d").contains(asset_key))
		return;

	s_assetDictionaries.at("object_3d").emplace(asset_key,
		std::make_shared<Object3D>(gpu_instance, std::format("assets/model/{}", asset_path)));
}

void hephics::asset::AssetManager::RegistFbx3D(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& asset_path, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("fbx_3d"))
		s_assetDictionaries["fbx_3d"] = {};

	if (s_assetDictionaries.at("fbx_3d").contains(asset_key))
		return;

	s_assetDictionaries.at("fbx_3d").emplace(asset_key,
		std::make_shared<Fbx3D>(gpu_instance, std::format("assets/model/{}", asset_path)));
}

void hephics::asset::AssetManager::RegistTexture(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& asset_path, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("texture"))
		s_assetDictionaries["texture"] = {};

	if (s_assetDictionaries.at("texture").contains(asset_key))
		return;

	s_assetDictionaries.at("texture").emplace(asset_key, std::make_shared<Texture>(gpu_instance, asset_path, asset_key));
}

void hephics::asset::AssetManager::RegistTexture(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& asset_key, const cv::Mat& cv_mat)
{
	if (!s_assetDictionaries.contains("texture"))
		s_assetDictionaries["texture"] = {};

	if (s_assetDictionaries.at("texture").contains(asset_key))
		return;

	RegistCvMat(asset_key, cv_mat);
	s_assetDictionaries.at("texture").emplace(asset_key, std::make_shared<Texture>(gpu_instance, std::make_shared<cv::Mat>(cv_mat)));
}

void hephics::asset::AssetManager::RegistTexture3D(const std::shared_ptr<VkInstance>& gpu_instance,
	const Texture3D& texture_3d, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("texture_3d"))
		s_assetDictionaries["texture_3d"] = {};

	if (s_assetDictionaries.at("texture_3d").contains(asset_key))
		return;

	s_assetDictionaries.at("texture_3d").emplace(asset_key, std::make_shared<Texture3D>(texture_3d));
}

const std::shared_ptr<cv::Mat>& hephics::asset::AssetManager::GetCvMat(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("cv_mat"))
		throw std::runtime_error("cv_mat: not found");

	const auto& asset_dictionary = s_assetDictionaries.at("cv_mat");
	if (!asset_dictionary.contains(asset_key))
		throw std::runtime_error("cv_mat: not found");

	return std::get<std::shared_ptr<cv::Mat>>(asset_dictionary.at(asset_key));
}

const std::shared_ptr<hephics::asset::Texture>&
hephics::asset::AssetManager::GetTexture(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("texture"))
		throw std::runtime_error("texture: not found");

	const auto& asset_dictionary = s_assetDictionaries.at("texture");
	if (!asset_dictionary.contains(asset_key))
		throw std::runtime_error("texture: not found");

	return std::get<std::shared_ptr<Texture>>(asset_dictionary.at(asset_key));
}

const std::shared_ptr<hephics::asset::Texture3D>& hephics::asset::AssetManager::GetTexture3D(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("texture_3d"))
		throw std::runtime_error("texture_3d: not found");

	const auto& asset_dictionary = s_assetDictionaries.at("texture_3d");
	if (!asset_dictionary.contains(asset_key))
		throw std::runtime_error("texture_3d: not found");

	return std::get<std::shared_ptr<Texture3D>>(asset_dictionary.at(asset_key));
}

const std::shared_ptr<hephics::asset::Object3D>&
hephics::asset::AssetManager::GetObject3D(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("object_3d"))
		throw std::runtime_error("object_3d: not found");

	const auto& asset_dictionary = s_assetDictionaries.at("object_3d");
	if (!asset_dictionary.contains(asset_key))
		throw std::runtime_error("object_3d: not found");

	return std::get<std::shared_ptr<Object3D>>(asset_dictionary.at(asset_key));
}

const std::shared_ptr<hephics::asset::Fbx3D>& hephics::asset::AssetManager::GetFbx3D(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("fbx_3d"))
		throw std::runtime_error("object_3d: not found");

	const auto& asset_dictionary = s_assetDictionaries.at("fbx_3d");
	if (!asset_dictionary.contains(asset_key))
		throw std::runtime_error("object_3d: not found");

	return std::get<std::shared_ptr<Fbx3D>>(asset_dictionary.at(asset_key));
}