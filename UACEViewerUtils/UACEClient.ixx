module;

#include "asio_includes.h"

#include <string_view>
#include <array>

#include <thread>
#include <atomic>

export module UACEClient;

//For Ptr
import UACEUnifiedBlockAllocator;
import UACERingBuffer;

using namespace UACE::MemManager::Literals;
export namespace UACE
{

	template<typename Alloc>
	class Client
	{

	public:

		explicit Client(Alloc* alloc, std::string_view ip, int port)
			:buffer(alloc), sock(ios)
		{

			asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(ip.data()), port);
			
			asio::error_code connError{};
			this->sock.connect(endpoint, connError);
			if (connError.value() != 0)
			{
				assert(false);
			}

			this->thProcess = std::jthread([&](std::stop_token stoken)
				{

					auto iosThread{ std::jthread([this](std::stop_token stoken)
					{
						while (!stoken.stop_requested())
						{
							this->ios.run();
						}
					}) };

					while (!stoken.stop_requested())
					{
						this->proceed(sock, stoken);
					}

				});

		}

		[[nodiscard]] auto getNumOfPkgs() const { return static_cast<int>(this->atNumOfPkgs.load()); }

		size_t popPkg(char* destBuffer, size_t destBufferSize)
		{
			
			if (this->atNumOfPkgs.load() == 0)
			{
				return 0;
			}

			const auto copiedSize{ this->buffer.copyAndPop(destBuffer, destBufferSize) };
			this->atNumOfPkgs--;
			return copiedSize;

		}

	private:
		bool read(auto& stoken, const auto& buffer, size_t transferSize)
		{
			size_t bytesReaded{ 0 };
			std::atomic_flag flComplete{};
			asio::async_read(
				this->sock,
				buffer,
				asio::transfer_exactly(transferSize),
				[&bytesReaded, &flComplete](const asio::error_code& er, std::size_t transfered)
				{
					bytesReaded = transfered;
					flComplete.test_and_set();
					flComplete.notify_one();
				});

			while (!stoken.stop_requested() && flComplete.test() == false) {}
			if (stoken.stop_requested())
			{
				return false;
			}
			return transferSize == bytesReaded;
		}

		void proceed(auto& sock, auto stoken)
		{

			unsigned int headData{ 0 };
			if (!this->read(stoken, asio::buffer(&headData, sizeof(headData)), sizeof(headData)))
			{
				return;
			}

			const auto needHead{ 0xFA'FB'FC'FD };
			if (strncmp(reinterpret_cast<const char*>(&headData), reinterpret_cast<const char*>(&needHead), sizeof(headData)) != 0)
			{
				return;
			}

			int pkgSize{ 0 };
			if (!this->read(stoken, asio::buffer(&pkgSize, sizeof(pkgSize)), sizeof(pkgSize)))
			{
				return;
			}
			auto writeBuffer{ this->buffer.append(pkgSize) };
			assert(writeBuffer != nullptr);

			if (!this->read(stoken, asio::buffer(writeBuffer, pkgSize), pkgSize))
			{
				return;
			}

			this->atNumOfPkgs++;
			iterId++;

		}

	private:

		UACE::RingBuffer<1_kb, Alloc> buffer;

		asio::io_service ios;
		asio::ip::tcp::socket sock;

		std::atomic_char8_t atNumOfPkgs{ 0 };

		std::jthread thProcess;

		int iterId = 0;

	};

}