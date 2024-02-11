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

	virtual void Initialize() override;
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

	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Render() override;
};

class SampleActor : public hephics::actor::Actor
{
private:
	virtual void LoadData() override;
	virtual void SetPipeline() override;

public:
	SampleActor() {}
	~SampleActor() {}

	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Render() override;
};

class SampleActorAnother : public hephics::actor::Actor
{
private:
	virtual void LoadData() override;
	virtual void SetPipeline() override;

public:
	SampleActorAnother() = default;
	~SampleActorAnother() {}

	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Render() override;
};

class MoveComponent : public hephics::actor::Component
{
private:
	virtual void LoadData() override {};
	virtual void SetPipeline() override {};

public:
	MoveComponent() = default;
	~MoveComponent() {}

	virtual void Initialize() override;
	virtual void Update(hephics::actor::Actor* const owner) override;
	virtual void Render() override;
};