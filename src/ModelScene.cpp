#pragma once
#include "ModelScene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Engine3D.h"

ModelScene::ModelScene()
{
}


ModelScene::~ModelScene()
{
}

void ModelScene::renderImGUI() {
	ImGui::SetNextWindowPos(ImVec2(10, 340), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Once);

	ImGui::Begin("Light Properties", p_open, window_flags);
	ImGui::DragFloat3("Light Pos (m)", &lightPosition[0], 0.1, -1000, 1000);
	ImGui::DragFloat3("Light lookAt (m)", &lightLookAt[0], 0.1, -1000, 1000);

	ImGui::DragFloat3("Ambient", &ambient[0], 0.01, 0.0, 1000.0);
	ImGui::DragFloat3("Diffuse", &diffuse[0], 0.1, 0.0, 1000.0);
	ImGui::DragFloat3("Specular", &specular[0], 0.1, 0.0, 1000.0);

	ImGui::DragFloat("Kc", &Kc, 0.1, 0.001, 100.0);
	ImGui::DragFloat("Kl", &Kl, 0.1, 0.001, 100.0);
	ImGui::DragFloat("Kq", &Kq, 0.001, 0.001, 100.0);

	ImGui::Text("Render Mode: ");
	ImGui::Combo("", &currentRenderMode, renderModes, IM_ARRAYSIZE(renderModes));


	ImGui::End();


	ImGui::SetNextWindowPos(ImVec2(WIDTH - 300, 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
	ImGui::Begin("Render Properties", p_open, window_flags);
	ImGui::DragFloat3("Scale ", &specular[0], 0.1, 0.0, 1000.0);
	ImGui::Text("View Mode: ");
	ImGui::Combo("", &currentViewMode, viewModes, IM_ARRAYSIZE(viewModes));
	ImGui::DragFloat("Gamma", &gamma, 0.01, 0.001, 100.0);
	ImGui::DragFloat("Exposure", &exposure, 0.01, 0.001, 100.0);
	ImGui::Checkbox("Gamma correction", &gammaCorrection);
	ImGui::Checkbox("Normal mapping", &normalMapping);
	ImGui::End();
}

