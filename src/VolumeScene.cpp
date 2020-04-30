#include "VolumeScene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Engine3D.h"


VolumeScene::VolumeScene()
{
}


VolumeScene::~VolumeScene()
{
}

void VolumeScene::renderImGUI(){
	ImGui::SetNextWindowPos(ImVec2(10, 340), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 450), ImGuiCond_Once);
	ImGui::Begin("Volume Properties", p_open, window_flags);
	ImGui::DragFloat3("Absorption (1/m)", &absorption[0], 0.001f, 0.001f, 5.0f);
	ImGui::DragFloat3("Scattering (1/m)", &scattering[0], 0.001f, 0.001f, 5000.0f);
	if (lockScattering) {
		scattering[1] = scattering[0];
		scattering[2] = scattering[0];
	}
	ImGui::Checkbox("Monochromatic Scattering", &lockScattering);
	ImGui::PushItemWidth(80.0f);
	ImGui::DragFloat("Density Multiplier", &densityCoef, 0.01, 0.001f, 30.0);
	ImGui::PopItemWidth();

	ImGui::Text("Model: ");
	ImGui::Text(currentModelResolution.c_str());
	ImGui::Combo("", &currentModel, models, IM_ARRAYSIZE(models));

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(WIDTH - 300, 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);

	ImGui::Begin("Light Properties", p_open, window_flags);
	ImGui::DragFloat3("Light Pos (m)", &lightPosition[0], 0.1, 0.1, 0.1);
	ImGui::DragFloat3("L. color", &lightColor[0], 0.01, 0.0, 30.0);

	ImGui::DragFloat("Ambient Strength", &ambientStrength, 0.1, 0.0, 1000.0);
	ImGui::DragFloat("Kc", &Kc, 0.1, 0.0, 100.0);
	ImGui::DragFloat("Kl", &Kl, 0.1, 0.0, 100.0);
	ImGui::DragFloat("Kq", &Kq, 0.1, 0.0, 100.0);
	ImGui::DragFloat("g", &g, 0.01, -1.0, 1.0);
	ImGui::DragFloat("N. of Steps", &numberOfSteps, 1, 0.0, 256);
	ImGui::DragFloat("Shadow Steps", &shadowSteps, 1, 0.0, 256);
	ImGui::Checkbox("Enable Shadow", &enableShadow);

	ImGui::Checkbox("Night Mode", &nightMode);

	ImGui::Text("Render Mode: ");
	ImGui::Combo("", &currentRenderMode, renderModes, IM_ARRAYSIZE(renderModes));

	ImGui::Combo("Phase Function", &phaseFunctionOption, phaseFunctionOptions, IM_ARRAYSIZE(phaseFunctionOptions));
	ImGui::End();
	ImGui::SetNextWindowPos(ImVec2(40, 810), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Once);
}
