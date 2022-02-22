module;

#include <string_view>
#include <format>
#include <span>

#include "include/SQLiteCpp/SQLiteCpp.h"

export module UACEDeviationLogger;
import UACEViewerUtils;

export namespace UACE
{

	class DeviationLogger
	{

	public:
		DeviationLogger() = delete;
		explicit DeviationLogger(const std::string_view databaseFile)
			:db(databaseFile.data(), SQLite::OPEN_READWRITE)
		{
		}

		[[nodiscard]] bool getObjectExist(size_t id)
		{

			const std::string_view query{ this->createStatement("SELECT Id FROM ObjectsList WHERE Id = '{}'", id) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			const auto bExist{ s.hasRow() };
			return bExist;

		}

		[[nodiscard]] bool getObjectExistByHash(size_t hashName)
		{

			const std::string_view query{ this->createStatement("SELECT Id FROM ObjectsList WHERE HashName = '{}'", hashName) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			const auto bExist{ s.hasRow() };
			return bExist;

		}

		[[nodiscard]] size_t getObjectId(size_t hashName)
		{

			const std::string_view query{ this->createStatement("SELECT Id FROM ObjectsList WHERE HashName = '{}'", hashName) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			if (!s.hasRow())
				return 0;

			const auto col{ s.getColumn(0) };
			const auto id{ col.getUInt() };

			return id;

		}

		[[nodiscard]] size_t getObjectHash(size_t objId)
		{

			const std::string_view query{ this->createStatement("SELECT HashName FROM ObjectsList WHERE Id = '{}'", objId) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			if (!s.hasRow())
				return 0;

			const auto col{ s.getColumn(0) };
			const auto hashName{ col.getUInt() };

			return hashName;

		}

		[[nodiscard]] auto logNewObject(size_t hashName, std::span<const char> metaData)
		{

			const auto lastId{ this->getLastId() };
			const size_t objId{ lastId + 1 };

			{
				const std::string_view query{
					this->createStatement("INSERT INTO ObjectsList (Id, HashName, BaseObjId) VALUES ('{}', '{}', NULL)",
					objId, hashName) };
				SQLite::Statement s(this->db, query.data());
				s.executeStep();
			}

			{
				const std::string_view query{
					this->createStatement("INSERT INTO ObjectsCacheData (Id, Transform, Data) VALUES ('{}', NULL, ?)",
					objId) };
				SQLite::Statement s(this->db, query.data());
				s.bindNoCopy(1, metaData.data(), metaData.size());
				s.executeStep();
			}
			
			return objId;

		}

		[[nodiscard]] bool setObjectTransform(size_t objId, std::span<char> data)
		{

			const auto bExist{ this->getObjectExist(objId) };
			if (!bExist)
			{
				return false;
			}

			const std::string_view query{
				this->createStatement("UPDATE ObjectsCacheData SET Transform=? WHERE Id=?") };

			SQLite::Statement s(this->db, query.data());
			s.bindNoCopy(1, data.data(), data.size());
			s.bind(2, objId);

			s.executeStep();

			return true;

		}

		[[nodiscard]] bool getObjectTransform(size_t objId, std::span<char> data)
		{

			const std::string_view query{ this->createStatement("SELECT Transform FROM ObjectsCacheData WHERE Id = '{}'", objId) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			if (!s.hasRow())
				return false;

			const auto col{ s.getColumn(0) };
			const auto blobSize{ col.getBytes() };
			if (blobSize != data.size())
			{
				return false;
			}

			std::memcpy(data.data(), col.getBlob(), blobSize);

			return true;

		}

		[[nodiscard]] bool setObjectMesh(size_t objId, std::span<char> data)
		{

			const auto bExist{ this->getObjectExist(objId) };
			if (!bExist)
			{
				return false;
			}

			const std::string_view query{
				this->createStatement("UPDATE ObjectsCacheData SET Data=? WHERE Id=?") };

			SQLite::Statement s(this->db, query.data());
			s.bindNoCopy(1, data.data(), data.size());
			s.bind(2, objId);

			s.executeStep();

			return true;

		}

		[[nodiscard]] bool getObjectMesh(size_t objId, std::span<char> data)
		{

			const std::string_view query{ this->createStatement("SELECT Data FROM ObjectsCacheData WHERE Id = '{}'", objId) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			if (!s.hasRow())
				return false;

			const auto col{ s.getColumn(0) };
			const auto blobSize{ col.getBytes() };
			if (blobSize != data.size())
			{
				return false;
			}

			std::memcpy(data.data(), col.getBlob(), blobSize);

			return true;

		}

		[[nodiscard]] bool setCamera(size_t objId, std::span<const char> data)
		{

			const auto bExist{ this->getObjectExist(objId) };
			if (!bExist)
			{
				return false;
			}

			const std::string_view query{
				this->createStatement("UPDATE ObjectsCacheData SET Data=? WHERE Id=?") };

			SQLite::Statement s(this->db, query.data());
			s.bindNoCopy(1, data.data(), data.size());
			s.bind(2, objId);

			s.executeStep();

			return true;

		}

		[[nodiscard]] bool getCamera(size_t objId, std::span<char> data)
		{

			const std::string_view query{ this->createStatement("SELECT Data FROM ObjectsCacheData WHERE Id = '{}'", objId) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			if (!s.hasRow())
				return false;

			const auto col{ s.getColumn(0) };
			const auto blobSize{ col.getBytes() };
			if (blobSize != data.size())
			{
				return false;
			}

			std::memcpy(data.data(), col.getBlob(), blobSize);

			return true;

		}

		void removeObject(size_t objId)
		{
			{
				const auto query{ this->createStatement("DELETE FROM ObjectsList WHERE Id={}", objId) };
				SQLite::Statement s{ this->db, query.data() };
				s.executeStep();
			}
			{
				const auto query{ this->createStatement("DELETE FROM ObjectsCacheData WHERE Id={}", objId) };
				SQLite::Statement s{ this->db, query.data() };
				s.executeStep();
			}
		}

		void removeObjectByHash(size_t hashName)
		{
			const auto objId{ this->getObjectId(hashName) };
			this->removeObject(objId);
		}

		void rename(size_t objId, size_t newHashName)
		{
			const auto query{ this->createStatement(" UPDATE ObjectsList SET HashName={} WHERE Id={}", newHashName, objId) };
			SQLite::Statement s{ this->db, query.data() };
			s.executeStep();
		}

		void clear()
		{
			{
				const std::string_view query{ this->createStatement("DELETE FROM ObjectsList") };
				SQLite::Statement s(this->db, query.data());
				s.executeStep();
			}
			{
				const std::string_view query{ this->createStatement("DELETE FROM ObjectsCacheData") };
				SQLite::Statement s(this->db, query.data());
				s.executeStep();
			}
		}

		[[nodiscard]] bool getIsReady() const { return true; }

	private:
		template <typename ... _Type>
		[[nodiscard]] constexpr std::string_view createStatement(const std::string_view text, const _Type& ... args) const
		{
			const auto q = std::format_to_n(this->buffer, sizeof(this->buffer), text, args...);
			if (q.size > sizeof(buffer))
				return {};
			*q.out = '\0';
			return { this->buffer, q.out };
		}

		[[nodiscard]] size_t getLastId()
		{

			const auto query{ this->createStatement("SELECT MAX(Id) FROM ObjectsList") };
			SQLite::Statement s(this->db, query.data());
			s.executeStep();
			if (!s.hasRow())
				return 0;

			const auto col{ s.getColumn(0) };
			const auto lastId{ col.getUInt() };
			return lastId;

		}

	private:
		SQLite::Database db;
		mutable char buffer[256];

		size_t lastIndexId{ 0 };

	};

};