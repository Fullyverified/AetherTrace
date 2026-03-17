#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <DirectXMath.h> // For XMMATRIX
#include <Windows.h>     // To make a window, of course
#include <d3d12.h>
#include <dxgi1_4.h>

#include <d3dcompiler.h> // for compiling shaders

#pragma comment(lib, "user32") // For DefWindowProcW, etc.
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

#include "MeshManager.h"
#include "MaterialManager.h"
#include "EntityManager.h"
#include "Vector.h"

#include "DX12ResourceManager.h"
#include "DX12TLASManager.h"
#include "DX12BLASManager.h"
#include "DX12MaterialManager.h"

#include "Window.h"

#include <imgui.h>
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx12.h"

struct ImGuiDescriptorAllocator {

	ID3D12DescriptorHeap* heap = nullptr;
	std::vector<int> freeIndices;
	UINT descriptorSize = 0;

	~ImGuiDescriptorAllocator() {
		if (heap) heap->Release();
	}


	void init(ID3D12Device* device, int capacity) {
		

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = capacity;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));

		descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		freeIndices.reserve(capacity);
		for (int n = 0; n < capacity; n++) {
			freeIndices.push_back(n);
		}

	}

	void alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) {

		if (freeIndices.empty()) return;

		int idx = freeIndices.back();
		freeIndices.pop_back();

		D3D12_CPU_DESCRIPTOR_HANDLE cpu = heap->GetCPUDescriptorHandleForHeapStart();
		cpu.ptr += (SIZE_T)idx * descriptorSize;

		D3D12_GPU_DESCRIPTOR_HANDLE gpu = heap->GetGPUDescriptorHandleForHeapStart();
		gpu.ptr += (SIZE_T)idx * descriptorSize;


		*out_cpu = cpu;
		*out_gpu = gpu;
	}

	void free(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu) {

		size_t offset = cpu.ptr - heap->GetCPUDescriptorHandleForHeapStart().ptr;
		int idx = (int)(offset / descriptorSize);
		freeIndices.push_back(idx);
	}
};

extern ImGuiDescriptorAllocator* ImGuiDescAlloc;

class DX12PathTracerPipeLine {
public:

	struct UploadDefaultBufferPair {
		ID3D12Resource* HEAP_UPLOAD_BUFFER; // cpu
		ID3D12Resource* HEAP_DEFAULT_BUFFER; // gpu
	};
	DX12PathTracerPipeLine(EntityManager* entityManager, MeshManager* meshManager, MaterialManager* materialManager, Window* window, IDXGIFactory4* factory, ID3D12Device5* d3dDevice, ID3D12CommandQueue* cmdQueue);

	~DX12PathTracerPipeLine() {
		// imgui
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
		
		if (dx12ResourceManager->rtvHeap) dx12ResourceManager->rtvHeap->Release();
		//delete ImGuiDescAlloc;
	}

	void init();
	void initSurfaces();
	void resize();
	void initCommand();
	void createFence(ID3D12Fence*& fence);
		
	void loadShaders();
	void initRootSignature();
	void initRayTracingPipeline();
	void initComputePipeline();
	void initRTShaderTables();

	void initGlobalDescriptors();
	void bindDescriptors();

	void traceRays();
	void postProcess();

	void render();
	void present();

	void initImgui();
	void createBackBufferRTVs();
	void imguiPresent(ID3D12Resource* backBuffer);

	void quit();

	void checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context);

	void barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	bool reset = false;

	// inherited from the DX12Renderer  -> this
	HWND hwnd;
	IDXGIFactory4* factory;
	ID3D12Device5* d3dDevice;
	ID3D12CommandQueue* cmdQueue;

	// fence
	ID3D12Fence* fence;
	UINT64 fenceState = 1;

	ID3D12CommandAllocator* cmdAlloc; // block of memory
	ID3D12GraphicsCommandList4* cmdList;

	// managers
	MeshManager* meshManager;
	MaterialManager* materialManager;
	EntityManager* entityManager;
	Window* window;

	DX12ResourceManager* dx12ResourceManager;
	DX12TLASManager* dx12TLASManager;
	DX12BLASManager* dx12BLASManager;
	DX12MaterialManager* dx12MaterialManager;
};