#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include "Vector.h"

class MaterialManager {

public:

	struct Material {
		std::string name;
		PT::Vector3 color;
		float roughness;
		float metallic;
		float ior;
		float transmission;
		float emission;
		std::string textureMap;
		std::string roughnessMap;
		std::string metallicMap;
		std::string emissionMap;
		std::string normalMap;
		std::string displacementMap;
	};

	MaterialManager() {};
	~MaterialManager() {
		cleanUp();
	};

	void createMaterial(Material* material, std::string name) {
		materials[name] = material;
	}

	void loadMaterials(std::string material_file_name);

	void saveMaterials(std::string material_file_name);

	void initDefaultMaterials();

	void initTextures();

	void cleanUp() {
		for (auto const& [name, material] : materials) {
			materials.erase(name);
			delete material;
		}
	
	}

	std::unordered_map<std::string, Material*> materials;

	// name -> relative path
	// model "button" = "button_albedo.png"
	std::unordered_map<std::string, std::string> albedos;
	std::unordered_map<std::string, std::string> roughness;
	std::unordered_map<std::string, std::string> metallic;
	std::unordered_map<std::string, std::string> emissive;
	std::unordered_map<std::string, std::string> normal;
};

