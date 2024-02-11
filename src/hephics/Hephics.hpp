#pragma once

#include "vulkan_interface/Interface.hpp"
#include "HephicsHelper.hpp"

namespace hephics
{
	constexpr auto BUFFERING_FRAME_NUM = 2;

	namespace window
	{
		struct WindowInfo
		{
			int32_t width = 0;
			int32_t height = 0;
			std::string title = "";
		};

		using InputKeyCallback = std::function<void(::GLFWwindow* ptr_window, int key, int scancode, int action, int mods)>;
		using CursorPositionCallback = std::function<void(::GLFWwindow* ptr_window, double xpos, double ypos)>;
		using MouseButtonCallback = std::function<void(::GLFWwindow* ptr_window, int button, int action, int mods)>;
		using MouseScrollCallback = std::function<void(::GLFWwindow* ptr_window, double xoffset, double yoffset, int dummy)>; // differentiate CursorPositionCallback
		using WindowResizedCallback = std::function<void(::GLFWwindow* window, int width, int height)>;
		using CallbackVariant = std::variant<InputKeyCallback, CursorPositionCallback,
			MouseButtonCallback, MouseScrollCallback, WindowResizedCallback>; // variant: In C, means "union", In python, means "any"

		class Window
		{
		private:
			static std::unordered_map<const ::GLFWwindow*,
				std::unordered_map<std::string, CallbackVariant>> s_callbackDictionary;

			void SetCallbacks();

		protected:
			friend class Manager;
			::GLFWwindow* m_ptrWindow;
			WindowInfo m_info;

		public:
			Window() = default;
			Window(const WindowInfo& info);
			~Window() {}

			::GLFWwindow* const GetPtrWindow() const { return m_ptrWindow; }
			::GLFWwindow* GetPtrWindow() { return m_ptrWindow; }
			const std::string& GetWindowTitle() const { return m_info.title; }
			const auto& GetWidth() const { return m_info.width; }
			const auto& GetHeight() const { return m_info.height; }

			void SetCallback(InputKeyCallback&& callback) const;
			void SetCallback(CursorPositionCallback&& callback) const;
			void SetCallback(MouseButtonCallback&& callback) const;
			void SetCallback(MouseScrollCallback&& callback) const;
			void SetCallback(WindowResizedCallback&& callback) const;

			template<typename T>
			static CallbackVariant GetCallback(const ::GLFWwindow* const ptr_window);
		};

		class Manager
		{
		private:
			static std::shared_ptr<Window> s_ptrWindow;
			static glm::vec2 s_cursorPosition;
			static glm::vec2 s_mouseScroll;

			Manager() = delete;
			~Manager() = delete;

		public:
			static void Initialize();
			static void Shutdown();

			static void InitializeWindow(const WindowInfo& info);

			static const std::shared_ptr<Window>& GetWindow();
			static void SetWindowSize(const int32_t& width, const int32_t& height);
			static void SetCursorPosition(const double& pos_x, const double& pos_y);
			static void SetMouseScroll(const double& x_offset, const double& y_offset);
			static const glm::vec2& GetCursorPosition();
			static const glm::vec2& GetMouseScroll();
			static const bool CheckPressKey(const int32_t& key);
		};
	};

	class VkInstance : public vk_interface::Instance
	{
	protected:
		friend class Renderer;

		std::vector<std::vector<std::shared_ptr<vk_interface::component::CommandBuffer>>> m_graphicCommandBuffers;

		virtual void SetInstance(const vk::ApplicationInfo& app_info);
		virtual void SetWindowSurface();
		virtual void SetPhysicalDevice();
		virtual void SetLogicalDeviceAndQueue();

		virtual void SetSwapChain();
		virtual void SetSwapChainImageViews();
		virtual void SetSwapChainRenderPass();
		virtual void SetSwapChainFramebuffers();
		virtual void SetSwapChainSyncObjects();

		virtual void SetCommandPools();
		virtual void SetCommandBuffers();

	public:

		VkInstance();
		~VkInstance()
		{
			if (m_logicalDevice)
				m_logicalDevice->waitIdle();
		}

		void ResetSwapChain(::GLFWwindow* const window);

		void SubmitCopyGraphicResource(const vk::SubmitInfo& submit_info);

		void SubmitRenderingCommand(const vk::SubmitInfo& submit_info);

		void PresentFrame(const vk::PresentInfoKHR& present_info);

		virtual vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates,
			const vk::ImageTiling& tilling, const vk::FormatFeatureFlags& features) const;
		virtual vk::Format FindDepthFormat() const;
		virtual uint32_t FindMemoryType(const uint32_t& memory_type_filter,
			const vk::MemoryPropertyFlags& memory_prop_flags) const;

		virtual vk::SampleCountFlagBits GetMultiSampleCount() const;

		auto& GetGraphicCommandBuffers() { return m_graphicCommandBuffers.at(m_ptrSwapChain->GetNextImageId()); }
		std::shared_ptr<vk_interface::component::CommandBuffer>& GetGraphicCommandBuffer(const std::string& purpose);
	};

	namespace asset
	{
		class Texture
		{
		private:
			std::shared_ptr<vk_interface::component::Image> m_ptrImage;
			vk::UniqueSampler m_sampler;
			uint32_t m_miplevel = 0U;

			void GenerateMipmaps(const uint32_t& width, const uint32_t& height);

		public:
			Texture()
			{
				m_ptrImage = std::make_shared<vk_interface::component::Image>();
			}
			Texture(const std::string& path, const std::string& cv_mat_key);
			Texture(const std::shared_ptr<cv::Mat>& cv_mat);
			~Texture() {}

			Texture(Texture&& other) noexcept
			{
				m_ptrImage = other.m_ptrImage;
				m_sampler = std::move(other.m_sampler);
			}

			Texture&& operator=(Texture&& other) noexcept
			{
				m_ptrImage = other.m_ptrImage;
				m_sampler = std::move(other.m_sampler);
			}

			void SetSampler(const vk::UniqueDevice& logical_device, const vk::SamplerCreateInfo& create_info);

			auto& GetImage() { return m_ptrImage; }

			const auto& GetSampler() const { return m_sampler; }

			void CopyTexture(const std::shared_ptr<cv::Mat>& cv_mat);

			const auto& GetMiplevel() const { return m_miplevel; }
		};

		struct VertexData
		{
		public:
			glm::vec3 pos;
			glm::vec3 color;
			glm::vec2 tex_coord;

			VertexData() = default;
			VertexData(const glm::vec3& pos, const glm::vec3& color, const glm::vec2& tex_coord)
				: pos(pos), color(color), tex_coord(tex_coord) {}
			~VertexData() {}

			bool operator==(const VertexData& other) const noexcept
			{
				return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
			}

			static auto get_binding_description()
			{
				return vk::VertexInputBindingDescription(0, sizeof(VertexData), vk::VertexInputRate::eVertex);
			}

			static auto get_attribute_descriptions()
			{
				std::array<vk::VertexInputAttributeDescription, 3> attribute_descriptions;
				attribute_descriptions.at(0) = { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexData, pos) };
				attribute_descriptions.at(1) = { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexData, color) };
				attribute_descriptions.at(2) = { 2, 0, vk::Format::eR32G32Sfloat, offsetof(VertexData, tex_coord) };

				return attribute_descriptions;
			}
		};

		class Asset3D
		{
		protected:
			std::vector<VertexData> m_vertices;
			std::vector<uint32_t> m_indices;
			std::shared_ptr<hephics_helper::GPUBuffer> m_ptrVertexBuffer;
			std::shared_ptr<hephics_helper::GPUBuffer> m_ptrIndexBuffer;

		public:
			Asset3D() = default;
			~Asset3D() {}

			const auto& GetVertices() const { return m_vertices; }
			const auto& GetIndices() const { return m_indices; }
			const auto& GetVertexBuffer() const { return m_ptrVertexBuffer; }
			const auto& GetIndexBuffer() const { return m_ptrIndexBuffer; }

			void CopyVertexBuffer() const;
			void CopyIndexBuffer() const;
		};

		class Texture3D : public Asset3D
		{
		protected:

		public:
			Texture3D()
			{
				m_ptrVertexBuffer = std::make_shared<hephics_helper::GPUBuffer>();
				m_ptrIndexBuffer = std::make_shared<hephics_helper::GPUBuffer>();
			}
			Texture3D(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices);
			~Texture3D() {}
		};

		class Object3D : public Asset3D
		{
		protected:
			std::shared_ptr<tinyobj::attrib_t> m_ptrAttribute;
			std::vector<tinyobj::shape_t> m_shapes;
			std::vector<tinyobj::material_t> m_materials;

		public:
			Object3D()
			{
				m_ptrVertexBuffer = std::make_shared<hephics_helper::GPUBuffer>();
				m_ptrIndexBuffer = std::make_shared<hephics_helper::GPUBuffer>();
				m_ptrAttribute = std::make_shared<tinyobj::attrib_t>();
			}
			Object3D(const std::string& path);
			~Object3D() {}

			const auto& GetAttribute() const { return m_ptrAttribute; }
			const auto& GetShapes() const { return m_shapes; }
			const auto& GetMaterials() const { return m_materials; }
		};

		class Fbx3D : public Asset3D
		{
		protected:

		public:
			Fbx3D()
			{
				m_ptrVertexBuffer = std::make_shared<hephics_helper::GPUBuffer>();
				m_ptrIndexBuffer = std::make_shared<hephics_helper::GPUBuffer>();
			}
			Fbx3D(const std::string& path) {}
			~Fbx3D() {}
		};

		using AssetVariant = std::variant
			<std::shared_ptr<cv::Mat>, std::shared_ptr<Texture>, std::shared_ptr<Texture3D>,
			std::shared_ptr<Object3D>, std::shared_ptr<Fbx3D>>;

		class AssetManager
		{
		private:
			static std::unordered_map<std::string, std::unordered_map<std::string, AssetVariant>> s_assetDictionaries;

			AssetManager() = delete;
			~AssetManager() = delete;

		public:
			static void RegistCvMat(const std::string& asset_path, const std::string& asset_key);
			static void RegistCvMat(const std::string& asset_key, const cv::Mat& cv_mat);
			static void RegistObject3D(const std::string& asset_path, const std::string& asset_key);
			static void RegistFbx3D(const std::string& asset_path, const std::string& asset_key);

			static void RegistTexture(const std::string& asset_path, const std::string& asset_key);
			static void RegistTexture(const std::string& asset_key, const cv::Mat& cv_mat);
			static void RegistTexture3D(const Texture3D& texture_3d, const std::string& asset_key);

			static const std::shared_ptr<cv::Mat>& GetCvMat(const std::string& asset_key);
			static const std::shared_ptr<Texture>& GetTexture(const std::string& asset_key);
			static const std::shared_ptr<Texture3D>& GetTexture3D(const std::string& asset_key);
			static const std::shared_ptr<Object3D>& GetObject3D(const std::string& asset_key);
			static const std::shared_ptr<Fbx3D>& GetFbx3D(const std::string& asset_key);

			static void Reset() { s_assetDictionaries.clear(); }
		};
	};

	namespace actor
	{
		class ShaderAttachment
		{
		protected:
			std::shared_ptr<vk_interface::graphic::Pipeline> m_ptrGraphicPipeline;
			std::shared_ptr<vk_interface::component::DescriptorSet> m_ptrDescriptorSet;
			std::unordered_map<std::string,
				std::array<std::shared_ptr<hephics_helper::UniformBuffer>, BUFFERING_FRAME_NUM>> m_uniformBuffersMap;

		public:
			ShaderAttachment()
			{
				m_ptrGraphicPipeline = std::make_shared<vk_interface::graphic::Pipeline>();
				m_ptrDescriptorSet = std::make_shared<vk_interface::component::DescriptorSet>();
			}
			~ShaderAttachment() {}

			auto& GetGraphicPipeline() { return m_ptrGraphicPipeline; }
			auto& GetDescriptorSet() { return m_ptrDescriptorSet; }
			auto& GetUniformBuffersMap() { return m_uniformBuffersMap; }
		};

		struct Position
		{
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 projection;
		};

		class Component
		{
		protected:
			std::shared_ptr<ShaderAttachment> m_ptrShaderAttachment;
			bool m_visible = false;

			virtual void LoadData() {}
			virtual void SetPipeline() {}

		public:
			Component() : m_visible(true)
			{
				m_ptrShaderAttachment = std::make_shared<ShaderAttachment>();
			}
			~Component() {}

			void SetVisible(const bool& is_visible) { m_visible = is_visible; }
			const auto& IsVisible() const { return m_visible; }

			virtual void Initialize() {}
			virtual void Update(class Actor* const owner) {}
			virtual void Render() {}
		};

		class Actor
		{
		protected:
			std::shared_ptr<ShaderAttachment> m_ptrShaderAttachment;
			std::vector<std::shared_ptr<Component>> m_components;
			std::shared_ptr<Position> m_ptrPosition;
			bool m_visible = false;

			virtual void LoadData() {}
			virtual void SetPipeline() {}

		public:
			Actor() : m_visible(true)
			{
				m_ptrShaderAttachment = std::make_shared<ShaderAttachment>();
				m_ptrPosition = std::make_shared<Position>();
			}
			~Actor() {}

			void SetVisible(const bool& is_visible) { m_visible = is_visible; }
			const auto& IsVisible() const { return m_visible; }

			virtual void Initialize() {}
			virtual void Update() {}
			virtual void Render() {}

			auto& GetPosition() { return m_ptrPosition; }
		};
	};

	class Scene
	{
	protected:
		static std::vector<std::shared_ptr<hephics_helper::StagingBuffer>> s_stagingBuffers;

		std::vector<std::shared_ptr<actor::Actor>> m_actors;
		std::string m_sceneName;
		std::string m_nextSceneName;
		std::string m_windowTitle;
		bool m_isContinuous = true;
		bool m_isChangedScene = false;

		Scene() = delete;

	public:
		Scene(const std::string& scene_name) : m_sceneName(scene_name) {}
		~Scene() {}

		virtual void Initialize();
		virtual void Update();
		virtual void Render();

		const auto& IsContinuous() const { return m_isContinuous; }
		const auto& IsChangedScene() const { return m_isChangedScene; }
		const auto& GetNextSceneName() const { return m_nextSceneName; }
		const auto& GetWindowTitle() const { return m_windowTitle; }
		static auto& GetStagingBuffers() { return s_stagingBuffers; }

		static void ResetScene();

		void WriteScreenImage() const;
	};

	class App
	{
	protected:
		std::unordered_map<std::string, std::shared_ptr<hephics::Scene>> m_sceneTempTree;
		std::unordered_map<std::string, std::function<std::shared_ptr<hephics::Scene>()>> m_sceneDictionary;
		std::shared_ptr<hephics::Scene> m_ptrCurrentScene;

	public:
		App() = default;
		~App() {}

		virtual void Initialize() {}

		virtual void Run() {}
	};

	class GPUHandler
	{
	private:
		static std::unordered_map<std::string, size_t> s_graphicPurposeDictionary;
		static std::shared_ptr<VkInstance> s_ptrGPUInstance;

		GPUHandler() = delete;
		~GPUHandler() = delete;

	public:
		static void Shutdown()
		{
			s_graphicPurposeDictionary.clear();
			Scene::ResetScene();
			s_ptrGPUInstance = nullptr;
		}

		static void AddPurpose(const std::vector<std::string>& purpose_list);

		static void InitializeInstance();

		static auto& GetInstance() { return s_ptrGPUInstance; }

		static void WaitIdle() { s_ptrGPUInstance->GetLogicalDevice()->waitIdle(); }

		static const size_t& GetPurposeIdx(const std::string& purpose);

		static const size_t GetPurposeNumber();
	};

	class Engine
	{
	protected:
		Engine() = delete;

	public:
		static void Invoke(std::unique_ptr<App>&& ptr_app)
		{
			ptr_app->Initialize();
			ptr_app->Run();

			GPUHandler::WaitIdle();
			ptr_app = nullptr;
		}

		static void Shutdown()
		{
			GPUHandler::Shutdown();
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			window::Manager::Shutdown();
		}
	};
};