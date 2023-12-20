#include "../hephics/Hephics.hpp"

class SampleApp : public hephics::App
{
private:

public:
	SampleApp() = default;
	~SampleApp() {}

	virtual void Initialize() override;
	virtual void Run() override;
};

class SampleScene : public hephics::Scene
{
private:

public:
	SampleScene(const std::string& scene_name)
		: hephics::Scene(scene_name)
	{}

	~SampleScene() {}

	virtual void Initialize(const std::shared_ptr<hephics::window::Window>& ptr_window) override;
	virtual void Update() override;
	virtual void Render() override;
};

class SampleSceneAnother : public hephics::Scene
{
private:

public:
	SampleSceneAnother(const std::string& scene_name)
		:hephics::Scene(scene_name)
	{}
	~SampleSceneAnother() {}

	virtual void Initialize(const std::shared_ptr<hephics::window::Window>& ptr_window) override;
	virtual void Update() override;
	virtual void Render() override;
};

class SampleActor : public hephics::actor::Actor
{
private:
	virtual void LoadData(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void SetPipeline(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;

public:
	SampleActor() = default;
	~SampleActor() {}

	virtual void Initialize(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void Update(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void Render(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
};

class SampleActorAnother : public hephics::actor::Actor
{
private:
	virtual void LoadData(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void SetPipeline(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;

public:
	SampleActorAnother() = default;
	~SampleActorAnother() {}

	virtual void Initialize(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void Update(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void Render(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
};

class MoveComponent : public hephics::actor::Component
{
private:
	virtual void LoadData(std::shared_ptr<hephics::VkInstance>& gpu_instance) override {};
	virtual void SetPipeline(std::shared_ptr<hephics::VkInstance>& gpu_instance) override {};

public:
	MoveComponent() = default;
	~MoveComponent() {}

	virtual void Initialize(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void Update(hephics::actor::Actor* const owner, std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
	virtual void Render(std::shared_ptr<hephics::VkInstance>& gpu_instance) override;
};