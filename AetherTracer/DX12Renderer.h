#pragma once

#include "DX12PathTracerPipeLine.h"

class DX12Renderer {

public:

	DX12Renderer(EntityManager* entityManager, MeshManager* meshManager, MaterialManager* materialManager, Window* window);
	~DX12Renderer() {};

	void init();
	void resize();
	void render();
	void present();

	DX12PathTracerPipeLine* dx12PathTracerPipeLine;

	EntityManager* entityManager;
	MeshManager* meshManager;
	MaterialManager* materialManager;

	Window* window;
};