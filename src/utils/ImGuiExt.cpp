#include "utils/ImGuiExt.h"
#include "utils/ImGuiStyle.h"

namespace ImGuiExt {
    static bool Items_ArrayGetter(void* data, int idx, const char** out_text){
        const char* const* items = (const char* const*)data;
        if (out_text)
            *out_text = items[idx];
        return true;
    }

    void ImGuiExt::PaddedText(const char* text, ImVec2 padding) {
        //Source: https://github.com/ocornut/imgui/issues/3145.
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::InvisibleButton("##invPaddedTextButton", textSize + ImVec2(padding));
        ImVec2 finalCursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(cursor + padding);
        ImGui::Text(text);
        ImGui::SetCursorPos(finalCursorPos);
    }

    bool ImGuiExt::ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items, ImVec4 activeTextColor){
        const bool value_changed = ListBox(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_items, activeTextColor);
        return value_changed;
    }

    bool ImGuiExt::ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items, ImVec4 activeTextColor){
        if (!ImGui::ListBoxHeader(label, items_count, height_in_items))
            return false;

        // Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
        ImGuiContext& g = *GImGui;
        bool value_changed = false;
        ImGuiListClipper clipper(items_count, ImGui::GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
        while (clipper.Step())
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++){
                const bool item_selected = (i == *current_item);
                const char* item_text;
                if (!items_getter(data, i, &item_text))
                    item_text = "*Unknown item*";

                ImGui::PushID(i);
                if (item_selected)
                    ImGui::PushStyleColor(ImGuiCol_Text, activeTextColor);
                if (ImGui::Selectable(item_text, item_selected))
                {
                    *current_item = i;
                    value_changed = true;
                }
                if (item_selected)
                    ImGui::SetItemDefaultFocus();
                ImGui::PopID();

                if (item_selected)
                    ImGui::PopStyleColor();
            }

        ImGui::ListBoxFooter();
        if (value_changed)
            ImGui::MarkItemEdited(g.LastItemData.ID);

        return value_changed;
    }

    bool CollapsingHeader(const char* label, ImVec4 shadowColor, ImGuiTreeNodeFlags flags) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetFontSize() / 2.2f, ImGui::GetFontSize() / 2.2f));

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        bool res = ImGui::CollapsingHeader(label, flags);
        ImVec2 leftCorner = ImGui::GetItemRectMin();
        ImVec2 rightCorner = ImGui::GetItemRectMax();
        ImVec2 size = ImGui::GetContentRegionMax();
        ImGui::PopStyleVar();

        leftCorner.y = leftCorner.y + ImGui::GetItemRectSize().y - 1;
        rightCorner = ImVec2(leftCorner.x + size.x, leftCorner.y);

        drawList->AddLine(leftCorner, rightCorner, ImGui::GetColorU32(shadowColor), 1.0f);

        return res;
    }

    void TextVec3(glm::vec3 v, std::string name) {
        ImGui::Text(name.c_str());
        ImGui::SameLine();
        ImGui::Text("(%.1f, %.1f, %.1f)", v.x, v.y, v.z);
    }

    ImVec4 add(ImVec4 vec4, float v) {
        return ImVec4(vec4.x + v, vec4.y + v, vec4.y + v, vec4.z + v);
    }

    void ColorPicker(const char* id, float* color, ImVec2 buttonSize, ImGuiColorEditFlags flags) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color[0], color[1], color[2], 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color[0], color[1], color[2], 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color[0], color[1], color[2], 1));
        if (ImGui::Button("", buttonSize))
            ImGui::OpenPopup("mypicker");

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, narvalengine::imGuiStyleDefs.windowPadding);
        if (ImGui::BeginPopup("mypicker")) {
            ImGui::ColorPicker4(id, color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }
};
