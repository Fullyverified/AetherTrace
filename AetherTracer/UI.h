#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Vector.h"
#include "Config.h"


class MeshManager;
class EntityManager;

class UI {
public:

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
	static void updateUIEntities();

	static std::vector<const char*> models;
	static int meshSelection;
	static std::vector<const char*> entities;
	static int entitySelection;

	static int renaming_index;
	static char renaming_buffer[128];

	static PT::Vector3 position;
	static PT::Vector3 rotation;
	static PT::Vector3 scale;

	static MeshManager* meshManager;
	static EntityManager* entityManager;
};

