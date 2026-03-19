#include "DX12Renderer.h"

DX12Renderer::DX12Renderer(EntityManager* entityManager, MeshManager* meshManager, MaterialManager* materialManager, Window* window) :
	entityManager(entityManager), meshManager(meshManager), materialManager(materialManager), window(window) {};

void DX12Renderer::init() {

	initDevice();

	dx12PathTracerPipeLine = new DX12PathTracerPipeLine(entityManager, meshManager, materialManager, window, factory, d3dDevice, cmdQueue);

	dx12PathTracerPipeLine->init();
}

// device

void DX12Renderer::initDevice() { 

	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)))) {
		CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	}


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
	D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&d3dDevice));

	// command queue
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT, };
	d3dDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));	

	if (d3dDevice == nullptr) {
		std::cout << "device nullptr" << std::endl;
	}
	else std::cout << "device exists" << std::endl;
}

void DX12Renderer::render() {

	dx12PathTracerPipeLine->render();

}

void DX12Renderer::present() {

	dx12PathTracerPipeLine->present();

}

void DX12Renderer::resize() {

	dx12PathTracerPipeLine->resize();

}