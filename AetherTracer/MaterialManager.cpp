#include "MaterialManager.h"

#include <simdjson.h>
#include <fstream>
#include <string>

void MaterialManager::createMaterial() {

    Material* new_material = new Material{};

    new_material->name = "New Material";
    new_material->color = { 1.0f, 1.0f, 1.0f };
    new_material->roughness = 1.0f;
    new_material->metallic = 0.0f;
    new_material->ior = 0.0f;
    new_material->transmission = 0.0f;

    materials["New Material"] = new_material;
}

void MaterialManager::loadMaterials(std::string material_file_name) {


    std::string filepath = "assets/materials/" + material_file_name + ".json";

    std::ifstream in(filepath, std::ios::binary | std::ios::ate);

    if (!in) {
        std::cout << "incorrect file name or path" << std::endl;
        return;
    }

    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::string buffer(size, ' ');
    in.read(buffer.data(), size);

    // parse
    simdjson::ondemand::parser parser;
    simdjson::padded_string padded = simdjson::padded_string(std::move(buffer));

    simdjson::ondemand::document doc;
    auto error = parser.iterate(padded).get(doc);
    if (error) {
        std::cout << "Parsing error: " << error << std::endl;
        return;
    }

    simdjson::ondemand::object root;
    error = doc.get_object().get(root);
    if (error) {
        std::cout << "Error getting root object: " << error << std::endl;
        return;
    }

    // materials
    simdjson::ondemand::array materials_arr;
    error = root["materials"].get_array().get(materials_arr);
    if (error) {
        std::cout << "Error getting materials array: " << error << std::endl;
        return;
    }

    materials.clear();

    std::string_view sv;
    double val;
    size_t idx;

    size_t material_idx = 0;
    for (auto material_val : materials_arr) {

        std::cout << "material_idx: " << material_idx << std::endl;
        material_idx++;

        simdjson::ondemand::object material_obj;
        material_val.get_object().get(material_obj);

        Material* material = new Material{};

        // material name
        material_val["material_name"].get_string().get(sv);
        material->name = sv;

        // color
        simdjson::ondemand::array col_arr;
        error = material_val["color"].get_array().get(col_arr);
        idx = 0;
        for (auto val_arr : col_arr) {
            val_arr.get_double().get(val);
          
            if (idx == 0) material->color.x = static_cast<float>(val);
            else if (idx == 1) material->color.y = static_cast<float>(val);
            else if (idx == 2) material->color.z = static_cast<float>(val);
            idx++;
        }


        // roughness

        material_val["roughness"].get_double().get(val);
        material->roughness = static_cast<float>(val);

        // metallic

        material_val["metallic"].get_double().get(val);
        material->metallic = static_cast<float>(val);

        // ior

        material_val["ior"].get_double().get(val);
        material->ior = static_cast<float>(val);

        // tranmission

        material_val["transmission"].get_double().get(val);
        material->transmission = static_cast<float>(val);

        // emission

        material_val["emission"].get_double().get(val);
        material->emission = static_cast<float>(val);

        // texture map

        material_val["texture_map"].get_string().get(sv);
        material->textureMap = sv;

        // roughness map

        material_val["roughness_map"].get_string().get(sv);
        material->roughnessMap = sv;

        // metallic map

        material_val["metallic_map"].get_string().get(sv);
        material->metallicMap = sv;

        // emission map

        material_val["emission_map"].get_string().get(sv);
        material->emissionMap = sv;

        // normal map

        material_val["mormal_map"].get_string().get(sv);
        material->normalMap = sv;

        // displacement map

        material_val["displacement_map"].get_string().get(sv);
        material->displacementMap = sv;

        std::cout << "material name: " << material->name << std::endl;
        std::cout << "material color: " << material->color.x << ", " << material->color.y << ", " << material->color.z << std::endl;
        std::cout << "material roughness: " << material->roughness << std::endl;
        std::cout << "material metallic: " << material->metallic << std::endl;

        materials[material->name] = material;
    }


}

void MaterialManager::saveMaterials(std::string material_file_name) {

    simdjson::builder::string_builder builder(64 * 1024);

    builder.start_object();

    // Entities
    builder.escape_and_append_with_quotes("materials");
    builder.append_colon();
    builder.start_array();
    bool first_material = true;
    for (const auto& pair : materials) {

        Material* material = pair.second;

        if (!first_material) {
            builder.append_comma();
        }
        first_material = false;
        builder.start_object();
        // mesh_name
        builder.escape_and_append_with_quotes("material_name");
        builder.append_colon();
        builder.escape_and_append_with_quotes(material->name);
        // comma
        builder.append_comma();
        // position
        builder.escape_and_append_with_quotes("color");
        builder.append_colon();
        builder.start_array();
        builder.append(material->color.x);
        builder.append_comma();
        builder.append(material->color.y);
        builder.append_comma();
        builder.append(material->color.z);
        builder.end_array();
        // comma
        builder.append_comma();
        // roughness
        builder.escape_and_append_with_quotes("roughness");
        builder.append_colon();
        builder.append(material->roughness);
        // comma
        builder.append_comma();
        // metallic
        builder.escape_and_append_with_quotes("metallic");
        builder.append_colon();
        builder.append(material->metallic);
        // comma
        builder.append_comma();
        // ior
        builder.escape_and_append_with_quotes("ior");
        builder.append_colon();
        builder.append(material->ior);
        // comma
        builder.append_comma();
        // tranmission
        builder.escape_and_append_with_quotes("transmission");
        builder.append_colon();
        builder.append(material->transmission);
        // comma
        builder.append_comma();
        // emission
        builder.escape_and_append_with_quotes("emission");
        builder.append_colon();
        builder.append(material->emission);
        // comma
        builder.append_comma();
        // texture map
        builder.escape_and_append_with_quotes("texture_map");
        builder.append_colon();
        builder.escape_and_append_with_quotes(material->textureMap);
        // comma
        builder.append_comma();
        // roughness map
        builder.escape_and_append_with_quotes("roughness_map");
        builder.append_colon();
        builder.escape_and_append_with_quotes(material->roughnessMap);
        // comma
        builder.append_comma();
        // metallic map
        builder.escape_and_append_with_quotes("metallic_map");
        builder.append_colon();
        builder.escape_and_append_with_quotes(material->metallicMap);
        // comma
        builder.append_comma();
        // emission map
        builder.escape_and_append_with_quotes("emission_map");
        builder.append_colon();
        builder.escape_and_append_with_quotes(material->emissionMap);
        // comma
        builder.append_comma();
        // normal map
        builder.escape_and_append_with_quotes("normal_map");
        builder.append_colon();
        builder.escape_and_append_with_quotes(material->normalMap);
        // comma
        builder.append_comma();
        // displacement map
        builder.escape_and_append_with_quotes("displacement_map");
        builder.append_colon();
        builder.escape_and_append_with_quotes(material->displacementMap);
        
        builder.end_object();
    }
    builder.end_array();
    builder.end_object();
    std::string_view json_view = builder;

    std::string filepath = "assets/materials/" + material_file_name + ".json";

    std::ofstream out(filepath, std::ios::binary);
    if (out) {
        out.write(json_view.data(), json_view.size());
    }

}


void MaterialManager::initDefaultMaterials() {

    loadMaterials("default_materials");

    if (materials.empty()) {
        materials["Missing material"] = new Material{ "Missing Material", {0.88f, 0.2f, 0.92f}, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f };
    }


    {
    //materials["White Plastic"] = new Material{ "White Plastic", {1.0f, 1.0f, 1.0f}, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    //materials["Red Plastic"] = new Material{ "Red Plastic", {1.0f, 0.0f, 0.0f}, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    //materials["Green Plastic"] = new Material{ "Green Plastic", {0.0f, 1.0f, 0.0f}, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    //materials["Blue Plastic"] = new Material{ "Blue Plastic", {0.0f, 0.0f, 1.0f}, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    //materials["Shiny Copper"] = new Material{ "Shiny Copper", {0.0f, 1.0f, 0.0f}, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    //materials["Rough Metal"] = new Material{ "Rough Metal", {1.0f, 1.0f, 0.0f}, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    //materials["Mirror"] = new Material{ "Mirror", {1.0f, 1.0f, 1}, 0.0f, 1, 1, 0, 0 };
    //materials["Light"] = new Material{ "Light", {1.0f, 1.0f, 1}, 0.0f, 0.0f, 1.0f, 0.0f, 15.0f };
    //materials["Glass"] = new Material{ "Glass", {1.0f, 1.0f, 1}, 0.0f, 0.0f, 1.5f, 1.0f, 0.0f };
    //materials["Orange Glass"] = new Material{ "Orange Glass", {1.0f, 0.4f, 0.0f}, 0.0f, 0.0f, 1.5f, 1.0f, 0.0f };
    //materials["Black Glass"] = new Material{ "Black Glass", {0.1f, 0.1f, 0.1f}, 0.0f, 0.0f, 1.5f, 1.0f, 0.0f };
    //materials["Diamond"] = new Material{ "Diamond", {1.0f, 1.0f, 1.0f}, 0.0f, 0.0f, 2.42f, 1.0f, 0.0f };
    }
	
}

void MaterialManager::initTextures() {

	// portal cube
	albedos["Weighted Cube"] = "weighted_cube_texture.png";
	albedos["Companion Cube"] = "companion_cube_texture.png";
	emissive["Weighted Cube"] = "weighted_cube_emission.png";
	emissive["Companion Cube"] = "companion_cube_emission.png";

	// portal button
	albedos["Button"] = "button_texture.png";

	// portal gun
	albedos["Portal Gun"] = "portal_gun_texture.png";
	roughness["Portal Gun"] = "portal_gun_rough.png";
	metallic["Portal Gun"] = "portal_gun_metallic.png";
	emissive["Portal Gun"] = "portal_gun_emission.png";
	normal["Portal Gun"] = "portal_gun_normal.png";

}