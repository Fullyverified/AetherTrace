#include "DX12ResourceManager.h"
#include <random>

#include <d3dcompiler.h>

DX12ResourceManager::DX12ResourceManager(MeshManager* meshManager, MaterialManager* materialManager, EntityManager* entityManager)
	: meshManager(meshManager), materialManager(materialManager), entityManager(entityManager) {

	global_descriptor_heap_allocator = new DX12ResourceManager::DescriptorAllocator{};
	//UAVClear_descriptor_heap_allocator = new DX12ResourceManager::DescriptorAllocator{};

	renderTarget = new DX12ResourceManager::ResourceHandle;
	accumulationTexture = new DX12ResourceManager::ResourceHandle;
};

void DX12ResourceManager::initDescriptorHeap(DX12ResourceManager::DescriptorAllocator* descriptor_allocator, UINT num_descriptors, bool is_shader_visible, std::string descriptor_name) {

	D3D12_DESCRIPTOR_HEAP_FLAGS flags = is_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = num_descriptors,
		.Flags = flags
	};

	std::cout << "d3dDevice->CreateDescriptorHeap" << std::endl;
	HRESULT hr = d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptor_allocator->desc_heap));

	checkHR(hr, nullptr, "Creating " + descriptor_name);

	std::cout << "Creating " + descriptor_name << std::endl;

	if (descriptor_allocator->desc_heap == nullptr) {
		std::cout << "heap desc null ptr" << std::endl;
	}

	descriptor_allocator->desc_increment_size = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	std::cout << "Filling free indices" << std::endl;

	for (int i = num_descriptors - 1; i >= 0; i--) {
		descriptor_allocator->free_indices.push_back(i);
	}

	//global_desctiptor_heap_allocator->desc_heap->SetName(descriptor_name);
}

void DX12ResourceManager::initGlobalDescriptors() {

	if (config.debug) std::cout << "creating SRVs" << std::endl;

	// Create views
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = global_descriptor_heap_allocator->desc_heap->GetCPUDescriptorHandleForHeapStart();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};

	// UAV for accumulation texture
	uavDesc = {
		.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
		.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D
	};
	d3dDevice->CreateUnorderedAccessView(accumulationTexture->default_buffer, nullptr, &uavDesc, cpuHandle);
	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	accumulationTexture->heap_index_uav = global_descriptor_heap_allocator->alloc();
	
	std::cout << "accumulationTexture->heap_index_uav: " << accumulationTexture->heap_index_uav << std::endl;


	// UAV for RNG Buffer
	uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN,
		uavDesc.Buffer.StructureByteStride = sizeof(uint64_t),
		uavDesc.Buffer.NumElements = randPattern.size();
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,

	d3dDevice->CreateUnorderedAccessView(randBuffer->default_buffer, nullptr, &uavDesc, cpuHandle);
	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	randBuffer->heap_index_uav = global_descriptor_heap_allocator->alloc();

	std::cout << "randBuffer->heap_index_uav: " << randBuffer->heap_index_uav << std::endl;

	// SRV for TLAS
	srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.RaytracingAccelerationStructure.Location = tlas->default_buffer->GetGPUVirtualAddress();

	d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);
	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	tlas->heap_index_srv = global_descriptor_heap_allocator->alloc();

	std::cout << "tlas->heap_index_srv: " << tlas->heap_index_srv << std::endl;

	// Vertex Buffer
	for (DX12ResourceManager::ResourceHandle* vertexBuffer : allVertexBuffers) {
		srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(vertexBuffer->default_buffer->GetDesc().Width / sizeof(MeshManager::Vertex));
		srvDesc.Buffer.StructureByteStride = sizeof(MeshManager::Vertex);
		d3dDevice->CreateShaderResourceView(vertexBuffer->default_buffer, &srvDesc, cpuHandle);

		cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
		vertexBuffer->heap_index_srv = global_descriptor_heap_allocator->alloc();

		std::cout << "srvNumElementsVertex: " << srvDesc.Buffer.NumElements << std::endl;
	}
	// Vertex Index Buffer
	for (DX12ResourceManager::ResourceHandle* indexBuffer : allIndexBuffers) {
		srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_UINT;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(indexBuffer->default_buffer->GetDesc().Width / sizeof(uint32_t));
		srvDesc.Buffer.StructureByteStride = 0;
		d3dDevice->CreateShaderResourceView(indexBuffer->default_buffer, &srvDesc, cpuHandle);

		cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
		indexBuffer->heap_index_srv = global_descriptor_heap_allocator->alloc();
		std::cout << "srvNumElementsIndex: " << srvDesc.Buffer.NumElements << std::endl;
	}

	// Material Buffer
	srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(dx12Materials.size());
	srvDesc.Buffer.StructureByteStride = sizeof(DX12ResourceManager::DX12Material);
	d3dDevice->CreateShaderResourceView(materialsBuffer->default_buffer, &srvDesc, cpuHandle);

	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	materialsBuffer->heap_index_srv = global_descriptor_heap_allocator->alloc();

	// Material Index Buffer
	srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_UINT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(materialIndexBuffer->default_buffer->GetDesc().Width / sizeof(uint32_t));
	srvDesc.Buffer.StructureByteStride = 0;
	d3dDevice->CreateShaderResourceView(materialIndexBuffer->default_buffer, &srvDesc, cpuHandle);

	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	materialIndexBuffer->heap_index_srv = global_descriptor_heap_allocator->alloc();

	// Camera CBV
	cbvDesc = {};
	cbvDesc.BufferLocation = cameraConstantBuffer->upload_buffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = sizeof(DX12ResourceManager::DX12Camera);

	d3dDevice->CreateConstantBufferView(&cbvDesc, cpuHandle);
	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	cameraConstantBuffer->heap_index_srv = global_descriptor_heap_allocator->alloc();

	// SRV for accumulationTexture
	srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	d3dDevice->CreateShaderResourceView(accumulationTexture->default_buffer, &srvDesc, cpuHandle);

	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	accumulationTexture->heap_index_srv = global_descriptor_heap_allocator->alloc();

	// UAV for renderTarget
	uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	d3dDevice->CreateUnorderedAccessView(renderTarget->default_buffer, nullptr, &uavDesc, cpuHandle);

	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	renderTarget->heap_index_uav = global_descriptor_heap_allocator->alloc();

	// UAV for maxLumBuffer
	uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN,
		uavDesc.Buffer.StructureByteStride = sizeof(UINT),
		uavDesc.Buffer.NumElements = 1;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,

	d3dDevice->CreateUnorderedAccessView(maxLumBuffer->default_buffer, nullptr, &uavDesc, cpuHandle);

	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	maxLumBuffer->heap_index_uav = global_descriptor_heap_allocator->alloc();


	// CBV post processing params
	cbvDesc = {};
	cbvDesc.BufferLocation = toneMappingConstantBuffer->upload_buffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = sizeof(DX12ResourceManager::ToneMappingParams);
	d3dDevice->CreateConstantBufferView(&cbvDesc, cpuHandle);
	
	cpuHandle.ptr += global_descriptor_heap_allocator->desc_increment_size;
	toneMappingConstantBuffer->heap_index_srv = global_descriptor_heap_allocator->alloc();

}

void DX12ResourceManager::initModelBuffers() {

	// for vertex and index buffer SRVs

	std::cout << "Creating upload and default buffers" << std::endl;

	// create upload and default buffers and store them
	for (auto& [name, loadedModel] : meshManager->loadedModels) {

		std::cout << "model name: " << name << std::endl;

		DX12ResourceManager::DX12Model* dx12Model = new DX12ResourceManager::DX12Model{};

		dx12Model->loadedModel = loadedModel;
		dx12Models[loadedModel->name] = dx12Model;

		for (size_t i = 0; i < loadedModel->meshes.size(); i++) {

			MeshManager::Mesh mesh = loadedModel->meshes[i];

			size_t vbSize = mesh.vertices.size() * sizeof(MeshManager::Vertex);
			size_t ibSize = mesh.indices.size() * sizeof(uint32_t);
			std::cout << ": vbSize=" << vbSize << " bytes, ibSize=" << ibSize << " bytes" << std::endl;

			DX12ResourceManager::ResourceHandle *vertexBuffer = createResourceHandle(mesh.vertices.data(), vbSize, D3D12_RESOURCE_STATE_COMMON, false);
			DX12ResourceManager::ResourceHandle* indexBuffer = createResourceHandle(mesh.indices.data(), ibSize, D3D12_RESOURCE_STATE_COMMON, false);

			vertexBuffer->upload_buffer->SetName(L"Object Vertex Upload Buffer");
			indexBuffer->upload_buffer->SetName(L"Object Index Upload Buffer");
			vertexBuffer->default_buffer->SetName(L"Object Vertex Default Buffer");
			indexBuffer->default_buffer->SetName(L"Object Index Default Buffer");

			// CPU memory buffers = HEAP_UPLOAD
			// GPU memory buffers = HEAP_DEFAULT

			dx12Model->vertexBuffers.push_back(vertexBuffer);
			dx12Model->indexBuffers.push_back(indexBuffer);
		}

	}

	std::cout << "Pushing buffers to VRAM" << std::endl;

	for (auto& [name, dx12Model] : dx12Models) {

		std::cout << dx12Model->loadedModel->name << std::endl;

		std::vector<MeshManager::Mesh>& meshes = dx12Model->loadedModel->meshes;

		for (size_t i = 0; i < dx12Model->loadedModel->meshes.size(); i++) {

			MeshManager::Mesh& mesh = dx12Model->loadedModel->meshes[i];

			size_t vbSize = mesh.vertices.size() * sizeof(MeshManager::Vertex);
			size_t ibSize = mesh.indices.size() * sizeof(uint32_t);

			std::cout << "Mesh " << i << ": vbSize=" << vbSize << " bytes, ibSize=" << ibSize << " bytes\n" << std::endl;

			pushResourceHandle(dx12Model->vertexBuffers[i], vbSize, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			pushResourceHandle(dx12Model->indexBuffers[i], ibSize, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		}

	}

}

DX12ResourceManager::ResourceHandle* DX12ResourceManager::makeAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, UINT64* update_scratch_size) {


	auto makeBuffer = [this](UINT64 size, auto initialState) {
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		desc.Width = size;
		ID3D12Resource* buffer;

		HRESULT hr = d3dDevice->CreateCommittedResource(
			&DEFAULT_HEAP,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			initialState,
			nullptr,
			IID_PPV_ARGS(&buffer)
		);

		checkHR(hr, nullptr, "CreateCommittedResource for AS failed");

		return buffer;
		};

	// debug
	if (inputs.NumDescs == 0 || inputs.pGeometryDescs == nullptr) {
		std::cerr << "Invalid inputs: no geometry\n";
		return nullptr;
	}

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	d3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	// debug
	std::cout << "Prebuild: scratch=" << prebuildInfo.ScratchDataSizeInBytes
		<< ", result=" << prebuildInfo.ResultDataMaxSizeInBytes << "\n";

	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0) {
		std::cerr << "Prebuild returned zero sizes — invalid geometry!\n";
		return nullptr;
	}

	if (update_scratch_size) *update_scratch_size = prebuildInfo.UpdateScratchDataSizeInBytes;

	auto* scratch = makeBuffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_COMMON);
	auto* as = makeBuffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	scratch->SetName(L"scratch buffer");
	as->SetName(L"BLAS");

	if (scratch == nullptr) std::cout << "scratch nullptr" << std::endl;
	if (as == nullptr) std::cout << "BLAS nullptr" << std::endl;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {
	.DestAccelerationStructureData = as->GetGPUVirtualAddress(), .Inputs = inputs, .ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress() };

	std::cout << "executing cmd list " << std::endl;

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	cmdList->Close();
	cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&cmdList));
	waitForGPU();
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc, nullptr);


	scratch->Release();
	DX12ResourceManager::ResourceHandle* resource_handle = new DX12ResourceManager::ResourceHandle{};
	resource_handle->default_buffer = as;
	return resource_handle;

}

void DX12ResourceManager::initModelBLAS() {

	if (config.debug) std::cout << "initBottomLevel()" << std::endl;

	for (auto& [name, model] : dx12Models) {

		delete model->BLAS;

		std::cout << "object name: " << name << std::endl;

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs;

		for (size_t i = 0; i < model->vertexBuffers.size(); i++) {

			D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
			geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			geometryDesc.Triangles.VertexBuffer.StartAddress = model->vertexBuffers[i]->default_buffer->GetGPUVirtualAddress();
			geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(MeshManager::Vertex);
			geometryDesc.Triangles.VertexCount = static_cast<UINT>(model->loadedModel->meshes[i].vertices.size());
			geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geometryDesc.Triangles.IndexBuffer = model->indexBuffers[i]->default_buffer->GetGPUVirtualAddress();
			geometryDesc.Triangles.IndexCount = static_cast<UINT>(model->loadedModel->meshes[i].indices.size());
			geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
			geomDescs.push_back(geometryDesc);
		}

		std::cout << "making accel struc inputs: " << std::endl;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
			.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
			.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
			.NumDescs = static_cast<UINT>(geomDescs.size()),
			.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
			.pGeometryDescs = geomDescs.data() };

		std::cout << "making accel struc " << std::endl;

		model->BLAS = makeAccelerationStructure(inputs);
		model->BLAS->default_buffer->SetName(L"Model BLAS");
	}

}

void DX12ResourceManager::updateCamera() {

	entityManager->camera->update();

	using namespace DirectX;

	EntityManager::Camera* entityCamera = entityManager->camera;

	dx12Camera->position = { entityCamera->position.x, entityCamera->position.y, entityCamera->position.z };
	dx12Camera->seed = seed;
	dx12Camera->sky = config.sky == true ? 1u : 0u;
	dx12Camera->skyBrighness = config.skyBrightness;
	dx12Camera->minBounces = config.minBounces;
	dx12Camera->maxBounces = config.maxBounces;
	dx12Camera->jitter = config.jitter == true ? 1u : 0u;

	PT::Vector3 position = entityCamera->position;
	PT::Vector3 right = entityCamera->right;
	PT::Vector3 up = entityCamera->up;
	PT::Vector3 forward = entityCamera->forward;
	float fovYDegrees = entityCamera->fovYDegrees;
	float fovYRad = XMConvertToRadians(fovYDegrees);
	float aspect = entityCamera->aspect;


	XMFLOAT3 pos = { position.x, position.y, position.z };
	XMVECTOR eyePos = XMLoadFloat3(&pos);

	XMFLOAT3 forw = { forward.x, forward.y, forward.z };

	XMVECTOR forwardVec = XMLoadFloat3(&forw);
	XMVECTOR targetPos = XMVectorAdd(eyePos, forwardVec);
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(eyePos, targetPos, worldUp);

	XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspect, 0.01f, 1000.0f);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMVECTOR det;

	XMMATRIX invViewProj = DirectX::XMMatrixInverse(&det, viewProj);

	XMStoreFloat4x4(&dx12Camera->invViewProj, invViewProj);


	if (!cameraConstantBuffer) {
		std::cout << "test" << std::endl;
		cameraConstantBuffer = new DX12ResourceManager::ResourceHandle{};
		createCBV(cameraConstantBuffer, sizeof(DX12ResourceManager::DX12Camera));
		cameraConstantBuffer->upload_buffer->SetName(L"Camera Default Buffer");
	}

	void* mapped = nullptr;
	HRESULT hr = cameraConstantBuffer->upload_buffer->Map(0, nullptr, &mapped);
	checkHR(hr, nullptr, "memcpy Camera CBV");
	memcpy(mapped, dx12Camera, sizeof(DX12ResourceManager::DX12Camera));
	cameraConstantBuffer->upload_buffer->Unmap(0, nullptr);

	// upload heap is always visible for cbv, no barriers


}

void DX12ResourceManager::initScene() {

	if (config.debug) std::cout << "initScene()" << std::endl;

	materials.clear();
	dx12entities.clear();

	for (size_t i = 0; i < entityManager->entities.size(); i++) {

		EntityManager::Entity* entity = entityManager->entities[i];
		DX12ResourceManager::DX12Entity* dx12Entity = new DX12ResourceManager::DX12Entity{};
		dx12Entity->entity = entity;
		dx12Entity->model = dx12Models[entity->model];

		std::cout << "Material name: " << entity->material << std::endl;

		// create material if it doesn't already exist
		if (materials.find(entity->material) == materials.end()) {
			std::cout << "Material doesnt exist " << std::endl;

			DX12ResourceManager::DX12Material* dx12Material = new DX12ResourceManager::DX12Material{ materialManager->materials[entity->material] };
			materials[entity->material] = dx12Material;
			dx12Entity->material = dx12Material;
		}
		else {
			std::cout << "Material exists " << std::endl;
			dx12Entity->material = materials[entity->material];
		}

		dx12entities.push_back(dx12Entity);
	}

	if (config.debug) std::cout << "creating instances" << std::endl;

	// create instances

	NUM_INSTANCES = static_cast<UINT>(dx12entities.size());

	D3D12_RESOURCE_DESC instancesDesc{};
	instancesDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	instancesDesc.Width = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * config.maxInstances;
	instancesDesc.Height = 1;
	instancesDesc.DepthOrArraySize = 1;
	instancesDesc.MipLevels = 1;
	instancesDesc.SampleDesc = NO_AA;
	instancesDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	instances = new DX12ResourceManager::ResourceHandle{};
	HRESULT hr = d3dDevice->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &instancesDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&instances->default_buffer));
	checkHR(hr, nullptr, "initScene, CreateComittedResource: ");

	instances->default_buffer->Map(0, nullptr, reinterpret_cast<void**>(&instanceData));

	if (config.debug) std::cout << "init scene" << std::endl;

	uint32_t instanceID = -1; // user provided
	uint32_t instanceIndex = 0;

	uniqueInstancesID.clear();

	for (DX12ResourceManager::DX12Entity* dx12Entity : dx12entities) {

		if (dx12Entity->model->BLAS == nullptr) std::cout << "BLAS nullptr " << std::endl;

		DX12ResourceManager::ResourceHandle* objectBlas = dx12Entity->model->BLAS;

		if (uniqueInstancesID.find(dx12Entity->entity->model) == uniqueInstancesID.end()) {
			instanceID = uniqueInstancesID.size();
			uniqueInstancesID[dx12Entity->entity->model] = instanceID;
		}
		else {
			instanceID = uniqueInstancesID[dx12Entity->entity->model];
		}

		instanceData[instanceIndex] = {
			.InstanceID = static_cast<UINT>(instanceID),
			.InstanceMask = 1,
			.InstanceContributionToHitGroupIndex = 0,
			.Flags = 0,
			.AccelerationStructure = objectBlas->default_buffer->GetGPUVirtualAddress(),
		};

		instanceIndex++;
	}

	// fill in dummy instances
	// to allow easily rebuildable BLAS

	DX12Model* dummyModel = dx12Models["cube"];

	for (UINT i = dx12entities.size(); i < config.maxInstances; i++) {
		
		instanceData[instanceIndex] = {
			.InstanceID = static_cast<UINT>(instanceID),
			.InstanceMask = 0,
			.InstanceContributionToHitGroupIndex = 0,
			.Flags = 0,
			.AccelerationStructure = dummyModel->BLAS->default_buffer->GetGPUVirtualAddress(),
		};

		instanceIndex++;
	}

	updateTransforms();
}

void DX12ResourceManager::updateTransforms() {

	//if (debugstage) std::cout << "Update Transforms" << std::endl;

	auto time = static_cast<float>(GetTickCount64()) / 1000.0f;

	size_t currentInstance = 0;

	// apply meshes stored transform

	for (DX12ResourceManager::DX12Entity* dx12Entity : dx12entities) {

		auto vecRotation = dx12Entity->entity->rotation;
		auto vecPosition = dx12Entity->entity->position;
		auto vecScale = dx12Entity->entity->scale;

		DirectX::XMMATRIX worldTransform = DirectX::XMMatrixScaling(vecScale.x, vecScale.y, vecScale.z);
			
		worldTransform *= DirectX::XMMatrixRotationRollPitchYaw(vecRotation.x, vecRotation.y, vecRotation.z);

		worldTransform *= DirectX::XMMatrixTranslation(vecPosition.x, vecPosition.y, vecPosition.z);

		auto* ptr = reinterpret_cast<DirectX::XMFLOAT3X4*>(&instanceData[currentInstance].Transform);
		XMStoreFloat3x4(ptr, worldTransform);

		currentInstance++;

	}

}

void DX12ResourceManager::initMaterialBuffer(bool is_update) {

	if (config.debug) std::cout << "initMaterialBuffer()" << std::endl;

	// similar to init top level
	// index buffer for materials
	// no material duplicates

	dx12Materials.clear();
	uniqueInstancesID.clear();
	materialIndices.clear();

	uint32_t instanceIndex = -1;

	for (DX12ResourceManager::DX12Entity* dx12Entity : dx12entities) {

		std::cout << "Entity Name: " << dx12Entity->entity->model << std::endl;

		DX12ResourceManager::DX12Material* dx12Mateiral = dx12Entity->material;
		std::cout << "Material name: " << dx12Entity->entity->material << std::endl;

		if (uniqueInstancesID.find(dx12Entity->entity->material) == uniqueInstancesID.end()) {
			instanceIndex = dx12Materials.size();
			uniqueInstancesID[dx12Entity->entity->material] = instanceIndex;
			dx12Materials.push_back(*dx12Entity->material);
			std::cout << "Not in map " << std::endl;
			std::cout << "instanceIndex: " << instanceIndex << std::endl;
		}
		else {
			instanceIndex = uniqueInstancesID[dx12Entity->entity->material];
			std::cout << "In map " << std::endl;
			std::cout << "instanceIndex: " << instanceIndex << std::endl;
		}

		materialIndices.push_back(instanceIndex);
	}

	if (config.debug) std::cout << "DX12Materials.size(): " << dx12Materials.size() << std::endl;
	if (config.debug) std::cout << "materialIndex.size(): " << materialIndices.size() << std::endl;


	size_t materialsSize = dx12Materials.size() * sizeof(DX12ResourceManager::DX12Material);
	size_t indexSize = materialIndices.size() * sizeof(uint32_t);

	if (!is_update) {
		materialsBuffer = createResourceHandle(dx12Materials.data(), materialsSize, D3D12_RESOURCE_STATE_COMMON, false);
		materialIndexBuffer = createResourceHandle(materialIndices.data(), indexSize, D3D12_RESOURCE_STATE_COMMON, false);
	}
	else {
		updateResourceHandle(materialsBuffer, dx12Materials.data(), materialsSize, D3D12_RESOURCE_STATE_COMMON, false);
		updateResourceHandle(materialIndexBuffer, materialIndices.data(), indexSize, D3D12_RESOURCE_STATE_COMMON, false);
	}
	

	materialsBuffer->upload_buffer->SetName(L"Materials Upload Buffer");
	materialIndexBuffer->upload_buffer->SetName(L"Materials Index Upload Default Buffer");
	materialsBuffer->default_buffer->SetName(L"Materials Default Buffer");
	materialIndexBuffer->default_buffer->SetName(L"Materials Index Default Buffer");

	pushResourceHandle(materialsBuffer, materialsSize, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	pushResourceHandle(materialIndexBuffer, indexSize, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER);


	if (config.debug) std::cout << "Finished material buffer" << std::endl;

}

void DX12ResourceManager::initTopLevelAS() {

	tlas_scratch = new ResourceHandle{};
	tlas = new ResourceHandle{};
	
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

void DX12ResourceManager::initVertexIndexBuffers() {

	allVertexBuffers.clear();
	allIndexBuffers.clear();

	uniqueInstances.clear();
	for (DX12ResourceManager::DX12Entity* dx12SceneObject : dx12entities) {


		if (uniqueInstances.count(dx12SceneObject->entity->model) == 0) {

			uniqueInstances.insert(dx12SceneObject->entity->model);
			std::vector<DX12ResourceManager::ResourceHandle*> indexBuffers = dx12SceneObject->model->indexBuffers;
			std::vector<DX12ResourceManager::ResourceHandle*> vertexBuffers = dx12SceneObject->model->vertexBuffers;

			size_t bufferSize = indexBuffers.size();

			for (size_t i = 0; i < bufferSize; i++) {
				allIndexBuffers.push_back(indexBuffers[i]);
				allVertexBuffers.push_back(vertexBuffers[i]);
			}
		}

	}

}

void DX12ResourceManager::updateRand() {

	std::cout << "update rand" << std::endl;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint64_t> dist;

	randPattern.resize(width * height);

	for (uint64_t x = 0; x < width; x++) {

		for (uint64_t y = 0; y < height; y++) {

			uint64_t state = ((y << 16u) | x) ^ (seed * 1664525u * static_cast<uint64_t>(GetTickCount64())) ^ 0xdeadbeefu;

			// PCG hash function

			uint64_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
			word = (word >> 22u) ^ word;

			uint64_t rot = state >> 28u;

			word = (word >> rot) | (word << (32u - rot));

			//randPattern[x + y * width] = word;
			randPattern[x + y * width] = dist(gen);
		}
	}

	size_t randSize = randPattern.size() * sizeof(uint64_t);
	randBuffer = createResourceHandle(randPattern.data(), randSize, D3D12_RESOURCE_STATE_COMMON, true);
	randBuffer->default_buffer->SetName(L"rng Defaut Buffer");
	pushResourceHandle(randBuffer, randSize, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

}

void DX12ResourceManager::initMaxLumBuffer() {

	std::vector<UINT> lum;
	lum.resize(1);
	lum[0] = 1u;
	maxLumBuffer = createResourceHandle(lum.data(), sizeof(UINT), D3D12_RESOURCE_STATE_COMMON, true);
	maxLumBuffer->default_buffer->SetName(L"Max Luminance Buffer");
	pushResourceHandle(maxLumBuffer, sizeof(UINT), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);


};

void DX12ResourceManager::updateToneParams() {

	//if (config.debug) std::cout << "updateTonePrams" << std::endl;

	if (!toneMappingParams) {
		toneMappingParams = new DX12ResourceManager::ToneMappingParams();
	}

	toneMappingParams->num_samples = samples;
	toneMappingParams->white_point = config.whitepoint;

	if (!toneMappingConstantBuffer) {

		std::cout << "Creating tone mapping buffer" << std::endl;
		toneMappingConstantBuffer = new DX12ResourceManager::ResourceHandle();
		createCBV(toneMappingConstantBuffer, sizeof(DX12ResourceManager::ToneMappingParams));
		toneMappingConstantBuffer->upload_buffer->SetName(L"Tone Mapping Default Buffer");
	}

	void* mapped = nullptr;
	toneMappingConstantBuffer->upload_buffer->Map(0, nullptr, &mapped);
	memcpy(mapped, toneMappingParams, sizeof(DX12ResourceManager::ToneMappingParams));
	toneMappingConstantBuffer->upload_buffer->Unmap(0, nullptr);

}

void DX12ResourceManager::rebuildBLAS() {

	initModelBuffers(); // only if a new unique model is added
	initModelBLAS(); // only if a new unique model is added

	initScene(); // only if a new entity is added

}

void DX12ResourceManager::updateTLAS() {

	if (entityManager->entities.size() > config.maxInstances) {
		std::cout << "Too many entity" << std::endl;
		// Completely rebuild TLAS
	}
	
	initScene();

	// Update exisitng TLAS

	updateTransforms();

	tlas_scratch = new ResourceHandle{};

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

// utility

void DX12ResourceManager::initRenderTarget(ResourceHandle* resource_handle, std::string resource_name) {

	std::cout << "initRenderTarget" << std::endl;

	D3D12_RESOURCE_DESC rtDesc = {
	   .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
	   .Width = width,
	   .Height = height,
	   .DepthOrArraySize = 1,
	   .MipLevels = 1,
	   .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	   .SampleDesc = NO_AA,
	   .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS };

	HRESULT hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &rtDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource_handle->default_buffer));
	checkHR(hr, nullptr, "Create render target");

	//renderTarget->default_buffer->SetName(resource_name);

	// move to a resize call
	randPattern.resize(width * height);

}

void DX12ResourceManager::initAccumulationTexture(ResourceHandle* resource_handle, std::string resource_name) {

	D3D12_RESOURCE_DESC accumDesc = {};
	accumDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	accumDesc.Width = width;
	accumDesc.Height = height;
	accumDesc.DepthOrArraySize = 1;
	accumDesc.MipLevels = 1;
	accumDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	accumDesc.SampleDesc = NO_AA;
	accumDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &accumDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource_handle->default_buffer));
	checkHR(hr, nullptr, "Create accumulation texture");
	
	//resource_handle->default_buffer->SetName(resource_name);

}

DX12ResourceManager::ResourceHandle* DX12ResourceManager::createResourceHandle(const void* data, size_t byteSize, D3D12_RESOURCE_STATES finalState, bool UAV) {
	std::cout << "byteSize: " << byteSize << std::endl;

	// CPU Buffer (upload buffer)
	D3D12_RESOURCE_DESC DESC = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = byteSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.SampleDesc = NO_AA,
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE,
	};


	ID3D12Resource* upload;
	d3dDevice->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &DESC, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));

	void* mapped;
	upload->Map(0, nullptr, &mapped); // mapped now points to the upload buffer
	memcpy(mapped, data, byteSize); // copy data to upload buffer
	upload->Unmap(0, nullptr); // 7

	// VRAM Buffer

	D3D12_HEAP_PROPERTIES DEFAULT_HEAP = { .Type = D3D12_HEAP_TYPE_DEFAULT };

	// Create target buffer in DEFAULT heap

	if (UAV) {
		DESC.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	ID3D12Resource* target = nullptr;
	d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &DESC, finalState, nullptr, IID_PPV_ARGS(&target));

	DX12ResourceManager::ResourceHandle* resource_handle = new DX12ResourceManager::ResourceHandle();
	resource_handle->upload_buffer = upload;
	resource_handle->default_buffer = target;
	return resource_handle;
}

void DX12ResourceManager::updateResourceHandle(DX12ResourceManager::ResourceHandle* resource_handle, const void* data, size_t byteSize, D3D12_RESOURCE_STATES finalState, bool UAV) {
	std::cout << "byteSize: " << byteSize << std::endl;

	resource_handle->upload_buffer->Release();
	resource_handle->default_buffer->Release();

	// CPU Buffer (upload buffer)
	D3D12_RESOURCE_DESC DESC = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = byteSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.SampleDesc = NO_AA,
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE,
	};


	ID3D12Resource* upload;
	d3dDevice->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &DESC, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));

	void* mapped;
	upload->Map(0, nullptr, &mapped); // mapped now points to the upload buffer
	memcpy(mapped, data, byteSize); // copy data to upload buffer
	upload->Unmap(0, nullptr); // 7

	// VRAM Buffer

	D3D12_HEAP_PROPERTIES DEFAULT_HEAP = { .Type = D3D12_HEAP_TYPE_DEFAULT };

	// Create target buffer in DEFAULT heap

	if (UAV) {
		DESC.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	ID3D12Resource* target = nullptr;
	d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &DESC, finalState, nullptr, IID_PPV_ARGS(&target));

	resource_handle->upload_buffer = upload;
	resource_handle->default_buffer = target;
}

void DX12ResourceManager::createCBV(DX12ResourceManager::ResourceHandle* resource_handle, size_t byte_size) {

	D3D12_RESOURCE_DESC cbDesc = {};
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Width = byte_size;
	cbDesc.Height = 1;
	cbDesc.DepthOrArraySize = 1;
	cbDesc.MipLevels = 1;
	cbDesc.SampleDesc = NO_AA;;
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES uploadHeapProps = { D3D12_HEAP_TYPE_UPLOAD };

	HRESULT hr = d3dDevice->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // cbv
		nullptr,
		IID_PPV_ARGS(&resource_handle->upload_buffer)
	);
	checkHR(hr, nullptr, "Create camera CB");

}

void DX12ResourceManager::pushResourceHandle(DX12ResourceManager::ResourceHandle* resource_handle, size_t data_size, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after) {

	// Barrier - transition vertex target to COPY_DEST
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource_handle->default_buffer;
	barrier.Transition.StateBefore = state_before;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);

	// Copy to GPU
	cmdList->CopyBufferRegion(resource_handle->default_buffer, 0, resource_handle->upload_buffer, 0, data_size);

	// Barrier - transition to final state
	barrier.Transition.pResource = resource_handle->default_buffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = state_after;
	cmdList->ResourceBarrier(1, &barrier);
}

// helper

void DX12ResourceManager::checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context) {
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

void DX12ResourceManager::waitForGPU() {

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