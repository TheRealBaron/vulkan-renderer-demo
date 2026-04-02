#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace animate {
	glm::mat4x4 drammaticMovement(float time);
	glm::mat4x4 drammaticRotation(float time);
}

