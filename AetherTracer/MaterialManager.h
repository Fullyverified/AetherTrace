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

	struct Texture {
		std::string name;
		int width, height, channels;
		unsigned char* data;

	};

	MaterialManager() {};
	~MaterialManager() {
		cleanUp();
	};

	void createMaterial();

	void loadMaterials(std::string material_file_name);

	void saveMaterials(std::string material_file_name);

	void initDefaultMaterials();

	void loadTextures();
	void saveTextures();
	
	void loadTextureFromFile(std::string path);

	void cleanUp() {
		for (auto const& [name, material] : materials) {
			materials.erase(name);
			delete material;
		}
	
	}

	std::unordered_map<std::string, Material*> materials;
	std::unordered_map<std::string, Texture*> textures;
};

