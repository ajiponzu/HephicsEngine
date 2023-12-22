#include "../Hephics.hpp"

std::vector<std::shared_ptr<hephics_helper::StagingBuffer>> hephics::Scene::s_staging_buffers;

void hephics::Scene::Initialize(const std::shared_ptr<window::Window>& ptr_window)
{
	auto& gpu_instance = hephics::GPUHandler::GetInstance();
	auto& copy_command_buffer = gpu_instance->GetGraphicCommandBuffer("copy");

	copy_command_buffer->ResetCommands({});
	copy_command_buffer->BeginRecordingCommands({});

	for (auto& actor : m_actors)
		actor->Initialize(gpu_instance);

	copy_command_buffer->EndRecordingCommands();

	std::cout << std::endl << std::endl;

	std::vector<vk::CommandBuffer> submitted_command_buffers;
	submitted_command_buffers.push_back(copy_command_buffer->GetCommandBuffer().get());

	vk::SubmitInfo submit_info({}, {}, submitted_command_buffers);
	gpu_instance->SubmitCopyGraphicResource(submit_info);
	Scene::GetStagingBuffers().clear();
}

void hephics::Scene::Update()
{
	auto& gpu_instance = hephics::GPUHandler::GetInstance();
	auto& logical_device = gpu_instance->GetLogicalDevice();
	auto& swap_chain = gpu_instance->GetSwapChain();

	swap_chain->AcquireNextImageIdx(logical_device); // update: next_image_idx, before using command buffer
	swap_chain->WaitFence(logical_device);

	for (auto& actor : m_actors)
		actor->Update(gpu_instance);

	swap_chain->CancelWaitFence(logical_device);
}

void hephics::Scene::Render()
{
	auto& gpu_instance = hephics::GPUHandler::GetInstance();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	const auto& command_buffers = gpu_instance->GetGraphicCommandBuffers();
	auto& render_command_buffer = gpu_instance->GetGraphicCommandBuffer("render");

	render_command_buffer->ResetCommands({});
	render_command_buffer->BeginRecordingCommands({});
	render_command_buffer->BeginRenderPass(swap_chain, vk::SubpassContents::eInline);

	for (auto& actor : m_actors)
		actor->Render(gpu_instance);

	render_command_buffer->EndRenderPass();
	render_command_buffer->EndRecordingCommands();

	std::vector<vk::CommandBuffer> submitted_command_buffers;
	submitted_command_buffers.push_back(render_command_buffer->GetCommandBuffer().get());

	const auto submit_info =
		swap_chain->GetRenderingSubmitInfo(submitted_command_buffers, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	gpu_instance->SubmitRenderingCommand(submit_info);

	const auto present_info = swap_chain->GetPresentInfo();
	gpu_instance->PresentFrame(present_info);
	swap_chain->PrepareNextFrame();
}

void hephics::Scene::ResetScene()
{
	GPUHandler::WaitIdle();
	hephics::asset::AssetManager::Reset();
	vk_interface::component::ShaderProvider::Reset();
}