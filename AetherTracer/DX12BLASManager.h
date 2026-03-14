#pragma once

#include <vector>
#include "DirectXMath.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h> // for compiling shaders
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

#include "MaterialManager.h"
#include "EntityManager.h"
#include "MeshManager.h"

#include "DX12ResourceManager.h"

class DX12BLASManager {

public:

	DX12BLASManager(MeshManager* meshManager, MaterialManager* materialManager, EntityManager* entityManager, IDXGIFactory4* factory, ID3D12Device5* d3dDevice, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList4* cmdList);

	void createModelBLAS(std::unordered_map<std::string, DX12ResourceManager::DX12Model*> models_map);
	DX12ResourceManager::ResourceHandle* makeAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs);

	// Utility
	void checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context);
	void waitForGPU();


	std::unordered_map<std::string, DX12ResourceManager::ResourceHandle*> dx12Models_BLAS_map;


	// inherited from the DX12Renderer -> DX12PathTracerPipeLine -> this
	IDXGIFactory4* factory;
	ID3D12Device5* d3dDevice;
	ID3D12CommandQueue* cmdQueue;
	ID3D12CommandAllocator* cmdAlloc; // block of memory
	ID3D12GraphicsCommandList4* cmdList;

	// fence
	ID3D12Fence* fence;
	UINT64 fenceState = 1;

	MeshManager* meshManager;
	MaterialManager* materialManager;
	EntityManager* entityManager;

	DXGI_SAMPLE_DESC NO_AA = { .Count = 1, .Quality = 0 };
	D3D12_HEAP_PROPERTIES UPLOAD_HEAP = { .Type = D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES DEFAULT_HEAP = { .Type = D3D12_HEAP_TYPE_DEFAULT };
};

