#include "FractalLayer.h"

#include <GLCore/Core/Input.h>
#include <GLCore/Core/KeyCodes.h>
#include <GLCore/Core/MouseButtonCodes.h>

#include <fstream>
#include <iomanip>
#include <filesystem>

#include <commdlg.h>

using namespace GLCore;
using namespace GLCore::Utils;

typedef int unsigned uint;

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static double map(const double& value, const double& inputMin, const double& inputMax, const double& outputMin, const double& outputMax)
{
	return outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (value - inputMin);
}

glm::dvec2 FractalLayer::MapPosToCoords(const glm::dvec2& pos)
{
	return
	{
		map(pos.x, m_xRange.x, m_xRange.y, 0, m_screenSize.x),
		map(pos.y, m_yRange.x, m_yRange.y, m_screenSize.y, 0)
	};
}

glm::dvec2 FractalLayer::MapCoordsToPos(const glm::dvec2& coords)
{
	return
	{
		map(coords.x, 0, m_screenSize.x, m_xRange.x, m_xRange.y),
		map(coords.y, m_screenSize.y, 0, m_yRange.x, m_yRange.y)
	};
}

void FractalLayer::UpdateRange()
{
	double aspect = (double)m_screenSize.x / (double)m_screenSize.y;

	m_yRange = { m_center.y - m_radius, m_center.y + m_radius };
	m_xRange = { m_center.x - aspect * m_radius, m_center.x + aspect * m_radius };

	glUseProgram(m_Shader);

	int location = glGetUniformLocation(m_Shader, "xRange");
	glUniform2d(location, m_xRange.x, m_xRange.y);

	location = glGetUniformLocation(m_Shader, "yRange");
	glUniform2d(location, m_yRange.x, m_yRange.y);

	m_frame = 0;
}

void FractalLayer::CleanOpenGL()
{
	glDeleteFramebuffers(1, &m_FrameBuffer);

	GLuint textures[] = { m_Texture, m_InData, m_OutData, m_InIter, m_OutIter };
	glDeleteTextures(IM_ARRAYSIZE(textures), textures);
}

void FractalLayer::CreateFrameBuffer()
{
	CleanOpenGL();

	glGenFramebuffers(1, &m_FrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Texture, 0);	

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
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Error creating framebuffer (" << m_size.x << ", " << m_size.y << ")\n";
	}
}

void FractalLayer::SetColorFunc(const ColorFunction& colorFunc)
{
	std::string source = m_coreShaderSrc;
	size_t color_loc = source.find("#color");
	if (color_loc == std::string::npos)
	{
		std::cout << "ERROR: Shader does not have '#color'";
		exit(EXIT_FAILURE);
	}
	source.erase(color_loc, 6);
	source.insert(color_loc, colorFunc.GetSource());

	if (m_Shader)
		glDeleteProgram(m_Shader);

	m_Shader = CreateShader(source);
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

void FractalLayer::RefreshColorFunctions()
{
	m_colors.clear();
	m_colors.reserve(10);
	for (const auto& path : std::filesystem::directory_iterator("assets/colors"))
	{
		std::ifstream colorSrc(path.path());
		m_colors.emplace_back(std::string((std::istreambuf_iterator<char>(colorSrc)), std::istreambuf_iterator<char>()));
	}

	if (m_selectedColor >= m_colors.size())
		m_selectedColor = 0;

	SetColorFunc(m_colors[m_selectedColor]);
}

glm::uvec2 FractalLayer::GetMultipliedResolution()
{
	return 
	{ 
		(uint)(m_screenSize.x * (m_resolutionPercentage / 100.f)), 
		(uint)(m_screenSize.y * (m_resolutionPercentage / 100.f)) 
	};
}

FractalLayer::FractalLayer()
	: m_radius(2)
	, m_center(0, 0)
	, m_itersPerFrame(100)
{
	m_screenSize.x = Application::Get().GetWindow().GetWidth();
	m_screenSize.y = Application::Get().GetWindow().GetHeight();
	m_size = GetMultipliedResolution();

	std::ifstream file("assets/mandelbrot_fast.glsl");
	m_coreShaderSrc = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	//m_center = { -0.74656412896773705068, 0.09886581010775417899 };
	//m_radius = 8.2212188006580699331e-12;
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

	Application::Get().GetWindow().SetVSync(true);

	CreateFrameBuffer();

	RefreshColorFunctions();

	UpdateRange();
}

void FractalLayer::OnDetach()
{
	CleanOpenGL();

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
			m_screenSize.x = e.GetWidth();
			m_screenSize.y = e.GetHeight();
			m_size = GetMultipliedResolution();

			if (m_screenSize.x > 0 && m_screenSize.y > 0)
			{
				m_minimized = false;
				CreateFrameBuffer();
				UpdateRange();
			}
			else
				m_minimized = true;
			
			return false;
		}
	);

	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse)
	{
		dispatcher.Dispatch<MouseButtonPressedEvent>(
			[&](MouseButtonPressedEvent& e)
			{
				if (e.GetMouseButton() == HZ_MOUSE_BUTTON_LEFT)
				{
					if (Input::IsKeyPressed(HZ_KEY_LEFT_CONTROL))
					{
						m_center = MapCoordsToPos({ Input::GetMouseX(), Input::GetMouseY() });
						UpdateRange();
					}
					else
					{
						m_mousePressed = true;
						m_startPos.x = Input::GetMouseX();
						m_startPos.y = Input::GetMouseY();
					}
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
}

void FractalLayer::OnUpdate(Timestep ts)
{
	frame_rate = 1 / ts.GetSeconds();

	if (m_shouldRefreshColors)
	{
		RefreshColorFunctions();
		m_shouldRefreshColors = false;
	}

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

	if (m_minimized)
		return;

	// Shader uniforms
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

	// Draw
	glViewport(0, 0, m_size.x, m_size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (m_frame == 0)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glBindVertexArray(m_QuadVA);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);


	// Copy output buffers into input buffers
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

	glBlitFramebuffer(0, 0, m_size.x, m_size.y, 0, 0, m_screenSize.x, m_screenSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	m_frame++;
}

void FractalLayer::OnImGuiRender()
{
	//ImGui::ShowDemoWindow();

	ImGuiStyle& style = ImGui::GetStyle();

	auto windowBgCol = style.Colors[ImGuiCol_WindowBg];
	windowBgCol.w = 0.6f;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgCol);

	ImGui::Begin("Controls");

	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << "General (" << frame_rate << "fps)";

	ImGui::Text(ss.str().c_str());

	if (ImGui::DragInt("Iterations per frame", (int*)&m_itersPerFrame, 10, 1, 2000))
	{
		if (m_itersPerFrame < 0) 
			m_itersPerFrame = 1;
		m_frame = 0;
	}

	double cmin = -2, cmax = 2;
	if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(m_center), 2, (float)m_radius / 20.f, &cmin, &cmax, "%.20f"))
		UpdateRange();

	double rmin = 1e-15, rmax = 50;
	static float v = 0;
	if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &m_radius, 0.01f, &rmin, &rmax, "%e", ImGuiSliderFlags_Logarithmic))
		UpdateRange();

	ImGui::Separator();
	if (ImGui::DragInt("Resolution percentage", &m_resolutionPercentage, 1, 30, 500, "%d%%", ImGuiSliderFlags_AlwaysClamp))
	{
		m_size = GetMultipliedResolution();
		CreateFrameBuffer();
		UpdateRange();
	}

	if (ImGui::Button("Screenshot"))
	{
		const char* filter = "PNG (*.png)\0*.png\0JPEG (*jpg; *jpeg)\0*.jpg;*.jpeg\0BMP (*.bmp)\0*.bmp\0TGA (*.tga)\0*.tga\0";

		OPENFILENAMEA ofn;
		CHAR szFile[260] = "screenshot";
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.hwndOwner = Application::Get().GetWindow().GetNativeWindow(); TODO ;-;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		// Sets the default extension by extracting it from the filter
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
			ExportTexture(m_Texture, ofn.lpstrFile);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Color function");
	ImGui::SameLine();

	float lineHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
	//if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
	if (ImGui::Button("Refresh"))
		m_shouldRefreshColors = true;

	ImGui::SameLine(); HelpMarker("Edit the files (or add) in the 'assets/colors' folder and they will appear here after a refresh");

	ImVec2 button_size = { 100, 50 };
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	for (size_t i = 0; i < m_colors.size(); i++)
	{
		ImGui::PushID((int)i);

		if (m_selectedColor == i)
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
		else
			ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);

		if (ImGui::ImageButton(m_colors[i].GetPreview(), button_size))
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

	ImGui::PopStyleColor();
}
