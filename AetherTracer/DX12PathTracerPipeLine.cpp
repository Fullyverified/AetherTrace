#include "DX12PathTracerPipeLine.h"

#include <iostream>
#include <random>

#include <d3dcompiler.h> // for compiling shaders

#include "Config.h"
#include "UI.h"


bool debug = true;
ImGuiDescriptorAllocator* ImGuiDescAlloc;

// for imgui
void ImGuiDX12AllocateSRV(ImGui_ImplDX12_InitInfo* init_info_unused, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) {

	(void)init_info_unused;  // Not used in most cases

	return ImGuiDescAlloc->alloc(out_cpu, out_gpu);
}

// for imgui
void ImGuiDX12FreeSRV(ImGui_ImplDX12_InitInfo* init_info_unused, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu) {
	(void)init_info_unused;
	ImGuiDescAlloc->free(cpu, gpu);
}

DX12PathTracerPipeLine::DX12PathTracerPipeLine(EntityManager* entityManager, MeshManager* meshManager, MaterialManager* materialManager, Window* window) : entityManager(entityManager), meshManager(meshManager), materialManager(materialManager), window(window) {

	rm = new DX12ResourceManager(meshManager, materialManager, entityManager);

}

void DX12PathTracerPipeLine::init() {

	rm->hwnd = static_cast<HWND>(window->getNativeHandle());

	loadShaders();

	initDevice();
	initSurfaces();
	initCommand();

	initImgui();

	rm->dx12Camera = new DX12ResourceManager::DX12Camera{};

	resize();

	std::cout << "init Descriptor Heaps" << std::endl;

	rm->initDescriptorHeap(rm->global_descriptor_heap_allocator, 1000, true, "Global Descriptor Heap");

	std::cout << "init RTResources" << std::endl;
	rm->updateCamera();
	rm->initAccumulationTexture(rm->accumulationTexture, "Accumulation Texture");

	rm->initModelBuffers(); // instanced vertex and index buffers
	rm->initModelBLAS(); // instanced model blas
	rm->initScene();
	rm->initTopLevelAS();
	rm->initMaterialBuffer(false);
	rm->initVertexIndexBuffers();

	rm->initRenderTarget(rm->renderTarget, "Render Target");
	rm->updateRand();
	rm->updateToneParams();
	rm->initMaxLumBuffer();

	std::cout << "init Global Descriptor Heap" << std::endl;
	rm->initGlobalDescriptors();

	initRootSignature();
	initComputePipeline();

	initRayTracingPipeline();
	initRTShaderTables();

	bindDescriptors();

	rm->cmdList->Close();
	rm->cmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&rm->cmdList));
	rm->waitForGPU();

	rm->cmdAlloc->Reset();
	rm->cmdList->Reset(rm->cmdAlloc, nullptr);

}

// device

void DX12PathTracerPipeLine::initDevice() {

	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&rm->factory))))
		CreateDXGIFactory2(0, IID_PPV_ARGS(&rm->factory));


	// D3D12 debug layer
	if (ID3D12Debug* debug; SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
		debug->EnableDebugLayer();
		debug->Release();
	}

	/*if (ID3D12Debug1* debug1; SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug1)))) {
		debug1->SetEnableGPUBasedValidation(true);
		debug1->Release();
	}*/

	// feature level dx12_2
	IDXGIAdapter* adapter = nullptr;
	D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&rm->d3dDevice));

	// command queue
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT, };
	rm->d3dDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&rm->cmdQueue));
	// fence
	rm->d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&rm->fence));


	if (rm->d3dDevice == nullptr) {
		std::cout << "device nullptr" << std::endl;
	}
	else std::cout << "device exists" << std::endl;
}

// swap chain

void DX12PathTracerPipeLine::initSurfaces() {

	// 8-bit SRGB
	// alternative: R16G16B16A16_FLOAT for HDR
	DXGI_SWAP_CHAIN_DESC1 scDesc = {
	   .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	   .SampleDesc = rm->NO_AA,
	   .BufferCount = 2,
	   .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
	   .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
	};
	IDXGISwapChain1* swapChain1;

	HRESULT hr = rm->factory->CreateSwapChainForHwnd(rm->cmdQueue, rm->hwnd, &scDesc, nullptr, nullptr, &swapChain1);
	checkHR(hr, nullptr, "Create swap chain: ");


	swapChain1->QueryInterface(&rm->swapChain);
	swapChain1->Release();

	// early factory release
	rm->factory->Release();

}

// render target
void DX12PathTracerPipeLine::resize() {

	std::cout << "Resize called" << std::endl;
	if (!rm->swapChain) {
		std::cout << "Resize: swapChain is null - skipping" << std::endl;
		return;
	}

	RECT rect;
	GetClientRect(rm->hwnd, &rect);
	rm->width = std::max<UINT>(rect.right - rect.left, 1);
	rm->height = std::max<UINT>(rect.bottom - rect.top, 1);

	rm->waitForGPU();

	rm->swapChain->ResizeBuffers(0, rm->width, rm->height, DXGI_FORMAT_UNKNOWN, 0);

	// Update render target and accumulation texture
	
	if (rm->renderTarget) {
		rm->randPattern.resize(rm->width * rm->height);
	}

	
	createBackBufferRTVs();

	if (rm->renderTarget) {
		rm->randPattern.resize(rm->width * rm->height);
	}

	std::cout << "resize" << std::endl;
}

// command list and allocator

void DX12PathTracerPipeLine::initCommand() {
	// only one
	rm->d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&rm->cmdAlloc));
	rm->d3dDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&rm->cmdList));

	rm->cmdAlloc->SetName(L"Default cmdAlloc");
	rm->cmdList->SetName(L"Default cmdList");

	rm->cmdAlloc->Reset();
	rm->cmdList->Reset(rm->cmdAlloc, nullptr);
}

void DX12PathTracerPipeLine::loadShaders() {
	
	HRESULT hr = D3DReadFileToBlob(L"raytracingshader.cso", &rm->rsBlob);
	checkHR(hr, nullptr, "Loading postprocessingshader");

	hr = D3DReadFileToBlob(L"postprocessingshader.cso", &rm->csBlob);
	checkHR(hr, nullptr, "Loading postprocessingshader");

}

void DX12PathTracerPipeLine::initRootSignature() {

	if (config.debug) std::cout << "initRTRootSignature()" << std::endl;

	UINT NUM_BUFFERS = static_cast<UINT>(rm->allVertexBuffers.size());

	D3D12_DESCRIPTOR_RANGE accumRangeUAV = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 0,
	.RegisterSpace = 0,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	D3D12_DESCRIPTOR_RANGE randRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 1,
	.RegisterSpace = 0,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	// u0, render target
	D3D12_DESCRIPTOR_RANGE rtRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 2,
	.RegisterSpace = 0,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	// u1, maxLum
	D3D12_DESCRIPTOR_RANGE maxLumRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 3,
	.RegisterSpace = 0,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	// t0, accumulation texture
	D3D12_DESCRIPTOR_RANGE accumRangeSRV = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 0,
	.RegisterSpace = 0,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	D3D12_DESCRIPTOR_RANGE sceneRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 1,
	.RegisterSpace = 1,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	D3D12_DESCRIPTOR_RANGE vertexRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	.NumDescriptors = NUM_BUFFERS,
	.BaseShaderRegister = 2,
	.RegisterSpace = 2,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	D3D12_DESCRIPTOR_RANGE indexRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	.NumDescriptors = NUM_BUFFERS,
	.BaseShaderRegister = 3,
	.RegisterSpace = 3,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	D3D12_DESCRIPTOR_RANGE materialRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 4,
	.RegisterSpace = 4,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};

	D3D12_DESCRIPTOR_RANGE materialIndexRange = {
	.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
	.NumDescriptors = 1,
	.BaseShaderRegister = 5,
	.RegisterSpace = 5,
	.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};


	// CBV for Camera
	D3D12_ROOT_PARAMETER cameraParam = {};
	cameraParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	cameraParam.Descriptor.ShaderRegister = 0;
	cameraParam.Descriptor.RegisterSpace = 0;
	cameraParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// CBV for tone mapping parameters
	D3D12_ROOT_PARAMETER toneParam = {};
	toneParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	toneParam.Descriptor.ShaderRegister = 1;
	toneParam.Descriptor.RegisterSpace = 0;
	toneParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	D3D12_ROOT_PARAMETER params[12] = {												// num desriptor ranges, descriptor range
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &accumRangeUAV}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &randRange}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &rtRange}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &maxLumRange}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &accumRangeSRV}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &sceneRange}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &vertexRange}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &indexRange}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &materialRange}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, .DescriptorTable = {1, &materialIndexRange}},
		cameraParam,
		toneParam,
	};


	D3D12_ROOT_SIGNATURE_DESC desc = {
		.NumParameters = 12,
		.pParameters = params,
		.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
	};

	ID3DBlob* blob;
	ID3DBlob* errorblob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errorblob);
	checkHR(hr, nullptr, "D3D12SerializeRootSignature: ");


	hr = rm->d3dDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rm->rootSignature));
	checkHR(hr, errorblob, "CreateRootSignature: ");

	blob->Release();

	rm->cmdList->SetComputeRootSignature(rm->rootSignature);

}

void DX12PathTracerPipeLine::initRTShaderTables() {

	if (config.debug) std::cout << "iniRTShaderTables()" << std::endl;

	D3D12_RESOURCE_DESC shaderIDDesc{};
	shaderIDDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	shaderIDDesc.Width = rm->NUM_SHADER_IDS * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;;
	shaderIDDesc.Height = 1;
	shaderIDDesc.DepthOrArraySize = 1;
	shaderIDDesc.MipLevels = 1;
	shaderIDDesc.SampleDesc = rm->NO_AA;
	shaderIDDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = rm->d3dDevice->CreateCommittedResource(&rm->UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &shaderIDDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&rm->shaderIDs));
	checkHR(hr, nullptr, "initShaderTables, CreateCommittedResource: ");
	ID3D12StateObjectProperties* props;
	rm->raytracingPSO->QueryInterface(&props);

	void* data;
	auto writeId = [&](const wchar_t* name) {
		void* id = props->GetShaderIdentifier(name);
		memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		data = static_cast<char*>(data) + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		};

	rm->shaderIDs->Map(0, nullptr, &data);
	writeId(L"RayGeneration");
	writeId(L"Miss");
	writeId(L"HitGroup");
	rm->shaderIDs->Unmap(0, nullptr);

	props->Release();


}

void DX12PathTracerPipeLine::initRayTracingPipeline() {
	
	if (config.debug) std::cout << "initRTPipeline()" << std::endl;

	D3D12_DXIL_LIBRARY_DESC lib = {
	.DXILLibrary = { rm->rsBlob->GetBufferPointer(), rm->rsBlob->GetBufferSize()} };

	D3D12_HIT_GROUP_DESC hitGroup = {
	.HitGroupExport = L"HitGroup",
	.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
	.AnyHitShaderImport = nullptr,
	.ClosestHitShaderImport = L"ClosestHit",
	.IntersectionShaderImport = nullptr
	};

	D3D12_RAYTRACING_SHADER_CONFIG shaderCfg = {
	.MaxPayloadSizeInBytes = 76,
	.MaxAttributeSizeInBytes = 8, // triangle attribs
	};


	D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { rm->rootSignature };

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = { .MaxTraceRecursionDepth = 4 };

	D3D12_STATE_SUBOBJECT subobjects[] = {
		{.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &lib},
		{.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &hitGroup},
		{.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, .pDesc = &shaderCfg},
		{.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &globalSig},
		{.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, .pDesc = &pipelineCfg},
	};

	D3D12_STATE_OBJECT_DESC psoDesc = {
		.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
		.NumSubobjects = std::size(subobjects),
		.pSubobjects = subobjects };

	HRESULT hr = rm->d3dDevice->CreateStateObject(&psoDesc, IID_PPV_ARGS(&rm->raytracingPSO));
	checkHR(hr, nullptr, "initPipeLine, CreateStateObject: ");

}

void DX12PathTracerPipeLine::initComputePipeline() {

	std::cout << "initComputePipeline" << std::endl;

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rm->rootSignature;
	psoDesc.CS = { rm->csBlob->GetBufferPointer(), rm->csBlob->GetBufferSize() };
	HRESULT hr = rm->d3dDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&rm->computePSO));
	checkHR(hr, nullptr, "Create compute pipeline state");

}

void DX12PathTracerPipeLine::bindDescriptors() {

	ID3D12DescriptorHeap* heaps[] = { rm->global_descriptor_heap_allocator->desc_heap };
	rm->cmdList->SetDescriptorHeaps(1, heaps);

	rm->cmdList->SetComputeRootSignature(rm->rootSignature);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = rm->global_descriptor_heap_allocator->desc_heap->GetGPUDescriptorHandleForHeapStart();
	UINT64 heap_start = gpuHandle.ptr;

	gpuHandle.ptr = heap_start + rm->accumulationTexture->heap_index_uav * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(0, gpuHandle); // u0 accum UAV
	
	gpuHandle.ptr = heap_start + rm->randBuffer->heap_index_uav * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(1, gpuHandle); // u1 rand UAV

	gpuHandle.ptr = heap_start + rm->renderTarget->heap_index_uav * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(2, gpuHandle); // u2 render target

	gpuHandle.ptr = heap_start + rm->maxLumBuffer->heap_index_uav * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(3, gpuHandle); // u3 uav



	gpuHandle.ptr = heap_start + rm->accumulationTexture->heap_index_srv * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(4, gpuHandle); // accum range SRV

	gpuHandle.ptr = heap_start + rm->tlas->heap_index_srv * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(5, gpuHandle); // t0 TLAS SRV

	gpuHandle.ptr = heap_start + rm->allVertexBuffers[0]->heap_index_srv * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(6, gpuHandle); // t1 vertex buffer SRV

	gpuHandle.ptr = heap_start + rm->allIndexBuffers[0]->heap_index_srv * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(7, gpuHandle); // t2 index buffer SRV

	gpuHandle.ptr = heap_start + rm->materialsBuffer->heap_index_srv * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(8, gpuHandle); // t3 material buffer SRV

	gpuHandle.ptr = heap_start + rm->materialIndexBuffer->heap_index_srv * rm->global_descriptor_heap_allocator->desc_increment_size;
	rm->cmdList->SetComputeRootDescriptorTable(9, gpuHandle); // t3 material index buffer SRV


	rm->cmdList->SetComputeRootConstantBufferView(10, rm->cameraConstantBuffer->upload_buffer->GetGPUVirtualAddress()); // b0 camera cbv

	rm->cmdList->SetComputeRootConstantBufferView(11, rm->toneMappingConstantBuffer->upload_buffer->GetGPUVirtualAddress()); // maxLum, etc


}

// command submission

void DX12PathTracerPipeLine::render() {

	rm->frames = (config.accumulate && !entityManager->camera->camMoved && !UI::accumulationUpdate) ? rm->frames + 1 : 1;
	rm->samples = rm->frames * config.raysPerPixel;
	UI::numRays = rm->samples;
	rm->seed++;

	if (UI::accelUpdate) rm->updateTLAS();
	//if (UI::materialUpdate) rm->initMaterialBuffer(true);


	bindDescriptors();

	rm->updateCamera();
	traceRays();
	postProcess();

	ImGui::Render();

}

void DX12PathTracerPipeLine::traceRays() {

	rm->cmdList->SetPipelineState1(rm->raytracingPSO);

	// clear accumulation texture
	if (!config.accumulate || entityManager->camera->camMoved || UI::accumulationUpdate || UI::accelUpdate) {
		// slot 0 UAV for accumulation texture
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rm->global_descriptor_heap_allocator->desc_heap->GetCPUDescriptorHandleForHeapStart();
		UINT64 heap_start = cpuHandle.ptr;
		cpuHandle.ptr = heap_start + rm->accumulationTexture->heap_index_uav * rm->global_descriptor_heap_allocator->desc_increment_size;

		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = rm->global_descriptor_heap_allocator->desc_heap->GetGPUDescriptorHandleForHeapStart();
		heap_start = gpuHandle.ptr;
		gpuHandle.ptr = heap_start + rm->accumulationTexture->heap_index_uav * rm->global_descriptor_heap_allocator->desc_increment_size;

		rm->cmdList->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, rm->accumulationTexture->default_buffer, rm->clearColor, 0, nullptr);

		UI::accelUpdate = false;
		UI::accumulationUpdate = false;
		UI::materialUpdate = false;
	}

	// Dispatch rays

	auto rtDesc = rm->renderTarget->default_buffer->GetDesc();

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {
		.RayGenerationShaderRecord = {
			.StartAddress = rm->shaderIDs->GetGPUVirtualAddress(),
			.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
		.MissShaderTable = {
			.StartAddress = rm->shaderIDs->GetGPUVirtualAddress() + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
			.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
		.HitGroupTable = {
			.StartAddress = rm->shaderIDs->GetGPUVirtualAddress() + 2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
			.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
		.Width = static_cast<UINT>(rtDesc.Width),
		.Height = rtDesc.Height,
		.Depth = 1 };

	for (size_t i = 0; i < config.raysPerPixel; i++) {
		rm->cmdList->DispatchRays(&dispatchDesc);
	}

	// transition accumulation texture from SRV TO UAV for tone mapping
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = rm->accumulationTexture->default_buffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	rm->cmdList->ResourceBarrier(1, &barrier);


}

void DX12PathTracerPipeLine::postProcess() {

	// tone mapping

	rm->cmdList->SetPipelineState(rm->computePSO);

	// start compute shader

	UINT groupsX = static_cast<UINT>((rm->renderTarget->default_buffer->GetDesc().Width + 15) / 16);
	UINT groupsY = static_cast<UINT>((rm->renderTarget->default_buffer->GetDesc().Height + 15) / 16);

	rm->toneMappingParams->stage = 0; // max Luminance
	rm->updateToneParams();

	rm->cmdList->Dispatch(groupsX, groupsY, 1);

	 // tone Map
	if (config.tone_mapper == reinhard_extended) rm->toneMappingParams->stage = 1;
	if (config.tone_mapper == hable_filmic) rm->toneMappingParams->stage = 2;
	if (config.tone_mapper == aces_filmic) rm->toneMappingParams->stage = 3;
	rm->updateToneParams();

	rm->cmdList->Dispatch(groupsX, groupsY, 1);

	// transition accumulation texture from SRV TO UAV for next frame
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = rm->accumulationTexture->default_buffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	rm->cmdList->ResourceBarrier(1, &barrier);

}

void DX12PathTracerPipeLine::imguiPresent(ID3D12Resource* backBuffer) {
	// set descriptor heap for imgui
	ID3D12DescriptorHeap* heaps[] = { ImGuiDescAlloc->heap };
	rm->cmdList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

	// Get current backbuffer RTV
	UINT bbIndex = rm->swapChain->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rm->rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += bbIndex * rm->rtvDescriptorSize;

	rm->cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), rm->cmdList);
}

void DX12PathTracerPipeLine::present() {

	// copy image onto swap chain's current buffer
	ID3D12Resource* backBuffer;
	rm->swapChain->GetBuffer(rm->swapChain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&backBuffer));
	backBuffer->SetName(L"Back Buffer");

	// transition render target and back buffer to copy src/dst states
	barrier(rm->renderTarget->default_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	barrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	rm->cmdList->CopyResource(backBuffer, rm->renderTarget->default_buffer);

	barrier(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);

	imguiPresent(backBuffer);

	barrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// transition for present

	// transition for next frame
	barrier(rm->renderTarget->default_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// release refernce to backbuffer
	backBuffer->Release();

	rm->cmdList->Close();
	ID3D12CommandList* lists[] = { rm->cmdList };
	rm->cmdQueue->ExecuteCommandLists(1, lists);

	rm->waitForGPU();
	rm->swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);

	// To finish whole frame??
	rm->cmdAlloc->Reset();
	rm->cmdList->Reset(rm->cmdAlloc, nullptr);
}

void DX12PathTracerPipeLine::quit() {
	delete meshManager;
}

void DX12PathTracerPipeLine::checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context) {
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

void DX12PathTracerPipeLine::barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {

	D3D12_RESOURCE_BARRIER rb = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Transition = {.pResource = resource,
						.StateBefore = before,
						.StateAfter = after},
	};

	rm->cmdList->ResourceBarrier(1, &rb);

}

void DX12PathTracerPipeLine::initImgui() {

	// thanks grok

	ImGuiDescAlloc = new ImGuiDescriptorAllocator{};
	ImGuiDescAlloc->init(rm->d3dDevice, 64);

	// for swap chain
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = rm->frameIndexInFlight;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rm->d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rm->rtvHeap));
	rm->rtvHeap->SetName(L"ImGui RTV Heap");
	rm->rtvDescriptorSize = rm->d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// ImGui init
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplSDL3_InitForD3D(window->getSDLHandle());

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = rm->d3dDevice;
	init_info.CommandQueue = rm->cmdQueue;
	init_info.NumFramesInFlight = rm->frameIndexInFlight;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	init_info.SrvDescriptorHeap = ImGuiDescAlloc->heap;
	init_info.SrvDescriptorAllocFn = ImGuiDX12AllocateSRV;
	init_info.SrvDescriptorFreeFn = ImGuiDX12FreeSRV;

	ImGui_ImplDX12_Init(&init_info);
	ImGui::StyleColorsDark();
	io.Fonts->AddFontDefault();
	io.Fonts->Build();

	createBackBufferRTVs();
}

// for imgui
void DX12PathTracerPipeLine::createBackBufferRTVs() {

	D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = rm->rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < rm->frameIndexInFlight; i++) {
		ID3D12Resource* backBuffer = nullptr;
		HRESULT hr = rm->swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		checkHR(hr, nullptr, "ImGui Backbuffer SRV");

		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvStart;
		handle.ptr += i * rm->rtvDescriptorSize;

		rm->d3dDevice->CreateRenderTargetView(backBuffer, nullptr, handle);

		backBuffer->Release();
	}

	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_CreateDeviceObjects();

}