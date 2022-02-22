module;

#define WIN32_LEAN_AND_MEAN
#include "DirectX-Headers/include/directx/d3dx12.h"
#include <d3d12.h>
#include <dxgi.h>
#include <DirectXColors.h>

#include <array>
#include <thread>
#include <span>
#include <latch>
#include <variant>

#include "imgui/imgui.h"

#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

export module RenderCore;
import D3d12Utils;
import Camera;
import UACEQueue;

import UACEMemPool;
import UACEUnifiedBlockAllocator;

import UACEMapStreamer;
import UACEMeshDataCache;
import UACEDirectoryHeader;
import UACEArray;

import UACEDeviationDecoder;
import UACEPkgBlobCoder;

using namespace UACE::MemManager::Literals;

export struct Vertex
{

	constexpr Vertex(const DirectX::XMFLOAT3& pos,
		const DirectX::XMFLOAT4& color, const DirectX::XMFLOAT2& texCoord)
		:position(pos), color(color), texCoord(texCoord)
	{}

	DirectX::XMFLOAT3 position{};
	DirectX::XMFLOAT4 color{};
	DirectX::XMFLOAT2 texCoord{};
};

static auto vertices{ std::to_array<Vertex>(
{
	Vertex(DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black), DirectX::XMFLOAT2(0.0f, 1.0f)),
	Vertex(DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White), DirectX::XMFLOAT2(0.0f, 0.0f)),
	Vertex(DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White), DirectX::XMFLOAT2(1.0f, 0.0f)),
	Vertex(DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black), DirectX::XMFLOAT2(1.0f, 1.0f)),
	Vertex(DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black), DirectX::XMFLOAT2(1.0f, 1.0f)),
	Vertex(DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::White), DirectX::XMFLOAT2(0.0f, 1.0f)),
	Vertex(DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::White), DirectX::XMFLOAT2(0.0f, 0.0f)),
	Vertex(DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black), DirectX::XMFLOAT2(1.0f, 0.0f))
}) };

using index_t = uint16_t;
static auto indices{ std::to_array<index_t>({
	// front face
	0, 1, 2,
	0, 2, 3,
	// back face
	4, 6, 5,
	4, 7, 6,
	// left face
	4, 5, 1,
	4, 1, 0,
	// right face
	3, 2, 6,
	3, 6, 7,
	// top face
	1, 5, 6,
	1, 6, 2,
	// bottom face
	4, 0, 3,
	4, 3, 7
}) };

struct PerObjectConstants
{

	DirectX::XMFLOAT4X4 worldViewProj{};

};

export namespace Purr
{

	template <typename T>
	using cptr_t = Microsoft::WRL::ComPtr<T>;

	constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT{ DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT };

	constexpr DXGI_FORMAT RENDER_TARGET_FORMAT{ DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM };
	constexpr UINT SWAP_CHAN_BUFFERS_COUNT{ 2 };

	namespace umem = UACE::MemManager;

	class Renderer
	{

	private:
		auto getCurrentBackBufferView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(this->rtvHeap->GetCPUDescriptorHandleForHeapStart(), this->currRtvId, this->rtvDescriptorSize);
		}

		auto getCurrentBackBuffer() const
		{
			return this->aSwapChainBuffers[this->currRtvId].Get();
		}

		auto getDepthStencilView() const
		{
			return this->dsvHeap->GetCPUDescriptorHandleForHeapStart();
		}

	public:

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		Renderer() = default;
		~Renderer() = default;

		bool init(HWND window, UINT width, UINT height)
		{
			if (!this->initD3d12(window, width, height))
				return false;

			this->camera.setPosition({ 5.f, 5.f, 5.f, 1.f });

			const auto thFuc = [this](std::stop_token stoken)
			{
				this->streamerThreadLogic(stoken);
			};
			this->thStreamer = std::jthread(thFuc);

			return true;
		}

		void streamerThreadLogic(std::stop_token stoken)
		{

			using ump = umem::Pool;

			umem::Pool pool(10_MB);
			umem::Domain* domain{ pool.createDomain(10_MB) };

			namespace upa = umem::UnifiedBlockAllocator;
			upa::UnifiedBlockAllocator ubAlloc{ upa::createAllocator(domain, 9_MB) };

			//const auto cmdExeResult{ system("py test_objects/server.py") };

			static constexpr size_t MESH_CACHE_SIZE{ 5 };
			UACE::Containers::Array<
				UACE::Map::StaticMeshCache<upa::UnifiedBlockAllocator>, 
				MESH_CACHE_SIZE,
				upa::UnifiedBlockAllocator
			> aMeshCache(&ubAlloc);

			for (size_t id{ 0 }; id < MESH_CACHE_SIZE; ++id)
			{
//				aMeshCache.append()
			}

			UACE::Map::DirectoryHeader(&ubAlloc);

			std::latch sync(2);

			//const auto console{ std::jthread([&sync]()
			//	{
			//		AllocConsole();
			//		sync.arrive_and_wait();
			//		const auto cmdExeResult{ system("script.bat") };
			//		if (cmdExeResult != 0)
			//		{
			//			MessageBox(nullptr, "Failed to run cmd command", "Error", MB_OK | MB_ICONERROR);
			//			return;
			//		}
			//	}) };
			//sync.arrive_and_wait();

			UACE::Map::Streamer<upa::UnifiedBlockAllocator> streamer(&ubAlloc);
			this->pStreamer = &streamer;

			const auto cbOnCameraCreation = [](size_t objId, std::span<const char> metadata)
			{
				objId = objId;
			};

			const auto cbOnStaticMeshCreation = [](size_t objId, std::span<const char> metadata)
			{
				objId = objId;
			};

			const auto cbOnRename = [](size_t objId, size_t, size_t, size_t, bool)
			{
				objId = 0;
			};

			const auto onDeletion = [](size_t objId)
			{
				objId = 0;
			};

			const auto cbOnTransform = [](size_t objId, std::span<const char>)
			{
				objId = 0;
			};

			const auto cbOnMesh = [](size_t objId, std::span<const char>)
			{
				objId = 0; 
			};

			const auto onCamera = [this](size_t objId, std::span<const char> rawCameraData)
			{
				const auto oCamData{ UACE::PkgBlobCoder::decodeCamera(rawCameraData) };
				assert(oCamData.has_value());
				UACE::Map::StreamerPkg::CameraData camData;
				camData.objId = objId;
				camData.camera = oCamData.value();
				const auto bSended{ this->pStreamer->sendCameraData(camData) };
				assert(bSended);
			};

			UACE::Deviation::Desc desc{ 
				cbOnCameraCreation, 
				cbOnStaticMeshCreation, 
				onDeletion, 
				cbOnRename, 
				cbOnTransform, 
				cbOnMesh, 
				onCamera };
			UACE::DeviationDecoder devDecoder(&ubAlloc, desc, "loggerDatabase.sqlite", "127.0.0.1", 50007, 5_MB);
			
			while (!stoken.stop_requested())
			{
				devDecoder.tick();
			}

		}

		void update()
		{



		}

		void draw(UINT width, UINT height)
		{

			this->commandAlloc->Reset();
			this->cmdList->Reset(this->commandAlloc.Get(), this->pso.Get());

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			static float f = 0.0f;
			static int counter = 0;

			static bool bShowCamControls{ false };

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Tools"))
				{
					ImGui::MenuItem("Show camera controls", NULL, &bShowCamControls);
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			static float camPos[]{5.f, -4.61f, 1.92f};
			static float camRot[]{ 75.3f, 0.575f, 0.706f, 0.412f };
			static float camDir[3];
			static float camUp[3];
			static const char* types[]{ "Perspective", "Orthographic"};
			static const char* currentType{ types[0] };
			static float orthographicScale{ 6.8f };

			static float clipStart{ 0.001f };
			static float clipEnd{ 100.f };

			static float aspectRatio{ static_cast<float>(width) / height };
			
			static float fov{ 39.6f };

			if (bShowCamControls)
			{

				ImGui::Begin("Camera controls");
				ImGui::SliderFloat3("Position", camPos, -5.f, 5.f);
				this->camera.setPosition(DirectX::XMFLOAT4(-camPos[0], camPos[2], -camPos[1], 1.f));
				//ImGui::SliderFloat3("Direction", camDir, -1.f, 1.f);
				//ImGui::SliderFloat3("Up vector", camUp, -1.f, 1.f);
				ImGui::SliderFloat("Rotation Angle", camRot, -180.f, 180.f);
				ImGui::SliderFloat3("Rotation Axis", camRot + 1, -1.f, 1.f);
				this->camera.setRotation(DirectX::XMConvertToRadians(camRot[0]), camRot[1], -camRot[3], camRot[2]);

				if (ImGui::BeginCombo("Camera type", currentType))
				{
					for (int n = 0; n < IM_ARRAYSIZE(types); n++)
					{
						bool bSelected = (currentType == types[n]);
						if (ImGui::Selectable(types[n], bSelected))
						{
							currentType = types[n];
						}
						if (bSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}


				if (currentType == types[0])
				{
					ImGui::SliderFloat("Aspect ratio", &aspectRatio, 0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
					this->camera.setAspectRatio(aspectRatio);
					ImGui::SliderFloat("Fov", &fov, 0.367f, 173.f);
					this->camera.setFov(DirectX::XMConvertToRadians(fov));
					this->camera.setType(Purr::Camera::Type::PERSPECTIVE);
				}
				else if (currentType == types[1])
				{
					ImGui::SliderFloat("Orthographic scale", &orthographicScale, 0.01f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);
					this->camera.setOrthographicScale(orthographicScale);
					this->camera.setType(Purr::Camera::Type::ORTHOGRAPHIC);
				}

				ImGui::SliderFloat("Clip start", &clipStart, 0.001f, clipEnd - 0.01f, "%.3f", ImGuiSliderFlags_Logarithmic);
				this->camera.setClipStart(clipStart);
				ImGui::SliderFloat("Clip end", &clipEnd, clipStart + 0.01f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);
				this->camera.setClipEnd(clipEnd);

				ImGui::End();

			}
			else
			{
				const auto vPkg{ this->pStreamer->getStreamPkg() };
				if (std::holds_alternative<UACE::Map::StreamerPkg::CameraData>(vPkg))
				{
					const auto camData{ std::get<UACE::Map::StreamerPkg::CameraData>(vPkg).camera };
					this->camera.setPosition(DirectX::XMFLOAT4(-camData.posX, camData.posZ, -camData.posY, 1.f));
					this->camera.setRotation(camData.rotAngle, camData.rotX, -camData.rotZ, camData.rotY);
					this->camera.setAspectRatio(camData.aspectRatio);
					this->camera.setFov(camData.fov);
					this->camera.setType(static_cast<Purr::Camera::Type>(camData.type));
					this->camera.setClipStart(camData.clipStart);
					this->camera.setClipEnd(camData.clipEnd);
					this->camera.setOrthographicScale(camData.orthographicScale);
				}
			}

			ImGui::Render();

			this->cmdList->RSSetViewports(1, &this->vp);
			this->cmdList->RSSetScissorRects(1, &this->scissor);

			CD3DX12_RESOURCE_BARRIER toRtBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(this->getCurrentBackBuffer(),
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET) };
			this->cmdList->ResourceBarrier(1, &toRtBarrier);

			this->cmdList->ClearRenderTargetView(this->getCurrentBackBufferView(), DirectX::Colors::Blue, 0, nullptr);
			this->cmdList->ClearDepthStencilView(this->getDepthStencilView(), D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL,
				1.f, 0, 0, nullptr);

			auto backBufferView{ this->getCurrentBackBufferView() };
			auto depthStencilView{ this->getDepthStencilView() };
			this->cmdList->OMSetRenderTargets(1, &backBufferView, true, &depthStencilView);

			this->drawCube(width, height);

			auto heap{ this->srvHeap.Get() };
			this->cmdList->SetDescriptorHeaps(1, &heap);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), this->cmdList.Get());

			CD3DX12_RESOURCE_BARRIER toPresentBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(this->getCurrentBackBuffer(),
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT) };
			this->cmdList->ResourceBarrier(1, &toPresentBarrier);

			this->cmdList->Close();

			ID3D12CommandList* listArray[]{ this->cmdList.Get() };
			this->qCmd->ExecuteCommandLists(_countof(listArray), listArray);

			this->swapChain->Present(0, 0);

			this->currRtvId = (this->currRtvId + 1) % SWAP_CHAN_BUFFERS_COUNT;

			this->flushCommandQueue();
			
		}

		bool initGUI(HWND hwin)
		{
			if (ImGui::CreateContext() == nullptr)
				return false;

			ImGuiIO& io = ImGui::GetIO(); (void)io;

			ImGui::StyleColorsDark();

			if (!ImGui_ImplWin32_Init(hwin))
				return false;
			if (!ImGui_ImplDX12_Init(this->device.Get(), 1, RENDER_TARGET_FORMAT, this->srvHeap.Get(),
				this->srvHeap->GetCPUDescriptorHandleForHeapStart(), this->srvHeap->GetGPUDescriptorHandleForHeapStart()))
				return false;

			return true;
		}

	private:

		bool initD3d12(HWND window, UINT width, UINT height)
		{

			const auto bDeviceCreated{ this->initDevice() };
			if (!bDeviceCreated)
			{
				return false;
			}

			const auto bFenceAndDescriptorsDataCreated{ this->initFenceAndDescriptorsData() };
			if (!bFenceAndDescriptorsDataCreated)
			{
				return false;
			}

			const auto bMultilevelSupport{ this->initMSAA() };
			if (!bMultilevelSupport)
			{
				return false;
			}

			const auto bCmdQueueAndList{ this->initCommandQueueAndList() };
			if (!bCmdQueueAndList)
			{
				return false;
			}

			const auto bSwapChainCreated{ this->initSwapChain(window, width, height) };
			if (!bSwapChainCreated)
			{
				return false;
			}

			const auto bRtvAndDsvDescHeaps{ this->initRtvAndDsvDescriptorHeaps() };
			if (!bRtvAndDsvDescHeaps)
			{
				return false;
			}

			const auto bCbHeap{ this->initCbDescriptorHeap() };
			if (!bCbHeap)
			{
				return false;
			}

			const auto bRtvCreated{ this->initRenderTargetView() };
			if (!bRtvCreated)
			{
				return false;
			}

			const auto bDsvCreated{ this->initDepthStencilBufferAndView(width, height) };
			if (!bDsvCreated)
			{
				return false;
			}

			this->initViewPortAndScissorRec(width, height);

			D3D12_DESCRIPTOR_HEAP_DESC hDesc{};
			hDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			hDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			hDesc.NumDescriptors = 1;
			hDesc.NodeMask = 0;

			const auto hResult{ device->CreateDescriptorHeap(&hDesc, IID_PPV_ARGS(&this->srvHeap)) };
			if (FAILED(hResult))
			{
				return false;
			}

			this->vBuffer = D3D12Utils::createDefaultBuffer(this->device.Get(), this->cmdList.Get(), 
				vertices.data(), vertices.size() * sizeof(Vertex), &this->vbUploader);
			if (this->vBuffer == nullptr)
			{
				return false;
			}

			this->vbv.BufferLocation = this->vBuffer->GetGPUVirtualAddress();
			this->vbv.SizeInBytes = vertices.size() * sizeof(Vertex);
			this->vbv.StrideInBytes = sizeof(Vertex);

			this->iBuffer = D3D12Utils::createDefaultBuffer(this->device.Get(), this->cmdList.Get(), 
				indices.data(), indices.size() * sizeof(index_t), &this->ibUploader);
			if (this->iBuffer == nullptr)
			{
				return false;
			}

			this->ibv.BufferLocation = this->iBuffer->GetGPUVirtualAddress();
			this->ibv.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
			this->ibv.SizeInBytes = indices.size() * sizeof(index_t);

			this->cBuffer.init(this->device.Get());
			this->createRootSignature();

			this->vShader = D3D12Utils::loadShader("../Debug/VertexShader.cso");
			this->pShader = D3D12Utils::loadShader("../Debug/PixelShader.cso");

			if (!this->createGPO())
			{
				return false;
			}

			{
				this->cmdList->Close();
				const auto cmdLists{ std::to_array({reinterpret_cast<ID3D12CommandList*>(this->cmdList.Get())}) };
				this->qCmd->ExecuteCommandLists(1, cmdLists.data());
				this->flushCommandQueue();
			}

			this->commandAlloc->Reset();
			this->cmdList->Reset(this->commandAlloc.Get(), this->pso.Get());

			{
				this->cmdList->Close();
				const auto cmdLists{ std::to_array({reinterpret_cast<ID3D12CommandList*>(this->cmdList.Get())}) };
				this->qCmd->ExecuteCommandLists(1, cmdLists.data());
				this->flushCommandQueue();
			}

			return true;

		}

		bool initDevice()
		{

			const auto hFactoryCreation{ CreateDXGIFactory1(IID_PPV_ARGS(&this->factory)) };
			if (FAILED(hFactoryCreation))
			{
				return false;
			}

			const auto hDebugCreation{ D3D12GetDebugInterface(IID_PPV_ARGS(&this->debugController)) };
			if (FAILED(hDebugCreation))
			{
				return false;
			}
			this->debugController->EnableDebugLayer();

			const auto result{ D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&this->device)) };
			if (FAILED(result))
			{
				return false;
			}

			return true;

		}

		bool initFenceAndDescriptorsData()
		{

			const auto hFenceCreation{ this->device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->fence)) };
			if (FAILED(hFenceCreation))
			{
				return false;
			}

			this->rtvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			this->srvDescriptorSize = this->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			this->cbvDescriptorSize = this->srvDescriptorSize;
			this->uavDescriptorSize = this->srvDescriptorSize;

			return true;

		}

		bool initMSAA()
		{


			D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisampleLevel{};
			multisampleLevel.Format = RENDER_TARGET_FORMAT;
			multisampleLevel.NumQualityLevels = 0;
			multisampleLevel.SampleCount = 4;
			multisampleLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS::D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
			const auto hSupport{ this->device->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multisampleLevel, sizeof(multisampleLevel)) };
			if (FAILED(hSupport))
			{
				return false;
			}

			this->numQualityLevelSupport = multisampleLevel.NumQualityLevels;

			return true;

		}

		bool initCommandQueueAndList()
		{

			D3D12_COMMAND_QUEUE_DESC qCmdDesc{};
			qCmdDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
			qCmdDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

			const auto hQueue{ this->device->CreateCommandQueue(&qCmdDesc, IID_PPV_ARGS(&this->qCmd)) };
			if (FAILED(hQueue))
			{
				return false;
			}

			const auto hCmdAlloc{ this->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->commandAlloc)) };
			if (FAILED(hCmdAlloc))
			{
				return false;
			}

			const auto hCmdList{ this->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, 
				this->commandAlloc.Get(), nullptr,  IID_PPV_ARGS(&this->cmdList)) };
			if (FAILED(hCmdList))
			{
				return false;
			}

			this->cmdList->Close();
			this->cmdList->Reset(this->commandAlloc.Get(), nullptr);

			return true;

		}

		bool initSwapChain(HWND window, UINT width, UINT height)
		{

			DXGI_SWAP_CHAIN_DESC desc{};

			desc.BufferDesc.Width = width;
			desc.BufferDesc.Height = height;
			desc.BufferDesc.Format = RENDER_TARGET_FORMAT;
			desc.BufferDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
			desc.BufferDesc.RefreshRate.Numerator = 60;
			desc.BufferDesc.RefreshRate.Denominator = 1;
			desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Windowed = true;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.BufferCount = SWAP_CHAN_BUFFERS_COUNT;
			desc.OutputWindow = window;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.Flags = DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

			const auto hResult{ this->factory->CreateSwapChain(this->qCmd.Get(), &desc, &this->swapChain)};
			if (FAILED(hResult))
			{
				return false;
			}

			return true;

		}

		bool initRtvAndDsvDescriptorHeaps()
		{

			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
			rtvHeapDesc.NumDescriptors = SWAP_CHAN_BUFFERS_COUNT;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtvHeapDesc.NodeMask = 0;

			const auto hRtvResult{ this->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&this->rtvHeap)) };
			if (FAILED(hRtvResult))
			{
				return false;
			}

			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			dsvHeapDesc.NodeMask = 0;

			const auto hDsvResult{ this->device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&this->dsvHeap)) };
			if (FAILED(hDsvResult))
			{
				return false;
			}

			return true;

		}

		bool initCbDescriptorHeap()
		{

			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.NumDescriptors = 1;
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			desc.NodeMask = 0;

			const auto hResult{ this->device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&this->cbHeap)) };
			if (FAILED(hResult))
			{
				return false;
			}

			return true;

		}

		bool initRenderTargetView()
		{

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescHandle(this->rtvHeap->GetCPUDescriptorHandleForHeapStart());

			for (UINT bufferId{ 0 }; auto & swapChainResource : this->aSwapChainBuffers)
			{

				const auto hGetBuffer{ this->swapChain->GetBuffer(bufferId, IID_PPV_ARGS(&swapChainResource)) };
				if (FAILED(hGetBuffer))
				{
					break;
				}

				this->device->CreateRenderTargetView(swapChainResource.Get(), nullptr, rtvDescHandle);
				rtvDescHandle.Offset(1, this->rtvDescriptorSize);
				bufferId++;

			}

			return true;

		}

		bool initDepthStencilBufferAndView(UINT width, UINT height)
		{

			D3D12_RESOURCE_DESC resDesc{};
			resDesc.Format = DEPTH_STENCIL_FORMAT;
			resDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resDesc.Alignment = 0;
			resDesc.Width = width;
			resDesc.Height = height;
			resDesc.DepthOrArraySize = 1;
			resDesc.MipLevels = 1;
			resDesc.SampleDesc.Count = 1;
			resDesc.SampleDesc.Quality = 0;
			resDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
			resDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_CLEAR_VALUE clearValue{};
			clearValue.Format = DEPTH_STENCIL_FORMAT;
			clearValue.DepthStencil.Depth = 1.f;
			clearValue.DepthStencil.Stencil = 0;

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);
			const auto hBuffer{ this->device->CreateCommittedResource(&heapProps,
				D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, &clearValue,
				IID_PPV_ARGS(&this->dsBuffer)) };
			if (FAILED(hBuffer))
			{
				return false;
			}

			this->device->CreateDepthStencilView(this->dsBuffer.Get(), nullptr, this->getDepthStencilView());
			D3D12_RESOURCE_BARRIER barrier{ CD3DX12_RESOURCE_BARRIER::Transition(this->dsBuffer.Get(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE) };

			this->cmdList->ResourceBarrier(1, &barrier);

			return true;

		}

		void initViewPortAndScissorRec(UINT width, UINT height)
		{

			this->vp.Width = static_cast<FLOAT>(width);
			this->vp.Height = static_cast<FLOAT>(height);
			this->vp.MinDepth = 0.f;
			this->vp.MaxDepth = 1.f;
			this->vp.TopLeftX = 0.f;
			this->vp.TopLeftY = 0.f;

			this->scissor.left = 0;
			this->scissor.right = static_cast<LONG>(width);
			this->scissor.top = 0;
			this->scissor.bottom = static_cast<LONG>(height);

		}

		void createRootSignature()
		{

			std::array<CD3DX12_ROOT_PARAMETER, 1> aRootParameters;

			aRootParameters[0].InitAsConstantBufferView(0);

			CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
			rsDesc.Init(aRootParameters.size(), aRootParameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ID3DBlob* serializedRootSig{ nullptr };
			ID3DBlob* errorBlob{ nullptr };
			const auto hSerialize{ D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1,
				&serializedRootSig, &errorBlob) };
			if (FAILED(hSerialize))
			{
				return;
			}

			const auto hRootSignature{ device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
				serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&this->rootSignature)) };
			if (FAILED(hRootSignature))
			{
				return;
			}

		}

		bool createGPO()
		{

			D3D12_INPUT_ELEMENT_DESC iPosDesc{};
			iPosDesc.AlignedByteOffset = 0;
			iPosDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
			iPosDesc.InputSlot = 0;
			iPosDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			iPosDesc.InstanceDataStepRate = 0;
			iPosDesc.SemanticIndex = 0;
			iPosDesc.SemanticName = "POSITION";

			D3D12_INPUT_ELEMENT_DESC iColorDesc{};
			iColorDesc.AlignedByteOffset = 12;
			iColorDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			iColorDesc.InputSlot = 0;
			iColorDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			iColorDesc.InstanceDataStepRate = 0;
			iColorDesc.SemanticIndex = 0;
			iColorDesc.SemanticName = "COLOR";

			const auto aIDescs{ std::to_array({iPosDesc, iColorDesc}) };

			D3D12_INPUT_LAYOUT_DESC ilDesc{};
			ilDesc.pInputElementDescs = aIDescs.data();
			ilDesc.NumElements = aIDescs.size();

			D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc{};
			gpsDesc.InputLayout = ilDesc;
			gpsDesc.pRootSignature = this->rootSignature.Get();
			gpsDesc.VS = { reinterpret_cast<BYTE*>(this->vShader->GetBufferPointer()), this->vShader->GetBufferSize() };
			gpsDesc.PS = { reinterpret_cast<BYTE*>(this->pShader->GetBufferPointer()), this->pShader->GetBufferSize() };
			gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			gpsDesc.SampleMask = UINT_MAX;
			gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			gpsDesc.NumRenderTargets = 1;
			gpsDesc.RTVFormats[0] = RENDER_TARGET_FORMAT;
			gpsDesc.SampleDesc.Count = 1;
			gpsDesc.SampleDesc.Quality = 0;
			gpsDesc.DSVFormat = DEPTH_STENCIL_FORMAT;

			const auto hPso{ device->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&this->pso)) };
			if (FAILED(hPso))
			{
				return false;
			}

			return true;

		}

		void flushCommandQueue()
		{

			this->currFenceId++;

			this->qCmd->Signal(this->fence.Get(), this->currFenceId);
			if (this->fence->GetCompletedValue() < this->currFenceId)
			{

				auto eventHandler{ CreateEventEx(nullptr, "", 0, EVENT_ALL_ACCESS) };
				this->fence->SetEventOnCompletion(this->currFenceId, eventHandler);
				WaitForSingleObject(eventHandler, INFINITE);
				CloseHandle(eventHandler);

			}

		}

		void drawCube(UINT width, UINT height)
		{

			this->cmdList->IASetVertexBuffers(0, 1, &this->vbv);
			this->cmdList->IASetIndexBuffer(&this->ibv);
			this->cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			const auto pos{ this->camera.getPosition() };

			//x = cos(yaw) * cos(pitch)
			//y = sin(yaw) * cos(pitch)
			//z = sin(pitch)
			const auto rot{ this->camera.getRotation() };
			//const auto xRot{ DirectX::XMConvertToRadians(rot.x - 90.f) };
			//const auto yRot{ -DirectX::XMConvertToRadians(rot.z) };
			//const auto zRot{ DirectX::XMConvertToRadians(rot.y) };

			//const auto q{DirectX::XMQuaternionRotationAxis()}

			//const auto rotVec{ DirectX::XMVectorSet(xRot, yRot, zRot, 0) };
			const auto rotMat{ rot };

			const auto direction{ DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.f, -1.f, 0.f, 0.f), rotMat) };
			const auto up{ DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(0.f, 0.f, -1.f, 0.f), rotMat) };

			const auto view{ DirectX::XMMatrixLookToLH(pos, direction, up) };
			const auto world{ DirectX::XMMatrixIdentity() };

			const auto proj = std::invoke([this, width, height]()
			{
				const auto cameraType{ this->camera.getType() };
				if (cameraType == Purr::Camera::Type::PERSPECTIVE)
				{
					return DirectX::XMMatrixPerspectiveFovLH(
						this->camera.getFov(),
						this->camera.getAspectRatio(), 
						this->camera.getClipStart(), 
						this->camera.getClipEnd());
				}
				else
				{

					return DirectX::XMMatrixOrthographicLH(
						1.f * this->camera.getOrthographicScale(), 
						static_cast<float>(height) / width * this->camera.getOrthographicScale(),
						this->camera.getClipStart(),
						this->camera.getClipEnd());
				}
			});

			const auto wvpMat{ DirectX::XMMatrixTranspose(world * view * proj) };

			PerObjectConstants cbStr{};
			DirectX::XMStoreFloat4x4(&cbStr.worldViewProj, wvpMat);

			this->cBuffer.copy(&cbStr);

			this->cmdList->SetGraphicsRootSignature(this->rootSignature.Get());

			this->cmdList->SetGraphicsRootConstantBufferView(0, this->cBuffer->GetGPUVirtualAddress());

			this->cmdList->DrawIndexedInstanced(indices.size(), 1, 0, 0, 0);

		}

	private:
		UINT rtvDescriptorSize{};
		UINT srvDescriptorSize{};
		UINT cbvDescriptorSize{};
		UINT uavDescriptorSize{};

		cptr_t<IDXGIFactory1> factory{ nullptr };

		cptr_t<ID3D12Debug> debugController{ nullptr };

		cptr_t<ID3D12Device> device{ nullptr };

		cptr_t<ID3D12Fence> fence{ nullptr };

		cptr_t<ID3D12CommandQueue> qCmd{ nullptr };
		cptr_t<ID3D12CommandAllocator> commandAlloc{ nullptr };
		cptr_t<ID3D12GraphicsCommandList> cmdList{ nullptr };

		cptr_t<IDXGISwapChain> swapChain{ nullptr };

		cptr_t<ID3D12DescriptorHeap> rtvHeap{ nullptr };
		cptr_t<ID3D12DescriptorHeap> dsvHeap{ nullptr };

		cptr_t<ID3D12DescriptorHeap> cbHeap{ nullptr };

		cptr_t<ID3D12Resource> dsBuffer{ nullptr };

		std::array<cptr_t<ID3D12Resource>, SWAP_CHAN_BUFFERS_COUNT> aSwapChainBuffers{};

		cptr_t<ID3D12DescriptorHeap> srvHeap{ nullptr };

		cptr_t<ID3D12Resource> vBuffer{ nullptr };
		cptr_t<ID3D12Resource> iBuffer{ nullptr };

		cptr_t<ID3D12Resource> vbUploader{ nullptr };
		cptr_t<ID3D12Resource> ibUploader{ nullptr };

		cptr_t<ID3D12RootSignature> rootSignature{ nullptr };

		D3D12_VERTEX_BUFFER_VIEW vbv;
		D3D12_INDEX_BUFFER_VIEW ibv;

		D3D12Utils::CBufferWrapper<PerObjectConstants> cBuffer;

		cptr_t<ID3DBlob> vShader{ nullptr };
		cptr_t<ID3DBlob> pShader{ nullptr };

		cptr_t<ID3D12PipelineState> pso{ nullptr };

		UINT numQualityLevelSupport{ 0 };
		int currRtvId{ 0 };

		D3D12_VIEWPORT vp{};
		RECT scissor{};

		UINT64 currFenceId{ 0 };

		Purr::Camera camera{};
		std::jthread thStreamer;

		UACE::Map::Streamer<umem::UnifiedBlockAllocator::UnifiedBlockAllocator>* pStreamer;

	};

};