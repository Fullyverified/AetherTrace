#pragma once

#include <vector>
#include "DirectXMath.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

#include "DX12ResourceManager.h"

class DX12AccelerationStructureManager {


public:

	DX12AccelerationStructureManager() {};
	~DX12AccelerationStructureManager() {};

	void makeBottomLevel(DX12ResourceManager::DX12Model* model, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList4* cmdList);

	void makeTopLevel();

	void waitForGPU();

	std::vector<ID3D12Resource*> scratch_buffers;

	ID3D12Device5* d3dDevice;
	ID3D12CommandQueue* cmdQueue;



	D3D12_HEAP_PROPERTIES UPLOAD_HEAP = { .Type = D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES DEFAULT_HEAP = { .Type = D3D12_HEAP_TYPE_DEFAULT };


};

