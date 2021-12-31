#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

#include <vector>
#include <sstream>

GLuint CreateShader(const std::string& source);

class ColorFunction
{
public:
	struct Uniform
	{
		std::string name;
		glm::vec2 range;
		float default_val;
		float val;

		Uniform(const std::string& name, const glm::vec2& range, float default_val)
			: name(name), range(range), default_val(default_val), val(default_val) {}
	};

private:
	std::vector<Uniform> m_uniforms;
	std::string m_src;

public:
	ImTextureID preview;

	ColorFunction(const std::string& src) : m_src(src) 
	{
		m_src = src;
		for (size_t start; (start = m_src.find("#uniform ")) != std::string::npos;)
		{
			size_t end = m_src.find(';');
			std::stringstream ss(m_src.substr(start + 9, end - start - 9));

			std::string name, min_s, max_s;
			float val;
			ss >> name >> val >> min_s >> max_s;

			glm::vec2 range;
			range.x = (min_s == "NULL" ? FLT_MIN : std::stof(min_s));
			range.y = (max_s == "NULL" ? FLT_MAX : std::stof(max_s));
			
			std::stringstream uniform;
			uniform << "uniform float " << name;

			m_src.erase(start, end - start);
			m_src.insert(start, uniform.str());

			m_uniforms.emplace_back(name, range, val);
		}
		
		glm::uvec2 previewSize = { 100, 50 };
		{
			GLuint fb;
			glGenFramebuffers(1, &fb);
			glBindFramebuffer(GL_FRAMEBUFFER, fb);

			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, previewSize.x, previewSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, buffers);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				std::cout << "NOT TOTOTOTOTOT\n";
				exit(EXIT_FAILURE);
			}

			glViewport(0, 0, previewSize.x, previewSize.y);

			std::stringstream ss;
			ss << "#version 400\n\n";
			ss << m_src << '\n';
			ss << R"(
layout (location = 0) out vec3 outColor;

uniform uint range;
uniform uvec2 size;

void main()
{
    int i = int((gl_FragCoord.x / size.x) * range);
    outColor = get_color(i);
}
		)";

			GLuint shader = CreateShader(ss.str());
			glUseProgram(shader);

			GLint loc = glGetUniformLocation(shader, "range");
			glUniform1ui(loc, 1000);

			loc = glGetUniformLocation(shader, "size");
			glUniform2ui(loc, previewSize.x, previewSize.y);

			for (const auto& u : m_uniforms)
			{
				loc = glGetUniformLocation(shader, u.name.c_str());
				glUniform1f(loc, u.default_val);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, fb);

			GLuint quadVA;
			glCreateVertexArrays(1, &quadVA);
			glBindVertexArray(quadVA);

			float vertices[] = {
				-1.0f, -1.0f,
				 1.0f, -1.0f,
				 1.0f,  1.0f,
				-1.0f,  1.0f
			};

			GLuint quadVB;
			glCreateBuffers(1, &quadVB);
			glBindBuffer(GL_ARRAY_BUFFER, quadVB);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

			uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
			GLuint quadIB;
			glCreateBuffers(1, &quadIB);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIB);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			glBindVertexArray(quadVA);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

			glDeleteVertexArrays(1, &quadVA);
			glDeleteBuffers(1, &quadVB);
			glDeleteBuffers(1, &quadIB);

			glDeleteFramebuffers(1, &fb);
			glDeleteProgram(shader);

			preview = (ImTextureID)(intptr_t)tex; // First cast to 'intptr_t' to avoid warning
		}
	}

	std::vector<Uniform>& GetUniforms()
	{
		return m_uniforms;
	}

	const std::vector<Uniform>& GetUniforms() const
	{
		return m_uniforms;
	}

	const std::string& GetSource() const
	{
		return m_src;
	}
};

class FractalLayer : public GLCore::Layer
{
public:
	FractalLayer();
	virtual ~FractalLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
private:

	glm::dvec2 MapPosToCoords(const glm::dvec2& pos);
	glm::dvec2 MapCoordsToPos(const glm::dvec2& coords);

	void UpdateRange();

	void CreateFrameBuffer();

	void SetColorFunc(const ColorFunction& colorFunc);

	float frame_rate;

	std::vector<ColorFunction> m_colors;
	size_t m_selectedColor = 0;

	std::string m_coreShaderSrc;
	GLuint m_Shader = 0;

	GLuint m_QuadVA, m_QuadVB, m_QuadIB;

	GLuint m_FrameBuffer, m_Texture;
	GLuint m_InData, m_OutData, m_InIter, m_OutIter;

	glm::uvec2 m_size;
	glm::dvec2 m_center;
	double m_radius;
	int m_itersPerFrame;
	int m_frame;

	glm::dvec2 m_xRange;
	glm::dvec2 m_yRange;

	bool m_mousePressed = false;
	glm::dvec2 m_startPos;
};