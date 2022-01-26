#pragma once

#include <string>
#include <glad/glad.h>

namespace GLCore::Utils {

	bool ExportTexture(GLuint textureID, const std::string& path, bool ignore_alpha = false);

}
