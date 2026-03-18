#include "DX12TLASManager.h"

DX12TLASManager::DX12TLASManager(MeshManager* meshManager, MaterialManager* materialManager, EntityManager* entityManager, IDXGIFactory4* factory, ID3D12Device5* d3dDevice, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList4* cmdList)
	: meshManager(meshManager), materialManager(materialManager), entityManager(entityManager), factory(factory), d3dDevice(d3dDevice), cmdQueue(cmdQueue), cmdAlloc(cmdAlloc), cmdList(cmdList) {
};

void DX12TLASManager::initTopLevelAS(DX12ResourceHandle* instances) {
	
	tlas_scratch = new DX12ResourceHandle{};
	tlas = new DX12ResourceHandle{};

	if (config.debug) std::cout << "initTopLevel()" << std::endl;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
	.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
	.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
	.NumDescs = config.maxInstances,
	.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
	.InstanceDescs = instances->default_buffer->GetGPUVirtualAddress() };

	std::cout << "instances width = " << instances->default_buffer->GetDesc().Width / sizeof(D3D12_RAYTRACING_INSTANCE_DESC) << std::endl;
	std::cout << "MAX_INSTANCES = " << config.maxInstances << std::endl;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	d3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = prebuildInfo.ScratchDataSizeInBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc = NO_AA;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&tlas_scratch->default_buffer));
	checkHR(hr, nullptr, "CreateCommittedResource for AS failed");

	desc.Width = prebuildInfo.ResultDataMaxSizeInBytes;

	hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&tlas->default_buffer));
	checkHR(hr, nullptr, "CreateCommittedResource for AS failed");

	tlas_scratch->default_buffer->SetName(L"Scratch");
	tlas->default_buffer->SetName(L"AS");

	if (tlas_scratch->default_buffer == nullptr) std::cout << "scratch nullptr" << std::endl;
	if (tlas->default_buffer == nullptr) std::cout << "BLAS nullptr" << std::endl;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {
	.DestAccelerationStructureData = tlas->default_buffer->GetGPUVirtualAddress(), .Inputs = inputs, .ScratchAccelerationStructureData = tlas_scratch->default_buffer->GetGPUVirtualAddress() };

	std::cout << "executing cmd list " << std::endl;

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	cmdList->Close();
	cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&cmdList));
	waitForGPU();
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc, nullptr);

	tlas_scratch->default_buffer->Release();
	delete tlas_scratch;
}

void DX12TLASManager::rebuildTLAS(DX12ResourceHandle* instances, UINT NUM_INSTANCES) {

}

void DX12TLASManager::updateTLAS(DX12ResourceHandle* instances, UINT NUM_INSTANCES) {

	// Update exisitng TLAS
	tlas_scratch = new DX12ResourceHandle{};

	if (config.debug) std::cout << "initTopLevel()" << std::endl;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
	.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
	.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE, // perform update
	.NumDescs = config.maxInstances,
	.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
	.InstanceDescs = instances->default_buffer->GetGPUVirtualAddress() };

	std::cout << "NUM_INSTANCES = " << NUM_INSTANCES << std::endl;
	std::cout << "MAX_INSTANCES = " << config.maxInstances << std::endl;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	d3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = prebuildInfo.UpdateScratchDataSizeInBytes; // update scratch size
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc = NO_AA;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&tlas_scratch->default_buffer));
	checkHR(hr, nullptr, "CreateCommittedResource for TLAS failed");

	desc.Width = prebuildInfo.ResultDataMaxSizeInBytes;

	if (tlas_scratch->default_buffer == nullptr) std::cout << "scratch nullptr" << std::endl;
	if (tlas->default_buffer == nullptr) std::cout << "TLAS nullptr" << std::endl;

	tlas_scratch->default_buffer->SetName(L"Scratch");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC updateDesc = {};
	updateDesc.DestAccelerationStructureData = tlas->default_buffer->GetGPUVirtualAddress();
	updateDesc.SourceAccelerationStructureData = tlas->default_buffer->GetGPUVirtualAddress(); // update, not rebuild
	updateDesc.Inputs = inputs;
	updateDesc.ScratchAccelerationStructureData = tlas_scratch->default_buffer->GetGPUVirtualAddress();

	std::cout << "executing cmd list " << std::endl;

	cmdList->BuildRaytracingAccelerationStructure(&updateDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = tlas->default_buffer;
	cmdList->ResourceBarrier(1, &uavBarrier);

	cmdList->Close();
	cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&cmdList));
	waitForGPU();
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc, nullptr);

	tlas_scratch->default_buffer->Release();
	delete tlas_scratch;

}

// helper

void DX12TLASManager::checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context) {
	if (FAILED(hr)) {
		std::cerr << context << "HRESULT 0x" << std::hex << hr << std::endl;
		if (hr == E_OUTOFMEMORY) std::cerr << "(Out of memory?)\n";
		else if (hr == E_INVALIDARG) std::cerr << "(Invalid arg — check desc)\n";
	}
	if (errorblob) {
		OutputDebugStringA((char*)errorblob->GetBufferPointer());
		errorblob->Release();
	}
}

void DX12TLASManager::waitForGPU() {

	const UINT64 currentFence = fenceState;
	cmdQueue->Signal(fence, currentFence);
	if (fence->GetCompletedValue() < currentFence) {
		HANDLE event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		fence->SetEventOnCompletion(currentFence, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	fenceState++;
}