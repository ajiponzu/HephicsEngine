#include "../SampleApp.hpp"

void SampleComputeScene::Initialize()
{
	const auto& window = hephics::window::Manager::GetWindow();

	window->SetCallback(
		[&](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			switch (action)
			{
			case GLFW_PRESS:
				switch (key)
				{
				case GLFW_KEY_ESCAPE:
					m_isContinuous = false;
					break;
				case GLFW_KEY_ENTER:
					m_isChangedScene = true;
					m_nextSceneName = "first";
					break;
				case GLFW_KEY_SPACE:
					WriteScreenImage();
					break;
				}
			}
		});

	m_actors.emplace_back(std::make_shared<SampleComputeActor>());

	Scene::Initialize();
}

void SampleComputeScene::Update()
{
	Scene::Update();
}

void SampleComputeScene::Render()
{
	const auto& gpu_instance = hephics::GPUHandler::GetInstance();
	const auto& swap_chain = gpu_instance->GetSwapChain();
	const auto& computing_sync_object = gpu_instance->GetComputingSyncObject();
	const auto& current_frame_id = computing_sync_object->GetCurrentFrameId();
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

	const auto wait_semaphores = {
		computing_sync_object->GetCurrentSemaphore().get(),
		swap_chain->GetCurrentImageAvailableSemaphore().get()
	};
	const auto wait_stage_flags = std::vector<vk::PipelineStageFlags>{
		vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput
	};
	auto submit_info =
		swap_chain->GetRenderingSubmitInfo(submitted_command_buffers, {});
	submit_info.setWaitSemaphores(wait_semaphores);
	submit_info.setWaitDstStageMask(wait_stage_flags);
	gpu_instance->SubmitRenderingCommand(submit_info);

	if (hephics::window::Manager::CheckPressKey(GLFW_KEY_SPACE))
		WriteScreenImage();

	const auto present_info = swap_chain->GetPresentInfo();
	gpu_instance->PresentFrame(present_info);
	swap_chain->PrepareNextFrame();

	gpu_instance->GetComputingSyncObject()->PrepareNextFrame();
}