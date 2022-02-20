module;
#include <DirectXMath.h>

export module Camera;

export namespace Purr
{

	class Camera
	{

	public:
		enum class Type
		{
			PERSPECTIVE,
			ORTHOGRAPHIC
		};

	public:

		void setPosition(const DirectX::XMFLOAT4* newPosition)
		{
			this->position = DirectX::XMLoadFloat4(newPosition);
		}
		void setPosition(const DirectX::XMFLOAT4& newPosition)
		{
			this->position = DirectX::XMLoadFloat4(&newPosition);
		}
		[[nodiscard]] constexpr const auto& getPosition()
		{
			return this->position;
		}

		void setRotation(float w, float x, float y, float z)
		{
			this->rotation = DirectX::XMMatrixRotationAxis( DirectX::XMVectorSet(x, y, z, 0), w);
		}
		[[nodiscard]] constexpr const auto& getRotation()
		{
			return this->rotation;
		}

		constexpr void setFov(float newFov)
		{
			this->fov = newFov;
		}
		[[nodiscard]] constexpr auto getFov()
		{
			return this->fov;
		}

		constexpr void setAspectRatio(float ratio)
		{
			this->aspectRatio = ratio;
		}
		[[nodiscard]] constexpr auto getAspectRatio()
		{
			return this->aspectRatio;
		}

		constexpr void setType(Type newType)
		{
			this->type = newType;
		}
		[[nodiscard]] constexpr auto getType()
		{
			return this->type;
		}

		constexpr void setOrthographicScale(float newScale)
		{
			this->orthographicScale = newScale;
		}
		[[nodiscard]] constexpr auto getOrthographicScale()
		{
			return this->orthographicScale;
		}

		constexpr void setClipStart(float newClip)
		{
			this->clipStart = newClip;
		}
		[[nodiscard]] constexpr auto getClipStart()
		{
			return this->clipStart;
		}

		constexpr void setClipEnd(float newClip)
		{
			this->clipEnd = newClip;
		}
		[[nodiscard]] constexpr auto getClipEnd()
		{
			return this->clipEnd;
		}

	private:
		DirectX::XMVECTOR position{};
		DirectX::XMMATRIX rotation{};

		Type type{ Type::PERSPECTIVE };

		float orthographicScale{ 6.f };

		float clipStart{ 0.001f };
		float clipEnd{ 100.f };

		float fov{ 36.9f };

		float aspectRatio{ 1.f };

	};

}