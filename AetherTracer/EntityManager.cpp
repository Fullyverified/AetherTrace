#include "EntityManager.h"
#include <string>


void EntityManager::initScene() {

    camera->position = { 15, 3, 0 };
    camera->rotation = { -1 , 0 };

    // CORNELL BOX

    entitys.emplace_back(new Entity{ "cornell", {23, -3, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Floor"}); // floor

    entitys.emplace_back(new Entity{ "cornell", {23, 9, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Roof" }); // roof

    entitys.emplace_back(new Entity{ "cornell", {23, 3, 6}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Red Plastic"], "Cornell Box Left Wall" }); // left wall
    entitys.emplace_back(new Entity{ "cornell", {23, 3, -6}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Green Plastic"], "Cornell Box Right Wall" }); // right wall

    entitys.emplace_back(new Entity{ "cornell", {29, 3, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Back Wall" }); // back wall

    entitys.emplace_back(new Entity{ "cube", {23, 6.925, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Light"], "Cornell Box Light" }); // light
    entitys.emplace_back(new Entity{ "cube", {29, 15, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Light"], "Cornell Box Upper Light" }); // high

    entitys.emplace_back(new Entity{ "cube", {24, 1, 1}, {0, -0.4, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Cube Lower" });
    entitys.emplace_back(new Entity{ "cube", {24, 3, 1}, {0, -0.4, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Cube Upper" });

    entitys.emplace_back(new Entity{ "cube", {22, 1, -1}, {0, 0.4, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Cube" });
    
    entitys.emplace_back(new Entity{ "floor", {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Cornell Box Floor" }); // floor
    //   CORNELL BOX
    
    entitys.emplace_back(new Entity{ "sphere", {15, 1, -5}, {0, 0.4, 0}, {1, 1, 1}, materialManager->materials["Glass"], "Sphere" });
    entitys.emplace_back(new Entity{ "sphere", {15, 1, -10}, {0, 0.4, 0}, {1, 1, 1}, materialManager->materials["Mirror"], "Sphere" });
    entitys.emplace_back(new Entity{ "diamondFlat", {10, 0, -7.5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Diamond"], "Diamond" });

    entitys.emplace_back(new Entity{ "lucyScaled", {15, -0.05, 5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["Glass"], "Lucy" });
    entitys.emplace_back(new Entity{ "TheStanfordDragon", {15, 0, 10}, {0, 200, 0}, {2, 2, 2}, materialManager->materials["Orange Glass"], "The Stanford Dragon" });

    entitys.emplace_back(new Entity{ "portalGun", {-5, 0, 5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Portal Gun" });
    entitys.emplace_back(new Entity{ "portalButton", {-5, 0, 1}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Portal Button" });
    entitys.emplace_back(new Entity{ "CompanionCube", {-5, 0, -1.5}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], "Companion Cube" });


   
    for (Entity* entity : entitys) {
        if (entity->material == nullptr) entity->material = materialManager->materials["White Plastic"];
    }
}

void EntityManager::addEntity(std::string meshName, std::string entityName) {
    
    std::cout << "Mesh Name" << meshName << std::endl;

    entitys.emplace_back(new Entity{ meshName, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, materialManager->materials["White Plastic"], entityName});

}

void EntityManager::cleanUp() {

    for (Entity* entity : entitys) {

        auto it = std::find(entitys.begin(), entitys.end(), entity);

        entitys.erase(it);
        delete entity;
    }

}

