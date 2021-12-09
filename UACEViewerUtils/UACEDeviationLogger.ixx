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

		bool getObjectExist(size_t id)
		{

			const std::string_view query{ this->createStatement("SELECT Id FROM ObjectsList WHERE Id = '{}'", id) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			const auto bExist{ s.hasRow() };
			return bExist;

		}

		bool getObjectExistByHash(size_t hashName)
		{

			const std::string_view query{ this->createStatement("SELECT Id FROM ObjectsList WHERE HashName = '{}'", hashName) };

			SQLite::Statement s(this->db, query.data());

			s.executeStep();
			const auto bExist{ s.hasRow() };
			return bExist;

		}

		size_t getObjectId(size_t hashName)
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

		size_t getObjectHash(size_t objId)
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

		auto logNewObject(size_t hashName, size_t baseObjHashName)
		{

			const auto lastId{ this->getLastId() };
			const size_t objId{ lastId + 1 };

			{
				const std::string_view query{
					this->createStatement("INSERT INTO ObjectsList (Id, HashName, BaseObjId) VALUES ('{}', '{}', '{}')",
					objId, hashName, baseObjHashName) };
				SQLite::Statement s(this->db, query.data());
				s.executeStep();
			}

			{
				const std::string_view query{
					this->createStatement("INSERT INTO ObjectsCacheData (Id, Transform, Mesh) VALUES ('{}', NULL, NULL)",
					objId) };
				SQLite::Statement s(this->db, query.data());
				s.executeStep();
			}
			
			return objId;

		}

		bool setObjectTransform(size_t objId, std::span<char> data)
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

		bool getObjectTransform(size_t objId, std::span<char> data)
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

		bool setObjectMesh(size_t objId, std::span<char> data)
		{

			const auto bExist{ this->getObjectExist(objId) };
			if (!bExist)
			{
				return false;
			}

			const std::string_view query{
				this->createStatement("UPDATE ObjectsCacheData SET Mesh=? WHERE Id=?") };

			SQLite::Statement s(this->db, query.data());
			s.bindNoCopy(1, data.data(), data.size());
			s.bind(2, objId);

			s.executeStep();

			return true;

		}

		bool getObjectMesh(size_t objId, std::span<char> data)
		{

			const std::string_view query{ this->createStatement("SELECT Mesh FROM ObjectsCacheData WHERE Id = '{}'", objId) };

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

		bool getIsReady() const { return true; }

	private:
		template <typename ... _Type>
		std::string_view createStatement(const std::string_view text, const _Type& ... args) const
		{
			const auto q = std::format_to_n(this->buffer, sizeof(this->buffer), text, args...);
			if (q.size > sizeof(buffer))
				return {};
			*q.out = '\0';
			return { this->buffer, q.out };
		}

		size_t getLastId()
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