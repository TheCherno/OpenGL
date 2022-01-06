#include "FractalVisualizer.h"

#include <fstream>
#include <filesystem>

static double map(const double& value, const double& inputMin, const double& inputMax, const double& outputMin, const double& outputMax)
{
	return outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (value - inputMin);
}


FractalVisualizer::FractalVisualizer(const std::string& shaderSrcPath)
{
	SetShader(shaderSrcPath);
}

FractalVisualizer::~FractalVisualizer()
{
	DeleteFramebuffer();
}

void FractalVisualizer::Update()
{
	if (m_ShouldCreateFramebuffer)
	{
		m_ShouldCreateFramebuffer = false; 

		DeleteFramebuffer();
		CreateFramebuffer();
		ResetRender();
	}

	// Shader uniforms
	glUseProgram(m_Shader);
	GLint location;

	for (const auto& u : m_ColorFunction->GetUniforms())
	{
		location = glGetUniformLocation(m_Shader, u.name.c_str());
		glUniform1f(location, u.val);
	}

	location = glGetUniformLocation(m_Shader, "i_Size");
	glUniform2ui(location, m_Size.x, m_Size.y);

	location = glGetUniformLocation(m_Shader, "i_ItersPerFrame");
	glUniform1ui(location, m_IterationsPerFrame);

	location = glGetUniformLocation(m_Shader, "i_Frame");
	glUniform1ui(location, m_Frame);

	auto [xRange, yRange] = GetRange();

	location = glGetUniformLocation(m_Shader, "i_xRange");
	glUniform2d(location, xRange.x, xRange.y);

	location = glGetUniformLocation(m_Shader, "i_yRange");
	glUniform2d(location, yRange.x, yRange.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_InData);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_InIter);

	// Draw
	glViewport(0, 0, m_Size.x, m_Size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (m_Frame == 0)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glBindVertexArray(GLCore::Application::GetDefaultQuadVA());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);


	// Copy output buffers into input buffers
	glCopyImageSubData(
		m_OutData, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_InData, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_Size.x, m_Size.y, 1);

	glCopyImageSubData(
		m_OutIter, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_InIter, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_Size.x, m_Size.y, 1);
	
	m_Frame++;
}

void FractalVisualizer::SetCenter(const glm::dvec2& center)
{
	m_Center = center;
	ResetRender();
}

void FractalVisualizer::SetRadius(double radius)
{
	m_Radius = radius;
	ResetRender();
}

void FractalVisualizer::SetSize(const glm::uvec2& size)
{
	m_Size = size;
	m_ShouldCreateFramebuffer = true;
}

void FractalVisualizer::SetShader(const std::string& shaderSrcPath)
{
	std::ifstream file(shaderSrcPath);
	m_ShaderSrc = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	if (m_ColorFunction)
		SetColorFunction(m_ColorFunction);

	m_ShouldCreateFramebuffer = true;
}

void FractalVisualizer::SetColorFunction(ColorFunction* const colorFunc)
{
	m_ColorFunction = colorFunc;
	
	//if (m_ColorFunction == nullptr)
	//	return;

	std::string source = m_ShaderSrc;
	size_t color_loc = source.find("#color");
	if (color_loc == std::string::npos)
	{
		std::cout << "ERROR: Shader does not have '#color'";
		exit(EXIT_FAILURE);
	}
	source.erase(color_loc, 6);
	source.insert(color_loc, m_ColorFunction->GetSource());

	if (m_Shader)
		glDeleteProgram(m_Shader);

	m_Shader = GLCore::Utils::CreateShader(source);
	glUseProgram(m_Shader);

	for (const auto& u : m_ColorFunction->GetUniforms())
	{
		GLint loc = glGetUniformLocation(m_Shader, u.name.c_str());
		glUniform1f(loc, u.val);
	}

	int location = glGetUniformLocation(m_Shader, "i_Data");
	glUniform1i(location, 0);

	location = glGetUniformLocation(m_Shader, "i_Iter");
	glUniform1i(location, 1);

	ResetRender();
}

void FractalVisualizer::SetIterationsPerFrame(int iterationsPerFrame)
{
	m_IterationsPerFrame = iterationsPerFrame;
	ResetRender();
}

void FractalVisualizer::ResetRender()
{
	m_Frame = 0;
}

std::pair<glm::dvec2, glm::dvec2> FractalVisualizer::GetRange() const
{
	double aspect = (double)m_Size.x / (double)m_Size.y;
	return 
	{
		{ m_Center.x - aspect * m_Radius, m_Center.x + aspect * m_Radius },
		{ m_Center.y - m_Radius, m_Center.y + m_Radius }
	};
}

ImVec2 FractalVisualizer::MapPosToCoords(const glm::dvec2& pos) const
{
	auto [xRange, yRange] = GetRange();
	return
	{
		(float)map(pos.x, xRange.x, xRange.y, 0, m_Size.x),
		(float)map(pos.y, yRange.x, yRange.y, m_Size.y, 0)
	};
}

glm::dvec2 FractalVisualizer::MapCoordsToPos(const ImVec2& coords) const
{
	auto [xRange, yRange] = GetRange();
	return
	{
		map(coords.x, 0, m_Size.x, xRange.x, xRange.y),
		map(coords.y, m_Size.y, 0, yRange.x, yRange.y)
	};
}

void FractalVisualizer::DeleteFramebuffer()
{
	glDeleteFramebuffers(1, &m_FrameBuffer);

	GLuint textures[] = { m_Texture, m_InData, m_OutData, m_InIter, m_OutIter };
	glDeleteTextures(IM_ARRAYSIZE(textures), textures);
}

void FractalVisualizer::CreateFramebuffer()
{
	// Framebuffer
	glGenFramebuffers(1, &m_FrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_Size.x, m_Size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Texture, 0);

	// Out Data
	glGenTextures(1, &m_OutData);
	glBindTexture(GL_TEXTURE_2D, m_OutData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, m_Size.x, m_Size.y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_OutData, 0);

	// In Data
	glGenTextures(1, &m_InData);
	glBindTexture(GL_TEXTURE_2D, m_InData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, m_Size.x, m_Size.y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Out Iter
	glGenTextures(1, &m_OutIter);
	glBindTexture(GL_TEXTURE_2D, m_OutIter);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_Size.x, m_Size.y, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_OutIter, 0);

	// In Data
	glGenTextures(1, &m_InIter);
	glBindTexture(GL_TEXTURE_2D, m_InIter);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_Size.x, m_Size.y, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, bufs);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Error creating framebuffer (" << m_Size.x << ", " << m_Size.y << ")\n";
	}
}
