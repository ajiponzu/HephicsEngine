#include "../Hephics.hpp"

std::vector<std::shared_ptr<hephics_helper::StagingBuffer>> hephics::Scene::s_staging_buffers;

void hephics::Scene::Initialize(const std::shared_ptr<window::Window>& ptr_window)
{
	auto gpu_instance_option = hephics::GPUHandler::GetInstance(ptr_window->GetWindowTitle());
	if (!gpu_instance_option.has_value())
		throw std::runtime_error("failed to find vulkan_instance");

	auto& gpu_instance = gpu_instance_option.value();
	auto& command_buffers = gpu_instance->GetGraphicCommandBuffers();

	for (size_t idx = 0; idx < std::min(m_actors.size(), command_buffers.size()); idx++)
	{
		auto& command_buffer = command_buffers.at(idx);
		command_buffer->ResetCommands({});
		command_buffer->BeginRecordingCommands({});
	}

	size_t idx = 0;
	for (auto& actor : m_actors)
	{
		actor->Initialize(gpu_instance, idx % command_buffers.size());
		idx++;
	}

	std::vector<vk::CommandBuffer> submitted_command_buffers;
	for (size_t idx = 0; idx < std::min(m_actors.size(), command_buffers.size()); idx++)
	{
		auto& command_buffer = command_buffers.at(idx);
		command_buffer->EndRecordingCommands();
		submitted_command_buffers.push_back(command_buffer->GetCommandBuffer().get());
	}

	vk::SubmitInfo submit_info({}, {}, submitted_command_buffers);
	gpu_instance->SubmitCopyGraphicResource(submit_info);
	Scene::GetStagingBuffers().clear();
}

void hephics::Scene::Update()
{
	auto gpu_instance_option = hephics::GPUHandler::GetInstance(m_windowTitle);
	if (!gpu_instance_option.has_value())
		throw std::runtime_error("failed to find vulkan_instance");

	auto& gpu_instance = gpu_instance_option.value();
	auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& swap_chain = gpu_instance->GetSwapChain();
	const auto& command_buffers_size = gpu_instance->GetGraphicCommandBuffers().size();

	swap_chain->AcquireNextImageIdx(logical_device); // update: next_image_idx, before using command buffer
	swap_chain->WaitFence(logical_device);

	size_t idx = 0;
	for (auto& actor : m_actors)
	{
		const auto command_buffer_idx = idx % command_buffers_size;
		actor->Update(gpu_instance, command_buffer_idx);
		idx++;
	}

	swap_chain->CancelWaitFence(logical_device);
}

void hephics::Scene::Render()
{
	auto gpu_instance_option = hephics::GPUHandler::GetInstance(m_windowTitle);
	if (!gpu_instance_option.has_value())
		throw std::runtime_error("failed to find vulkan_instance");

	auto& gpu_instance = gpu_instance_option.value();
	const auto& command_buffers = gpu_instance->GetGraphicCommandBuffers();
	const auto& swap_chain = gpu_instance->GetSwapChain();

	for (size_t idx = 0; idx < std::min(m_actors.size(), command_buffers.size()); idx++)
	{
		auto& command_buffer = command_buffers.at(idx);
		command_buffer->ResetCommands({});
		command_buffer->BeginRecordingCommands({});
	}

	size_t idx = 0;
	for (auto& actor : m_actors)
	{
		const auto command_buffer_idx = idx % command_buffers.size();
		actor->Render(gpu_instance, command_buffer_idx);
		idx++;
	}

	std::vector<vk::CommandBuffer> submitted_command_buffers;
	for (size_t idx = 0; idx < std::min(m_actors.size(), command_buffers.size()); idx++)
	{
		auto& command_buffer = command_buffers.at(idx);
		command_buffer->EndRecordingCommands();
		submitted_command_buffers.emplace_back(command_buffer->GetCommandBuffer().get());
	}

	const auto submit_info =
		swap_chain->GetRenderingSubmitInfo(submitted_command_buffers, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	gpu_instance->SubmitRenderingCommand(submit_info);

	const auto present_info = swap_chain->GetPresentInfo();
	gpu_instance->PresentFrame(present_info);
	swap_chain->PrepareNextFrame();
}

void hephics::Scene::ResetScene(const std::shared_ptr<VkInstance>& gpu_instance)
{
	gpu_instance->GetLogicalDevice()->waitIdle();

	hephics::asset::AssetManager::Reset();
	vk_interface::component::ShaderProvider::Reset();
}