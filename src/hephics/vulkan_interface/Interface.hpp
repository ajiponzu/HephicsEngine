#pragma once

template <>
struct std::hash<vk::QueueFlags>
{
	std::size_t operator()(const vk::QueueFlags& flags) const
	{
		return std::hash<uint32_t>()(static_cast<uint32_t>(flags));
	}
};

namespace vk_interface
{
	namespace component
	{
		using QueueFamilyArray = std::array<uint32_t, 2>;
		struct QueueFamilyIndices
		{
		public:
			QueueFamilyIndices()
			{
				family_array = QueueFamilyArray();
			}
			~QueueFamilyIndices() {}

			std::optional<uint32_t> graphics_and_compute_family;
			std::optional<uint32_t> present_family;

			QueueFamilyArray family_array{};

			bool is_complete() const
			{
				return graphics_and_compute_family.has_value() && present_family.has_value();
			}

			bool is_equal_families() const
			{
				return graphics_and_compute_family.value() == present_family.value();
			}

			const QueueFamilyArray& get_families_array() const
			{
				return family_array;
			}
		};

		struct SwapChainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> present_modes;
		};

		class DescriptorSet
		{
		protected:
			vk::UniqueDescriptorPool m_descriptorPool;
			vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
			std::vector<vk::UniqueDescriptorSet> m_descriptorSets;

		public:
			DescriptorSet() = default;
			~DescriptorSet() {}

			DescriptorSet(DescriptorSet&& other) noexcept
			{
				m_descriptorSetLayout = std::move(other.m_descriptorSetLayout);
				m_descriptorPool = std::move(other.m_descriptorPool);
			}

			DescriptorSet& operator=(DescriptorSet&& other) noexcept
			{
				m_descriptorSetLayout = std::move(other.m_descriptorSetLayout);
				m_descriptorPool = std::move(other.m_descriptorPool);
			}

			void SetDescriptorSetLayout(const vk::UniqueDevice& logical_device,
				const std::vector<vk::DescriptorSetLayoutBinding>& bindings);

			void SetDescriptorPool(const vk::UniqueDevice& logical_device,
				const vk::DescriptorPoolCreateInfo& create_info);

			void SetDescriptorSet(const vk::UniqueDevice& logical_device, const uint8_t& concurrent_frame_num);

			void UpdateDescriptorSet(const vk::UniqueDevice& logical_device, const size_t& target_idx,
				std::vector<vk::WriteDescriptorSet>&& write_descriptor_sets);

			const auto& GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
			const auto& GetDescriptorSetPool() const { return m_descriptorPool; }
			const auto& GetDescriptorSet(const size_t& target_idx) const { return m_descriptorSets.at(target_idx); }
		};

		class Buffer
		{
		protected:
			vk::UniqueDeviceMemory m_memory;
			vk::UniqueBuffer m_buffer;
			vk::DeviceSize m_size{};

		public:
			Buffer() = default;
			~Buffer() {}

			Buffer(Buffer&& other) noexcept
			{
				m_buffer = std::move(other.m_buffer);
				m_memory = std::move(other.m_memory);
			}

			Buffer& operator=(Buffer&& other) noexcept
			{
				m_buffer = std::move(other.m_buffer);
				m_memory = std::move(other.m_memory);
			}

			void SetBuffer(const vk::UniqueDevice& logical_device, const vk::BufferCreateInfo& create_info);

			void SetMemory(const vk::UniqueDevice& logical_device, const vk::MemoryAllocateInfo& allocate_info);

			const auto& GetBuffer() const { return m_buffer; }

			auto GetMemoryRequirements(const vk::UniqueDevice& logical_device) const
			{
				return logical_device->getBufferMemoryRequirements(m_buffer.get());
			}

			const auto& GetSize() const { return m_size; }

			void CopyBufferMemoryData(const vk::UniqueDevice& logical_device, const void* src_address);

			void* Mapping(const vk::UniqueDevice& logical_device);

			void Unmapping(const vk::UniqueDevice& logical_device);

			void BindMemory(const vk::UniqueDevice& logical_device);
		};

		class Shader
		{
		protected:
			vk::UniqueShaderModule m_module;

		public:
			Shader() = default;
			~Shader() {}

			Shader(Shader&& other) noexcept
			{
				m_module = std::move(other.m_module);
			}

			Shader& operator=(Shader&& other) noexcept
			{
				m_module = std::move(other.m_module);
			}

			void SetModule(const vk::UniqueDevice& logical_device, const vk::ShaderModuleCreateInfo& create_info);

			const auto& GetModule() const { return m_module; }
		};

		// static
		class ShaderProvider
		{
		private:
			static std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<Shader>>> s_shaderDictionary;

			ShaderProvider() = delete;
			~ShaderProvider() = delete;

		public:
			static void AddShader(const vk::UniqueDevice& logical_device,
				const std::string& shader_code_path, const std::string& shader_key);

			static const std::shared_ptr<Shader>& GetShader(
				const std::string& shader_type_key, const std::string& shader_key);

			static void Reset() { s_shaderDictionary.clear(); }
		};

		class Image
		{
		protected:
			vk::UniqueDeviceMemory m_memory;
			vk::UniqueImage m_image;
			vk::UniqueImageView m_view;

		public:
			Image() = default;
			~Image() {}

			Image(Image&& other) noexcept
			{
				m_image = std::move(other.m_image);
				m_memory = std::move(other.m_memory);
				m_view = std::move(other.m_view);
			}

			Image& operator=(Image&& other) noexcept
			{
				m_image = std::move(other.m_image);
				m_memory = std::move(other.m_memory);
				m_view = std::move(other.m_view);
			}

			void SetImage(const vk::UniqueDevice& logical_device, const vk::ImageCreateInfo& create_info);

			void SetMemory(const vk::UniqueDevice& logical_device, const vk::MemoryAllocateInfo& allocate_info);

			void BindMemory(const vk::UniqueDevice& logical_device);

			void SetImageView(const vk::UniqueDevice& logical_device, const vk::ImageViewCreateInfo& create_info);

			auto GetMemoryRequirements(const vk::UniqueDevice& logical_device) const
			{
				return logical_device->getImageMemoryRequirements(m_image.get());
			}

			void Clear(const vk::UniqueDevice& logical_device);

			const auto& GetImage() const { return m_image; }
			const auto& GetView() const { return m_view; }
		};

		template<typename T>
		class Pipeline
		{
		protected:
			vk::UniquePipeline m_pipeline;
			vk::UniquePipelineLayout m_layout;

		public:
			Pipeline() = default;
			~Pipeline() {}

			Pipeline(Pipeline&& other) noexcept
			{
				m_pipeline = std::move(other.m_pipeline);
				m_layout = std::move(other.m_layout);
			}

			Pipeline& operator=(Pipeline&& other) noexcept
			{
				m_pipeline = std::move(other.m_pipeline);
				m_layout = std::move(other.m_layout);
			}

			virtual void SetPipeline(const vk::UniqueDevice& logical_device, const T& create_info) = 0;

			virtual void SetLayout(const vk::UniqueDevice& logical_device, const vk::PipelineLayoutCreateInfo& create_info)
			{
				m_layout = logical_device->createPipelineLayoutUnique(create_info);
			}

			const auto& GetPipeline() const { return m_pipeline; }

			const auto& GetLayout() const { return m_layout; }
		};

		class Fence
		{
		protected:
			vk::UniqueFence m_fence;

		public:
			Fence() = default;
			~Fence() {}

			Fence(Fence&& other) noexcept
			{
				m_fence = std::move(other.m_fence);
			}

			Fence& operator=(Fence&& other) noexcept
			{
				m_fence = std::move(other.m_fence);
			}

			void SetFence(const vk::UniqueDevice& logical_device, const vk::FenceCreateInfo& create_info);

			auto& GetFence() const { return m_fence; }

			void Signal(const vk::UniqueDevice& logical_device);

			void Wait(const vk::UniqueDevice& logical_device, const uint64_t& timeout);
		};

		class SwapChain
		{
		protected:
			vk::UniqueSwapchainKHR m_swapChain;
			std::vector<vk::Image> m_images;
			std::vector<vk::UniqueImageView> m_imageViews;
			std::vector<vk::UniqueFramebuffer> m_framebuffers;
			std::shared_ptr<Image> m_ptrDepthImage;
			std::shared_ptr<Image> m_ptrColorImage;
			vk::UniqueRenderPass m_renderPass;
			vk::Format m_imageFormat{};
			vk::Extent2D m_extent;
			std::vector<vk::UniqueSemaphore> m_imageAvailableSemaphores;
			std::vector<vk::UniqueSemaphore> m_finishedSemaphores;
			std::vector<std::shared_ptr<Fence>> m_swapFences;
			std::vector<vk::Fence> m_tempFences;
			uint32_t m_bufferingNum = 0U;
			uint32_t m_currentFrameId = 0U;
			uint32_t m_nextImageId = 0U;

		public:
			SwapChain()
			{
				m_ptrDepthImage = std::make_shared<Image>();
				m_ptrColorImage = std::make_shared<Image>();
			}
			~SwapChain() {}

			SwapChain(SwapChain&& other) noexcept
			{
				m_swapChain = std::move(other.m_swapChain);
				m_renderPass = std::move(other.m_renderPass);
				m_ptrDepthImage = other.m_ptrDepthImage;
				m_ptrColorImage = other.m_ptrColorImage;
			}

			SwapChain& operator=(SwapChain&& other) noexcept
			{
				m_swapChain = std::move(other.m_swapChain);
				m_renderPass = std::move(other.m_renderPass);
				m_ptrDepthImage = other.m_ptrDepthImage;
				m_ptrColorImage = other.m_ptrColorImage;
			}

			void SetSwapChain(const vk::UniqueDevice& logical_device, const vk::SwapchainCreateInfoKHR& create_info);

			void SetImageViews(const vk::UniqueDevice& logical_device, const vk::ImageViewCreateInfo& create_info);

			void SetRenderPass(const vk::UniqueDevice& logical_device,
				const vk::RenderPassCreateInfo& create_info);

			void SetFramebuffers(const vk::UniqueDevice& logical_device, const vk::FramebufferCreateInfo& create_info);

			void SetSyncObjects(const vk::UniqueDevice& logical_device, const int32_t& buffering_num);

			const auto& GetFramebuffers() const { return m_framebuffers; }

			const auto& GetImageFormat() const { return m_imageFormat; }

			const auto& GetExtent2D() const { return m_extent; }

			auto& GetDepthImage() { return m_ptrDepthImage; }
			const auto& GetDepthImage() const { return m_ptrDepthImage; }

			auto& GetColorImage() { return m_ptrColorImage; }
			const auto& GetColorImage() const { return m_ptrColorImage; }

			const auto& GetRenderPass() const { return m_renderPass; }

			const auto& GetBufferingNum() const { return m_bufferingNum; }

			const auto& GetCurrentFrameId() const { return m_currentFrameId; }

			const auto& GetCurrentImage() const { return m_images.at(m_currentFrameId); }

			const auto& GetNextImageId() const { return m_nextImageId; }

			const auto& GetCurrentSwapFence() const { return m_swapFences.at(m_currentFrameId); }

			const auto& GetCurrentImageAvailableSemaphore() const
			{
				return m_imageAvailableSemaphores.at(m_currentFrameId);
			}

			const auto& GetCurrentFinishedSemaphore() const
			{
				return m_finishedSemaphores.at(m_currentFrameId);
			}

			void AcquireNextImageIdx(const vk::UniqueDevice& logical_device);

			void WaitFence(const vk::UniqueDevice& logical_device);

			void CancelWaitFence(const vk::UniqueDevice& logical_device);

			void PrepareNextFrame();

			void Clear(const vk::UniqueDevice& logical_device);

			vk::RenderPassBeginInfo GetRenderPassBeginInfo(const std::vector<vk::ClearValue>& clear_values) const;

			std::pair<vk::Viewport, vk::Rect2D> GetViewportAndScissor() const;

			vk::SubmitInfo GetRenderingSubmitInfo(
				const std::vector<vk::CommandBuffer>& submitted_command_buffers, const vk::PipelineStageFlags& wait_stage_flags) const;

			vk::PresentInfoKHR GetPresentInfo() const;
		};

		class CommandBuffer
		{
		protected:
			vk::UniqueCommandBuffer m_commandBuffer;

		public:
			CommandBuffer() = default;
			~CommandBuffer() {}

			CommandBuffer(CommandBuffer&& other) noexcept
			{
				m_commandBuffer = std::move(other.m_commandBuffer);
			}

			CommandBuffer& operator=(CommandBuffer&& other) noexcept
			{
				m_commandBuffer = std::move(other.m_commandBuffer);
			}

			virtual void BeginRecordingCommands(const vk::CommandBufferBeginInfo& begin_info);

			virtual void EndRecordingCommands();

			virtual void ResetCommands(const vk::CommandBufferResetFlags& reset_flag);

			void BeginRenderPass(const std::shared_ptr<SwapChain>& swap_chain, const vk::SubpassContents& subpass_contents);

			void EndRenderPass();

			void SetViewportAndScissor(const std::shared_ptr<SwapChain>& swap_chain);

			void TransitionImageCommandLayout(const vk::Image& vk_image, const vk::Format& vk_format,
				const std::pair<vk::ImageLayout, vk::ImageLayout>& transition_layout_pair, const uint32_t& miplevel);

			void TransitionImageCommandLayout(const std::shared_ptr<Image>& vk_image, const vk::Format& vk_format,
				const std::pair<vk::ImageLayout, vk::ImageLayout>& transition_layout_pair, const uint32_t& miplevel);

			void CopyBuffer(const std::shared_ptr<Buffer>& src_buffer,
				const std::shared_ptr<Buffer>& dst_buffer, const size_t& device_size);

			void CopyTexture(const std::shared_ptr<Buffer>& staging_buffer,
				const std::shared_ptr<Image>& texture_image, const vk::Extent2D& extent);

			void SetCommandBuffer(std::vector<vk::UniqueCommandBuffer>&& command_buffers);

			auto& GetCommandBuffer() { return m_commandBuffer; }
		};
	};

	namespace graphic
	{
		class Pipeline : public component::Pipeline<vk::GraphicsPipelineCreateInfo>
		{
		protected:

		public:
			Pipeline() = default;
			~Pipeline() {}

			virtual void SetPipeline(const vk::UniqueDevice& logical_device,
				const vk::GraphicsPipelineCreateInfo& create_info) override;
		};
	};

	namespace compute
	{
		class Pipeline : public component::Pipeline<vk::ComputePipelineCreateInfo>
		{
		protected:

		public:
			Pipeline() = default;
			~Pipeline() {}

			virtual void SetPipeline(const vk::UniqueDevice& logical_device,
				const vk::ComputePipelineCreateInfo& create_info) override;
		};
	};

	namespace ray_tracing
	{
		class Pipeline : public component::Pipeline<vk::RayTracingPipelineCreateInfoKHR>
		{
		protected:

		public:
			Pipeline() = default;
			~Pipeline() {}

			virtual void SetPipeline(const vk::UniqueDevice& logical_device,
				const vk::RayTracingPipelineCreateInfoKHR& create_info) override;
		};
	};

	class Instance
	{
	protected:
		vk::UniqueInstance m_instance;
#ifdef _DEBUG
		vk::UniqueDebugUtilsMessengerEXT m_debugUtilsMessenger;
#endif
		vk::UniqueSurfaceKHR m_windowSurface;
		vk::PhysicalDevice m_physicalDevice;
		vk::UniqueDevice m_logicalDevice;
		std::unordered_map<vk::QueueFlags, std::unordered_map<std::string, vk::Queue>>
			m_queuesDictionary;
		std::shared_ptr<component::SwapChain> m_ptrSwapChain;
		std::unordered_map<vk::QueueFlags, std::vector<std::unordered_map<std::string, vk::UniqueCommandPool>>>
			m_commandPoolsDictionary;
		component::QueueFamilyIndices m_queueFamilyIndices;

	public:
		Instance()
		{
			m_ptrSwapChain = std::make_shared<component::SwapChain>();
		}
		~Instance() {}

		Instance& operator=(Instance&& other) noexcept
		{
#ifdef _DEBUG
			m_debugUtilsMessenger = std::move(other.m_debugUtilsMessenger);
#endif

			m_instance = std::move(other.m_instance);
			m_windowSurface = std::move(other.m_windowSurface);
			m_logicalDevice = std::move(other.m_logicalDevice);
		}

		const auto& GetLogicalDevice() const { return m_logicalDevice; }
		const auto& GetSwapChain() const { return m_ptrSwapChain; }
		const auto& GetPhysicalDevice() const { return m_physicalDevice; }
		const auto& GetWindowSurface() const { return m_windowSurface; }
		const auto& GetQueueFamilyIndices() const { return m_queueFamilyIndices; }
		};
	};