#if 1

#include "MainLayer.h"

#include <iomanip>
#include <filesystem>
#include <fstream>
#include <commdlg.h>


ImVec2 operator+(const ImVec2& l, const ImVec2& r)
{
	return { l.x + r.x, l.y + r.y };
}

ImVec2 operator-(const ImVec2& l, const ImVec2& r)
{
	return { l.x - r.x, l.y - r.y };
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


template<typename T>
ImVec2 GlmToImVec(const glm::vec<2, T>& vec)
{
	return { (float)vec.x, (float)vec.y };
}

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

void MainLayer::RefreshColorFunctions()
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

	m_Mandelbrot.SetColorFunction(&m_colors[m_selectedColor]);
}

MainLayer::MainLayer()
	: m_Mandelbrot("assets/mandelbrot.glsl")
	, m_Julia("assets/julia.glsl")
{
	RefreshColorFunctions();

	m_Mandelbrot.SetColorFunction(&m_colors[m_selectedColor]);
	m_Julia.SetColorFunction(&m_colors[m_selectedColor]);
}

MainLayer::~MainLayer()
{
	for (auto prev : m_colorsPreview)
		glDeleteTextures(1, &prev);
}

void MainLayer::OnUpdate(GLCore::Timestep ts)
{
	frame_rate = 1 / ts.GetSeconds();

	if (m_ShouldRefreshColors)
	{
		RefreshColorFunctions();
		m_ShouldRefreshColors = false;
	}

	GLint loc = glGetUniformLocation(m_Julia.GetShader(), "i_JuliaC");
	glUniform2d(loc, m_JuliaC.x, m_JuliaC.y);

	m_Mandelbrot.Update();
	m_Julia.Update();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MainLayer::OnImGuiRender()
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

	// Mandelbrot
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Mandelbrot");

		// Resize
		{
			auto size = m_Mandelbrot.GetSize();

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			if (viewportPanelSize.x != size.x || viewportPanelSize.y != size.y)
			{
				m_Mandelbrot.SetSize({ viewportPanelSize.x, viewportPanelSize.y });
			}
		}

		// Events
		if (ImGui::IsWindowHovered())
		{
			auto mousePos = ImGui::GetMousePos() - ImGui::GetWindowPos() - ImGui::GetWindowContentRegionMin();

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || 
				(ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0) && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)))
			{
				m_JuliaC = m_Mandelbrot.MapCoordsToPos(mousePos);
				m_Julia.ResetRender();
			}

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0))
			{
				if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)
				{
					glm::dvec2 center = m_Mandelbrot.MapCoordsToPos(m_Mandelbrot.MapPosToCoords(m_Mandelbrot.GetCenter()) - io.MouseDelta);
					m_Mandelbrot.SetCenter(center);
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyCtrl)
			{
				m_Mandelbrot.SetCenter(m_Mandelbrot.MapCoordsToPos(mousePos));
			}

			if (io.MouseWheel != 0)
			{
				glm::dvec2 iMousePos = m_Mandelbrot.MapCoordsToPos(mousePos);

				m_Mandelbrot.SetRadius(m_Mandelbrot.GetRadius() / std::pow(1.1f, io.MouseWheel));

				glm::dvec2 fMousePos = m_Mandelbrot.MapCoordsToPos(mousePos);

				glm::dvec2 delta = fMousePos - iMousePos;
				m_Mandelbrot.SetCenter(m_Mandelbrot.GetCenter() - delta);
			}
		}

		ImGui::Image((ImTextureID)(intptr_t)m_Mandelbrot.GetTexture(), GlmToImVec(m_Mandelbrot.GetSize()), ImVec2{0, 1}, ImVec2{1, 0});

		ImGui::End();
		ImGui::PopStyleVar();
	}

	// Julia
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Julia");

		// Resize
		{
			auto size = m_Julia.GetSize();

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			if (viewportPanelSize.x != size.x || viewportPanelSize.y != size.y)
			{
				m_Julia.SetSize({ viewportPanelSize.x, viewportPanelSize.y });
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
					glm::dvec2 center = m_Julia.MapCoordsToPos(m_Julia.MapPosToCoords(m_Julia.GetCenter()) - io.MouseDelta);
					m_Julia.SetCenter(center);
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyCtrl)
			{
				m_Julia.SetCenter(m_Julia.MapCoordsToPos(mousePos));
			}

			if (io.MouseWheel != 0)
			{
				glm::dvec2 iMousePos = m_Julia.MapCoordsToPos(mousePos);

				m_Julia.SetRadius(m_Julia.GetRadius() / std::pow(1.1f, io.MouseWheel));

				glm::dvec2 fMousePos = m_Julia.MapCoordsToPos(mousePos);

				glm::dvec2 delta = fMousePos - iMousePos;
				m_Julia.SetCenter(m_Julia.GetCenter() - delta);
			}
		}

		ImGui::Image((ImTextureID)(intptr_t)m_Julia.GetTexture(), GlmToImVec(m_Julia.GetSize()), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

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
			m_Mandelbrot.SetShader(m_MandelbrotSrcPath);
			m_Julia.SetShader(m_JuliaSrcPath);
		}

		static int itersPerFrame = 100;
		if (ImGui::DragInt("Iterations per frame", &itersPerFrame, 10, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp))
		{
			m_Mandelbrot.SetIterationsPerFrame(itersPerFrame);
			m_Julia.SetIterationsPerFrame(itersPerFrame);
		}

		double cmin = -2, cmax = 2;
		glm::dvec2 center = m_Mandelbrot.GetCenter();
		if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(center), 2, (float)m_Mandelbrot.GetRadius() / 70.f, &cmin, &cmax, "%.15f"))
			m_Mandelbrot.SetCenter(center);

		double rmin = 1e-15, rmax = 50;
		double radius = m_Mandelbrot.GetRadius();
		if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &radius, 0.01f, &rmin, &rmax, "%e", ImGuiSliderFlags_Logarithmic))
			m_Mandelbrot.SetRadius(radius);

		ImGui::Separator();
		//if (ImGui::DragInt("Resolution percentage", &m_resolutionPercentage, 1, 30, 500, "%d%%", ImGuiSliderFlags_AlwaysClamp))
		//{
		//	m_size = GetMultipliedResolution();
		//	CreateFrameBuffer();
		//	UpdateRange();
		//}

		if (ImGui::Button("Screenshot"))
		{
			const char* filter = "PNG (*.png)\0*.png\0JPEG (*jpg; *jpeg)\0*.jpg;*.jpeg\0BMP (*.bmp)\0*.bmp\0TGA (*.tga)\0*.tga\0";

			OPENFILENAMEA ofn;
			CHAR szFile[260];

			auto center = m_Mandelbrot.GetCenter();
			sprintf_s(szFile, "%.15f,%.15f", center.x, center.y);

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
				GLCore::Utils::ExportTexture(m_Mandelbrot.GetTexture(), ofn.lpstrFile);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Color function");
		ImGui::SameLine();

		float lineHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		//if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
		if (ImGui::Button("Refresh"))
			m_ShouldRefreshColors = true;

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
				m_Mandelbrot.SetColorFunction(&m_colors[m_selectedColor]);
				m_Julia.SetColorFunction(&m_colors[m_selectedColor]);
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
			{
				m_Mandelbrot.ResetRender();
				m_Julia.ResetRender();
			}
		}

		ImGui::End(); // Controls
		ImGui::PopStyleColor();
	}

	ImGui::End(); // Dockspace
}


#endif