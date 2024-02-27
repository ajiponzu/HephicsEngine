#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_USE_DOUBLE
#include <tiny_obj_loader.h>

#include <opencv2/opencv.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <memory>
#include <exception>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <set>
#include <functional>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <array>
#include <variant>
#include <optional>
#include <random>
#include <numbers>
