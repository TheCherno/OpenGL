#pragma once

#include <string>

#include <glad/glad.h>

namespace GLCore::Utils {

	class Shader
	{
	public:
		~Shader();

		GLuint GetRendererID() { return m_RendererID; }

		static Shader* FromGLSLTextFiles(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	private:
		Shader() = default;

		void LoadFromGLSLTextFiles(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
		GLuint CompileShader(GLenum type, const std::string& source);
	private:
		GLuint m_RendererID;
	};

}