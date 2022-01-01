#pragma once

#include <string>

#include <glad/glad.h>

namespace GLCore::Utils {

	GLuint CreateShader(const std::string& source);

}