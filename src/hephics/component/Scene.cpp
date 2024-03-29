#include "../Hephics.hpp"

std::chrono::steady_clock::time_point hephics::Scene::s_startTimePoint;
float_t hephics::Scene::s_deltaTime = 0.0f;
std::vector<std::shared_ptr<hephics_helper::StagingBuffer>> hephics::Scene::s_stagingBuffers;

void hephics::Scene::Initialize()
{
	const auto& gpu_instance = GPUHandler::GetInstance();
	const auto& copy_command_buffer = gpu_instance->GetGraphicCommandBuffer("copy");

	copy_command_buffer->ResetCommands({});
	copy_command_buffer->BeginRecordingCommands({});

	for (const auto& actor : m_actors)
		actor->Initialize();

	copy_command_buffer->EndRecordingCommands();

	std::vector<vk::CommandBuffer> submitted_command_buffers;
	submitted_command_buffers.push_back(copy_command_buffer->GetCommandBuffer().get());

	vk::SubmitInfo submit_info({}, {}, submitted_command_buffers);
	gpu_instance->SubmitCopyGraphicResource(submit_info);
	Scene::GetStagingBuffers().clear();

	s_startTimePoint = std::chrono::high_resolution_clock::now();
}

void hephics::Scene::Update()
{
	const auto& gpu_instance = GPUHandler::GetInstance();
	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();

	auto current_time_point = std::chrono::high_resolution_clock::now();
	s_deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(
		current_time_point - s_startTimePoint).count();

	swap_chain->AcquireNextImageIdx(logical_device); // update: next_image_idx, before using command buffer
	swap_chain->WaitFence(logical_device);

	for (const auto& actor : m_actors)
		actor->Update();

	swap_chain->CancelWaitFence(logical_device);
}

void hephics::Scene::Render()
{
	const auto& gpu_instance = GPUHandler::GetInstance();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	const auto& render_command_buffer = gpu_instance->GetGraphicCommandBuffer("render");

	render_command_buffer->ResetCommands({});
	render_command_buffer->BeginRecordingCommands({});
	render_command_buffer->BeginRenderPass(swap_chain, vk::SubpassContents::eInline);

	for (const auto& actor : m_actors)
		actor->Render();

	render_command_buffer->EndRenderPass();
	render_command_buffer->EndRecordingCommands();

	std::vector<vk::CommandBuffer> submitted_command_buffers;
	submitted_command_buffers.push_back(render_command_buffer->GetCommandBuffer().get());

	const auto submit_info =
		swap_chain->GetRenderingSubmitInfo(submitted_command_buffers, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	gpu_instance->SubmitRenderingCommand(submit_info);

	if (window::Manager::CheckPressKey(GLFW_KEY_SPACE))
		WriteScreenImage();

	const auto present_info = swap_chain->GetPresentInfo();
	gpu_instance->PresentFrame(present_info);
	swap_chain->PrepareNextFrame();

	if (!GPUHandler::GetComputePurpose().empty())
		gpu_instance->GetComputingSyncObject()->PrepareNextFrame();
}

void hephics::Scene::ResetScene()
{
	GPUHandler::WaitIdle();
	asset::Manager::Reset();
	vk_interface::component::ShaderProvider::Reset();
}

void hephics::Scene::WriteScreenImage() const
{
	const auto& gpu_instance = GPUHandler::GetInstance();

	const auto& logical_device = gpu_instance->GetLogicalDevice();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	const auto& copy_command_buffer = gpu_instance->GetGraphicCommandBuffer("copy");

	const auto image_wid = swap_chain->GetExtent2D().width;
	const auto image_high = swap_chain->GetExtent2D().height;

	const auto buffer_size = image_wid * image_high * 4;

	vk::BufferImageCopy image_copy_region(0, 0, 0,
		vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
		vk::Offset3D(0, 0, 0), vk::Extent3D(swap_chain->GetExtent2D(), 1U));

	auto staging_buffer = std::make_shared<hephics_helper::StagingBuffer>(gpu_instance, buffer_size);

	copy_command_buffer->ResetCommands({});
	copy_command_buffer->BeginRecordingCommands({});
	copy_command_buffer->TransitionImageCommandLayout(swap_chain->GetCurrentImage(), swap_chain->GetImageFormat(),
		{ vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eTransferSrcOptimal }, 1U);
	copy_command_buffer->GetCommandBuffer()->copyImageToBuffer(swap_chain->GetCurrentImage(),
		vk::ImageLayout::eTransferSrcOptimal, staging_buffer->GetBuffer().get(), image_copy_region);
	copy_command_buffer->TransitionImageCommandLayout(swap_chain->GetCurrentImage(), swap_chain->GetImageFormat(),
		{ vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::ePresentSrcKHR }, 1U);
	copy_command_buffer->EndRecordingCommands();

	std::vector<vk::CommandBuffer> submitted_command_buffers;
	submitted_command_buffers.push_back(copy_command_buffer->GetCommandBuffer().get());

	vk::SubmitInfo submit_info({}, {}, submitted_command_buffers);
	gpu_instance->SubmitCopyGraphicResource(submit_info);

	auto staging_map_address = staging_buffer->Mapping(logical_device);
	cv::Mat screen_image(image_high, image_wid, CV_8UC4, staging_map_address);
	cv::cvtColor(screen_image, screen_image, cv::COLOR_RGBA2BGR);

	const auto current_time = std::chrono::system_clock::now();
	std::chrono::sys_seconds sec_tp = std::chrono::floor<std::chrono::seconds>(current_time);
	cv::imwrite(std::format("output/screenshot/screenshot_{:%Y_%m%d_%H%M%S}.png", sec_tp), screen_image);
	staging_buffer->Unmapping(logical_device);
}