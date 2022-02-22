module;
#include <variant>
#include <optional>
#include <array>
#include <string_view>

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/writer.h"

#include <cassert>

export module UACEJsonCoder;

export namespace UACE::JsonCoder
{

	template<size_t len>
	using lstr_t = std::array<char, len>;

	enum class PKG_TYPE
	{
		NONE = 0,
		DEVIATION = 1,
		LAST_DEVIATION_ID = 2
	};

	enum class ObjectType
	{
		NONE,
		CAMERA,
		MESH
	};

	struct PkgNone
	{

	};

	struct PkgDeviation
	{
		uint32_t deviationId{};
		lstr_t<32> objectName{};
		lstr_t<32> deviationType{};
		ObjectType objectType{};
	};

	struct PkgLastDeviationId
	{
		uint32_t lastDeviationId{};
	};

	using PkgType_t = std::variant<PkgNone, PkgDeviation, PkgLastDeviationId>;

}

namespace UACE::JsonCoder
{

	template <typename T>
	[[nodiscard]] std::optional<T> parseAs(const rapidjson::Document& doc, std::string_view memberName)
	{

		if (!doc.HasMember(memberName.data()))
		{
			return std::nullopt;
		}

		const auto mPkgType{ doc.FindMember(memberName.data()) };

		if constexpr (std::is_same_v<T, std::string_view>)
		{
			if (!mPkgType->value.IsString())
			{
				return std::nullopt;
			}

			return mPkgType->value.GetString();
		}
		else if constexpr (std::is_same_v<T, uint32_t>)
		{
			if (!mPkgType->value.IsUint())
			{
				return std::nullopt;
			}

			return mPkgType->value.GetUint();
		}
		else
		{
			assert(false);
			//static_assert(false, "Cannot parse JSON of current type");
		}

	}

	template <size_t _len>
	bool filllstr(lstr_t<_len>& dest, const std::string_view src)
	{
		if (dest.size() < src.size())
			return false;
		std::memcpy(dest.data(), src.data(), src.size());
		return true;
	}

};

export namespace UACE::JsonCoder
{

	[[nodiscard]] PkgType_t decode(std::string_view str)
	{

		rapidjson::Document doc;
		doc.Parse(str.data(), str.size());
		if (!doc.IsObject())
		{
			return PkgNone();
		}

		const auto oPkgType{ parseAs<std::string_view>(doc, "PkgType") };
		if (oPkgType == std::nullopt)
		{
			return PkgNone();
		}

		if (strcmp(oPkgType->data(), "Deviation") == 0)
		{

			const auto oObjName{ parseAs<std::string_view>(doc, "ObjectName") };
			if (oObjName == std::nullopt)
			{
				return PkgNone();
			}
			const auto oObjType{ parseAs<std::string_view>(doc, "ObjectType") };
			if (oObjType == std::nullopt)
			{
				return PkgNone();
			}

			const auto oDevType{ parseAs<std::string_view>(doc, "DeviationType") };
			if (oDevType == std::nullopt)
			{
				return PkgNone();
			}
			const auto oDevId{ parseAs<uint32_t>(doc, "DeviationId") };
			if (oDevId == std::nullopt)
			{
				return PkgNone();
			}

			PkgDeviation pkg{};
			pkg.deviationId = oDevId.value();

			bool bSuccess{ true };
			bSuccess = bSuccess && filllstr(pkg.deviationType, oDevType.value());
			bSuccess = bSuccess && filllstr(pkg.objectName, oObjName.value());
			bSuccess = bSuccess && oObjType.has_value();

			if (!bSuccess)
			{
				return PkgNone();
			}

			if (const auto objType{ oObjType.value_or("") }; std::strcmp(objType.data(), "CAMERA") == 0)
				pkg.objectType = ObjectType::CAMERA;
			else if (std::strcmp(objType.data(), "MESH") == 0)
				pkg.objectType = ObjectType::MESH;

			return pkg;

		}
		else if (strcmp(oPkgType->data(), "DevId") == 0)
		{

			const auto oLastDeviationId{ parseAs<uint32_t>(doc, "LastDeviationId") };
			if (oLastDeviationId == std::nullopt)
			{
				return PkgNone();
			}
			
			PkgLastDeviationId pkg{};
			pkg.lastDeviationId = oLastDeviationId.value();

			return pkg;

		}
		else
		{
			return PkgNone();
		}

	}

	[[nodiscard]] auto encodeLastDeviationId(unsigned int lastDeviationId)
	{

		rapidjson::StringBuffer sb;
		rapidjson::Writer writer(sb);

		writer.StartObject();
		writer.Key("PkgType");
		writer.String("DevId");
		writer.Key("LastDeviationId");
		writer.Uint(lastDeviationId);
		writer.EndObject();

		lstr_t<64> outBuf{};
		filllstr(outBuf, sb.GetString());

		return outBuf;

	}

};