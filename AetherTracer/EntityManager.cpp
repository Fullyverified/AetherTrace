#include "EntityManager.h"

#include <simdjson.h>
#include <fstream>
#include <string>

void EntityManager::initScene() {

    camera->position = { 15, 3, 0 };
    camera->rotation = { -1 , 0 };

    {

        // CORNELL BOX

        //entities.emplace_back(new Entity{ "cornell", {23, -3, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Floor"}); // floor

        //entities.emplace_back(new Entity{ "cornell", {23, 9, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Roof" }); // roof

        //entities.emplace_back(new Entity{ "cornell", {23, 3, 6}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Red Plastic"], "Cornell Box Left Wall" }); // left wall
        //entities.emplace_back(new Entity{ "cornell", {23, 3, -6}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Green Plastic"], "Cornell Box Right Wall" }); // right wall

        //entities.emplace_back(new Entity{ "cornell", {29, 3, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Back Wall" }); // back wall

        //entities.emplace_back(new Entity{ "cube", {23, 6.925, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Light"], "Cornell Box Light" }); // light
        //entities.emplace_back(new Entity{ "cube", {29, 15, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Light"], "Cornell Box Upper Light" }); // high

        //entities.emplace_back(new Entity{ "cube", {24, 1, 1}, {0, -0.4, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Cube Lower" });
        //entities.emplace_back(new Entity{ "cube", {24, 3, 1}, {0, -0.4, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Cube Upper" });

        //entities.emplace_back(new Entity{ "cube", {22, 1, -1}, {0, 0.4, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Cube" });

        //   CORNELL BOX

        //entities.emplace_back(new Entity{ "floor", {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Floor" }); // floor

        //entities.emplace_back(new Entity{ "sphere", {15, 1, -5}, {0, 0.4, 0}, {1, 1, 1}, materialManager->materials["Glass"], "Sphere" });
        //entities.emplace_back(new Entity{ "sphere", {15, 1, -10}, {0, 0.4, 0}, {1, 1, 1}, materialManager->materials["Mirror"], "Sphere" });
        //entities.emplace_back(new Entity{ "diamondFlat", {10, 0, -7.5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Diamond"], "Diamond" });

        //entities.emplace_back(new Entity{ "lucyScaled", {15, -0.05, 5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Glass"], "Lucy" });
        //entities.emplace_back(new Entity{ "TheStanfordDragon", {15, 0, 10}, {0, 200, 0}, {2, 2, 2}, materialManager->materials["Orange Glass"], "The Stanford Dragon" });

        //entities.emplace_back(new Entity{ "portalGun", {-5, 0, 5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Portal Gun" });
        //entities.emplace_back(new Entity{ "portalButton", {-5, 0, 1}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Portal Button" });
        //entities.emplace_back(new Entity{ "CompanionCube", {-5, 0, -1.5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Companion Cube" });

        //entities.emplace_back(new Entity{ "cube", {0, 15, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Light"], "Light center" }); // high

    }

    loadScene("default_scene");

    for (Entity* entity : entities) {

        if (materialManager->materials.find(entity->material) == materialManager->materials.end()) {

            entity->material = "Missing material";
        }

    }
}

void EntityManager::addEntity(std::string meshName, std::string entityName) {
    
    std::cout << "Mesh Name" << meshName << std::endl;

    entities.emplace_back(new Entity{ meshName, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, "White Plastic", entityName});

}

void EntityManager::swapEntityModel(std::string model_name, size_t entity_idx) {

    entities[entity_idx]->model = model_name;

}


void EntityManager::deleteEntity(size_t index) {

    auto it = entities.begin() + index;
    entities.erase(it);
}

void EntityManager::loadScene(std::string scene_name) {

    std::string filepath = "assets/scenes/" + scene_name + ".json";

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

    // load camera
    simdjson::ondemand::object cam_obj;
    root["camera"].get_object().get(cam_obj);
  
    // position
    simdjson::ondemand::array pos_arr;
    cam_obj["position"].get_array().get(pos_arr);
   
    size_t idx = 0;
    for (auto val : pos_arr) {
        double v;
        val.get_double().get(v);
        if (idx == 0) camera->position.x = static_cast<float>(v);
        else if (idx == 1) camera->position.y = static_cast<float>(v);
        else if (idx == 2) camera->position.z = static_cast<float>(v);
        idx++;
    }

    // rotation
    simdjson::ondemand::array rot_arr;
    cam_obj["rotation"].get_array().get(rot_arr);
    idx = 0;
    for (auto val : rot_arr) {
        double v;
        error = val.get_double().get(v);
        if (error) {
            std::cout << "Error getting camera rotation value: " << error << std::endl;
            return;
        }
        if (idx == 0) camera->rotation.x = static_cast<float>(v);
        else if (idx == 1) camera->rotation.y = static_cast<float>(v);
        idx++;
    }

    // entities
    simdjson::ondemand::array entities_arr;
    root["entities"].get_array().get(entities_arr);
    entities.clear();

    size_t entity_idx = 0;
    for (auto entity_val : entities_arr) {

        std::cout << "entity_idx: " << entity_idx << std::endl;
        entity_idx++;

        simdjson::ondemand::object ent_obj;
        entity_val.get_object().get(ent_obj);
       
        Entity* entity = new Entity{};

        // model name
        std::string_view sv;
        ent_obj["mesh_name"].get_string().get(sv);
        entity->model = sv;

        // position
        ent_obj["position"].get_array().get(pos_arr);
       
        idx = 0;
        for (auto val : pos_arr) {
            double v;
            val.get_double().get(v);
            if (idx == 0) entity->position.x = static_cast<float>(v);
            else if (idx == 1) entity->position.y = static_cast<float>(v);
            else if (idx == 2) entity->position.z = static_cast<float>(v);
            idx++;
        }

        // rotation
        ent_obj["rotation"].get_array().get(rot_arr);

        idx = 0;
        for (auto val : rot_arr) {
            double v;
            val.get_double().get(v);
            if (idx == 0) entity->rotation.x = static_cast<float>(v);
            else if (idx == 1) entity->rotation.y = static_cast<float>(v);
            else if (idx == 2) entity->rotation.z = static_cast<float>(v);
            idx++;
        }

        // scale
        simdjson::ondemand::array scale_arr;
        ent_obj["scale"].get_array().get(scale_arr);

        idx = 0;
        for (auto val : scale_arr) {
            double v;
            val.get_double().get(v);

            if (idx == 0) entity->scale.x = static_cast<float>(v);
            else if (idx == 1) entity->scale.y = static_cast<float>(v);
            else if (idx == 2) entity->scale.z = static_cast<float>(v);
            idx++;
        }

        // material
        ent_obj["material_name"].get_string().get(sv);
        std::string material_name{ sv };
        entity->material = materialManager->materials.find(material_name) == materialManager->materials.end() ? "Missing Material" : material_name;

        // entity name
        ent_obj["entity_name"].get_string().get(sv);
        entity->name = sv;

        std::cout << "Entity Name" << entity->name << std::endl;

        entities.push_back(entity);
    }

}

void EntityManager::saveScene(std::string scene_name) {

    simdjson::builder::string_builder builder(64 * 1024);

    builder.start_object();
    // Camera
    builder.escape_and_append_with_quotes("camera");
    builder.append_colon();
    builder.start_object();
    // position
    builder.escape_and_append_with_quotes("position");
    builder.append_colon();
    builder.start_array();
    builder.append(camera->position.x);
    builder.append_comma();
    builder.append(camera->position.y);
    builder.append_comma();
    builder.append(camera->position.z);
    builder.end_array();
    // comma for next key
    builder.append_comma();
    // rotation
    builder.escape_and_append_with_quotes("rotation");
    builder.append_colon();
    builder.start_array();
    builder.append(camera->rotation.x);
    builder.append_comma();
    builder.append(camera->rotation.y);
    builder.end_array();
    builder.end_object();
    // comma for next top-level key
    builder.append_comma();
    // Entities
    builder.escape_and_append_with_quotes("entities");
    builder.append_colon();
    builder.start_array();
    bool first_entity = true;
    for (const Entity* entity : entities) {
        if (!first_entity) {
            builder.append_comma();
        }
        first_entity = false;
        builder.start_object();
        // mesh_name
        builder.escape_and_append_with_quotes("mesh_name");
        builder.append_colon();
        builder.escape_and_append_with_quotes(entity->model);
        // comma
        builder.append_comma();
        // position
        builder.escape_and_append_with_quotes("position");
        builder.append_colon();
        builder.start_array();
        builder.append(entity->position.x);
        builder.append_comma();
        builder.append(entity->position.y);
        builder.append_comma();
        builder.append(entity->position.z);
        builder.end_array();
        // comma
        builder.append_comma();
        // rotation
        builder.escape_and_append_with_quotes("rotation");
        builder.append_colon();
        builder.start_array();
        builder.append(entity->rotation.x);
        builder.append_comma();
        builder.append(entity->rotation.y);
        builder.append_comma();
        builder.append(entity->rotation.z);
        builder.end_array();
        // comma
        builder.append_comma();
        // scale
        builder.escape_and_append_with_quotes("scale");
        builder.append_colon();
        builder.start_array();
        builder.append(entity->scale.x);
        builder.append_comma();
        builder.append(entity->scale.y);
        builder.append_comma();
        builder.append(entity->scale.z);
        builder.end_array();
        // comma
        builder.append_comma();
        // material_name
        builder.escape_and_append_with_quotes("material_name");
        builder.append_colon();
        builder.escape_and_append_with_quotes(entity->material);
        // comma
        builder.append_comma();
        // entity_name
        builder.escape_and_append_with_quotes("entity_name");
        builder.append_colon();
        builder.escape_and_append_with_quotes(entity->name);
        builder.end_object();
    }
    builder.end_array();
    builder.end_object();
    std::string_view json_view = builder;

    std::string filepath = "assets/scenes/" + scene_name + ".json";

    std::ofstream out(filepath, std::ios::binary);
    if (out) {
        out.write(json_view.data(), json_view.size());
    }

}

void EntityManager::deleteScene(std::string scene_name) {


}

void EntityManager::cleanUp() {

    for (Entity* entity : entities) {

        auto it = std::find(entities.begin(), entities.end(), entity);

        entities.erase(it);
        delete entity;
    }

}

