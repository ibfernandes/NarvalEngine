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
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Once);

	ImGui::Begin("Light Properties", p_open, window_flags);
	ImGui::DragFloat3("Light Pos (m)", &lightPosition[0], 0.1, 0.1, 0.1);
	//ImGui::DragFloat3("L. color", &lightColor[0], 0.01, 0.0, 30.0);

	//ImGui::DragFloat("Ambient Strength", &ambientStrength, 0.1, 0.0, 1000.0);
	ImGui::DragFloat("Kc", &Kc, 0.1, 0.001, 100.0);
	ImGui::DragFloat("Kl", &Kl, 0.1, 0.001, 100.0);
	ImGui::DragFloat("Kq", &Kq, 0.1, 0.001, 100.0);
	ImGui::End();

}

