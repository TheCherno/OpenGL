#include <glpch.h>

#include "TextureUtils.h"

#include <stb_image_write.h>

namespace GLCore::Utils {

    static std::string ToLower(std::string str)
    {
        for (auto& c : str)
            c = (char)std::tolower((int)c);

        return str;
    }

	bool ExportTexture(GLuint textureID, const std::string& filename)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);

		int width, height;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		if (width > 0 && height > 0)
		{
			BYTE* pixels = new BYTE[width * height * 3];

			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			stbi_flip_vertically_on_write(1);

            // Code from SFML
            // Extract the extension
            const std::size_t dot = filename.find_last_of('.');
            const std::string extension = dot != std::string::npos ? ToLower(filename.substr(dot + 1)) : "";

            if (extension == "bmp")
            {
                if (stbi_write_bmp(filename.c_str(), width, height, 3, pixels))
                    return true;
            }
            else if (extension == "tga")
            {
                if (stbi_write_tga(filename.c_str(), width, height, 3, pixels))
                    return true;
            }
            else if (extension == "png")
            {
                if (stbi_write_png(filename.c_str(), width, height, 3, pixels, 0))
                    return true;
            }
            else if (extension == "jpg" || extension == "jpeg")
            {
                if (stbi_write_jpg(filename.c_str(), width, height, 3, &pixels[0], 90))
                    return true;
            }

			delete[] pixels;
		}
        return false;
	}

}
