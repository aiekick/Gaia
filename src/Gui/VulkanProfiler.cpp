/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Gaia/Gui/VulkanProfiler.h>

#include <cstdarg> /* va_list, va_start, va_arg, va_end */
#include <cmath>

#include <ctools/Logger.h>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Core/VulkanSubmitter.h>

#ifdef _MSC_VER
#include <Windows.h>
#define DEBUG_BREAK          \
    if (IsDebuggerPresent()) \
    __debugbreak()
#else
#define DEBUG_BREAK
#endif

static bool PlayPauseButton(bool& vPlayPause) {
    bool res = false;
    const char* play_pause_label = "Pause";
    if (vPlayPause) {
        play_pause_label = "Play";
    }
    if (ImGui ::Button(play_pause_label)) {
        vPlayPause = !vPlayPause;
        res = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Play/Pause Profiling");
    }
    return res;
}

namespace GaiApi {

// contrast from 1 to 21
// https://www.w3.org/TR/WCAG20/#relativeluminancedef
static float CalcContrastRatio(const ImU32& backgroundColor, const ImU32& foreGroundColor) {
    const float sa0 = (float)((backgroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    const float sa1 = (float)((foreGroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    static float sr = 0.2126f / 255.0f;
    static float sg = 0.7152f / 255.0f;
    static float sb = 0.0722f / 255.0f;
    const float contrastRatio =
        (sr * sa0 * ((backgroundColor >> IM_COL32_R_SHIFT) & 0xFF) + sg * sa0 * ((backgroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
            sb * sa0 * ((backgroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f) /
        (sr * sa1 * ((foreGroundColor >> IM_COL32_R_SHIFT) & 0xFF) + sg * sa1 * ((foreGroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
            sb * sa1 * ((foreGroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f);
    if (contrastRatio < 1.0f)
        return 1.0f / contrastRatio;
    return contrastRatio;
}

static bool PushStyleColorWithContrast(
    const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio) {
    const float contrastRatio = CalcContrastRatio(backGroundColor, ImGui::GetColorU32(foreGroundColor));
    if (contrastRatio < maxContrastRatio) {
        ImGui::PushStyleColor(foreGroundColor, invertedColor);
        return true;
    }
    return false;
}

static std::string toStr(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char TempBuffer[1024 * 3 + 1];
    const int w = vsnprintf(TempBuffer, 3072, fmt, args);
    va_end(args);
    if (w) {
        return std::string(TempBuffer, (size_t)w);
    }
    return std::string();
}

////////////////////////////////////////////////////////////
/////////////////////// QUERY ZONE /////////////////////////
////////////////////////////////////////////////////////////

uint32_t vkProfQueryZone::sMaxDepthToOpen = 100U;  // the max by default
bool vkProfQueryZone::sShowLeafMode = false;
float vkProfQueryZone::sContrastRatio = 4.3f;
bool vkProfQueryZone::sActivateLogger = false;
std::vector<vkProfQueryZoneWeak> vkProfQueryZone::sTabbedQueryZones = {};
uint32_t vkProfQueryZone::sCurrentDepth = 0U;
uint32_t vkProfQueryZone::sMaxDepth = 0U;

vkProfQueryZonePtr vkProfQueryZone::create(
    void* vThread, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot) {
    auto res = std::make_shared<vkProfQueryZone>(vThread, vPtr, vName, vSectionName, vIsRoot);
    res->m_This = res;
    return res;
}
vkProfQueryZone::circularSettings vkProfQueryZone::sCircularSettings;

vkProfQueryZone::vkProfQueryZone(void* vThread, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot)
    : m_Context(vThread), name(vName), m_IsRoot(vIsRoot), m_SectionName(vSectionName), m_Ptr(vPtr) {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    depth = vkProfQueryZone::sCurrentDepth;
    imGuiLabel = vName + "##vkProfQueryZone_" + std::to_string((intptr_t)this);
}

vkProfQueryZone::~vkProfQueryZone() {
    name.clear();
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    zonesOrdered.clear();
    zonesDico.clear();
}

void vkProfQueryZone::Clear() {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
}

void vkProfQueryZone::SetStartTimeStamp(const uint64_t& vValue) {
    m_StartTimeStamp = vValue;
    ++m_StartFrameId;
}

void vkProfQueryZone::SetEndTimeStamp(const uint64_t& vValue) {
    m_EndTimeStamp = vValue;
    ++m_EndFrameId;

#ifdef vkProf_DEBUG_MODE_LOGGING
    vkProf_DEBUG_MODE_LOGGING("%*s end id retrieved : %u", depth, "", ids[1]);
#endif

    // start computation of elapsed time
    // no needed after
    // will be used for Graph and labels
    // so DrawMetricGraph must be the first
    ComputeElapsedTime();

    if (vkProfQueryZone::sActivateLogger && zonesOrdered.empty())  // only the leafs
    {
        /*double v = (double)vValue / 1e9;
        LogVarLightInfo("<profiler section=\"%s\" epoch_time=\"%f\" name=\"%s\" render_time_ms=\"%f\">",
            m_SectionName.c_str(), v, name.c_str(), m_ElapsedTime);*/
    }
}

void vkProfQueryZone::ComputeElapsedTime() {
    // we take the last frame
    if (m_StartFrameId == m_EndFrameId) {
        m_AverageStartValue.AddValue(m_StartTimeStamp);  // ns to ms
        m_AverageEndValue.AddValue(m_EndTimeStamp);      // ns to ms
        m_StartTime = (double)(m_AverageStartValue.GetAverage() * 1e-6);
        m_EndTime = (double)(m_AverageEndValue.GetAverage() * 1e-6);
        m_ElapsedTime = m_EndTime - m_StartTime;
    }
}

void vkProfQueryZone::DrawDetails() {
    if (m_StartFrameId) {
        bool res = false;

        ImGui::TableNextColumn();  // tree

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen;

        bool any_childs_to_show = false;
        for (const auto zone : zonesOrdered) {
            if (zone != nullptr && zone->m_ElapsedTime > 0.0) {
                any_childs_to_show = true;
                break;
            }
        }

        if (!any_childs_to_show) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_Highlighted) {
            flags |= ImGuiTreeNodeFlags_Framed;
        }

        const auto colorU32 = ImGui::ColorConvertFloat4ToU32(cv4);
        const bool pushed = PushStyleColorWithContrast(colorU32, ImGuiCol_Text, ImVec4(0, 0, 0, 1), vkProfQueryZone::sContrastRatio);

        ImGui::PushStyleColor(ImGuiCol_Header, cv4);
        const auto hovered_color = ImVec4(cv4.x * 0.9f, cv4.y * 0.9f, cv4.z * 0.9f, 1.0f);
        const auto active_color = ImVec4(cv4.x * 0.8f, cv4.y * 0.8f, cv4.z * 0.8f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hovered_color);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, active_color);

        if (m_IsRoot) {
            res = ImGui::TreeNodeEx(this, flags, "%s : frame [%u]", name.c_str(), m_StartFrameId - 1U);
        } else if (!m_SectionName.empty()) {
            res = ImGui::TreeNodeEx(this, flags, "%s : %s", m_SectionName.c_str(), name.c_str());
        } else {
            res = ImGui::TreeNodeEx(this, flags, "%s", name.c_str());
        }

        ImGui::PopStyleColor(3);

        if (pushed) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered()) {
            m_Highlighted = true;
        }

#ifdef vkProf_SHOW_COUNT
        ImGui::TableNextColumn();  // Elapsed time
        ImGui::Text("%u", last_count);
#endif

        ImGui::TableNextColumn();  // Elapsed time
        ImGui::Text("%.5f ms", m_ElapsedTime);
        ImGui::TableNextColumn();  // Max fps
        if (m_ElapsedTime > 0.0f) {
            ImGui::Text("%.2f f/s", 1000.0f / m_ElapsedTime);
        } else {
            ImGui::Text("%s", "Infinite");
        }
        ImGui::TableNextColumn();  // start time
        ImGui::Text("%.5f ms", m_StartTime);
        ImGui::TableNextColumn();  // end time
        ImGui::Text("%.5f", m_EndTime);

        if (res) {
            m_Expanded = true;
            ImGui::Indent();
            for (const auto zone : zonesOrdered) {
                if (zone != nullptr && zone->m_ElapsedTime > 0.0) {
                    zone->DrawDetails();
                }
            }
            ImGui::Unindent();
        } else {
            m_Expanded = false;
        }
    }
}

bool vkProfQueryZone::DrawFlamGraph(
    vkProfGraphTypeEnum vGraphType, vkProfQueryZoneWeak& vOutSelectedQuery, vkProfQueryZoneWeak vParent, uint32_t vDepth) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    bool pressed = false;
    switch (vGraphType) {
        case vkProfGraphTypeEnum::IN_APP_GPU_HORIZONTAL:  // horizontal flame graph (standard and legacy)
            pressed = m_DrawHorizontalFlameGraph(m_This.lock(), vOutSelectedQuery, vParent, vDepth);
            break;
        case vkProfGraphTypeEnum::IN_APP_GPU_CIRCULAR:  // circular flame graph
            pressed = m_DrawCircularFlameGraph(m_This.lock(), vOutSelectedQuery, vParent, vDepth);
            break;
    }
    return pressed;
}

void vkProfQueryZone::UpdateBreadCrumbTrail() {
    if (parentPtr != nullptr) {
        int32_t _depth = depth;
        vkProfQueryZonePtr _parent_ptr = m_This.lock();
        while (_parent_ptr != rootPtr) {
            _parent_ptr = _parent_ptr->parentPtr;
            if (_parent_ptr && _parent_ptr->depth == (_depth - 1U)) {
                _depth = _parent_ptr->depth;
                if (_depth < m_BreadCrumbTrail.size()) {
                    m_BreadCrumbTrail[_depth] = _parent_ptr;
                } else {
                    DEBUG_BREAK;
                    // maybe you need to define greater value for RECURSIVE_LEVELS_COUNT
                    break;
                }
            }
        }

        // update the imgui title
        imGuiTitle.clear();
        for (uint32_t idx = 0U; idx < depth; ++idx) {
            if (idx < m_BreadCrumbTrail.size()) {
                auto ptr = m_BreadCrumbTrail[idx].lock();
                if (ptr != nullptr) {
                    if (idx > 0U) {
                        imGuiTitle += " > ";
                    }
                    imGuiTitle += ptr->name;
                }
            } else {
                DEBUG_BREAK;
                // maybe you need to define greater value for RECURSIVE_LEVELS_COUNT
                break;
            }
        }
        // add the current
        imGuiTitle += " > " + name;

        // add the unicity string
        imGuiTitle += "##vkProfQueryZone_ " + std::to_string((intptr_t)this);
    }
}

void vkProfQueryZone::DrawBreadCrumbTrail(vkProfQueryZoneWeak& vOutSelectedQuery) {
    ImGui::PushID("DrawBreadCrumbTrail");
    for (uint32_t idx = 0U; idx < depth; ++idx) {
        if (idx < m_BreadCrumbTrail.size()) {
            auto ptr = m_BreadCrumbTrail[idx].lock();
            if (ptr != nullptr) {
                if (idx > 0U) {
                    ImGui::SameLine();
                    ImGui::Text("%s", ">");
                    ImGui::SameLine();
                }
                ImGui::PushID(ptr.get());
                if (ImGui ::Button(ptr->imGuiLabel.c_str())) {
                    vOutSelectedQuery = m_BreadCrumbTrail[idx];
                }
                ImGui::PopID();
            }
        } else {
            DEBUG_BREAK;
            // maybe you need to define greater value for RECURSIVE_LEVELS_COUNT
            break;
        }
    }
    if (depth > 0) {
        ImGui::SameLine();
        ImGui::Text("> %s", name.c_str());
    }
    ImGui::PopID();
}

void vkProfQueryZone::m_DrawList_DrawBar(const char* vLabel, const ImRect& vRect, const ImVec4& vColor, const bool& vHovered) {
    const ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = ImGui::CalcTextSize(vLabel, nullptr, true);

    const auto colorU32 = ImGui::ColorConvertFloat4ToU32(vColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::RenderFrame(vRect.Min, vRect.Max, colorU32, true, 2.0f);
    if (vHovered) {
        const auto selectU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f - cv4.x, 1.0f - cv4.y, 1.0f - cv4.z, 1.0f));
        window->DrawList->AddRect(vRect.Min, vRect.Max, selectU32, true, 0, 2.0f);
    }
    ImGui::PopStyleVar();

    const bool pushed = PushStyleColorWithContrast(colorU32, ImGuiCol_Text, ImVec4(0, 0, 0, 1), vkProfQueryZone::sContrastRatio);
    ImGui::RenderTextClipped(vRect.Min + style.FramePadding, vRect.Max - style.FramePadding,  //
        vLabel, nullptr, &label_size, ImVec2(0.5f, 0.5f), &vRect);
    if (pushed) {
        ImGui::PopStyleColor();
    }
}

bool vkProfQueryZone::m_ComputeRatios(
    vkProfQueryZonePtr vRoot, vkProfQueryZoneWeak vParent, uint32_t vDepth, float& vOutStartRatio, float& vOutSizeRatio) {
    if (depth > vkProfQueryZone::sMaxDepthToOpen) {
        return false;
    }
    if (vRoot == nullptr) {
        vRoot = m_This.lock();
    }
    if (vParent.expired()) {
        vParent = m_This;
    }
    if (vRoot != nullptr && vRoot->m_ElapsedTime > 0.0) {  // avoid div by zero
        if (vDepth == 0) {
            vOutStartRatio = 0.0f;
            vOutSizeRatio = 1.0f;
            if (rootPtr == nullptr) {
                hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / vRoot->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
            } else {
                hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / rootPtr->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
            }
        } else {
            auto parent_ptr = vParent.lock();
            if (parent_ptr) {
                if (parent_ptr->m_ElapsedTime > 0.0) {  // avoid div by zero

                    ////////////////////////////////////////////////////////
                    // for correct rounding isssue with average values
                    if (parent_ptr->m_StartTime > m_StartTime) {
                        m_StartTime = parent_ptr->m_StartTime;
                    }
                    if (parent_ptr->m_EndTime < m_EndTime) {
                        m_EndTime = parent_ptr->m_EndTime;
                    }
                    if (m_EndTime < m_StartTime) {
                        m_EndTime = m_StartTime;
                    }
                    m_ElapsedTime = m_EndTime - m_StartTime;
                    if (m_ElapsedTime < 0.0) {
                        DEBUG_BREAK;
                    }
                    if (m_ElapsedTime > parent_ptr->m_ElapsedTime) {
                        m_ElapsedTime = parent_ptr->m_ElapsedTime;
                    }
                    ////////////////////////////////////////////////////////

                    vOutStartRatio = (float)((m_StartTime - vRoot->m_StartTime) / vRoot->m_ElapsedTime);
                    vOutSizeRatio = (float)(m_ElapsedTime / vRoot->m_ElapsedTime);
                    if (rootPtr == nullptr) {
                        hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / vRoot->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
                    } else {
                        hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / rootPtr->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bool vkProfQueryZone::m_DrawHorizontalFlameGraph(
    vkProfQueryZonePtr vRoot, vkProfQueryZoneWeak& vOutSelectedQuery, vkProfQueryZoneWeak vParent, uint32_t vDepth) {
    bool pressed = false;
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float aw = ImGui::GetContentRegionAvail().x - style.FramePadding.x;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    float barStartRatio = 0.0f;
    float barSizeRatio = 0.0f;
    if (m_ComputeRatios(vRoot, vParent, vDepth, barStartRatio, barSizeRatio)) {
        if (barSizeRatio > 0.0f) {
            if ((zonesOrdered.empty() && vkProfQueryZone::sShowLeafMode) || !vkProfQueryZone::sShowLeafMode) {
                ImGui::PushID(this);
                m_BarLabel = toStr("%s (%.2f ms | %.2f f/s)", name.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
                const char* label = m_BarLabel.c_str();
                const ImGuiID id = window->GetID(label);
                ImGui::PopID();
                float bar_start = aw * barStartRatio;
                float bar_size = aw * barSizeRatio;
                const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
                const float height = label_size.y + style.FramePadding.y * 2.0f;
                const ImVec2 bPos = ImVec2(bar_start + style.FramePadding.x, vDepth * height + style.FramePadding.y);
                const ImVec2 bSize = ImVec2(bar_size - style.FramePadding.x, 0.0f);
                const ImVec2 pos = window->DC.CursorPos + bPos;
                const ImVec2 size = ImVec2(bar_size, height);
                const ImRect bb(pos, pos + size);
                bool hovered, held;
                pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held,  //
                    ImGuiButtonFlags_PressedOnClick |                     //
                        ImGuiButtonFlags_MouseButtonLeft |                //
                        ImGuiButtonFlags_MouseButtonRight);
                if (pressed) {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        vOutSelectedQuery = m_This;  // open in the main window
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        sTabbedQueryZones.push_back(m_This);  // open new window
                    }
                }
                m_Highlighted = false;
                if (hovered) {
                    ImGui::SetTooltip("Section : [%s : %s]\nElapsed time : %.5f ms\nElapsed FPS : %.5f f/s",  //
                        m_SectionName.c_str(), name.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
                    m_Highlighted = true;  // to highlight label graph by this button
                } else if (m_Highlighted) {
                    hovered = true;  // highlight this button by the label graph
                }
                ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
                cv4.w = 1.0f;
                ImGui::RenderNavHighlight(bb, id);
                m_DrawList_DrawBar(label, bb, cv4, hovered);
                ++vDepth;
            }

            // we dont show child if this one have elapsed time to 0.0
            for (const auto zone : zonesOrdered) {
                if (zone != nullptr) {
                    pressed |= zone->m_DrawHorizontalFlameGraph(vRoot, vOutSelectedQuery, m_This, vDepth);
                }
            }
        } else {
#ifdef vkProf_DEBUG_MODE_LOGGING
            vkProf_DEBUG_MODE_LOGGING("Bar Ms not displayed", name.c_str());
#endif
        }
    }

    if (depth == 0 && ((zonesOrdered.empty() && vkProfQueryZone::sShowLeafMode) || !vkProfQueryZone::sShowLeafMode)) {
        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImVec2(aw, ImGui::GetFrameHeight() * (vkProfQueryZone::sMaxDepth + 1U));
        ImGui::ItemSize(size);
        const ImRect bb(pos, pos + size);
        const ImGuiID id = window->GetID((name + "##canvas").c_str());
        if (!ImGui::ItemAdd(bb, id)) {
            return pressed;
        }
    }

    return pressed;
}

bool vkProfQueryZone::m_DrawCircularFlameGraph(
    vkProfQueryZonePtr vRoot, vkProfQueryZoneWeak& vOutSelectedQuery, vkProfQueryZoneWeak vParent, uint32_t vDepth) {
    bool pressed = false;

    if (vDepth == 0U) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Settings")) {
                ImGui::SliderFloat("count points", &sCircularSettings.count_point, 1.0f, 240.0f);
                ImGui::SliderFloat("base_radius", &sCircularSettings.base_radius, 0.0f, 240.0f);
                ImGui::SliderFloat("space", &sCircularSettings.space, 0.0f, 240.0f);
                ImGui::SliderFloat("thick", &sCircularSettings.thick, 0.0f, 240.0f);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    float barStartRatio = 0.0f;
    float barSizeRatio = 0.0f;
    if (m_ComputeRatios(vRoot, vParent, vDepth, barStartRatio, barSizeRatio)) {
        if (barSizeRatio > 0.0f) {
            if ((zonesOrdered.empty() && vkProfQueryZone::sShowLeafMode) || !vkProfQueryZone::sShowLeafMode) {
                ImVec2 center = window->DC.CursorPos + ImGui::GetContentRegionAvail() * 0.5f;
                ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
                cv4.w = 1.0f;

                float min_radius = sCircularSettings.base_radius + sCircularSettings.space * vDepth + sCircularSettings.thick * vDepth;
                float max_radius = sCircularSettings.base_radius + sCircularSettings.space * vDepth + sCircularSettings.thick * (vDepth + 1U);

                auto draw_list_ptr = window->DrawList;
                auto colU32 = ImGui::GetColorU32(cv4);

                float full_length = _1PI_;
                float full_offset = _1PI_;

                float base_st = full_length / sCircularSettings.count_point;

                float bar_start = full_length * barStartRatio;
                float bar_size = full_length * (barStartRatio + barSizeRatio);
                float st = bar_size / ImMax(floor(bar_size / base_st), 3.0f);  // 2 points mini par barre

                ImVector<ImVec2> path;
                float co = 0.0f, si = 0.0f, ac = 0.0f, oc = 0.0f;
                for (ac = bar_start; ac < bar_size; ac += st) {
                    ac = ImMin(ac, bar_size);
                    oc = ac + full_offset;
                    co = std::cos(oc) * sCircularSettings.scaleX;
                    si = std::sin(oc) * sCircularSettings.scaleY;
                    m_P0.x = co * min_radius + center.x;
                    m_P0.y = si * min_radius + center.y;
                    m_P1.x = co * max_radius + center.x;
                    m_P1.y = si * max_radius + center.y;
                    if (ac > bar_start) {
                        // draw_list_ptr->AddQuadFilled(m_P0, m_P1, m_LP1, m_LP0, colU32);
                        draw_list_ptr->AddQuad(m_P0, m_P1, m_LP1, m_LP0, colU32, 2.0f);  // m_BlackU32

                        // draw_list_ptr->PathLineTo(m_P0);
                        // draw_list_ptr->PathLineTo(m_P1);
                        // draw_list_ptr->PathLineTo(m_LP1);
                        // draw_list_ptr->PathLineTo(m_LP0);
                    }
                    m_LP0 = m_P0;
                    m_LP1 = m_P1;
                }
                // draw_list_ptr->PathStroke(colU32, ImDrawFlags_Closed, 2.0f);

#ifdef _DEBUG
                // draw_list_ptr->AddLine(center, center - ImVec2(0, 150.0f), colU32, 2.0f);
#endif

                ++vDepth;
            }

            // we dont show child if this one have elapsed time to 0.0
            // childs
            for (const auto zone : zonesOrdered) {
                if (zone != nullptr) {
                    pressed |= zone->m_DrawCircularFlameGraph(vRoot, vOutSelectedQuery, m_This, vDepth);
                }
            }
        }
    }

    return pressed;
}

////////////////////////////////////////////////////////////
/////////////////////// SCOPED ZONE ////////////////////////
////////////////////////////////////////////////////////////

vkScopedZone::vkScopedZone(       //
    VulkanCoreWeak vVulkanCore,   //
    const VkCommandBuffer& vCmd,  //
    const bool& vIsRoot,          //
    const void* vPtr,             //
    const std::string& vSection,  //
    const char* fmt,              //
    ...) {                        //
    auto corePtr = vVulkanCore.lock();
    assert(corePtr != nullptr);
    profilerPtr = corePtr->getVkProfiler().lock().get();
    assert(profilerPtr != nullptr);
    if (profilerPtr->canRecordTimeStamp()) {
        va_list args;
        va_start(args, fmt);
        static char TempBuffer[256];
        const int w = vsnprintf(TempBuffer, 256, fmt, args);
        va_end(args);
        if (w) {
            const auto& label = std::string(TempBuffer, (size_t)w);
            queryPtr = profilerPtr->GetQueryZoneForName(vPtr, label, vSection, vIsRoot);
            if (queryPtr != nullptr) {
                commandBuffer = vCmd;
                profilerPtr->BeginMarkTime(vCmd, queryPtr);
                vkProfQueryZone::sCurrentDepth++;
            }
        }
    }
}

vkScopedZone::~vkScopedZone() {
    assert(profilerPtr != nullptr);
    if (profilerPtr->canRecordTimeStamp()) {
        assert(queryPtr != nullptr);
#ifdef vkProf_DEBUG_MODE_LOGGING
        if (queryPtr->depth > 0) {
            vkProf_DEBUG_MODE_LOGGING("%*s end : [%u:%u] (depth:%u)",  //
                (queryPtr->depth - 1U), "", queryPtr->ids[0], queryPtr->ids[1], queryPtr->depth);
        } else {
            vkProf_DEBUG_MODE_LOGGING("end : [%u:%u] (depth:%u)",  //
                queryPtr->ids[0], queryPtr->ids[1], 0);
        }
#endif
        profilerPtr->EndMarkTime(commandBuffer, queryPtr);
        ++queryPtr->current_count;
        --vkProfQueryZone::sCurrentDepth;
    }
}

////////////////////////////////////////////////////////////
/////////////////////// 3D PROFILER ////////////////////////
////////////////////////////////////////////////////////////

vkProfilerPtr vkProfiler::create(VulkanCoreWeak vVulkanCore, const uint32_t& vMaxQueryCount) {
    auto res = std::make_shared<vkProfiler>();
    res->m_This = res;
    if (!res->Init(vVulkanCore, vMaxQueryCount)) {
        res.reset();
    }
    return res;
}

vkProfiler::~vkProfiler() {
    Unit();
};

bool vkProfiler::Init(VulkanCoreWeak vVulkanCore, const uint32_t& vMaxQueryCount) {
    m_VulkanCore = vVulkanCore;

    m_QueryCount = vMaxQueryCount;  // base count

    vk::QueryPoolCreateInfo poolInfos = {};
    poolInfos.setQueryCount((uint32_t)m_QueryCount);
    poolInfos.setQueryType(vk::QueryType::eTimestamp);

    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);

    vk::Result creation_result = vk::Result::eNotReady;
    while ((creation_result = corePtr->getDevice().createQueryPool(&poolInfos, nullptr, &m_QueryPool)) != vk::Result::eSuccess){
        m_QueryCount /= 2U;
        poolInfos.queryCount = (uint32_t)m_QueryCount;
    }

    m_FrameFences[0] = corePtr->getDevice().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    m_FrameFences[1] = corePtr->getDevice().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    auto vkDevicePtr = corePtr->getFrameworkDevice().lock();
    assert(vkDevicePtr != nullptr);
    auto cmdPools = vkDevicePtr->getQueue(vk::QueueFlagBits::eGraphics).cmdPools;
    auto cmds = corePtr->getDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo(cmdPools, vk::CommandBufferLevel::ePrimary, 2));
    m_FrameCommandBuffers[0] = cmds[0];
    m_FrameCommandBuffers[1] = cmds[1];

    m_QueryTail = 0U;
    m_QueryHead = 0U;

    return (creation_result == vk::Result::eSuccess);
}

void vkProfiler::Unit() {
    Clear();
    auto corePtr = m_VulkanCore.lock();
    assert(corePtr != nullptr);
    corePtr->getDevice().waitIdle();
    corePtr->getDevice().destroyQueryPool(m_QueryPool);
    corePtr->getDevice().destroyFence(m_FrameFences[0]);
    corePtr->getDevice().destroyFence(m_FrameFences[1]);
}

void vkProfiler::Clear() {
    m_RootZone.reset();
    m_QueryIDToZone.clear();
    m_DepthToLastZone.clear();
    m_TimeStampMeasures.clear();
}

void vkProfiler::Collect() {
    if (canRecordTimeStamp()) {
        // the query zone stack must be empty
        // else we have an error, some missing endZone
        assert(m_QueryStack.empty());

        auto corePtr = m_VulkanCore.lock();
        assert(corePtr != nullptr);
        if (!m_TimeStampMeasures.empty() &&  //
                                             // this version is better than the vkhpp one
                                             // sicne the vkhpp have a un wanted copy of the arry dtats
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetQueryPoolResults(corePtr->getDevice(), m_QueryPool, (uint32_t)m_QueryTail, (uint32_t)m_QueryHead,
                sizeof(vkTimeStamp) * m_TimeStampMeasures.size(), m_TimeStampMeasures.data(), sizeof(vkTimeStamp),
                VK_QUERY_RESULT_64_BIT /* | VK_QUERY_RESULT_WAIT_BIT*/) == VK_NOT_READY) {
            return;
        }

        corePtr->getDevice().resetQueryPool(m_QueryPool, 0U, (uint32_t)m_QueryCount);

        for (size_t id = 0; id < m_TimeStampMeasures.size(); ++id) {
            auto query_it = m_QueryIDToZone.find((uint32_t)id);
            if (query_it != m_QueryIDToZone.end()) {
                vkTimeStamp value64 = m_TimeStampMeasures[id];
                auto ptr = query_it->second;
                if (ptr != nullptr) {
                    if (id == ptr->ids[0]) {
                        ptr->SetStartTimeStamp(value64);
                    } else if (id == ptr->ids[1]) {
                        ptr->last_count = ptr->current_count;
                        ptr->current_count = 0U;
                        ptr->SetEndTimeStamp(value64);
                    } else {
                        DEBUG_BREAK;
                    }
                }
            }
        }
    }
}

bool& vkProfiler::isActiveRef() {
    return m_IsActive;
}

const bool& vkProfiler::isActive() {
    return m_IsActive;
}

bool& vkProfiler::isPausedRef() {
    return m_IsPaused;
}

const bool& vkProfiler::isPaused() {
    return m_IsPaused;
}

const bool vkProfiler::canRecordTimeStamp() {
    return m_IsActive && !m_IsPaused;
}

void vkProfiler::DrawFlamGraph(const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags) {
    if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor(vLabel, pOpen, vFlags | ImGuiWindowFlags_MenuBar)) {
        DrawFlamGraphNoWin();
    }
    if (m_ImGuiEndFunctor != nullptr) {
        m_ImGuiEndFunctor();
    }

    DrawFlamGraphChilds(vFlags);

    DrawDetails(vFlags);
}

void vkProfiler::DrawFlamGraphNoWin() {
    if (isActive()) {
        m_DrawMenuBar();
        DrawFlamGraph(m_GraphType);
    }
}

void vkProfiler::DrawFlamGraphChilds(ImGuiWindowFlags vFlags) {
    m_SelectedQuery.reset();
    m_QueryZoneToClose = -1;
    for (size_t idx = 0U; idx < vkProfQueryZone::sTabbedQueryZones.size(); ++idx) {
        auto ptr = vkProfQueryZone::sTabbedQueryZones[idx].lock();
        if (ptr != nullptr) {
            bool opened = true;
            ImGui::SetNextWindowSizeConstraints(ImVec2(300, 100), ImGui::GetIO().DisplaySize);
            if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor(ptr->imGuiTitle.c_str(), &opened, vFlags)) {
                if (isActive()) {
                        ptr->DrawFlamGraph(m_GraphType, m_SelectedQuery);
                }
            }
            if (m_ImGuiEndFunctor != nullptr) {
                m_ImGuiEndFunctor();
            }
            if (!opened) {
                m_QueryZoneToClose = (int32_t)idx;
            }
        }
    }
    if (m_QueryZoneToClose > -1) {
        vkProfQueryZone::sTabbedQueryZones.erase(  //
            vkProfQueryZone::sTabbedQueryZones.begin() + m_QueryZoneToClose);
    }
}

void vkProfiler::SetImGuiBeginFunctor(const ImGuiBeginFunctor& vImGuiBeginFunctor) {
    m_ImGuiBeginFunctor = vImGuiBeginFunctor;
}

void vkProfiler::SetImGuiEndFunctor(const ImGuiEndFunctor& vImGuiEndFunctor) {
    m_ImGuiEndFunctor = vImGuiEndFunctor;
}

void vkProfiler::m_DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (vkProfQueryZone::sMaxDepth) {
            vkProfQueryZone::sMaxDepthToOpen = vkProfQueryZone::sMaxDepth;
        }

        PlayPauseButton(isPausedRef());

        if (ImGui ::Button("Details")) {
            m_ShowDetails = !m_ShowDetails;
        }

#ifdef vkProf_DEV_MODE
        ImGui::Checkbox("Logging", &vkProfQueryZone::sActivateLogger);

        if (ImGui::BeginMenu("Graph Types")) {
            if (ImGui::MenuItem("Horizontal", nullptr, m_GraphType == vkProfGraphTypeEnum::IN_APP_GPU_HORIZONTAL)) {
                m_GraphType = vkProfGraphTypeEnum::IN_APP_GPU_HORIZONTAL;
            }
            if (ImGui::MenuItem("Circular", nullptr, m_GraphType == vkProfGraphTypeEnum::IN_APP_GPU_CIRCULAR)) {
                m_GraphType = vkProfGraphTypeEnum::IN_APP_GPU_CIRCULAR;
            }
            ImGui::EndMenu();
        }
#endif

        ImGui::EndMenuBar();
    }
}

void vkProfiler::DrawFlamGraph(const vkProfGraphTypeEnum& vGraphType) {
    if (m_RootZone != nullptr) {
        if (!m_SelectedQuery.expired()) {
            auto ptr = m_SelectedQuery.lock();
            if (ptr) {
                ptr->DrawBreadCrumbTrail(m_SelectedQuery);
                ptr->DrawFlamGraph(vGraphType, m_SelectedQuery);
            }
        } else {
            m_RootZone->DrawFlamGraph(vGraphType, m_SelectedQuery);
        }
    }
}

vkProfQueryZonePtr vkProfiler::GetQueryZoneForName(const void* vPtr, const std::string& vName, const std::string& vSection, const bool& vIsRoot) {
    vkProfQueryZonePtr res = nullptr;

    //////////////// CREATION ///////////////////

    // there is many link issues with 'max' in cross compilation so we dont using it
    if (vkProfQueryZone::sCurrentDepth > vkProfQueryZone::sMaxDepth) {
        vkProfQueryZone::sMaxDepth = vkProfQueryZone::sCurrentDepth;
    }

    if (vkProfQueryZone::sCurrentDepth == 0) {  // root zone
        m_DepthToLastZone.clear();
        if (m_RootZone == nullptr) {
            res = vkProfQueryZone::create(m_Context, vPtr, vName, vSection, vIsRoot);
            if (res != nullptr) {
                res->ids[0] = m_GetNextQueryId();
                res->ids[1] = m_GetNextQueryId();
                res->depth = vkProfQueryZone::sCurrentDepth;
                res->UpdateBreadCrumbTrail();
                m_QueryIDToZone[res->ids[0]] = res;
                m_QueryIDToZone[res->ids[1]] = res;
                m_RootZone = res;
            }
        } else {
            res = m_RootZone;
        }
    } else {  // else child zone
        auto root = m_GetQueryZoneFromDepth(vkProfQueryZone::sCurrentDepth - 1U);
        if (root != nullptr) {
            bool found = false;
            const auto& key_str = vSection + vName;
            const auto& ptr_iter = root->zonesDico.find(vPtr);
            if (ptr_iter == root->zonesDico.end()) {  // not found
                found = false;
            } else {                                                                 // found
                found = (ptr_iter->second.find(key_str) != ptr_iter->second.end());  // not found
            }
            if (!found) {  // not found
                res = vkProfQueryZone::create(m_Context, vPtr, vName, vSection, vIsRoot);
                if (res != nullptr) {
                        res->ids[0] = m_GetNextQueryId();
                        res->ids[1] = m_GetNextQueryId();
                    res->parentPtr = root;
                    res->rootPtr = m_RootZone;
                    res->depth = vkProfQueryZone::sCurrentDepth;
                    res->UpdateBreadCrumbTrail();
                    m_QueryIDToZone[res->ids[0]] = res;
                    m_QueryIDToZone[res->ids[1]] = res;
                    root->zonesDico[vPtr][key_str] = res;
                    root->zonesOrdered.push_back(res);
                } else {
                    DEBUG_BREAK;
                }
            } else {
                res = root->zonesDico[vPtr][key_str];
            }
        } else {
            return res;  // happen when profiling is activated inside a profiling zone
        }
    }

    //////////////// UTILISATION ////////////////

    if (res != nullptr) {
        m_SetQueryZoneForDepth(res, vkProfQueryZone::sCurrentDepth);
        if (res->name != vName) {
            // at depth 0 there is only one frame
            LogVarDebugError("was registerd at depth %u %s. but we got %s\nwe clear the profiler",  //
                vkProfQueryZone::sCurrentDepth, res->name.c_str(), vName.c_str());
            // maybe the scoped frame is taken outside of the main frame
            Clear();
        }
    }

    return res;
}

void vkProfiler::BeginMarkTime(const VkCommandBuffer& vCmd, vkProfQueryZoneWeak vQueryZone) {
    auto queryPtr = vQueryZone.lock();
    assert(queryPtr != nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdWriteTimestamp(vCmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_QueryPool, queryPtr->ids[0]);
#ifdef vkProf_DEBUG_MODE_LOGGING
    vkProf_DEBUG_MODE_LOGGING("%*s begin : [%u:%u] (depth:%u) (%s)",  //
        vQueryPtr->depth, "", vQueryPtr->ids[0], vQueryPtr->ids[1], vQueryPtr->depth, label.c_str());
#endif
}

void vkProfiler::EndMarkTime(const VkCommandBuffer& vCmd, vkProfQueryZoneWeak vQueryZone) {
    auto queryPtr = vQueryZone.lock();
    assert(queryPtr != nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdWriteTimestamp(vCmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_QueryPool, queryPtr->ids[1]);
}

void vkProfiler::BeginFrame(const char* vLabel) {
    if (canRecordTimeStamp()) {
        auto corePtr = m_VulkanCore.lock();
        assert(corePtr != nullptr);
        corePtr->getDevice().resetFences(m_FrameFences);
        m_FrameCommandBuffers[0].begin(vk::CommandBufferBeginInfo());
        beginZone(m_FrameCommandBuffers[0], true, nullptr, "", "%s", vLabel);
        m_FrameCommandBuffers[0].end();
        auto submitInfos = vk::SubmitInfo(0, nullptr, nullptr, 1, &m_FrameCommandBuffers[0], 0, nullptr);
        if (GaiApi::VulkanSubmitter::Submit(m_VulkanCore, vk::QueueFlagBits::eGraphics, submitInfos, m_FrameFences[0])) {
            corePtr->getDevice().waitForFences(1, &m_FrameFences[0], VK_TRUE, UINT64_MAX);
        }
    }
}

void vkProfiler::EndFrame() {
    if (canRecordTimeStamp() && !m_QueryStack.empty()) {
        m_FrameCommandBuffers[1].begin(vk::CommandBufferBeginInfo());
        if (endZone(m_FrameCommandBuffers[1])) {
            m_FrameCommandBuffers[1].end();
            auto submitInfos = vk::SubmitInfo(0, nullptr, nullptr, 1, &m_FrameCommandBuffers[1], 0, nullptr);
            if (GaiApi::VulkanSubmitter::Submit(m_VulkanCore, vk::QueueFlagBits::eGraphics, submitInfos, m_FrameFences[1])) {
                auto corePtr = m_VulkanCore.lock(); assert(corePtr != nullptr);
                corePtr->getDevice().waitForFences(1, &m_FrameFences[1], VK_TRUE, UINT64_MAX);
            }
        }
    }
}

bool vkProfiler::beginZone(const VkCommandBuffer& vCmd, const bool& vIsRoot, const void* vPtr, const std::string& vSection, const char* fmt, ...) {
    if (canRecordTimeStamp()) {
        va_list args;
        va_start(args, fmt);
        static char TempBuffer[256];
        const int w = vsnprintf(TempBuffer, 256, fmt, args);
        va_end(args);
        if (w) {
            const auto& label = std::string(TempBuffer, (size_t)w);
            auto queryZonePtr = GetQueryZoneForName(vPtr, label, vSection, vIsRoot);
            if (queryZonePtr != nullptr) {
                m_QueryStack.push(queryZonePtr);
                BeginMarkTime(vCmd, queryZonePtr);
                ++vkProfQueryZone::sCurrentDepth;
                return true;
            }
        }
    }
    return false;
}

bool vkProfiler::endZone(const VkCommandBuffer& vCmd) {
    if (canRecordTimeStamp() && !m_QueryStack.empty()) {
        auto queryZone = m_QueryStack.top();
        m_QueryStack.pop();
        auto queryPtr = queryZone.lock();
        if (queryPtr != nullptr) {
            EndMarkTime(vCmd, queryPtr);
            ++queryPtr->current_count;
            --vkProfQueryZone::sCurrentDepth;
            return true;
        }
    }
    return false;
}

void vkProfiler::DrawDetails(ImGuiWindowFlags vFlags) {
    if (m_ShowDetails) {
        if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor("Profiler Details", &m_ShowDetails, vFlags)) {
            DrawDetailsNoWin();
        }
        if (m_ImGuiEndFunctor != nullptr) {
            m_ImGuiEndFunctor();
        }
    }
}

void vkProfiler::DrawDetailsNoWin() {
    if (!isActive()) {
        return;
    }

    if (m_RootZone != nullptr) {
        int32_t count_tables = 5;
#ifdef vkProf_SHOW_COUNT
        ++count_tables;
#endif
        static ImGuiTableFlags flags =        //
            ImGuiTableFlags_SizingFixedFit |  //
            ImGuiTableFlags_RowBg |           //
            ImGuiTableFlags_Hideable |        //
            ImGuiTableFlags_ScrollY |         //
            ImGuiTableFlags_NoHostExtendY;
        const auto& size = ImGui::GetContentRegionAvail();
        auto listViewID = ImGui::GetID("##vkProfiler_DrawDetails");
        if (ImGui::BeginTableEx("##vkProfiler_DrawDetails", listViewID, count_tables, flags, size, 0.0f)) {
            ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthStretch);
#ifdef vkProf_SHOW_COUNT
            ImGui::TableSetupColumn("Count");
#endif
            ImGui::TableSetupColumn("Elapsed time");
            ImGui::TableSetupColumn("Max fps");
            ImGui::TableSetupColumn("Start time");
            ImGui::TableSetupColumn("End time");
            ImGui::TableHeadersRow();
            m_RootZone->DrawDetails();
            ImGui::EndTable();
        }
    }
}


void vkProfiler::m_SetQueryZoneForDepth(vkProfQueryZonePtr vvkProfQueryZone, uint32_t vDepth) {
    m_DepthToLastZone[vDepth] = vvkProfQueryZone;
}

vkProfQueryZonePtr vkProfiler::m_GetQueryZoneFromDepth(uint32_t vDepth) {
    vkProfQueryZonePtr res = nullptr;

    if (m_DepthToLastZone.find(vDepth) != m_DepthToLastZone.end()) {  // found
        res = m_DepthToLastZone[vDepth];
    }

    return res;
}

int32_t vkProfiler::m_GetNextQueryId() {
    const auto id = m_QueryHead;
    m_QueryHead = (m_QueryHead + 1) % m_QueryCount;
    assert(m_QueryHead != m_QueryTail);
    m_TimeStampMeasures.push_back(0);  // append a value
    return (uint32_t)id;
}

}  // namespace GaiApi
