#include "../SampleApp.hpp"

void SampleActor::LoadData(std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	auto& ref_descriptor_set = m_ptrShaderAttachment->GetDescriptorSet();

	hephics::asset::AssetManager::RegistTexture(gpu_instance, "sample_3d.png", "room");
	hephics::asset::AssetManager::RegistObject3D(gpu_instance, "sample_3d.obj", "room");

	vk::DescriptorSetLayoutBinding vertex_uniform_layout_binding(0, vk::DescriptorType::eUniformBuffer,
		1, vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutBinding fragment_sampler_layout_binding(1, vk::DescriptorType::eCombinedImageSampler,
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

		const auto& texture = hephics::asset::AssetManager::GetTexture("room");
		vk::DescriptorImageInfo image_info(texture->GetSampler().get(),
			texture->GetImage()->GetView().get(), vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::WriteDescriptorSet buffer_write_desc_set({}, 0, 0, vk::DescriptorType::eUniformBuffer, nullptr, buffer_info, nullptr);
		vk::WriteDescriptorSet image_write_desc_set({}, 1, 0, vk::DescriptorType::eCombinedImageSampler, image_info, nullptr, nullptr);
		auto write_descriptor_sets = std::vector{ buffer_write_desc_set, image_write_desc_set };
		ref_descriptor_set->UpdateDescriptorSet(logical_device, idx, std::move(write_descriptor_sets));
	}
}

void SampleActor::SetPipeline(std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& render_pass = gpu_instance->GetSwapChain()->GetRenderPass();
	auto& ref_descriptor_set = m_ptrShaderAttachment->GetDescriptorSet();
	auto& ref_graphic_pipeline = m_ptrShaderAttachment->GetGraphicPipeline();

	vk_interface::component::ShaderProvider::AddShader(logical_device, "vert/sample_shader_3d.vert", "room");
	vk_interface::component::ShaderProvider::AddShader(logical_device, "frag/sample_shader_3d.frag", "room");

	const auto& vert_shader_module = vk_interface::component::ShaderProvider::GetShader("vert", "room");
	const auto& frag_shader_module = vk_interface::component::ShaderProvider::GetShader("frag", "room");

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

void SampleActor::Initialize(std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	m_components.emplace_back(std::make_shared<MoveComponent>());

	LoadData(gpu_instance);
	SetPipeline(gpu_instance);

	const auto& logical_device = gpu_instance->GetLogicalDevice();

	{
		const auto& texture = hephics::asset::AssetManager::GetTexture("room");
		const auto& cv_mat = hephics::asset::AssetManager::GetCvMat("room");
		texture->CopyTexture(gpu_instance, cv_mat);
	}

	{
		const auto& object_3d = hephics::asset::AssetManager::GetObject3D("room");
		object_3d->CopyVertexBuffer(gpu_instance);
		object_3d->CopyIndexBuffer(gpu_instance);
	}

	for (auto& component : m_components)
		component->Initialize(gpu_instance);
}

void SampleActor::Update(std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(
		current_time - start_time).count();

	/* firstly, component's update method */
	for (auto& component : m_components)
		component->Update(this, gpu_instance);

	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();

	const auto& current_frame_id = swap_chain->GetCurrentFrameId();
	auto& uniform_buffer = m_ptrShaderAttachment->GetUniformBuffersMap().at("position").at(current_frame_id);
	auto uniform_address = uniform_buffer->Mapping(logical_device);

	m_ptrPosition->model = glm::rotate(glm::mat4(1.0), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_ptrPosition->view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_ptrPosition->projection = glm::perspective(glm::radians(45.0f),
		swap_chain->GetExtent2D().width / static_cast<float_t>(swap_chain->GetExtent2D().height), 0.1f, 10.0f);
	m_ptrPosition->projection[1][1] *= -1;

	std::memcpy(uniform_address, m_ptrPosition.get(), sizeof(decltype(*m_ptrPosition)));
	uniform_buffer->Unmapping(logical_device);
}

void SampleActor::Render(std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	/* firstly, actor's render method */
	auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	auto& render_command_buffer = gpu_instance->GetGraphicCommandBuffer("render")->GetCommandBuffer();
	const auto& pipeline = m_ptrShaderAttachment->GetGraphicPipeline();
	const auto& desc_set =
		m_ptrShaderAttachment->GetDescriptorSet()->GetDescriptorSet(swap_chain->GetCurrentFrameId());

	const auto& object_3d = hephics::asset::AssetManager::GetObject3D("room");

	render_command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->GetPipeline().get());
	render_command_buffer->bindVertexBuffers(0, { object_3d->GetVertexBuffer()->GetBuffer().get() }, { 0 });
	render_command_buffer->bindIndexBuffer(object_3d->GetIndexBuffer()->GetBuffer().get(), 0, vk::IndexType::eUint32);
	render_command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		pipeline->GetLayout().get(), 0, desc_set.get(), nullptr);
	render_command_buffer->drawIndexed(static_cast<uint32_t>(object_3d->GetIndices().size()), 1, 0, 0, 0);

	for (auto& component : m_components)
		component->Render(gpu_instance);
}