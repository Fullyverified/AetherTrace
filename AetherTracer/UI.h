#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Vector.h"
#include "Config.h"


class MeshManager;
class MaterialManager;
class EntityManager;


class UI {
public:

	static bool materialUpdate;
	static bool accelUpdate;
	static bool accumulationUpdate;
	static bool isWindowHovered;
	static bool renderUI;


	static void renderSettings();

	static float frameTime;
	static uint64_t raysPerSecond;
	static uint32_t numRays;
	static int current_tone_mapper;


	static void sceneEditor();
	static void updateUIModels();
	static void updateUIentity();

	static std::vector<const char*> models;
	static int mesh_selection_idx;
	static std::vector<const char*> entity;
	static int entity_selection_idx;

	static std::vector<const char*> materials;
	static int material_selection_idx;

	static int renaming_index_entity;
	static char renaming_buffer_entity[128];

	static int deleting_index_entity;

	static PT::Vector3 position;
	static PT::Vector3 rotation;
	static PT::Vector3 scale;

	static void materialEditor();
	static void updateUIMaterials();

	static int renaming_index_material;
	static char renaming_buffer_material[128];

	static PT::Vector3 color;
	static float roughness;
	static float metallic;
	static float IOR;
	static float transmission;
	static float emission;

	static MeshManager* meshManager;
	static EntityManager* entityManager;
	static MaterialManager* materialManager;
};

