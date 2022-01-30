#include "MainLayer.h"

#include <iomanip>
#include <filesystem>
#include <fstream>
#include <commdlg.h>

#include <imgui_internal.h>

ImVec2 operator+(const ImVec2& l, const ImVec2& r)
{
	return { l.x + r.x, l.y + r.y };
}

ImVec2 operator-(const ImVec2& l, const ImVec2& r)
{
	return { l.x - r.x, l.y - r.y };
}

ImVec2 operator*(const ImVec2& vec, float scalar)
{
	return { vec.x * scalar, vec.y * scalar };
}

ImVec2 operator/(const ImVec2& vec, float scalar)
{
	return { vec.x / scalar, vec.y / scalar };
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

template<size_t file_size>
static bool SaveImageDialog(char (&fileName)[file_size])
{
	const char* filter = "PNG (*.png)\0*.png\0JPEG (*jpg; *jpeg)\0*.jpg;*.jpeg\0BMP (*.bmp)\0*.bmp\0TGA (*.tga)\0*.tga\0";

	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	//ofn.hwndOwner = Application::Get().GetWindow().GetNativeWindow(); TODO ;-;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = sizeof(fileName);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// Sets the default extension by extracting it from the filter
	ofn.lpstrDefExt = strchr(filter, '\0') + 1;

	return GetSaveFileNameA(&ofn) == TRUE;
}

void MainLayer::RefreshColorFunctions()
{
	// Crear previews colors
	m_Colors.clear();

	for (auto prev : m_ColorsPreview)
		glDeleteTextures(1, &prev);
	m_ColorsPreview.clear();

	// Allocate new colors
	m_Colors.reserve(10);
	for (const auto& path : std::filesystem::directory_iterator("assets/colors"))
	{
		std::ifstream colorSrc(path.path());
		m_Colors.emplace_back(std::string((std::istreambuf_iterator<char>(colorSrc)), std::istreambuf_iterator<char>()));
	}

	// Allocate the prevews
	m_ColorsPreview.reserve(m_Colors.size());
	for (const auto& c : m_Colors)
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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, buffers);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				std::cout << "Failed to create the color function preview framebuffer\n";
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
			glUniform1ui(loc, 100);

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

			m_ColorsPreview.push_back(tex);
		}
	}

	if (m_SelectedColor >= m_Colors.size())
		m_SelectedColor = 0;

	m_Mandelbrot.SetColorFunction(&m_Colors[m_SelectedColor]);
	m_Julia.SetColorFunction(&m_Colors[m_SelectedColor]);
}

MainLayer::MainLayer()
	: m_MandelbrotSrcPath("assets/mandelbrot.glsl")
	, m_Mandelbrot(m_MandelbrotSrcPath)
	, m_JuliaSrcPath("assets/julia.glsl")
	, m_Julia(m_JuliaSrcPath)
{
	RefreshColorFunctions();

	m_Mandelbrot.SetColorFunction(&m_Colors[m_SelectedColor]);
	m_Julia.SetColorFunction(&m_Colors[m_SelectedColor]);

	m_Mandelbrot.SetIterationsPerFrame(m_ItersPerFrame);
	m_Julia.SetIterationsPerFrame(m_ItersPerFrame);

	m_Mandelbrot.SetMaxEpochs(m_MaxEpochs);
	m_Julia.SetMaxEpochs(m_MaxEpochs);

	m_Mandelbrot.SetSmoothColor(m_SmoothColor);
	m_Julia.SetSmoothColor(m_SmoothColor);

	m_Mandelbrot.SetCenter({ -0.5, 0 });
	m_Julia.SetRadius(1.3);
}

MainLayer::~MainLayer()
{
	for (auto prev : m_ColorsPreview)
		glDeleteTextures(1, &prev);
}

void MainLayer::OnUpdate(GLCore::Timestep ts)
{
	m_FrameRate = 1 / ts.GetSeconds();

	if (m_ShouldRefreshColors)
	{
		RefreshColorFunctions();
		m_ShouldRefreshColors = false;
	}

	glUseProgram(m_Julia.GetShader());
	GLint loc = glGetUniformLocation(m_Julia.GetShader(), "i_JuliaC");
	glUniform2d(loc, m_JuliaC.x, m_JuliaC.y);

	if (!m_MandelbrotMinimized)
		m_Mandelbrot.Update();

	if (!m_JuliaMinimized)
		m_Julia.Update();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void DisableBlendCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	glDisable(GL_BLEND);
}

void EnableBlendCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	glEnable(GL_BLEND);
}

ImVec2 ImagePosToWindowPos(const ImVec2& imagePos, int resolutionPercentage)
{
	return imagePos / (resolutionPercentage / 100.f) + ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
}

ImVec2 WindowPosToImagePos(const ImVec2& windowPos, int resolutionPercentage)
{
	return (windowPos - ImGui::GetWindowPos() - ImGui::GetWindowContentRegionMin()) * (resolutionPercentage / 100.f);
}

void FractalHandleInteract(FractalVisualizer& fract, int resolutionPercentage)
{
	ImGuiIO& io = ImGui::GetIO();
	auto mouseDeltaScaled = io.MouseDelta * (resolutionPercentage / 100.f);

	auto mousePos = WindowPosToImagePos(ImGui::GetMousePos(), resolutionPercentage);
	
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0) && mousePos.y >= 0)
	{
		if (mouseDeltaScaled.x != 0 || mouseDeltaScaled.y != 0)
		{
			glm::dvec2 center = fract.MapCoordsToPos(fract.MapPosToCoords(fract.GetCenter()) - mouseDeltaScaled);
			fract.SetCenter(center);
		}
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyCtrl)
	{
		fract.SetCenter(fract.MapCoordsToPos(mousePos));
	}

	if (io.MouseWheel != 0)
	{
		glm::dvec2 iMousePos = fract.MapCoordsToPos(mousePos);

		fract.SetRadius(fract.GetRadius() / std::pow(1.1f, io.MouseWheel));

		glm::dvec2 fMousePos = fract.MapCoordsToPos(mousePos);

		glm::dvec2 delta = fMousePos - iMousePos;
		fract.SetCenter(fract.GetCenter() - delta);
	}
}

void FractalHandleResize(FractalVisualizer& fract, int resolutionPercentage)
{
	ImVec2 viewportPanelSizeScaled = ImGui::GetContentRegionAvail() * (resolutionPercentage / 100.f);

	auto size = fract.GetSize();
	if (glm::uvec2{ viewportPanelSizeScaled.x, viewportPanelSizeScaled.y } != size)
	{
		fract.SetSize(glm::uvec2{ viewportPanelSizeScaled.x, viewportPanelSizeScaled.y });
	}
}

void DrawIterations(const glm::dvec2& z0, const glm::dvec2& c, const ImColor& baseColor, FractalVisualizer& fract, int resolutionPercentage)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	glm::dvec2 z = z0;
	for (int i = 0; i < 100; i++)
	{
		auto p0 = ImagePosToWindowPos(fract.MapPosToCoords(z), resolutionPercentage);
		z = glm::dvec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
		auto p1 = ImagePosToWindowPos(fract.MapPosToCoords(z), resolutionPercentage);

		float scale = 10.f;
		ImColor color(baseColor);
		color.Value.w *= scale / (i + scale);
		draw_list->AddLine(p0, p1, color, 2.f);
	}
}

void MainLayer::OnImGuiRender()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGui::DockSpaceOverViewport();

	//ImGui::ShowDemoWindow();

	// Help
	if (ImGui::IsKeyPressed(ImGuiKey_H))
		m_ShowHelp = !m_ShowHelp;

	if (m_ShowHelp)
	{
		ImGui::Begin("Help", &m_ShowHelp, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Controls:");
		ImGui::BulletText("Mouse drag to pan");
		ImGui::BulletText("Mouse wheel to zoom");
		ImGui::BulletText("CTRL + left click to set the center to the mouse location");
		ImGui::BulletText("Middle mouse button to show the first iterations of the equation");
		ImGui::BulletText("Left click the mandelbrot set to set the julia c to the mouse location");
		ImGui::BulletText("H to toggle this help window");

		ImGui::Spacing();

		ImGui::Text("Features:");
		ImGui::BulletText("All panels (including this one) can be moved and docked wherever you want.");
		ImGui::BulletText("You can edit (or add) the color functions by editing (or adding) the .glsl\n"
						  "shaders in the ./assets/colors folder. In this files use the preprocessor\n"
						  "command `#uniform <name> <default> <slider_speed> <min> <max>` to set a\n"
						  "custom float uniform which will be exposed through the UI. You can set the\n"
						  "min and/or max values to NULL to indicate it is unbounded.");

		ImGui::End();
	}

	// Mandelbrot
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		m_MandelbrotMinimized = !ImGui::Begin("Mandelbrot");

		// Resize
		FractalHandleResize(m_Mandelbrot, m_ResolutionPercentage);

		// Draw
		ImGui::GetCurrentWindow()->DrawList->AddCallback(DisableBlendCallback, nullptr);
		ImGui::Image((ImTextureID)(intptr_t)m_Mandelbrot.GetTexture(), ImGui::GetContentRegionAvail(), ImVec2{0, 1}, ImVec2{1, 0});
		ImGui::GetCurrentWindow()->DrawList->AddCallback(EnableBlendCallback, nullptr);

		// Events
		if (ImGui::IsWindowHovered())
		{
			FractalHandleInteract(m_Mandelbrot, m_ResolutionPercentage);

			auto mousePos = WindowPosToImagePos(ImGui::GetMousePos(), m_ResolutionPercentage);
			
			// Right click to set `julia c`
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
				(ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0) && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)))
			{
				m_JuliaC = m_Mandelbrot.MapCoordsToPos(mousePos);
				m_Julia.ResetRender();
			}

			if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				glm::dvec2 c = m_Mandelbrot.MapCoordsToPos(mousePos);
				DrawIterations(c, c, m_IterationsColor, m_Mandelbrot, m_ResolutionPercentage);
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	// Julia
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		m_JuliaMinimized = !ImGui::Begin("Julia");

		// Resize
		FractalHandleResize(m_Julia, m_ResolutionPercentage);

		// Draw
		ImGui::GetCurrentWindow()->DrawList->AddCallback(DisableBlendCallback, nullptr);
		ImGui::Image((ImTextureID)(intptr_t)m_Julia.GetTexture(), ImGui::GetContentRegionAvail(), ImVec2{0, 1}, ImVec2{1, 0});
		ImGui::GetCurrentWindow()->DrawList->AddCallback(EnableBlendCallback, nullptr);

		// Events
		if (ImGui::IsWindowHovered())
		{
			FractalHandleInteract(m_Julia, m_ResolutionPercentage);

			auto mousePos = WindowPosToImagePos(ImGui::GetMousePos(), m_ResolutionPercentage);

			if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				DrawIterations(m_Julia.MapCoordsToPos(mousePos), m_JuliaC, m_IterationsColor, m_Julia, m_ResolutionPercentage);
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	// Controls
	{
		auto windowBgCol = style.Colors[ImGuiCol_WindowBg];
		windowBgCol.w = 0.5f;
		ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgCol);
		ImGui::Begin("Controls");

		ImGui::Text("%.1ffps", m_FrameRate);

		ImGui::Spacing();

		if (ImGui::CollapsingHeader("General"))
		{
			if (ImGui::DragInt("Iterations per frame", &m_ItersPerFrame, 10, 1, 10000, "%d", ImGuiSliderFlags_AlwaysClamp))
			{
				m_Mandelbrot.SetIterationsPerFrame(m_ItersPerFrame);
				m_Julia.SetIterationsPerFrame(m_ItersPerFrame);
			}

			if (ImGui::DragInt("Resolution percentage", &m_ResolutionPercentage, 1, 30, 500, "%d%%", ImGuiSliderFlags_AlwaysClamp))
			{
				glm::uvec2 mandelbrotSize = (glm::vec2)m_Mandelbrot.GetSize() * (m_ResolutionPercentage / 100.f);
				m_Mandelbrot.SetSize(mandelbrotSize);

				glm::uvec2 juliaSize = (glm::vec2)m_Mandelbrot.GetSize() * (m_ResolutionPercentage / 100.f);
				m_Julia.SetSize(juliaSize);
			}

			// If unlimited epochs is checked, set m_MaxEpochs to 0 but keep the slider value to the previous value
			static bool unlimited_epochs = m_MaxEpochs == 0;
			static int max_epochs = unlimited_epochs ? 100 : m_MaxEpochs;
			if (ImGui::Checkbox("Unlimited epochs", &unlimited_epochs))
			{
				if (unlimited_epochs)
					m_MaxEpochs = 0;
				else
					m_MaxEpochs = max_epochs;

				m_Mandelbrot.SetMaxEpochs(m_MaxEpochs);
				m_Julia.SetMaxEpochs(m_MaxEpochs);
			}

			ImGui::SameLine(); HelpMarker("You may want to limit the maximum number of epochs to avoid getting a blurry image.");

			ImGui::BeginDisabled(unlimited_epochs);
			{
				ImGui::Indent();

				if (ImGui::DragInt("Max epochs", &max_epochs, 1, 1, 2000, "%d", ImGuiSliderFlags_AlwaysClamp))
				{
					m_MaxEpochs = max_epochs;

					m_Mandelbrot.SetMaxEpochs(m_MaxEpochs);
					m_Julia.SetMaxEpochs(m_MaxEpochs);
				}

				ImGui::Unindent();
			}
			ImGui::EndDisabled();

			ImGui::Spacing();

			ImGui::AlignTextToFramePadding();
			ImGui::ColorEdit4("Iterations color", &m_IterationsColor.Value.x);

			ImGui::SameLine(); HelpMarker("Press the middle mouse button to show the first iterations at that point");

			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Color function"))
		{
			if (ImGui::Checkbox("Smooth Color", &m_SmoothColor))
			{
				m_Mandelbrot.SetSmoothColor(m_SmoothColor);
				m_Julia.SetSmoothColor(m_SmoothColor);
			}

			if (ImGui::Button("Refresh"))
				m_ShouldRefreshColors = true;

			ImGui::SameLine(); HelpMarker("Edit (or add) the files in the 'assets/colors' folder and they will appear here after a refresh.");

			ImVec2 button_size = { 100, 50 };
			float window_visible_x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			for (size_t i = 0; i < m_Colors.size(); i++)
			{
				ImGui::PushID((int)i);

				if (m_SelectedColor == i)
					ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
				else
					ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_Button]);

				if (ImGui::ImageButton((ImTextureID)(intptr_t)m_ColorsPreview[i], button_size))
				{
					m_SelectedColor = i;
					m_Mandelbrot.SetColorFunction(&m_Colors[m_SelectedColor]);
					m_Julia.SetColorFunction(&m_Colors[m_SelectedColor]);
				}

				ImGui::PopStyleColor();

				float last_button_x = ImGui::GetItemRectMax().x;
				float next_button_x = last_button_x + style.ItemSpacing.x + button_size.x; // Expected position if next button was on same line

				if (i + 1 < m_Colors.size() && next_button_x < window_visible_x)
					ImGui::SameLine();

				ImGui::PopID();
			}

			ImGui::Spacing();
			ImGui::Text("Color function parameters");
			for (auto& u : m_Colors[m_SelectedColor].GetUniforms())
			{
				if (ImGui::DragFloat(u.name.c_str(), &u.val, u.speed, u.range.x, u.range.y))
				{
					m_Mandelbrot.ResetRender();
					m_Julia.ResetRender();
				}
			}

			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Mandelbrot"))
		{
			ImGui::PushID(0);

			if (ImGui::Button("Reload Mandelbrot Shader"))
				m_Mandelbrot.SetShader(m_MandelbrotSrcPath);

			double cmin = -2, cmax = 2;
			glm::dvec2 center = m_Mandelbrot.GetCenter();
			if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(center), 2, (float)m_Mandelbrot.GetRadius() / 70.f, &cmin, &cmax, "%.15f"))
				m_Mandelbrot.SetCenter(center);

			double rmin = 1e-15, rmax = 50;
			double radius = m_Mandelbrot.GetRadius();
			if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &radius, 0.01f, &rmin, &rmax, "%e", ImGuiSliderFlags_Logarithmic))
				m_Mandelbrot.SetRadius(radius);


			if (ImGui::Button("Screenshot"))
			{
				CHAR fileName[260];
				auto center = m_Mandelbrot.GetCenter();
				sprintf_s(fileName, "mandelbrot_%.15f,%.15f", center.x, center.y);

				if (SaveImageDialog(fileName))
					GLCore::Utils::ExportTexture(m_Mandelbrot.GetTexture(), fileName, true);
			}

			ImGui::Spacing();
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Julia"))
		{
			ImGui::PushID(1);

			if (ImGui::Button("Reload Julia Shader"))
				m_Julia.SetShader(m_JuliaSrcPath);

			double cmin = -2, cmax = 2;
			glm::dvec2 center = m_Julia.GetCenter();
			if (ImGui::DragScalarN("Center", ImGuiDataType_Double, glm::value_ptr(center), 2, (float)m_Julia.GetRadius() / 200.f, &cmin, &cmax, "%.15f"))
				m_Julia.SetCenter(center);

			double rmin = 1e-15, rmax = 50;
			double radius = m_Julia.GetRadius();
			if (ImGui::DragScalar("Radius", ImGuiDataType_Double, &radius, 0.01f, &rmin, &rmax, "%e", ImGuiSliderFlags_Logarithmic))
				m_Julia.SetRadius(radius);

			if (ImGui::DragScalarN("C value", ImGuiDataType_Double, glm::value_ptr(m_JuliaC), 2, (float)m_Julia.GetRadius() * 1e-5f, &cmin, &cmax, "%.15f"))
				m_Julia.ResetRender();


			if (ImGui::Button("Screenshot"))
			{
				CHAR fileName[260];
				sprintf_s(fileName, "julia_%.15f,%.15f", m_JuliaC.x, m_JuliaC.y);

				if (SaveImageDialog(fileName))
					GLCore::Utils::ExportTexture(m_Julia.GetTexture(), fileName, true);
			}

			ImGui::Spacing();
			ImGui::PopID();
		}

		ImGui::End(); // Controls
		ImGui::PopStyleColor();
	}
}