#include "UI.h"
#include "Imgui.h"

#include "MeshManager.h"
#include "EntityManager.h"

MeshManager* UI::meshManager = nullptr;
EntityManager* UI::entityManager = nullptr;

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

    ImGui::Checkbox("Accumulate", &config.accumulate);

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

    if (ImGui::Checkbox("Jitter", &config.jitter)) {
        accumulationUpdate = true;
    }

    // Drop down list of materials

    const char* tone_mappers[] = { "Reinhard Extended", "Hable Filmic", "Aces Filmic" };

    ImGui::Text("Tone Mapping: ");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    ImGui::DragFloat("##White Point", &config.whitepoint, 0.1f, 0.0f, 0.0f, "White Point: %.3f");

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::Combo("##Tone Mapping", &current_tone_mapper, tone_mappers, static_cast<int>(Tone_Mapper::Count))) {
        config.tone_mapper = static_cast<Tone_Mapper>(current_tone_mapper);
    }

    ImGui::End();

}

std::vector<const char*> UI::models;
int UI::meshSelection = 0;
std::vector<const char*> UI::entities;
int UI::entitySelection = 0;

PT::Vector3 UI::position = {};
PT::Vector3 UI::rotation = {};
PT::Vector3 UI::scale = {};


void UI::sceneEditor() {
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.5f);

    ImGui::Begin("Scene Editor", nullptr, ImGuiWindowFlags_None);

    if (ImGui::Checkbox("Sky", &config.sky)) {
        accumulationUpdate = true;
    }

    // Drop down to select a mesh Type

    ImGui::Combo("##Mesh Objects", &meshSelection, UI::models.data(), static_cast<int>(UI::models.size()));

    ImGui::SameLine();

    if (ImGui::Button("Add Entity")) {

        entityManager->addEntity(UI::models[meshSelection], "Test");
        accumulationUpdate = true;
        accelUpdate = true;
    }


    // Drop down to select an Entity

    ImGui::Combo("##Entities", &entitySelection, UI::entities.data(), static_cast<int>(UI::entities.size()));

    ImGui::Text("Translation: ");
    position = entityManager->entitys[entitySelection]->position;

    // X Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Position X", &position.x, 0.1f, 0.0f, 0.0f, "X %.3f")) {

        entityManager->entitys[entitySelection]->position = position;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    // Y Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Position Y", &position.y, 0.1f, 0.0f, 0.0f, "Y %.3f")) {
        entityManager->entitys[entitySelection]->position = position;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    // Z Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Position Z", &position.z, 0.1f, 0.0f, 0.0f, "Z %.3f")) {
        entityManager->entitys[entitySelection]->position = position;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    ImGui::Text("Rotation: ");
    rotation = entityManager->entitys[entitySelection]->rotation;

    // X Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Rotation X", &rotation.x, 0.1f, 0.0f, 0.0f, "X %.3f")) {

        entityManager->entitys[entitySelection]->rotation = rotation;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    // Y Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Rotation Y", &rotation.y, 0.1f, 0.0f, 0.0f, "Y %.3f")) {
        entityManager->entitys[entitySelection]->rotation = rotation;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    // Z Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##rotation Z", &rotation.z, 0.1f, 0.0f, 0.0f, "Z %.3f")) {
        entityManager->entitys[entitySelection]->rotation = rotation;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    ImGui::Text("Scale: ");
    scale = entityManager->entitys[entitySelection]->scale;

    // X Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Scale X", &scale.x, 0.1f, 0.0f, 0.0f, "X %.3f")) {

        entityManager->entitys[entitySelection]->scale = scale;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    // Y Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Scale Y", &scale.y, 0.1f, 0.0f, 0.0f, "Y %.3f")) {
        entityManager->entitys[entitySelection]->scale = scale;
        accumulationUpdate = true;
        accelUpdate = true;
    }

    // Z Axis
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); // Set width to the available space
    if (ImGui::DragFloat("##Scale Z", &scale.z, 0.1f, 0.0f, 0.0f, "Z %.3f")) {
        entityManager->entitys[entitySelection]->scale = scale;
        accumulationUpdate = true;
        accelUpdate = true;
    }
 
    
    ImGui::End();
}

void UI::updateUIModels() {

    UI::models.clear();

    for (size_t i = 0; i < meshManager->models.size(); i++) {
        UI::models.push_back(meshManager->models[i].c_str());
    }

}

void UI::updateUIEntities() {

    UI::entities.clear();

    for (size_t i = 0; i < entityManager->entitys.size(); i++) {
        UI::entities.push_back(entityManager->entitys[i]->name.c_str());
    }

}