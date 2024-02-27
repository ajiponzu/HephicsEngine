#include "../../Hephics.hpp"

void hephics::vfx::particle_system::Engine::LoadData()
{
	const auto& gpu_instance = GPUHandler::GetInstance();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& copy_command_buffer = gpu_instance->GetGraphicCommandBuffer("copy");
	const auto& swap_chain_extent = gpu_instance->GetSwapChain()->GetExtent2D();
	auto& ref_descriptor_set = m_ptrComputingSystem->GetDescriptorSet();

	std::random_device random_seed_generator;
	std::mt19937_64 random_engine(random_seed_generator());
	std::uniform_real_distribution<float_t> random_distribution(0.0f, 1.0f);

	for (auto& particle : m_particles)
	{
		float_t radius = 0.25f * std::sqrtf(random_distribution(random_engine));
		float_t theta_radian = random_distribution(random_engine) * 2.0f * std::numbers::pi_v<float_t>;
		float_t position_x = radius * std::cos(theta_radian) * swap_chain_extent.height / swap_chain_extent.width;
		float_t position_y = radius * std::sin(theta_radian);

		particle.position = glm::vec2(position_x, position_y);
		particle.velocity = glm::normalize(particle.position) * 0.00015f;
		particle.color = glm::vec4(random_distribution(random_engine),
			random_distribution(random_engine), random_distribution(random_engine), 1.0f);
	}

	{
		const size_t particle_buffer_size = sizeof(Particle) * m_particles.size();

		vk::DescriptorSetLayoutBinding delta_time_layout_binding(10, vk::DescriptorType::eUniformBuffer,
			1, vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex, nullptr);
		vk::DescriptorSetLayoutBinding particle_input_layout_binding(11, vk::DescriptorType::eStorageBuffer,
			1, vk::ShaderStageFlagBits::eCompute, nullptr);
		vk::DescriptorSetLayoutBinding particle_output_layout_binding(12, vk::DescriptorType::eStorageBuffer,
			1, vk::ShaderStageFlagBits::eCompute, nullptr);

		auto desc_layout_bindings = std::vector
		{
			delta_time_layout_binding, particle_input_layout_binding, particle_output_layout_binding
		};
		ref_descriptor_set->SetDescriptorSetLayout(logical_device, desc_layout_bindings);

		vk::DescriptorPoolSize uniform_desc_pool_size(vk::DescriptorType::eUniformBuffer, hephics::BUFFERING_FRAME_NUM);
		vk::DescriptorPoolSize  storage_desc_pool_size(vk::DescriptorType::eStorageBuffer, hephics::BUFFERING_FRAME_NUM);
		auto desc_pool_size_list = std::vector{ uniform_desc_pool_size, storage_desc_pool_size, storage_desc_pool_size };
		vk::DescriptorPoolCreateInfo desc_pool_create_info(
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, hephics::BUFFERING_FRAME_NUM, desc_pool_size_list);
		ref_descriptor_set->SetDescriptorPool(logical_device, desc_pool_create_info);

		ref_descriptor_set->SetDescriptorSet(logical_device, hephics::BUFFERING_FRAME_NUM);

		const auto delta_time_uniform_buffer_size = sizeof(decltype(Scene::GetDeltaTime()));
		auto& uniform_buffers_map = m_ptrComputingSystem->GetUniformBuffersMap();
		uniform_buffers_map["delta_time"] = {};
		for (auto& uniform_buffer : uniform_buffers_map.at("delta_time"))
			uniform_buffer.reset(new hephics_helper::UniformBuffer(gpu_instance, delta_time_uniform_buffer_size));

		auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, particle_buffer_size);
		auto staging_map_address = staging_buffer->Mapping(logical_device);
		std::memcpy(staging_map_address, m_particles.data(), particle_buffer_size);
		staging_buffer->Unmapping(logical_device);

		for (auto& storage_buffer : m_vertexStorageBuffers)
		{
			storage_buffer.reset(new hephics_helper::GPUBuffer(gpu_instance, particle_buffer_size,
				vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer));
			copy_command_buffer->CopyBuffer(staging_buffer, storage_buffer, particle_buffer_size);
		}

		auto& staging_buffers = Scene::GetStagingBuffers();
		staging_buffers.emplace_back(std::move(staging_buffer));

		for (size_t idx = 0; idx < hephics::BUFFERING_FRAME_NUM; idx++)
		{
			const auto& delta_time_uniform_buffers = uniform_buffers_map.at("delta_time");
			vk::DescriptorBufferInfo delta_time_buffer_info(
				delta_time_uniform_buffers.at(idx)->GetBuffer().get(), 0, delta_time_uniform_buffer_size);

			const auto& particle_input_storage_buffer = m_vertexStorageBuffers.at((idx - 1) % hephics::BUFFERING_FRAME_NUM);
			vk::DescriptorBufferInfo particle_input_buffer_info(
				particle_input_storage_buffer->GetBuffer().get(), 0, particle_buffer_size);

			const auto& particle_output_storage_buffer = m_vertexStorageBuffers.at(idx);
			vk::DescriptorBufferInfo particle_output_buffer_info(
				particle_output_storage_buffer->GetBuffer().get(), 0, particle_buffer_size);

			vk::WriteDescriptorSet delta_time_write_desc_set({}, 10, 0, vk::DescriptorType::eUniformBuffer, nullptr, delta_time_buffer_info, nullptr);
			vk::WriteDescriptorSet particle_input_write_desc_set({}, 11, 0, vk::DescriptorType::eStorageBuffer, nullptr, particle_input_buffer_info, nullptr);
			vk::WriteDescriptorSet particle_output_write_desc_set({}, 12, 0, vk::DescriptorType::eStorageBuffer, nullptr, particle_output_buffer_info, nullptr);
			auto write_descriptor_sets = std::vector
			{ delta_time_write_desc_set, particle_input_write_desc_set, particle_output_write_desc_set };
			ref_descriptor_set->UpdateDescriptorSet(logical_device, idx, std::move(write_descriptor_sets));
		}
	}
}

void hephics::vfx::particle_system::Engine::SetPipeline()
{
	const auto& gpu_instance = GPUHandler::GetInstance();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& ref_compute_pipeline = m_ptrComputingSystem->GetComputePipeline();
	auto& ref_graphic_pipeline = m_ptrRenderer->GetGraphicPipeline();

	vk_interface::component::ShaderProvider::AddShader(logical_device, "vert/particle.vert", "particle");
	vk_interface::component::ShaderProvider::AddShader(logical_device, "frag/particle.frag", "particle");
	vk_interface::component::ShaderProvider::AddShader(logical_device, "comp/particle.comp", "particle");

	const auto& vertex_shader_module = vk_interface::component::ShaderProvider::GetShader("vert", "particle");
	const auto& fragment_shader_module = vk_interface::component::ShaderProvider::GetShader("frag", "particle");
	const auto& compute_shader_module = vk_interface::component::ShaderProvider::GetShader("comp", "particle");

	auto& ref_descriptor_set = m_ptrComputingSystem->GetDescriptorSet();

	{
		vk::PipelineShaderStageCreateInfo compute_shader_stage_info({}, vk::ShaderStageFlagBits::eCompute,
			compute_shader_module->GetModule().get(), "main");

		vk::PipelineLayoutCreateInfo pipeline_layout_info({}, ref_descriptor_set->GetDescriptorSetLayout().get(), {});
		ref_compute_pipeline->SetLayout(logical_device, pipeline_layout_info);

		vk::ComputePipelineCreateInfo pipeline_info({}, compute_shader_stage_info, ref_compute_pipeline->GetLayout().get(), {});
		ref_compute_pipeline->SetPipeline(logical_device, pipeline_info);
	}

	{
		const auto& render_pass = gpu_instance->GetSwapChain()->GetRenderPass();

		vk::PipelineShaderStageCreateInfo vertex_shader_stage_info({}, vk::ShaderStageFlagBits::eVertex,
			vertex_shader_module->GetModule().get(), "main");

		vk::PipelineShaderStageCreateInfo fragment_shader_stage_info({}, vk::ShaderStageFlagBits::eFragment,
			fragment_shader_module->GetModule().get(), "main");

		const auto shader_stages = { vertex_shader_stage_info, fragment_shader_stage_info };

		auto vertex_binding_descs = std::vector{ Particle::get_binding_description() };
		auto vertex_attribute_descs = Particle::get_attribute_descriptions();
		vk::PipelineVertexInputStateCreateInfo vertex_input_info({}, vertex_binding_descs, vertex_attribute_descs);

		vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::ePointList, VK_FALSE);

		vk::PipelineViewportStateCreateInfo viewport_state({}, 1, {}, 1, {});

		vk::PipelineRasterizationStateCreateInfo rasterizer({}, VK_FALSE, VK_FALSE,
			vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise,
			VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f);

		vk::PipelineMultisampleStateCreateInfo multisampling({}, gpu_instance->GetMultiSampleCount(), VK_FALSE);

		vk::PipelineDepthStencilStateCreateInfo depth_stencil({}, VK_TRUE, VK_TRUE,
			vk::CompareOp::eLessOrEqual, VK_FALSE, VK_FALSE);
		depth_stencil.setMinDepthBounds(0.0f);
		depth_stencil.setMaxDepthBounds(1.0f);

		vk::PipelineColorBlendAttachmentState color_blend_attachment(VK_FALSE);
		color_blend_attachment.setColorWriteMask(
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
		color_blend_attachment.setBlendEnable(VK_TRUE);
		color_blend_attachment.setColorBlendOp(vk::BlendOp::eAdd);
		color_blend_attachment.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
		color_blend_attachment.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
		color_blend_attachment.setAlphaBlendOp(vk::BlendOp::eAdd);
		color_blend_attachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
		color_blend_attachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);

		vk::PipelineColorBlendStateCreateInfo color_blending({}, VK_FALSE, vk::LogicOp::eCopy, color_blend_attachment);

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
}

void hephics::vfx::particle_system::Engine::Initialize()
{
	m_ptrRenderer = std::make_shared<actor::Renderer>();
	m_ptrComputingSystem = std::make_shared<actor::ComputingSystem>();

	LoadData();
	SetPipeline();
}

void hephics::vfx::particle_system::Engine::Update(actor::Actor* const owner)
{
	const auto& gpu_instance = GPUHandler::GetInstance();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& computing_sync_object = gpu_instance->GetComputingSyncObject();

	const auto delta_time = Scene::GetDeltaTime();

	const auto& current_frame_id = computing_sync_object->GetCurrentFrameId();
	auto& delta_time_buffer =
		m_ptrComputingSystem->GetUniformBuffersMap().at("delta_time").at(current_frame_id);

	computing_sync_object->WaitFence(logical_device);
	auto mapping_address = delta_time_buffer->Mapping(logical_device);
	std::memcpy(mapping_address, &delta_time, sizeof(delta_time));
	delta_time_buffer->Unmapping(logical_device);
	computing_sync_object->CancelWaitFence(logical_device);

	{
		const auto& compute_command_buffer = gpu_instance->GetComputeCommandBuffer("particle");
		compute_command_buffer->ResetCommands({});
		compute_command_buffer->BeginRecordingCommands({});
	}

	{
		const auto& compute_command_buffer = gpu_instance->GetComputeCommandBuffer("particle")->GetCommandBuffer();
		const auto& compute_pipeline = m_ptrComputingSystem->GetComputePipeline();
		const auto& descriptor_set = m_ptrComputingSystem->GetDescriptorSet();

		compute_command_buffer->bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline->GetPipeline().get());
		compute_command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute_pipeline->GetLayout().get(),
			0, descriptor_set->GetDescriptorSet(current_frame_id).get(), nullptr);
		compute_command_buffer->dispatch(static_cast<uint32_t>(m_particles.size() / 256U), 1, 1);
	}

	{
		const auto& compute_command_buffer = gpu_instance->GetComputeCommandBuffer("particle");
		compute_command_buffer->EndRecordingCommands();

		std::vector<vk::CommandBuffer> submitted_command_buffers;
		submitted_command_buffers.push_back(compute_command_buffer->GetCommandBuffer().get());
		const auto submit_info = computing_sync_object->GetComputingSubmitInfo(submitted_command_buffers);
		gpu_instance->SubmitComputingCommand(submit_info);
	}
}

void hephics::vfx::particle_system::Engine::Render()
{
	const auto& gpu_instance = hephics::GPUHandler::GetInstance();

	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	const auto& render_command_buffer = gpu_instance->GetGraphicCommandBuffer("render")->GetCommandBuffer();
	const auto& pipeline = m_ptrRenderer->GetGraphicPipeline();
	const auto& computing_sync_object = gpu_instance->GetComputingSyncObject();
	const auto& current_frame_id = computing_sync_object->GetCurrentFrameId();

	render_command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->GetPipeline().get());
	render_command_buffer->bindVertexBuffers(0,
		{ m_vertexStorageBuffers.at(current_frame_id)->GetBuffer().get() }, { 0 });
	render_command_buffer->draw(static_cast<uint32_t>(m_particles.size()), 1, 0, 0);
}