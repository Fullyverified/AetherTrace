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



class DX12ResourceManager {
public:

	struct ResourceHandle {
		// for resource that don't need an upload buffer, only the default_buffer is used
		ID3D12Resource* upload_buffer;
		ID3D12Resource* default_buffer;

		// offset in the global heap
		UINT64 heap_index_srv; // or cbv
		UINT64 heap_index_uav;
	};


	struct DescriptorAllocator {
		ID3D12DescriptorHeap* desc_heap;
		std::vector<UINT> free_indices;
		UINT desc_increment_size;

		UINT alloc() {
			UINT idx = free_indices.back();
			free_indices.pop_back();
			return idx;
		}

		void free(UINT idx) {
			free_indices.push_back(idx);
		}


	};

	DX12ResourceManager(MeshManager* meshManager, MaterialManager* materialManager, EntityManager* entityManager);
	~DX12ResourceManager() {};

	
	ResourceHandle* createResourceHandle(const void* data, size_t byte_size, D3D12_RESOURCE_STATES final_state, bool is_UAV);
	void pushResourceHandle(DX12ResourceManager::ResourceHandle* resource_handle, size_t data_size, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after);
	void createCBV(DX12ResourceManager::ResourceHandle* resource_handle, size_t byte_size);

	ResourceHandle* makeAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, UINT64* update_scratch_size = nullptr);

	// INIT Resource
	void initAccumulationTexture(ResourceHandle* resource_handle, std::string resource_name);
	void initRenderTarget(ResourceHandle* resource_handle, std::string resource_name);

	void initModelBuffers();
	void initModelBLAS();
	void initScene();
	void initTopLevelAS();
	void initMaterialBuffer();
	void initVertexIndexBuffers();
	void initMaxLumBuffer();

	void updateToneParams();
	void updateCamera();
	void updateTransforms();
	void updateRand();

	void initDescriptorHeap(DX12ResourceManager::DescriptorAllocator* descriptorAllocator, UINT num_descriptors, bool is_shader_visible, std::string descriptor_name);

	void initGlobalDescriptors();

	// Utility
	void checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context);
	void waitForGPU();

	// RAY TRACING STAGE

	struct DX12Material {
		DirectX::XMFLOAT3 color;
		float roughness;
		float metallic;
		float ior;
		float transmission;
		float emission;

		DX12Material(MaterialManager::Material* material) {
			color.x = material->color.x;
			color.y = material->color.y;
			color.z = material->color.z;
			roughness = material->roughness;
			metallic = material->metallic;
			ior = material->ior;
			transmission = material->transmission;
			emission = material->emission;
		}
	};

	struct DX12Model {
		DX12Model() : loadedModel(nullptr), BLAS(nullptr) {};

		MeshManager::LoadedModel* loadedModel; // mesh and raw vertex data
		ResourceHandle* BLAS;

		std::vector<ResourceHandle*> vertexBuffers;
		std::vector<ResourceHandle*> indexBuffers;;
	};

	struct DX12Entity {
		DX12Entity() : entity(nullptr), model(nullptr) {};

		EntityManager::Entity* entity; // name, position, rotation, scale
		DX12Model* model;
		DX12Material* material;
	};

	struct alignas(256)DX12Camera {

		DX12Camera() : position{} {};
		DX12Camera(PT::Vector3 position) : position{ position.x, position.y, position.z } {};

		DirectX::XMFLOAT3 position;
		float pad0;

		DirectX::XMFLOAT4X4 invViewProj;
		UINT seed;
		UINT sky;
		float skyBrighness;
		UINT minBounces;
		UINT maxBounces;
		UINT jitter;
	};

	// MODEL
	std::vector<ResourceHandle*> allVertexBuffers;
	std::vector<ResourceHandle*> allIndexBuffers;

	ResourceHandle* materialsBuffer;
	std::vector<DX12Material> dx12Materials;
	ResourceHandle* materialIndexBuffer;
	std::vector<uint32_t> materialIndices;

	ResourceHandle* cameraConstantBuffer;

	// ENTITY / BLAS
	std::unordered_map<std::string, DX12Material*> materials;
	std::unordered_map<std::string, DX12Model*> dx12Models;

	std::vector<DX12Entity*> dx12Entitys;
	DX12Camera* dx12Camera;

	// TLAS
	ResourceHandle* tlas;
	ResourceHandle* tlas_scratch;

	UINT NUM_INSTANCES = 0;
	ResourceHandle* instances;
	D3D12_RAYTRACING_INSTANCE_DESC* instanceData;
	std::unordered_map<std::string, uint32_t> uniqueInstancesID;
	std::unordered_set<std::string> uniqueInstances;

	// COMPUTE STAGE

	ResourceHandle* renderTarget;

	struct alignas(256)ToneMappingParams {
		ToneMappingParams() : stage(0), num_samples(1), exposure(1.0f) {};
		UINT stage;
		float exposure;
		UINT num_samples;
	};
	ToneMappingParams* toneMappingParams;
	ResourceHandle* toneMappingConstantBuffer;
	ResourceHandle* maxLumBuffer;

	// SHARED

	ResourceHandle* accumulationTexture;

	ResourceHandle* randBuffer;
	std::vector<uint64_t> randPattern;


	UINT samples = 0;
	UINT frames = 0;
	UINT seed = 1;

	UINT frameIndexInFlight = 2;


	UINT width;
	UINT height;

	DXGI_SAMPLE_DESC NO_AA = { .Count = 1, .Quality = 0 };

	
	// device init
	HWND hwnd;
	IDXGIFactory4* factory;
	ID3D12Device5* d3dDevice;
	ID3D12CommandQueue* cmdQueue;
	ID3D12Fence* fence;
	UINT64 fenceState = 1;

	// swap chain and uav
	IDXGISwapChain3* swapChain;

	// Command list and allocator

	ID3D12CommandAllocator* cmdAlloc; // block of memory
	ID3D12GraphicsCommandList4* cmdList;

	ID3D12RootSignature* rootSignature;

	ID3D12StateObject* raytracingPSO;
	ID3D12PipelineState* computePSO;

	// ray tracing shader tables
	UINT64 NUM_SHADER_IDS = 3;
	ID3D12Resource* shaderIDs;

	// Heap Management
	DescriptorAllocator* global_descriptor_heap_allocator;
	//DescriptorAllocator* UAVClear_descriptor_heap_allocator;


	D3D12_HEAP_PROPERTIES UPLOAD_HEAP = { .Type = D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES DEFAULT_HEAP = { .Type = D3D12_HEAP_TYPE_DEFAULT };

	// imgui
	ID3D12DescriptorHeap* rtvHeap = nullptr;
	UINT rtvDescriptorSize = 0;


	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };


	// cpu non shader visible descriptor heap
	ID3D12DescriptorHeap* cpuDescHeap;

	// shaders
	ID3DBlob* rsBlob;
	ID3DBlob* csBlob;

	MeshManager* meshManager;
	MaterialManager* materialManager;
	EntityManager* entityManager;
};