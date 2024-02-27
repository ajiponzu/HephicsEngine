#include "../SampleApp.hpp"

void SampleComputeActor::LoadData()
{
}

void SampleComputeActor::SetPipeline()
{
}

void SampleComputeActor::Initialize()
{
	m_ptrPosition = std::make_shared<hephics::actor::Position>();
	m_ptrRenderer = std::make_shared<hephics::actor::Renderer>();
	m_attachments.emplace_back(std::make_shared<hephics::vfx::particle_system::Engine>(8192U));

	LoadData();
	SetPipeline();

	for (auto& attachment : m_attachments)
		attachment->Initialize();
}

void SampleComputeActor::Update()
{
	for (const auto& component : m_attachments)
		component->Update(this);
}

void SampleComputeActor::Render()
{
	for (const auto& attachment : m_attachments)
		attachment->Render();
}