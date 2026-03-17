#pragma once

#define NOMINMAX

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

#include "Vector.h"

class DX12MaterialManager {

public:

	struct DX12Material {

		std::string name;
		UINT textureMap; // indices
		UINT roughnessMap;
		UINT metallicMap;
		UINT emissionMap;
		UINT normalMap;
		UINT displacementMap;

	};

	DX12MaterialManager(MeshManager* meshManager, MaterialManager* materialManager, EntityManager* entityManager, IDXGIFactory4* factory, ID3D12Device5* d3dDevice, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList4* cmdList);

	void initMaterials(std::vector<DX12ResourceManager::DX12Entity*> dx12entities);

	DX12ResourceManager::ResourceHandle* createTextureFromVector(PT::Vector3 value);
	void createTextureFromImage(std::string file_path);

	void pushTexture(DX12ResourceManager::ResourceHandle* texture_handle, size_t data_size, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after);


	// Utility
	void checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context);
	void waitForGPU();

	std::unordered_map<std::string, DX12Material*> dx12materials_map;

	std::vector<DX12ResourceManager::ResourceHandle*> texture_maps;
	std::unordered_map<std::string, UINT> texture_indices_map;

	std::vector<DX12ResourceManager::ResourceHandle*> roughness_maps;
	std::unordered_map<std::string, UINT> roughness_indices_map;

	std::vector<DX12ResourceManager::ResourceHandle*> metallic_maps;
	std::unordered_map<std::string, UINT> metallic_indices_map;

	std::vector<DX12ResourceManager::ResourceHandle*> emission_maps;
	std::unordered_map<std::string, UINT> emission_indices_map;

	std::vector<DX12ResourceManager::ResourceHandle*> normal_maps;
	std::unordered_map<std::string, UINT> normal_indices_map;

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

