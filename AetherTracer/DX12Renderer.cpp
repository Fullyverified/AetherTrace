#include "DX12Renderer.h"

DX12Renderer::DX12Renderer(EntityManager* entityManager, MeshManager* meshManager, MaterialManager* materialManager, Window* window) :
	entityManager(entityManager), meshManager(meshManager), materialManager(materialManager), window(window) {

	dx12PathTracerPipeLine = new DX12PathTracerPipeLine(entityManager, meshManager, materialManager, window);
};

void DX12Renderer::init() {

	dx12PathTracerPipeLine->init();

}

void DX12Renderer::render() {

	dx12PathTracerPipeLine->render();

}

void DX12Renderer::present() {

	dx12PathTracerPipeLine->present();

}

void DX12Renderer::resize() {

	dx12PathTracerPipeLine->resize();

	
}