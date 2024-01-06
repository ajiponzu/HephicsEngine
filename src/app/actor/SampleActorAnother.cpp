#include "../SampleApp.hpp"

void SampleActorAnother::LoadData(const std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	const auto& physical_device = gpu_instance->GetPhysicalDevice();
	const auto& window_surface = gpu_instance->GetWindowSurface();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	auto& ref_descriptor_set = m_ptrShaderAttachment->GetDescriptorSet();

	auto lenna_image = cv::imread("assets/img/sample_2d.png");
	cv::cvtColor(lenna_image, lenna_image, cv::COLOR_BGR2RGBA);
	hephics::asset::AssetManager::RegistTexture(gpu_instance, "lenna", lenna_image);

	static const auto vertices = std::vector<hephics::asset::VertexData>{
		{{-0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	static const std::vector<uint32_t> indices = {
		0, 1, 2, 2, 3, 0,

		4, 5, 6, 6, 7, 4,
	};

	const hephics::asset::Texture3D texture_3d = hephics::asset::Texture3D(gpu_instance, vertices, indices);
	hephics::asset::AssetManager::RegistTexture3D(gpu_instance, texture_3d, "lenna");

	vk::DescriptorSetLayoutBinding vertex_uniform_layout_binding(2, vk::DescriptorType::eUniformBuffer,
		1, vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutBinding fragment_sampler_layout_binding(3, vk::DescriptorType::eCombinedImageSampler,
		1, vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutBinding fragment_timer_layout_binding(4, vk::DescriptorType::eUniformBuffer,
		1, vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutBinding fragment_mouse_layout_binding(5, vk::DescriptorType::eUniformBuffer,
		1, vk::ShaderStageFlagBits::eFragment, nullptr);
	auto desc_layout_bindings = std::vector
	{ vertex_uniform_layout_binding, fragment_sampler_layout_binding, fragment_timer_layout_binding, fragment_mouse_layout_binding };
	ref_descriptor_set->SetDescriptorSetLayout(logical_device, desc_layout_bindings);

	vk::DescriptorPoolSize uniform_desc_pool_size(vk::DescriptorType::eUniformBuffer, hephics::BUFFERING_FRAME_NUM);
	vk::DescriptorPoolSize  sampler_desc_pool_size(vk::DescriptorType::eCombinedImageSampler, hephics::BUFFERING_FRAME_NUM);
	vk::DescriptorPoolSize  timer_desc_pool_size(vk::DescriptorType::eUniformBuffer, hephics::BUFFERING_FRAME_NUM);
	vk::DescriptorPoolSize  mouse_desc_pool_size(vk::DescriptorType::eUniformBuffer, hephics::BUFFERING_FRAME_NUM);
	auto desc_pool_size_list = std::vector{ uniform_desc_pool_size, sampler_desc_pool_size, timer_desc_pool_size, mouse_desc_pool_size };
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

	const auto timer_uniform_buffer_size = sizeof(float_t);
	uniform_buffers_map["timer"] = {};
	for (size_t idx = 0; idx < hephics::BUFFERING_FRAME_NUM; idx++)
	{
		auto& uniform_buffer = uniform_buffers_map.at("timer").at(idx);
		uniform_buffer.reset(new hephics_helper::UniformBuffer(gpu_instance, timer_uniform_buffer_size));
	}

	const auto cursor_uniform_buffer_size = sizeof(glm::vec2);
	uniform_buffers_map["cursor"] = {};
	for (size_t idx = 0; idx < hephics::BUFFERING_FRAME_NUM; idx++)
	{
		auto& uniform_buffer = uniform_buffers_map.at("cursor").at(idx);
		uniform_buffer.reset(new hephics_helper::UniformBuffer(gpu_instance, cursor_uniform_buffer_size));
	}

	for (size_t idx = 0; idx < hephics::BUFFERING_FRAME_NUM; idx++)
	{
		const auto& position_uniform_buffers = uniform_buffers_map.at("position");
		vk::DescriptorBufferInfo position_buffer_info(
			position_uniform_buffers.at(idx)->GetBuffer().get(), 0, position_uniform_buffer_size);

		const auto& timer_uniform_buffers = uniform_buffers_map.at("timer");
		vk::DescriptorBufferInfo timer_buffer_info(
			timer_uniform_buffers.at(idx)->GetBuffer().get(), 0, timer_uniform_buffer_size);

		const auto& cursor_uniform_buffers = uniform_buffers_map.at("cursor");
		vk::DescriptorBufferInfo cursor_buffer_info(
			cursor_uniform_buffers.at(idx)->GetBuffer().get(), 0, cursor_uniform_buffer_size);

		const auto& texture = hephics::asset::AssetManager::GetTexture("lenna");
		vk::DescriptorImageInfo image_info(texture->GetSampler().get(),
			texture->GetImage()->GetView().get(), vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::WriteDescriptorSet position_buffer_write_desc_set({}, 2, 0, vk::DescriptorType::eUniformBuffer, nullptr, position_buffer_info, nullptr);
		vk::WriteDescriptorSet image_write_desc_set({}, 3, 0, vk::DescriptorType::eCombinedImageSampler, image_info, nullptr, nullptr);
		vk::WriteDescriptorSet timer_buffer_write_desc_set({}, 4, 0, vk::DescriptorType::eUniformBuffer, nullptr, timer_buffer_info, nullptr);
		vk::WriteDescriptorSet cursor_buffer_write_desc_set({}, 5, 0, vk::DescriptorType::eUniformBuffer, nullptr, cursor_buffer_info, nullptr);
		auto write_descriptor_sets = std::vector
		{ position_buffer_write_desc_set, image_write_desc_set, timer_buffer_write_desc_set, cursor_buffer_write_desc_set };
		ref_descriptor_set->UpdateDescriptorSet(logical_device, idx, std::move(write_descriptor_sets));
	}
}

void SampleActorAnother::SetPipeline(const std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& render_pass = gpu_instance->GetSwapChain()->GetRenderPass();
	auto& ref_descriptor_set = m_ptrShaderAttachment->GetDescriptorSet();
	auto& ref_graphic_pipeline = m_ptrShaderAttachment->GetGraphicPipeline();

	vk_interface::component::ShaderProvider::AddShader(logical_device, "vert/sample_shader.vert", "lenna");
	vk_interface::component::ShaderProvider::AddShader(logical_device, "frag/sample_shader.frag", "lenna");

	const auto& vert_shader_module = vk_interface::component::ShaderProvider::GetShader("vert", "lenna");
	const auto& frag_shader_module = vk_interface::component::ShaderProvider::GetShader("frag", "lenna");

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

void SampleActorAnother::Initialize(const std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	LoadData(gpu_instance);
	SetPipeline(gpu_instance);

	const auto& logical_device = gpu_instance->GetLogicalDevice();

	{
		const auto& texture = hephics::asset::AssetManager::GetTexture("lenna");
		const auto& cv_mat = hephics::asset::AssetManager::GetCvMat("lenna");
		texture->CopyTexture(gpu_instance, cv_mat);
	}

	{
		const auto& texture_3d = hephics::asset::AssetManager::GetTexture3D("lenna");
		texture_3d->CopyVertexBuffer(gpu_instance);
		texture_3d->CopyIndexBuffer(gpu_instance);
	}

	for (auto& component : m_components)
		component->Initialize(gpu_instance);
}

void SampleActorAnother::Update(const std::shared_ptr<hephics::VkInstance>& gpu_instance)
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

	{
		const auto& current_frame_id = swap_chain->GetCurrentFrameId();
		auto& cursor_uniform_buffer = m_ptrShaderAttachment->GetUniformBuffersMap().at("cursor").at(current_frame_id);
		auto cursor_uniform_address = cursor_uniform_buffer->Mapping(logical_device);

		const auto& window = hephics::window::WindowManager::GetWindow(gpu_instance->GetWindowTitle());
		const auto& cursor_pos = hephics::window::WindowManager::GetCursorPosition(gpu_instance->GetWindowTitle());
		const auto& diff_x = cursor_pos[0] - window->GetWidth() / 2.0f;
		const auto& diff_y = cursor_pos[1] - window->GetHeight() / 2.0f;
		glm::vec2 new_cursor_pos{ diff_x, diff_y };
		std::memcpy(cursor_uniform_address, &new_cursor_pos, sizeof(float_t));
		cursor_uniform_buffer->Unmapping(logical_device);
	}

	{
		const auto& current_frame_id = swap_chain->GetCurrentFrameId();
		auto& position_uniform_buffer = m_ptrShaderAttachment->GetUniformBuffersMap().at("position").at(current_frame_id);
		auto position_uniform_address = position_uniform_buffer->Mapping(logical_device);

		const auto& window = hephics::window::WindowManager::GetWindow(gpu_instance->GetWindowTitle());
		const auto& cursor_pos = hephics::window::WindowManager::GetCursorPosition(gpu_instance->GetWindowTitle());
		const auto& diff_x = cursor_pos[0] / window->GetWidth() - 0.5f;
		const auto& diff_y = cursor_pos[1] / window->GetHeight() - 0.5f;
		m_ptrPosition->model = glm::mat4(1.0f);
		m_ptrPosition->view = glm::lookAt(glm::vec3(diff_x * 5.0f, -diff_y * 5.0f, -2.0f),
			glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_ptrPosition->projection = glm::perspective(glm::radians(45.0f),
			swap_chain->GetExtent2D().width / static_cast<float_t>(swap_chain->GetExtent2D().height), 0.1f, 10.0f);

		std::memcpy(position_uniform_address, m_ptrPosition.get(), sizeof(decltype(*m_ptrPosition)));
		position_uniform_buffer->Unmapping(logical_device);
	}

	{
		const auto& current_frame_id = swap_chain->GetCurrentFrameId();
		auto& uniform_buffer = m_ptrShaderAttachment->GetUniformBuffersMap().at("timer").at(current_frame_id);
		auto uniform_address = uniform_buffer->Mapping(logical_device);

		std::memcpy(uniform_address, &time, sizeof(float_t));
		uniform_buffer->Unmapping(logical_device);
	}
}

void SampleActorAnother::Render(const std::shared_ptr<hephics::VkInstance>& gpu_instance)
{
	/* firstly, actor's render method */
	auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	auto& render_command_buffer = gpu_instance->GetGraphicCommandBuffer("render")->GetCommandBuffer();
	const auto& pipeline = m_ptrShaderAttachment->GetGraphicPipeline();
	const auto& desc_set =
		m_ptrShaderAttachment->GetDescriptorSet()->GetDescriptorSet(swap_chain->GetCurrentFrameId());

	const auto& texture_3d = hephics::asset::AssetManager::GetTexture3D("lenna");

	render_command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->GetPipeline().get());
	render_command_buffer->bindVertexBuffers(0, { texture_3d->GetVertexBuffer()->GetBuffer().get() }, { 0 });
	render_command_buffer->bindIndexBuffer(texture_3d->GetIndexBuffer()->GetBuffer().get(), 0, vk::IndexType::eUint32);
	render_command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		pipeline->GetLayout().get(), 0, desc_set.get(), nullptr);
	render_command_buffer->drawIndexed(static_cast<uint32_t>(texture_3d->GetIndices().size()), 1, 0, 0, 0);

	for (auto& component : m_components)
		component->Render(gpu_instance);
}