#include "DX12AccelerationStructureManager.h"

void DX12AccelerationStructureManager::makeBottomLevel(DX12ResourceManager::DX12Model* model, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList4* cmdList) {

	delete model->BLAS;

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geometryDesc.Triangles.VertexBuffer.StartAddress = model->vertexBuffers[0]->default_buffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(MeshManager::Vertex);
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(model->loadedModel->meshes[0].vertices.size());
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = model->indexBuffers[0]->default_buffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(model->loadedModel->meshes[0].indices.size());
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;


	std::cout << "Making BLAS Inputs" << std::endl;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
		.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
		.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
		.NumDescs = 1,
		.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
		.pGeometryDescs = &geometryDesc };

	std::cout << "Making BLAS" << std::endl;


	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	d3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);


	D3D12_RESOURCE_DESC BLASdesc = {};
	BLASdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BLASdesc.Width = prebuildInfo.ScratchDataSizeInBytes;
	BLASdesc.Height = 1;
	BLASdesc.DepthOrArraySize = 1;
	BLASdesc.MipLevels = 1;
	BLASdesc.SampleDesc.Count = 1;
	BLASdesc.SampleDesc.Quality = 0;
	BLASdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BLASdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &BLASdesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&model->BLAS->default_buffer));
	checkHR(hr, nullptr, "CreateCommittedResource for BLAS Scratch failed");
	model->BLAS->default_buffer->SetName(L"BLAS");


	ID3D12Resource* scratch_buffer;

	D3D12_RESOURCE_DESC scratchDesc = {};
	scratchDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	scratchDesc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
	scratchDesc.Height = 1;
	scratchDesc.DepthOrArraySize = 1;
	scratchDesc.MipLevels = 1;
	scratchDesc.SampleDesc.Count = 1;
	scratchDesc.SampleDesc.Quality = 0;
	scratchDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	scratchDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&scratch_buffer));
	checkHR(hr, nullptr, "CreateCommittedResource for BLAS Scratch failed");
	scratch_buffer->SetName(L"scratch buffer");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {
	.DestAccelerationStructureData = model->BLAS->default_buffer->GetGPUVirtualAddress(), .Inputs = inputs, .ScratchAccelerationStructureData = scratch_buffer->GetGPUVirtualAddress() };

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	cmdList->Close();
	cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&cmdList));


	scratch_buffers.push_back(scratch_buffer); // store and clear after all commands submitted.

	waitForGPU();
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc, nullptr);


	//scratch_buffer->Release();
	delete scratch_buffer;

};


void DX12AccelerationStructureManager::makeTopLevel(DX12ResourceManager::ResourceHandle* TLAS, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList4* cmdList) {

	tlas_scratch = new DX12ResourceManager::ResourceHandle{};
	tlas = new DX12ResourceManager::ResourceHandle{};

	if (config.debug) std::cout << "initTopLevel()" << std::endl;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
	.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
	.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
	.NumDescs = config.maxInstances,
	.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
	.InstanceDescs = instances->default_buffer->GetGPUVirtualAddress() };

	std::cout << "NUM_INSTANCES = " << NUM_INSTANCES << std::endl;
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


void DX12AccelerationStructureManager::waitForGPU() {

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