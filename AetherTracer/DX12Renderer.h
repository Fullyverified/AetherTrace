#pragma once

#include "DX12PathTracerPipeLine.h"

#include <vector>
#include "DirectXMath.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h> // for compiling shaders
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")


class DX12Renderer {

public:

	DX12Renderer(EntityManager* entityManager, MeshManager* meshManager, MaterialManager* materialManager, Window* window);
	~DX12Renderer() {};

	void init();

	void initDevice();


	void resize();
	void render();
	void present();

	DX12PathTracerPipeLine* dx12PathTracerPipeLine;

	EntityManager* entityManager;
	MeshManager* meshManager;
	MaterialManager* materialManager;

	Window* window;

	// device init
	HWND hwnd;
	IDXGIFactory4* factory;
	ID3D12Device5* d3dDevice;
	ID3D12CommandQueue* cmdQueue;
};