#include "glpch.h"
#include "Shader.h"

#include <fstream>

namespace GLCore::Utils {

	GLuint CreateShader(const std::string& source)
	{
		GLuint program = glCreateProgram();

		GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
		const char* src = source.c_str();

		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

			glDeleteShader(shader);

			LOG_ERROR("{0}", infoLog.data());
		}

		glAttachShader(program, shader);

		// Link
		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram(program);

			glDeleteShader(shader);

			LOG_ERROR("{0}", infoLog.data());
		}

		glDetachShader(program, shader);
		glDeleteShader(shader);

		return program;
	}

}