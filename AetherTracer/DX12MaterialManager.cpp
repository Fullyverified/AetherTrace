#include "DX12MaterialManager.h"


#include <limits>

DX12MaterialManager::DX12MaterialManager(MeshManager* meshManager, MaterialManager* materialManager, EntityManager* entityManager, IDXGIFactory4* factory, ID3D12Device5* d3dDevice, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList4* cmdList)
	: meshManager(meshManager), materialManager(materialManager), entityManager(entityManager), factory(factory), d3dDevice(d3dDevice), cmdQueue(cmdQueue), cmdAlloc(cmdAlloc), cmdList(cmdList) {
}
void DX12MaterialManager::initMaterials(std::vector<DX12ResourceManager::DX12Entity*> dx12entities) {

	for (auto& [string, DX12Material] : dx12materials_map) {
		delete DX12Material;
	}

	dx12materials_map.clear();

	for (size_t i = 0; i < entityManager->entities.size(); i++) {

		EntityManager::Entity* entity = entityManager->entities[i];
		DX12ResourceManager::DX12Entity* dx12Entity = dx12entities[i];

		std::cout << "Material name: " << entity->material << std::endl;
		std::cout << "Entity model" << entity->model << std::endl;

		// create material if it doesn't already exist
		if (dx12materials_map.find(entity->material) == dx12materials_map.end()) {

			std::cout << "Material doesnt exist " << std::endl;

			std::cout << "Creating material " << std::endl;

			DX12Material* dx12Material = new DX12Material {}; // DX12 Texture material

			std::cout << "Fetching original material " << std::endl;

			MaterialManager::Material* material = materialManager->materials[entity->material]; // original material

			std::cout << "Creating albedo texture " << std::endl;

			// albedo texture
			// instancing for the 1x1 float values
			// convert vector3 to string and look up in map
			std::string texture_name = material->textureMap != "" ? material->textureMap : material->name + " abledo";
			std::cout << "Texture name " << texture_name <<std::endl;
			if (texture_indices_map.find(texture_name) == texture_indices_map.end()) { // not in map
				std::cout << "not in map " << std::endl;

				DX12ResourceManager::ResourceHandle* texture;

				// create from vector / float
				if (material->textureMap == "") {
					texture = createTextureFromVector(material->color);
					pushTexture(texture, sizeof(UINT), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
					texture_maps.push_back(texture);
					texture_indices_map[texture_name] = texture_maps.size() - 1;
					dx12Material->textureMap = texture_maps.size() - 1;
	
				}
				// load from file
				else {

					
				
				}

				
				

			} else {
			// in map
				dx12Material->textureMap = texture_indices_map[texture_name];
				std::cout << "in map " << std::endl;
			}

			std::cout << "Creating roughness texture " << std::endl;


			// roughness
			std::string roughness_name = material->roughnessMap != "" ? material->roughnessMap : material->name + " roughness";

			if (roughness_indices_map.find(roughness_name) == roughness_indices_map.end()) { // not in map
				DX12ResourceManager::ResourceHandle* texture = createTextureFromVector({ material->roughness });
				pushTexture(texture, sizeof(UINT), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
				roughness_maps.push_back(texture);
				roughness_indices_map[texture_name] = roughness_maps.size() - 1;
				dx12Material->roughnessMap = roughness_maps.size() - 1;

			}
			else {
				// in map
				dx12Material->roughnessMap = roughness_indices_map[texture_name];
			}



			// metallic
			std::string metallic_name = material->metallicMap != "" ? material->metallicMap : material->name + " metallic";

			if (metallic_indices_map.find(metallic_name) == metallic_indices_map.end()) { // not in map
				DX12ResourceManager::ResourceHandle* texture = createTextureFromVector({ material->metallic });
				pushTexture(texture, sizeof(UINT), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
				metallic_maps.push_back(texture);
				metallic_indices_map[texture_name] = metallic_maps.size() - 1;
				dx12Material->metallicMap = metallic_maps.size() - 1;

			}
			else {
				// in map
				dx12Material->metallicMap = metallic_indices_map[texture_name];
			}


			// emission
			std::string emission_name = material->emissionMap != "" ? material->emissionMap : material->name + " emission";

			if (emission_indices_map.find(emission_name) == emission_indices_map.end()) { // not in map
				DX12ResourceManager::ResourceHandle* texture = createTextureFromVector({ material->emission });
				pushTexture(texture, sizeof(UINT), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
				emission_maps.push_back(texture);
				emission_indices_map[texture_name] = emission_maps.size() - 1;
				dx12Material->emissionMap = emission_maps.size() - 1;
				
			}
			else {
				// in map
				dx12Material->emissionMap = emission_indices_map[texture_name];
			}



			// normalMap
			std::string normal_name = material->normalMap != "" ? material->normalMap : "normalmap_unused";

			if (normal_indices_map.find(normal_name) == normal_indices_map.end()) { // not in map - first time
				DX12ResourceManager::ResourceHandle* texture = createTextureFromVector({ 0.0f }); // triple 0.0f normal
				pushTexture(texture, sizeof(UINT), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
				normal_maps.push_back(texture);
				normal_indices_map[texture_name] = normal_maps.size() - 1;
				dx12Material->normalMap = normal_maps.size() - 1;

			}
			else {
				// in map
				dx12Material->normalMap = normal_indices_map[texture_name];
				// every "regular" material (not using an image map) should default to this. normals read from obj file or computed.
			}



			dx12materials_map[entity->material] = dx12Material;
		}

	}

	if (config.debug) std::cout << "Finished Materials" << std::endl;

}


DX12ResourceManager::ResourceHandle* DX12MaterialManager::createTextureFromVector(PT::Vector3 value) {

	std::cout << "createFromTexture()" << std::endl;


	uint8_t r = static_cast<uint8_t>(std::round(value.x * 255.0f));
	uint8_t g = static_cast<uint8_t>(std::round(value.y * 255.0f));
	uint8_t b = static_cast<uint8_t>(std::round(value.z * 255.0f));
	uint8_t a = 255;  // usually opaque for color textures

	uint32_t pixel = (a << 24) | (b << 16) | (g << 8) | r;

	std::cout << "New resource handle " << std::endl;
	DX12ResourceManager::ResourceHandle* texture = new DX12ResourceManager::ResourceHandle{};

	// upload buffer
	D3D12_RESOURCE_DESC descUpload = {};
	descUpload.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // upload buffer must be a dimension buffer
	descUpload.Width = sizeof(UINT);
	descUpload.Height = 1;
	descUpload.DepthOrArraySize = 1;
	descUpload.MipLevels = 1;
	descUpload.SampleDesc = NO_AA;
	descUpload.Format = DXGI_FORMAT_UNKNOWN;
	descUpload.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	descUpload.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = d3dDevice->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &descUpload, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&texture->upload_buffer));
	checkHR(hr, nullptr, "Create texture upload buffer");

	std::cout << "upload data" << std::endl;
	// upload data
	void* mapped;
	texture->upload_buffer->Map(0, nullptr, &mapped); // mapped now points to the upload buffer
	memcpy(mapped, &pixel, sizeof(UINT)); // copy data to upload buffer
	texture->upload_buffer->Unmap(0, nullptr); //
	
	std::cout << "target buffer" << std::endl;
	// Create target buffer in DEFAULT heap (VRAM)
	D3D12_RESOURCE_DESC descDefault = {};
	descDefault.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // default buffer can be TEXTURE2D
	descDefault.Width = 1;
	descDefault.Height = 1;
	descDefault.DepthOrArraySize = 1;
	descDefault.MipLevels = 1;
	descDefault.SampleDesc.Count = 1;
	descDefault.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descDefault.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // GPU choses swizzle layout
	descDefault.Flags = D3D12_RESOURCE_FLAG_NONE;


	hr = d3dDevice->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &descDefault, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture->default_buffer));
	checkHR(hr, nullptr, "Create texture target buffer");

	
	std::cout << "push " << std::endl;
	pushTexture(texture, sizeof(UINT), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON); // 1x1 texture now exists on GPU
	
	std::cout << "return texture " << std::endl;
	return texture;
}

void DX12MaterialManager::pushTexture(DX12ResourceManager::ResourceHandle* texture_handle, size_t data_size, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after) {

	// Barrier - transition target to COPY_DEST
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture_handle->default_buffer;
	barrier.Transition.StateBefore = state_before;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//cmdList->ResourceBarrier(1, &barrier); // already in copydest

	// Copy to GPU
	D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
	dstLoc.pResource = texture_handle->default_buffer;
	dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLoc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
	srcLoc.pResource = texture_handle->upload_buffer;
	srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	footprint.Offset = 0;
	footprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	footprint.Footprint.Width = 1;
	footprint.Footprint.Height = 1;
	footprint.Footprint.Depth = 1;
	footprint.Footprint.RowPitch = 4;               // 1 pixel × 4 bytes

	srcLoc.PlacedFootprint = footprint;

	cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

	// Barrier - transition to final state
	barrier.Transition.pResource = texture_handle->default_buffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = state_after;
	cmdList->ResourceBarrier(1, &barrier);
	
}

// helper

void DX12MaterialManager::checkHR(HRESULT hr, ID3DBlob* errorblob, std::string context) {
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

void DX12MaterialManager::waitForGPU() {

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