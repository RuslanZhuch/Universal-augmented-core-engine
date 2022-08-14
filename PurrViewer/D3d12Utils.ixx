module;

#include "DirectX-Headers/include/directx/d3dx12.h"
#include <d3d12.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include <string_view>
#include <fstream>

export module D3d12Utils;

export namespace D3D12Utils
{

	ID3D12Resource* createDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		const void* initData, long byteSize, ID3D12Resource** uploaderResource)
	{

		if (*uploaderResource != nullptr)
		{
			(*uploaderResource)->Release();
			(*uploaderResource) = nullptr;
		}

		ID3D12Resource* defaultBuffer{ nullptr };

		D3D12_RESOURCE_DESC resourceDesc{ CD3DX12_RESOURCE_DESC::Buffer(byteSize) };

		D3D12_HEAP_PROPERTIES defaultHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT) };
		const auto hDefBuffer{ device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_PPV_ARGS(&defaultBuffer)) };
		if (FAILED(hDefBuffer))
		{
			return nullptr;
		}

		D3D12_HEAP_PROPERTIES updloadHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD) };
		const auto hUploadBuffer{ device->CreateCommittedResource(&updloadHeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(uploaderResource)) };
		if (FAILED(hUploadBuffer))
		{
			return nullptr;
		}

		if (initData == nullptr)
			return defaultBuffer;

		D3D12_SUBRESOURCE_DATA subData{};
		subData.pData = initData;
		subData.RowPitch = byteSize;
		subData.SlicePitch = subData.RowPitch;

		D3D12_RESOURCE_BARRIER copyDestBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST) };
		cmdList->ResourceBarrier(1, &copyDestBarrier);

		UpdateSubresources<1>(cmdList, defaultBuffer, *uploaderResource, 0, 0, 1, &subData);

		D3D12_RESOURCE_BARRIER commonBarrier{ CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON) };
		cmdList->ResourceBarrier(1, &commonBarrier);

		return defaultBuffer;
	}

	ID3DBlob* loadShader(std::string_view filename)
	{

		std::ifstream f(filename.data(), std::ios::binary);
		f.seekg(0, std::ios::end);
		const auto fSize{ static_cast<size_t>(f.tellg()) };
		f.seekg(0, std::ios::beg);
		ID3DBlob* shaderData{ nullptr };
		const auto hResult{ D3DCreateBlob(fSize, &shaderData) };
		if (FAILED(hResult))
		{
			return nullptr;
		}

		f.read(static_cast<char*>(shaderData->GetBufferPointer()), fSize);
		f.close();

		return shaderData;

	}

	constexpr UINT calculateCBufferSize(UINT size)
	{
		return (size + 255) & ~255;
	}

	constexpr UINT align(UINT uLocation, UINT uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			return 0;
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}

	template<typename T>
	class CBufferWrapper
	{

	public:
		CBufferWrapper() = default;

		~CBufferWrapper()
		{
			if (this->buffer != nullptr)
			{
				this->buffer->Unmap(0, nullptr);
				this->buffer->Release();
			}
		}

		CBufferWrapper(const CBufferWrapper<T>&) = delete;
		CBufferWrapper(CBufferWrapper<T>&&) = delete;
		CBufferWrapper<T>& operator=(const CBufferWrapper<T>&) = delete;
		CBufferWrapper<T>& operator=(CBufferWrapper<T>&&) = delete;

		ID3D12Resource* operator->()
		{
			return this->buffer;
		}

		constexpr auto getSize() const { return this->bufferSize; }

		bool init(ID3D12Device* device)
		{

			this->bufferSize = calculateCBufferSize(sizeof(T));

			D3D12_HEAP_PROPERTIES heapUpload{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD) };
			D3D12_RESOURCE_DESC uploadDesc{ CD3DX12_RESOURCE_DESC::Buffer(this->bufferSize * 1) };
			device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &uploadDesc,
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&this->buffer));

			this->buffer->Map(0, nullptr, reinterpret_cast<void**>(&this->data));

			return true;

		}

		void copy(T* dataToCopy)
		{
			if (this->data == nullptr)
				return;
			memcpy(this->data, dataToCopy, this->bufferSize);
		}

		ID3D12Resource* get()
		{
			return this->buffer;
		}

	private:

	private:
		UINT bufferSize{ 0 };
		ID3D12Resource* buffer{ nullptr };

		T* data{ nullptr };

	};

};