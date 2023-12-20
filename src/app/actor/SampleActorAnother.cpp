#include "../SampleApp.hpp"

void SampleActorAnother::LoadData(std::shared_ptr<hephics::VkInstance>& gpu_instance, const size_t& command_buffer_idx)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	auto& ref_descriptor_set = m_ptrShaderAttachment->GetDescriptorSet();

	hephics::asset::AssetManager::RegistTexture(gpu_instance, "sample_2d.png", "lenna");

	static const auto vertices = std::vector<hephics::asset::VertexData>{
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	static const std::vector<uint32_t> indices = {
		0, 1, 2, 2, 3, 0,

		4, 5, 6, 6, 7, 4
	};

	const hephics::asset::Texture3D texture_3d = hephics::asset::Texture3D(gpu_instance, vertices, indices);
	hephics::asset::AssetManager::RegistTexture3D(gpu_instance, texture_3d, "lenna");

	vk::DescriptorSetLayoutBinding vertex_uniform_layout_binding(2, vk::DescriptorType::eUniformBuffer,
		1, vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutBinding fragment_sampler_layout_binding(3, vk::DescriptorType::eCombinedImageSampler,
		1, vk::ShaderStageFlagBits::eFragment, nullptr);
	auto desc_layout_bindings = std::vector{ vertex_uniform_layout_binding, fragment_sampler_layout_binding };
	ref_descriptor_set->SetDescriptorSetLayout(logical_device, desc_layout_bindings);

	vk::DescriptorPoolSize uniform_desc_pool_size(vk::DescriptorType::eUniformBuffer, hephics::BUFFERING_FRAME_NUM);
	vk::DescriptorPoolSize  sampler_desc_pool_size(vk::DescriptorType::eCombinedImageSampler, hephics::BUFFERING_FRAME_NUM);
	auto desc_pool_size_list = std::vector{ uniform_desc_pool_size, sampler_desc_pool_size };
	vk::DescriptorPoolCreateInfo desc_pool_create_info(
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, hephics::BUFFERING_FRAME_NUM, desc_pool_size_list);
	ref_descriptor_set->SetDescriptorPool(logical_device, desc_pool_create_info);

	ref_descriptor_set->SetDescriptorSet(logical_device, hephics::BUFFERING_FRAME_NUM);

	const auto position_uniform_buffer_size = sizeof(decltype(*m_ptrPosition));
	auto& uniform_buffers_map = m_ptrShaderAttachment->GetUniformBuffersMap();
	uniform_buffers_map["position"] = {};
	for (size_t idx = 0; idx < hephics::BUFFERING_FRAME_NUM; idx++)
	{
		auto& uniform_buffer = uniform_buffers_map.at("position").at(idx);
		uniform_buffer.reset(new hephics_helper::UniformBuffer(gpu_instance, position_uniform_buffer_size));
	}

	for (size_t idx = 0; idx < hephics::BUFFERING_FRAME_NUM; idx++)
	{
		const auto& uniform_buffers = uniform_buffers_map.at("position");
		vk::DescriptorBufferInfo buffer_info(
			uniform_buffers.at(idx)->GetBuffer().get(), 0, position_uniform_buffer_size);

		const auto& texture_option = hephics::asset::AssetManager::GetTexture("lenna");
		if (!texture_option.has_value())
			throw std::runtime_error("texture: not found");

		const auto& ptr_texture = texture_option.value();
		vk::DescriptorImageInfo image_info(ptr_texture->GetSampler().get(),
			ptr_texture->GetImage()->GetView().get(), vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::WriteDescriptorSet buffer_write_desc_set({}, 2, 0, vk::DescriptorType::eUniformBuffer, nullptr, buffer_info, nullptr);
		vk::WriteDescriptorSet image_write_desc_set({}, 3, 0, vk::DescriptorType::eCombinedImageSampler, image_info, nullptr, nullptr);
		auto write_descriptor_sets = std::vector{ buffer_write_desc_set, image_write_desc_set };
		ref_descriptor_set->UpdateDescriptorSet(logical_device, idx, std::move(write_descriptor_sets));
	}
}

void SampleActorAnother::SetPipeline(std::shared_ptr<hephics::VkInstance>& gpu_instance, const size_t& command_buffer_idx)
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& render_pass = gpu_instance->GetSwapChain()->GetRenderPass();
	auto& ref_descriptor_set = m_ptrShaderAttachment->GetDescriptorSet();
	auto& ref_graphic_pipeline = m_ptrShaderAttachment->GetGraphicPipeline();

	vk_interface::component::ShaderProvider::AddShader(logical_device, "vert/sample_shader.vert", "lenna");
	vk_interface::component::ShaderProvider::AddShader(logical_device, "frag/sample_shader.frag", "lenna");

	const auto vert_shader_option = vk_interface::component::ShaderProvider::GetShader("vert", "lenna");
	if (!vert_shader_option.has_value())
		throw std::runtime_error("vertex_shader: not found");

	const auto frag_shader_option = vk_interface::component::ShaderProvider::GetShader("frag", "lenna");
	if (!frag_shader_option.has_value())
		throw std::runtime_error("fragment_shader: not found");

	const auto& vert_shader_module = vert_shader_option.value();
	const auto& frag_shader_module = frag_shader_option.value();

	vk::PipelineShaderStageCreateInfo vert_shader_stage_info({}, vk::ShaderStageFlagBits::eVertex,
		vert_shader_module->GetModule().get(), "main");
	vk::PipelineShaderStageCreateInfo frag_shader_stage_info({}, vk::ShaderStageFlagBits::eFragment,
		frag_shader_module->GetModule().get(), "main");

	const auto shader_stages = { vert_shader_stage_info, frag_shader_stage_info };

	auto vertex_binding_descs = std::vector{ hephics::asset::VertexData::get_binding_description() };
	auto vertex_attribute_descs = hephics::asset::VertexData::get_attribute_descriptions();
	vk::PipelineVertexInputStateCreateInfo vertex_input_info({}, vertex_binding_descs, vertex_attribute_descs);

	vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, {}, 1, {});

	vk::PipelineRasterizationStateCreateInfo rasterizer({}, VK_FALSE, VK_FALSE,
		vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise,
		VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);

	vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, VK_FALSE);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil({}, VK_TRUE, VK_TRUE,
		vk::CompareOp::eLess, VK_FALSE, VK_FALSE);

	vk::PipelineColorBlendAttachmentState color_blend_attachment(VK_FALSE);
	color_blend_attachment.setColorWriteMask(
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendStateCreateInfo color_blending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment);

	std::vector<vk::DynamicState> dynamic_states =
	{
		vk::DynamicState::eScissor, vk::DynamicState::eViewport
	};
	vk::PipelineDynamicStateCreateInfo dynamic_state_info({}, dynamic_states);

	vk::PipelineLayoutCreateInfo pipeline_layout_info({}, ref_descriptor_set->GetDescriptorSetLayout().get());
	ref_graphic_pipeline->SetLayout(logical_device, pipeline_layout_info);

	vk::GraphicsPipelineCreateInfo pipeline_info({}, shader_stages, &vertex_input_info, &input_assembly, {},
		&viewport_state, &rasterizer, &multisampling, &depth_stencil, &color_blending, &dynamic_state_info,
		ref_graphic_pipeline->GetLayout().get(), render_pass.get());
	ref_graphic_pipeline->SetPipeline(logical_device, pipeline_info);
}

void SampleActorAnother::Initialize(std::shared_ptr<hephics::VkInstance>& gpu_instance, const size_t& command_buffer_idx)
{
	LoadData(gpu_instance, command_buffer_idx);
	SetPipeline(gpu_instance, command_buffer_idx);

	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer(command_buffer_idx);

	{
		const auto texture_option = hephics::asset::AssetManager::GetTexture("lenna");
		if (!texture_option.has_value())
			throw std::runtime_error("texture: not found");
		const auto& texture = texture_option.value();

		const auto cv_mat_option = hephics::asset::AssetManager::GetCvMat("lenna");
		if (!cv_mat_option.has_value())
			throw std::runtime_error("texture: not found");
		const auto& cv_mat = cv_mat_option.value();

		texture->CopyTexture(gpu_instance, cv_mat, command_buffer_idx);
	}

	{
		const auto texture_3d_option = hephics::asset::AssetManager::GetTexture3D("lenna");
		if (!texture_3d_option.has_value())
			throw std::runtime_error("texture_3d: not found");

		const auto& texture_3d = texture_3d_option.value();
		texture_3d->CopyVertexBuffer(gpu_instance, command_buffer_idx);
		texture_3d->CopyIndexBuffer(gpu_instance, command_buffer_idx);
	}

	for (auto& component : m_components)
		component->Initialize(gpu_instance, command_buffer_idx);
}

void SampleActorAnother::Update(std::shared_ptr<hephics::VkInstance>& gpu_instance, const size_t& command_buffer_idx)
{
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(
		current_time - start_time).count();

	/* firstly, component's update method */
	for (auto& component : m_components)
		component->Update(this, gpu_instance, command_buffer_idx);

	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();

	const auto& current_frame_id = swap_chain->GetCurrentFrameId();
	auto& uniform_buffer = m_ptrShaderAttachment->GetUniformBuffersMap().at("position").at(current_frame_id);
	auto uniform_address = uniform_buffer->Mapping(logical_device);

	m_ptrPosition->model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_ptrPosition->view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_ptrPosition->projection = glm::perspective(glm::radians(90.0f),
		swap_chain->GetExtent2D().width / static_cast<float_t>(swap_chain->GetExtent2D().height), 0.1f, 5.0f);
	m_ptrPosition->projection[1][1] *= -1;

	std::memcpy(uniform_address, m_ptrPosition.get(), sizeof(decltype(*m_ptrPosition)));
	uniform_buffer->Unmapping(logical_device);
}

void SampleActorAnother::Render(std::shared_ptr<hephics::VkInstance>& gpu_instance, const size_t& command_buffer_idx)
{
	/* firstly, actor's render method */
	auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	auto& command_buffer = gpu_instance->GetGraphicCommandBuffer(command_buffer_idx)->GetCommandBuffer();
	const auto& pipeline = m_ptrShaderAttachment->GetGraphicPipeline();
	const auto& desc_set =
		m_ptrShaderAttachment->GetDescriptorSet()->GetDescriptorSet(swap_chain->GetCurrentFrameId());

	const auto render_pass_info = swap_chain->GetRenderPassBeginInfo();

	const auto [viewport, scissor] = swap_chain->GetViewportAndScissor();

	const auto texture_3d_option = hephics::asset::AssetManager::GetTexture3D("lenna");
	if (!texture_3d_option.has_value())
		throw std::runtime_error("texture_3d: not found");

	const auto& texture_3d = texture_3d_option.value();

	command_buffer->beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
	command_buffer->setViewport(0, viewport);
	command_buffer->setScissor(0, scissor);
	command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->GetPipeline().get());
	command_buffer->bindVertexBuffers(0, { texture_3d->GetVertexBuffer()->GetBuffer().get() }, { 0 });
	command_buffer->bindIndexBuffer(texture_3d->GetIndexBuffer()->GetBuffer().get(), 0, vk::IndexType::eUint32);
	command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		pipeline->GetLayout().get(), 0, desc_set.get(), nullptr);
	command_buffer->drawIndexed(static_cast<uint32_t>(texture_3d->GetIndices().size()), 1, 0, 0, 0);
	command_buffer->endRenderPass();

	for (auto& component : m_components)
		component->Render(gpu_instance, command_buffer_idx);
}