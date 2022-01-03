#include <glpch.h>

#include "TextureUtils.h"

#include <stb_image_write.h>

namespace GLCore::Utils {

	void ExportTexture(GLuint textureID, const std::string& path)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);

		int width, height;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		BYTE* pixels = new BYTE[width * height * 3];

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		stbi_flip_vertically_on_write(1);
		stbi_write_png(path.c_str(), width, height, 3, pixels, width * 3);

		delete[] pixels;
	}

}
