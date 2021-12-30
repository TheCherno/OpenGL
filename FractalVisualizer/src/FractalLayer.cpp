#include "FractalLayer.h"

#include <GLCore/Core/Input.h>
#include <GLCore/Core/KeyCodes.h>
#include <GLCore/Core/MouseButtonCodes.h>

#include <fstream>
#include <streambuf>
#include <iomanip>

using namespace GLCore;
using namespace GLCore::Utils;


static GLuint CreateShader(const std::string& source)
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

double map(const double& value, const double& inputMin, const double& inputMax, const double& outputMin, const double& outputMax)
{
	return outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (value - inputMin);
}

glm::dvec2 FractalLayer::MapPosToCoords(const glm::dvec2& pos)
{
	return
	{
		map(pos.x, m_xRange.x, m_xRange.y, 0, m_size.x),
		map(pos.y, m_yRange.x, m_yRange.y, m_size.y, 0)
	};
}

glm::dvec2 FractalLayer::MapCoordsToPos(const glm::dvec2& coords)
{
	return
	{
		map(coords.x, 0, m_size.x, m_xRange.x, m_xRange.y),
		map(coords.y, m_size.y, 0, m_yRange.x, m_yRange.y)
	};
}

void FractalLayer::UpdateRange()
{
	double aspect = (double)m_size.x / (double)m_size.y;

	m_yRange = { m_center.y - m_radius, m_center.y + m_radius };
	m_xRange = { m_center.x - aspect * m_radius, m_center.x + aspect * m_radius };

	glUseProgram(m_Shader);

	int location = glGetUniformLocation(m_Shader, "xRange");
	glUniform2d(location, m_xRange.x, m_xRange.y);

	location = glGetUniformLocation(m_Shader, "yRange");
	glUniform2d(location, m_yRange.x, m_yRange.y);

	m_frame = 0;
}

void FractalLayer::CreateFrameBuffer()
{
	glDeleteFramebuffers(1, &m_FrameBuffer);
	glDeleteTextures(1, &m_Texture);

	glGenFramebuffers(1, &m_FrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Texture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Error creating framebuffer\n";
	}

	// Out Data
	glGenTextures(1, &m_OutData);
	glBindTexture(GL_TEXTURE_2D, m_OutData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, m_size.x, m_size.y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_OutData, 0);


	// In Data
	glGenTextures(1, &m_InData);
	glBindTexture(GL_TEXTURE_2D, m_InData);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, m_size.x, m_size.y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	// Out Iter
	glGenTextures(1, &m_OutIter);
	glBindTexture(GL_TEXTURE_2D, m_OutIter);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_size.x, m_size.y, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_OutIter, 0);

	// In Data
	glGenTextures(1, &m_InIter);
	glBindTexture(GL_TEXTURE_2D, m_InIter);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_size.x, m_size.y, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, bufs);

	glViewport(0, 0, m_size.x, m_size.y);
}

void FractalLayer::SetColorFunc(const ColorFunction& colorFunc)
{
	std::stringstream ss;
	ss << "#version 400\n\n";
	for (const auto& u : colorFunc.GetUniforms())
		ss << "uniform float " << u.name << ";\n";

	ss << colorFunc.GetSource() << '\n';
	ss << m_coreShaderSrc;

	if (m_Shader)
		glDeleteProgram(m_Shader);

	m_Shader = CreateShader(ss.str());
	glUseProgram(m_Shader);

	for (const auto& u : colorFunc.GetUniforms())
	{
		GLint loc = glGetUniformLocation(m_Shader, u.name.c_str());
		glUniform1f(loc, u.val);
	}

	int location = glGetUniformLocation(m_Shader, "i_Data");
	glUniform1i(location, 0);

	location = glGetUniformLocation(m_Shader, "i_Iter");
	glUniform1i(location, 1);

	UpdateRange();
}

FractalLayer::FractalLayer()
	: m_radius(2)
	, m_center(0, 0)
	, m_itersPerFrame(100)
{
	m_size.x = Application::Get().GetWindow().GetWidth();
	m_size.y = Application::Get().GetWindow().GetHeight();

	std::ifstream file("assets/mandelbrot.glsl");
	m_coreShaderSrc = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	m_colors.push_back(ColorFunction(R"(
vec3 colors[] = vec3[](
	vec3(0, 0, 0),
	vec3(0.13, 0.142, 0.8),
	vec3(1, 1, 1),
	vec3(1, 0.667, 0),
	vec3(0, 0, 0)
);
vec3 get_color(int iters)
{
	float x  = iters / colorMult;
    x = mod(x, colors.length() - 1);

    if (x == colors.length())
        x = 0;

    if (floor(x) == x)
        return colors[int(x)];

    int i = int(floor(x));
    float t = mod(x, 1);
    return mix(colors[i], colors[i+1], t);
}
	)").AddUniform("colorMult", { 1, 300 }, 200));

	m_colors.push_back(ColorFunction(R"(
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 get_color(int i)
{
    return hsv2rgb(vec3(i / colorMult, 1, 1));
}
	)").AddUniform("colorMult", { 1.1f, 5000 }, 1000));

	m_colors.push_back(ColorFunction(R"(
vec3 get_color(int i)
{
    float t = exp(-i / colorMult);
    return mix(vec3(1, 1, 1), vec3(0.1, 0.1, 1), t);
}
	)").AddUniform("colorMult", { 1, 1000 }, 200));

	m_colors.push_back(ColorFunction(R"(
vec3 get_color(int i)
{
    float x = i / colorMult;
    float n1 = sin(x) * 0.5 + 0.5;
    float n2 = cos(x) * 0.5 + 0.5;
    return vec3(n1, n2, 1.0) * 1;
}
	)").AddUniform("colorMult", { 1, 300 }, 200));

	//m_center = { -0.74656412896773705068, 0.09886581010775417899 };
	//m_radius = 8.2212188006580699331e-12;
}

FractalLayer::~FractalLayer()
{

}

void FractalLayer::OnAttach()
{
	EnableGLDebugging();

	// Create default quad
	{
		glCreateVertexArrays(1, &m_QuadVA);
		glBindVertexArray(m_QuadVA);

		float vertices[] = {
			-1.0f, -1.0f,
			 1.0f, -1.0f,
			 1.0f,  1.0f,
			-1.0f,  1.0f
		};

		glCreateBuffers(1, &m_QuadVB);
		glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

		uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
		glCreateBuffers(1, &m_QuadIB);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_QuadIB);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	}

	// Create colors previews
	glm::uvec2 m_previewSize = { 100, 50 };
	for (auto& cf : m_colors)
	{
		GLuint fb;
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);

		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_previewSize.x, m_previewSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

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

		glViewport(0, 0, m_previewSize.x, m_previewSize.y);

		std::stringstream ss;

		ss << "#version 400\n\n";
		for (const auto& u : cf.GetUniforms())
			ss << "uniform float " << u.name << ";\n";

		ss << cf.GetSource() << '\n';
		ss << R"(
layout (location = 0) out vec3 outColor;

uniform uint range;
uniform uvec2 size;

void main()
{
    int i = int((gl_FragCoord.x / size.x) * range);
    outColor = get_color(i);
	//outColor = vec3(sin((gl_FragCoord.x / size.x) * range), 0, 0);
}
		)";

		GLuint shader = CreateShader(ss.str());
		glUseProgram(shader);

		GLint loc = glGetUniformLocation(shader, "range");
		glUniform1ui(loc, 1000);

		loc = glGetUniformLocation(shader, "size");
		glUniform2ui(loc, m_previewSize.x, m_previewSize.y);

		for (const auto& u : cf.GetUniforms())
		{
			loc = glGetUniformLocation(shader, u.name.c_str());
			glUniform1f(loc, u.default_val);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fb);

		glBindVertexArray(m_QuadVA);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		glDeleteFramebuffers(1, &fb);
		glDeleteProgram(shader);

		cf.preview = (ImTextureID)(intptr_t)tex; // Fist cast to 'intptr_t' to avoid warning
	}

	CreateFrameBuffer();

	SetColorFunc(m_colors[m_selectedColor]);

	glUseProgram(m_Shader);
	UpdateRange();
}

void FractalLayer::OnDetach()
{
	glDeleteVertexArrays(1, &m_QuadVA);
	glDeleteBuffers(1, &m_QuadVB);
	glDeleteBuffers(1, &m_QuadIB);
}

void FractalLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(
		[&](WindowResizeEvent& e)
		{
			m_size.x = e.GetWidth();
			m_size.y = e.GetHeight();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, m_size.x, m_size.y);

			glDeleteFramebuffers(1, &m_FrameBuffer);
			glDeleteTextures(1, &m_Texture);

			CreateFrameBuffer();

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				std::cout << "Error creating framebuffer\n";
			}

			UpdateRange();

			return false;
		}
	);
	dispatcher.Dispatch<MouseButtonPressedEvent>(
		[&](MouseButtonPressedEvent& e)
		{
			if (e.GetMouseButton() == HZ_MOUSE_BUTTON_LEFT)
			{
				m_mousePressed = true;
				m_startPos.x = Input::GetMouseX();
				m_startPos.y = Input::GetMouseY();
			}
			return false;
		}
	);
	dispatcher.Dispatch<MouseButtonReleasedEvent>(
		[&](MouseButtonReleasedEvent& e)
		{
			if (e.GetMouseButton() == HZ_MOUSE_BUTTON_LEFT)
			{
				m_mousePressed = false;
			}
			return false;
		}
	);
	dispatcher.Dispatch<MouseScrolledEvent>(
		[&](MouseScrolledEvent& e)
		{
			m_radius /= std::pow(1.1f, e.GetYOffset());

			// Zoom where the mouse is
			glm::dvec2 iMousePos = MapCoordsToPos({ Input::GetMouseX(), Input::GetMouseY() });

			UpdateRange();

			glm::dvec2 fMousePos = MapCoordsToPos({ Input::GetMouseX(), Input::GetMouseY() });

			glm::dvec2 delta = fMousePos - iMousePos;

			m_center -= delta;

			UpdateRange();

			return false;
		}
	);
}

void FractalLayer::OnUpdate(Timestep ts)
{
	frame_rate = 1 / ts.GetSeconds();

	if (m_mousePressed)
	{
		glm::dvec2 mousePos = MapCoordsToPos({ Input::GetMouseX(), Input::GetMouseY() });
		glm::dvec2 delta = MapCoordsToPos(m_startPos) - mousePos;

		if (glm::length(delta) > 0)
		{
			m_center += delta;

			m_startPos.x = Input::GetMouseX();
			m_startPos.y = Input::GetMouseY();

			UpdateRange();
		}
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (m_frame == 0)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glUseProgram(m_Shader);
	GLint location;

	for (const auto& u : m_colors[m_selectedColor].GetUniforms())
	{
		location = glGetUniformLocation(m_Shader, u.name.c_str());
		glUniform1f(location, u.val);
	}

	location = glGetUniformLocation(m_Shader, "size");
	glUniform2ui(location, m_size.x, m_size.y);

	location = glGetUniformLocation(m_Shader, "itersPerFrame");
	glUniform1ui(location, m_itersPerFrame);

	location = glGetUniformLocation(m_Shader, "frame");
	glUniform1ui(location, m_frame);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_InData);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_InIter);

	glBindVertexArray(m_QuadVA);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

	glCopyImageSubData(
		m_OutData, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_InData, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_size.x, m_size.y, 1);

	glCopyImageSubData(
		m_OutIter, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_InIter, GL_TEXTURE_2D, 0, 0, 0, 0,
		m_size.x, m_size.y, 1);

	// Screen
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBlitFramebuffer(0, 0, m_size.x, m_size.y, 0, 0, m_size.x, m_size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	m_frame++;
}

void FractalLayer::OnImGuiRender()
{
	//ImGui::ShowDemoWindow();

	ImGui::Begin("Controls");

	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << "General (" << frame_rate << "fps)";

	ImGui::Text(ss.str().c_str());

	if (ImGui::DragInt("Iterations per frame", (int*)&m_itersPerFrame, 10, 1, 2000))
	{
		if (m_itersPerFrame < 0) m_itersPerFrame = 1;
		m_frame = 0;
	}

	double cmin = -2, cmax = 2;
	if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(m_center), 2, (float)m_radius / 20.f, &cmin, &cmax, "%.20f"))
		UpdateRange();

	double rmin = 0, rmax = 5;
	if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &m_radius, 1e-2f, &rmin, &rmax, "%e", 10.f))
		UpdateRange();

	ImGui::Spacing();
	ImGui::Text("Color function");
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 button_size = { 100, 50 };
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	for (size_t i = 0; i < m_colors.size(); i++)
	{
		ImGui::PushID((int)i);

		if (m_selectedColor == i)
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
		else
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);

		if (ImGui::ImageButton(m_colors[i].preview, button_size))
		{
			m_selectedColor = i;
			SetColorFunc(m_colors[m_selectedColor]);
		}

		ImGui::PopStyleColor();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_size.x; // Expected position if next button was on same line

		if (i + 1 < m_colors.size() && next_button_x2 < window_visible_x2)
			ImGui::SameLine();

		ImGui::PopID();
	}

	ImGui::Spacing();
	ImGui::Text("Color function parameters");
	for (auto& u : m_colors[m_selectedColor].GetUniforms())
	{
		if (ImGui::DragFloat(u.name.c_str(), &u.val, 1, u.range.x, u.range.y))
			m_frame = 0;
	}

	ImGui::End();
}
