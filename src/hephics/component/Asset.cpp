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

hephics::asset::Texture::Texture(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& path, const std::string& cv_mat_key)
{
	hephics::asset::AssetManager::RegistCvMat(path, cv_mat_key);
	const auto cv_mat_option = hephics::asset::AssetManager::GetCvMat(cv_mat_key);
	if (!cv_mat_option.has_value())
		throw std::runtime_error("cv_mat: not found");

	const auto& cv_mat = cv_mat_option.value();

	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto queue_family_array = gpu_instance->FindQueueFamilies().get_families_array();

	m_ptrImage = std::make_shared<vk_interface::component::Image>();
	m_ptrImage->SetImage(logical_device,
		hephics_helper::simple_create_info::get_texture_image_info(
			gpu_instance,
			vk::Extent2D{
				static_cast<uint32_t>(cv_mat->size().width),
				static_cast<uint32_t>(cv_mat->size().height)
			}));

	const auto memory_requirements = logical_device->getImageMemoryRequirements(m_ptrImage->GetImage().get());
	const auto memory_type_idx =
		gpu_instance->FindMemoryType(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	vk::MemoryAllocateInfo alloc_info(memory_requirements.size, memory_type_idx);
	m_ptrImage->SetMemory(logical_device, alloc_info);

	m_ptrImage->BindMemory(logical_device);

	m_ptrImage->SetImageView(logical_device,
		hephics_helper::simple_create_info::get_texture_image_view_info(m_ptrImage->GetImage()));

	SetSampler(logical_device,
		hephics_helper::simple_create_info::get_texture_sampler_info(gpu_instance));
}

hephics::asset::Texture::Texture(const std::shared_ptr<VkInstance>& gpu_instance, const std::shared_ptr<cv::Mat>& cv_mat)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto queue_family_array = gpu_instance->FindQueueFamilies().get_families_array();

	auto ptr_texture = std::make_shared<hephics::asset::Texture>();
	m_ptrImage->SetImage(logical_device,
		hephics_helper::simple_create_info::get_texture_image_info(
			gpu_instance,
			vk::Extent2D{
				static_cast<uint32_t>(cv_mat->size().width),
				static_cast<uint32_t>(cv_mat->size().height)
			}));

	const auto memory_requirements = logical_device->getImageMemoryRequirements(m_ptrImage->GetImage().get());
	const auto memory_type_idx =
		gpu_instance->FindMemoryType(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	vk::MemoryAllocateInfo alloc_info(memory_requirements.size, memory_type_idx);
	m_ptrImage->SetMemory(logical_device, alloc_info);

	m_ptrImage->BindMemory(logical_device);

	m_ptrImage->SetImageView(logical_device,
		hephics_helper::simple_create_info::get_texture_image_view_info(m_ptrImage->GetImage()));

	SetSampler(logical_device,
		hephics_helper::simple_create_info::get_texture_sampler_info(gpu_instance));
}

void hephics::asset::Texture::SetSampler(const vk::UniqueDevice& logical_device,
	const vk::SamplerCreateInfo& create_info)
{
	m_sampler = logical_device->createSamplerUnique(create_info);
}

void hephics::asset::Texture::CopyTexture(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::shared_ptr<cv::Mat>& cv_mat, const size_t& command_buffer_idx)
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer(command_buffer_idx);

	const auto& cv_mat_size = cv_mat->size();
	const auto& buffer_size = cv_mat->total() * cv_mat->elemSize();

	auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, buffer_size);
	auto staging_map_address = staging_buffer->Mapping(logical_device);
	std::memcpy(staging_map_address, cv_mat->data, buffer_size);
	staging_buffer->Unmapping(logical_device);

	command_buffer->TransitionImageCommandLayout(GetImage(), vk::Format::eR8G8B8A8Srgb,
		{ vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal });
	command_buffer->CopyTexture(staging_buffer, GetImage(),
		vk::Extent2D{ static_cast<uint32_t>(cv_mat_size.width), static_cast<uint32_t>(cv_mat_size.height) });
	command_buffer->TransitionImageCommandLayout(GetImage(), vk::Format::eR8G8B8A8Srgb,
		{ vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal });

	auto& staging_buffers = hephics::Scene::GetStagingBuffers();
	staging_buffers.emplace_back(std::move(staging_buffer));
}

void hephics::asset::Asset3D::CopyVertexBuffer(
	const std::shared_ptr<VkInstance>& gpu_instance, const size_t& command_buffer_idx) const
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer(command_buffer_idx);

	const auto& buffer_size = GetVertexBuffer()->GetSize();
	auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, buffer_size);
	auto staging_map_address = staging_buffer->Mapping(logical_device);
	std::memcpy(staging_map_address, GetVertices().data(), buffer_size);
	staging_buffer->Unmapping(logical_device);

	command_buffer->CopyBuffer(staging_buffer, GetVertexBuffer(), buffer_size);

	auto& staging_buffers = hephics::Scene::GetStagingBuffers();
	staging_buffers.emplace_back(std::move(staging_buffer));
}

void hephics::asset::Asset3D::CopyIndexBuffer(
	const std::shared_ptr<VkInstance>& gpu_instance, const size_t& command_buffer_idx) const
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffers().at(command_buffer_idx);

	const auto& buffer_size = GetIndexBuffer()->GetSize();
	auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, buffer_size);
	auto staging_map_address = staging_buffer->Mapping(logical_device);
	std::memcpy(staging_map_address, GetIndices().data(), buffer_size);
	staging_buffer->Unmapping(logical_device);

	command_buffer->CopyBuffer(staging_buffer, GetIndexBuffer(), buffer_size);

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

void hephics::asset::AssetManager::RegistObject3D(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& asset_path, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("object_3d"))
		s_assetDictionaries["object_3d"] = {};

	if (s_assetDictionaries.at("object_3d").contains(asset_key))
		return;

	auto object_3d = Object3D(gpu_instance, std::format("assets/model/{}", asset_path));
	s_assetDictionaries.at("object_3d").emplace(asset_key, std::make_shared<Object3D>(object_3d));
}

void hephics::asset::AssetManager::RegistFbx3D(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& asset_path, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("fbx_3d"))
		s_assetDictionaries["fbx_3d"] = {};

	if (s_assetDictionaries.at("fbx_3d").contains(asset_key))
		return;

	auto fbx_3d = Fbx3D(gpu_instance, std::format("assets/model/{}", asset_path));
	s_assetDictionaries.at("fbx_3d").emplace(asset_key, std::make_shared<Fbx3D>(fbx_3d));
}

void hephics::asset::AssetManager::RegistTexture(const std::shared_ptr<VkInstance>& gpu_instance,
	const std::string& asset_path, const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("texture"))
		s_assetDictionaries["texture"] = {};

	if (s_assetDictionaries.at("texture").contains(asset_key))
		return;

	auto texture = Texture(gpu_instance, asset_path, asset_key);
	s_assetDictionaries.at("texture").emplace(asset_key, std::make_shared<Texture>(std::move(texture)));
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

const std::optional<std::shared_ptr<cv::Mat>> hephics::asset::AssetManager::GetCvMat(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("cv_mat"))
		return std::nullopt;

	const auto& asset_dictionary = s_assetDictionaries.at("cv_mat");
	if (!asset_dictionary.contains(asset_key))
		return std::nullopt;

	return std::get<std::shared_ptr<cv::Mat>>(asset_dictionary.at(asset_key));
}

const std::optional<std::shared_ptr<hephics::asset::Texture>>
hephics::asset::AssetManager::GetTexture(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("texture"))
		return std::nullopt;

	const auto& asset_dictionary = s_assetDictionaries.at("texture");
	if (!asset_dictionary.contains(asset_key))
		return std::nullopt;

	return std::get<std::shared_ptr<Texture>>(asset_dictionary.at(asset_key));
}

const std::optional<std::shared_ptr<hephics::asset::Texture3D>> hephics::asset::AssetManager::GetTexture3D(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("texture_3d"))
		return std::nullopt;

	const auto& asset_dictionary = s_assetDictionaries.at("texture_3d");
	if (!asset_dictionary.contains(asset_key))
		return std::nullopt;

	return std::get<std::shared_ptr<Texture3D>>(asset_dictionary.at(asset_key));
}

const std::optional<std::shared_ptr<hephics::asset::Object3D>>
hephics::asset::AssetManager::GetObject3D(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("object_3d"))
		return std::nullopt;

	const auto& asset_dictionary = s_assetDictionaries.at("object_3d");
	if (!asset_dictionary.contains(asset_key))
		return std::nullopt;

	return std::get<std::shared_ptr<Object3D>>(asset_dictionary.at(asset_key));
}

const std::optional<std::shared_ptr<hephics::asset::Fbx3D>> hephics::asset::AssetManager::GetFbx3D(const std::string& asset_key)
{
	if (!s_assetDictionaries.contains("fbx_3d"))
		return std::nullopt;

	const auto& asset_dictionary = s_assetDictionaries.at("fbx_3d");
	if (!asset_dictionary.contains(asset_key))
		return std::nullopt;

	return std::get<std::shared_ptr<Fbx3D>>(asset_dictionary.at(asset_key));
}