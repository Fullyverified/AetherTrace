#include "UI.h"
#include "Imgui.h"

#include <sstream>
#include <iomanip>  // For std::fixed and std::setprecision

bool UI::isWindowHovered = false;
bool UI::accelUpdate = false; // reset by the renderer
bool UI::accumulationUpdate = false; // reset by the renderer
bool UI::renderUI = true;

uint64_t UI::raysPerSecond = 0;
float UI::frameTime = 0;
uint32_t UI::numRays = 0;

int UI::current_tone_mapper = static_cast<int>(config.tone_mapper);

void UI::renderSettings() {

    isWindowHovered = ImGui::GetIO().WantCaptureMouse ? true : false;

    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.5f);

    ImGui::Begin("Render Settings", nullptr, ImGuiWindowFlags_None);

    raysPerSecond = (config.resX * config.resY) * (1000.0f / frameTime);
    ImGui::Text("Rays /s: %u", raysPerSecond);

    std::string frameTimeStr = std::to_string(static_cast<uint64_t>(frameTime * 1000)) + " ms";
    ImGui::Text("Frame Time: %s", frameTimeStr.c_str());

    ImGui::Text("Num Rays: %d", numRays);

    if (ImGui::Checkbox("Accumulate", &config.accumulate));

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::SliderInt("##Samples Per Pixel", &config.raysPerPixel, 1, 50, "Samples Per Pixel %i")) {
        accumulationUpdate = true;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::SliderInt("##Min ray bounces", &config.minBounces, 0, config.minBouncesMax, "Min Bounces %i")) {
        if (config.maxBounces < config.minBounces) {
            config.maxBounces = config.minBounces;
            config.maxBounces = config.maxBounces;
        }
        accumulationUpdate = true;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::SliderInt("##Max ray bounces", &config.maxBounces, 0, config.maxBouncesMax, "Max bounces %i")) {
        if (config.maxBounces < config.minBounces) {
            config.maxBounces = config.minBounces;
        }
        accumulationUpdate = true;
    }

    if (ImGui::Checkbox("Sky", &config.sky)) {
        accumulationUpdate = true;
    }

    if (ImGui::Checkbox("Jitter", &config.jitter)) {
        accumulationUpdate = true;
    }

    // Drop down list of materials

    const char* tone_mappers[] = { "Reinhard Extended", "Hable Filmic", "Aces Filmic" };

    ImGui::Text("Tone Mapping: ");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::DragFloat("##White Point", &config.whitepoint, 0.1f, 0.0f, 0.0f, "White Point: %.3f"))

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::Combo("##Tone Mapping", &current_tone_mapper, tone_mappers, static_cast<int>(Tone_Mapper::Count))) {
        config.tone_mapper = static_cast<Tone_Mapper>(current_tone_mapper);
    }

    ImGui::End();

}