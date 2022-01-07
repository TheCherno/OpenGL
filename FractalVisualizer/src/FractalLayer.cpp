#if 0

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

std::ostream& operator<<(std::ostream& os, const ImVec2& vec)
{
	os << '(' << vec.x << ", " << vec.y << ')';
	return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const glm::vec<2, T>& vec)
{
	os << '(' << vec.x << ", " << vec.y << ')';
	return os;
}

ImVec2 operator+(const ImVec2& l, const ImVec2& r)
{
	return { l.x + r.x, l.y + r.y };
}

ImVec2 operator-(const ImVec2& l, const ImVec2& r)
{
	return { l.x - r.x, l.y - r.y };
}

static double map(const double& value, const double& inputMin, const double& inputMax, const double& outputMin, const double& outputMax)
{
	return outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (value - inputMin);
}

ImVec2 FractalLayer::MapPosToCoords(const glm::dvec2& pos)
{
	return
	{
		(float)map(pos.x, m_xRange.x, m_xRange.y, 0, m_viewportSize.x),
		(float)map(pos.y, m_yRange.x, m_yRange.y, m_viewportSize.y, 0)
	};
}

glm::dvec2 FractalLayer::MapCoordsToPos(const ImVec2& coords)
{
	return
	{
		map(coords.x, 0, m_viewportSize.x, m_xRange.x, m_xRange.y),
		map(coords.y, m_viewportSize.y, 0, m_yRange.x, m_yRange.y)
	};
}

glm::dvec2 FractalLayer::DeltaPixelsToPos(const ImVec2& delta)
{
	return
	{
		map(-delta.x, 0, m_viewportSize.x, 0, abs(m_xRange.y - m_xRange.x)),
		map(delta.y, 0, m_viewportSize.y, 0, abs(m_yRange.y - m_yRange.x))
	};
}

void FractalLayer::UpdateRange()
{
	double aspect = (double)m_viewportSize.x / (double)m_viewportSize.y;

	m_yRange = { m_center.y - m_radius, m_center.y + m_radius };
	m_xRange = { m_center.x - aspect * m_radius, m_center.x + aspect * m_radius };

	glUseProgram(m_Shader);

	int location = glGetUniformLocation(m_Shader, "i_xRange");
	glUniform2d(location, m_xRange.x, m_xRange.y);

	location = glGetUniformLocation(m_Shader, "i_yRange");
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_size.x, m_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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
	// Crear previews colors
	m_colors.clear();

	for (auto prev : m_colorsPreview)
		glDeleteTextures(1, &prev);
	m_colorsPreview.clear();

	// Allocate new colors
	m_colors.reserve(10);
	for (const auto& path : std::filesystem::directory_iterator("assets/colors"))
	{
		std::ifstream colorSrc(path.path());
		m_colors.emplace_back(std::string((std::istreambuf_iterator<char>(colorSrc)), std::istreambuf_iterator<char>()));
	}

	// Allocate the prevews
	m_colorsPreview.reserve(m_colors.size());
	for (const auto& c : m_colors)
	{
		// Make the preview
		glm::uvec2 previewSize = { 100, 1 };
		{
			// Framebuffer
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

			// Shader
			std::stringstream ss;
			ss << "#version 400\n\n";
			ss << c.GetSource() << '\n';
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

			GLuint shader = GLCore::Utils::CreateShader(ss.str());
			glUseProgram(shader);
			GLint loc;

			loc = glGetUniformLocation(shader, "range");
			glUniform1ui(loc, 1000);

			loc = glGetUniformLocation(shader, "size");
			glUniform2ui(loc, previewSize.x, previewSize.y);

			for (const auto& u : c.GetUniforms())
			{
				loc = glGetUniformLocation(shader, u.name.c_str());
				glUniform1f(loc, u.default_val);
			}

			// Drawing
			glViewport(0, 0, previewSize.x, previewSize.y);
			glDisable(GL_BLEND);

			glBindVertexArray(GLCore::Application::GetDefaultQuadVA());
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

			// Cleaning up
			glDeleteFramebuffers(1, &fb);
			glDeleteProgram(shader);

			m_colorsPreview.push_back(tex);
		}
	}

	if (m_selectedColor >= m_colors.size())
		m_selectedColor = 0;

	SetColorFunc(m_colors[m_selectedColor]);
}

glm::uvec2 FractalLayer::GetMultipliedResolution()
{
	return 
	{ 
		(uint)(m_viewportSize.x * (m_resolutionPercentage / 100.f)), 
		(uint)(m_viewportSize.y * (m_resolutionPercentage / 100.f)) 
	};
}

FractalLayer::FractalLayer()
	: m_radius(2)
	, m_center(0, 0)
	, m_itersPerFrame(100)
{
	m_viewportSize.x = Application::Get().GetWindow().GetWidth();
	m_viewportSize.y = Application::Get().GetWindow().GetHeight();
	m_size = GetMultipliedResolution();

	std::ifstream file("assets/mandelbrot.glsl");
	m_coreShaderSrc = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	//m_center = { -0.74656412896773705068, 0.09886581010775417899 };
	//m_radius = 8.2212188006580699331e-12;

	//m_Mandelbrot.SetShader("assets/mandelbrot.glsl");
	//m_Mandelbrot.SetSize(m_size);

}

void FractalLayer::OnAttach()
{
	EnableGLDebugging();

	Application::Get().GetWindow().SetVSync(true);

	CreateFrameBuffer();

	RefreshColorFunctions();

	UpdateRange();
}

void FractalLayer::OnDetach()
{
	CleanOpenGL();
}

void FractalLayer::OnEvent(Event& event)
{
	
}

void FractalLayer::OnUpdate(Timestep ts)
{
	frame_rate = 1 / ts.GetSeconds();

	if (m_shouldRefreshColors)
	{
		RefreshColorFunctions();
		m_shouldRefreshColors = false;
	}

	// Shader uniforms
	glUseProgram(m_Shader);
	GLint location;

	for (const auto& u : m_colors[m_selectedColor].GetUniforms())
	{
		location = glGetUniformLocation(m_Shader, u.name.c_str());
		glUniform1f(location, u.val);
	}

	location = glGetUniformLocation(m_Shader, "i_Size");
	glUniform2ui(location, m_size.x, m_size.y);

	location = glGetUniformLocation(m_Shader, "i_ItersPerFrame");
	glUniform1ui(location, m_itersPerFrame);

	location = glGetUniformLocation(m_Shader, "i_Frame");
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

	glBindVertexArray(Application::GetDefaultQuadVA());
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

	//glBlitFramebuffer(0, 0, m_size.x, m_size.y, 0, 0, m_viewportSize.x, m_viewportSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	m_frame++;
}

void FractalLayer::OnImGuiRender()
{
	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking; //| ImGuiWindowFlags_MenuBar;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
	}

	ImGui::ShowDemoWindow();

	// Viewport
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport", nullptr);

		// Resize
		{
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			if (viewportPanelSize.x != m_viewportSize.x || viewportPanelSize.y != m_viewportSize.y)
			{
				m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };
				m_size = GetMultipliedResolution();
				CreateFrameBuffer();
				UpdateRange();
			}
		}

		// Events
		if (ImGui::IsWindowHovered())
		{
			auto mousePos = ImGui::GetMousePos() - ImGui::GetWindowPos() - ImGui::GetWindowContentRegionMin();

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0))
			{
				if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)
				{
					m_center += DeltaPixelsToPos(io.MouseDelta);
					UpdateRange();
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyCtrl)
			{
				m_center = MapCoordsToPos(mousePos);
				UpdateRange();
			}

			if (io.MouseWheel != 0)
			{
				m_radius /= std::pow(1.1f, io.MouseWheel);

				// Zoom where the mouse is
				glm::dvec2 iMousePos = MapCoordsToPos(mousePos);

				UpdateRange();

				glm::dvec2 fMousePos = MapCoordsToPos(mousePos);

				glm::dvec2 delta = fMousePos - iMousePos;

				m_center -= delta;

				UpdateRange();
			}
		}

		ImGui::Image((ImTextureID)(intptr_t)m_Texture, ImVec2{ (float)m_viewportSize.x, (float)m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		ImGui::End();
		ImGui::PopStyleVar();
	}
	
	// Controls
	{
		auto windowBgCol = style.Colors[ImGuiCol_WindowBg];
		windowBgCol.w = 0.5f;
		ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgCol);
		ImGui::Begin("Controls");

		std::stringstream ss;
		ss << std::fixed << std::setprecision(1) << "General (" << frame_rate << "fps)";

		ImGui::AlignTextToFramePadding();
		ImGui::Text(ss.str().c_str());
		ImGui::SameLine();
		if (ImGui::Button("Reload Core Shader"))
		{
			std::ifstream file("assets/julia.glsl");
			m_coreShaderSrc = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

			SetColorFunc(m_colors[m_selectedColor]);
		}

		if (ImGui::DragInt("Iterations per frame", (int*)&m_itersPerFrame, 10, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp))
			m_frame = 0;

		double cmin = -2, cmax = 2;
		if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(m_center), 2, (float)m_radius / 70.f, &cmin, &cmax, "%.15f"))
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
			CHAR szFile[260];
			sprintf_s(szFile, "%.15f,%.15f", m_center.x, m_center.y);

			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			//ofn.hwndOwner = Application::Get().GetWindow().GetNativeWindow(); TODO ;-;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = filter;
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

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

			if (ImGui::ImageButton((ImTextureID)(intptr_t)m_colorsPreview[i], button_size))
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

		ImGui::End(); // Controls
		ImGui::PopStyleColor();
	}

	ImGui::End(); // Dockspace
}

#endif