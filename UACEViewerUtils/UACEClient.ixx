module;

#include <string_view>
#include <array>

#include <thread>
#include <atomic>

#include <mutex>
#include <cassert>

#include "WinsockTCP.h"

//#include "include/sockpp/tcp_connector.h"


export module UACEClient;
import UACEJsonCoder;

//For Ptr
//import UACEAllocator;
//import UACERingBuffer;
import fovere.RingBuffer;
import hfog.Core;

static constexpr size_t MAX_PACKAGES{ 16 };

using namespace hfog::MemoryUtils::Literals;
export namespace UACE
{

	template<hfog::CtAllocator Alloc>
	class Client
	{

	public:

		explicit constexpr Client(Alloc* alloc, std::string_view ip, int port, size_t bufferSize = 1_kB)
			:buffer(alloc, bufferSize)
		{

			WSADATA wsaData;
			WORD DllVersion = MAKEWORD(2, 2);
			if (auto result{ WSAStartup(DllVersion, &wsaData) }; result != 0)
			{
				assert(result == 0);
			}
	
			std::string getInput = "";
			SOCKADDR_IN addr;
			int addrLen = sizeof(addr);
			addr.sin_addr.s_addr = inet_addr(ip.data());
			addr.sin_port = htons(port);
			addr.sin_family = AF_INET;

			this->sock = socket(AF_INET, SOCK_STREAM, NULL);

			SecureZeroMemory((PVOID)&this->recvOverlapped, sizeof(WSAOVERLAPPED));
			this->recvOverlapped.hEvent = WSACreateEvent();
			if (this->recvOverlapped.hEvent == NULL) {
				const auto errorCode{ WSAGetLastError() };
				assert(false);
			}

			const auto connectResult{ connect(this->sock, (SOCKADDR*)&addr, addrLen) };
			if (connectResult != 0)
			{
				const auto errorCode{ WSAGetLastError() };
				assert(false);
			}

			const uint32_t headerData{ 0xFA'FB'FC'FD };
			send(this->sock, reinterpret_cast<const char*>(&headerData), sizeof(headerData), 0);
			constexpr char awaitString[]{ "\
					{\"ClientType\": \"CEngine\",\
					\"ClientName\" : \"NameEngine0\",\
					\"LastDeviationId\" : -1}\
				" };
			const uint32_t pkgLen{ sizeof(awaitString) };
			send(this->sock, reinterpret_cast<const char*>(&pkgLen), sizeof(pkgLen), 0);
			send(this->sock, awaitString, pkgLen, 0);

			this->thProcess = std::jthread([&](std::stop_token stoken)
				{

					while (!stoken.stop_requested())
					{
						this->proceed(stoken);
					}

					closesocket(this->sock);
					WSACleanup();

				});

		}

		[[nodiscard]] constexpr auto getNumOfPkgs() const { return static_cast<int>(this->atNumOfPkgs.load()); }

		[[nodiscard]] constexpr size_t popPkg(char* destBuffer, size_t destBufferSize)
		{
			
			if (this->atNumOfPkgs.load() == 0)
			{
				return 0;
			}

			const auto copiedSize{ this->buffer.copyAndPop(destBuffer, destBufferSize) };
			
			this->atNumOfPkgs--;
			return copiedSize;

		}

		[[nodiscard]] void sendDeviationId(int devId)
		{

			const unsigned int headerData{ 0xFA'FB'FC'FD };
			send(this->sock, reinterpret_cast<const char*>(&headerData), sizeof(headerData), 0);

			const auto pkg{ UACE::JsonCoder::encodeLastDeviationId(devId) };
			const auto pkgLen{ static_cast<uint32_t>(pkg.size()) };
			send(this->sock, reinterpret_cast<const char*>(&pkgLen), sizeof(pkgLen), 0);

			send(this->sock, reinterpret_cast<const char*>(pkg.data()), pkgLen, 0);

		}

	private:
		[[nodiscard]] constexpr bool read(std::stop_token stoken, auto* pDest, size_t transferSize)
		{
			if (transferSize == 0)
				return true;
			//const auto bytesReaded{ recv(this->sock, reinterpret_cast<char*>(pDest), transferSize, MSG_WAITALL) };
			//assert(bytesReaded != SOCKET_ERROR);
			//return transferSize == bytesReaded;
			
			DWORD bytesReaded{ 0 };
			DWORD flags{ MSG_WAITALL };

			WSABUF dataBuf;
			dataBuf.buf = reinterpret_cast<CHAR*>(pDest);
			dataBuf.len = transferSize;

			const auto startResult{ WSARecv(this->sock, &dataBuf, 1, &bytesReaded, &flags, &this->recvOverlapped, nullptr) };
			if (const auto err{ WSAGetLastError() }; (startResult == SOCKET_ERROR) && (WSA_IO_PENDING != err)) 
			{
				assert(false);
			}

			while ((bytesReaded != transferSize) && (!stoken.stop_requested()))
			{
				const auto recResult{ WSAGetOverlappedResult(this->sock, &this->recvOverlapped, &bytesReaded, FALSE, &flags) };
				if (const auto err{ WSAGetLastError() }; !recResult&& (err != WSA_IO_INCOMPLETE))
				{
					assert(false);
				}
			}
			WSAResetEvent(this->recvOverlapped.hEvent);

			return bytesReaded == transferSize;
		}

		constexpr void proceed(std::stop_token stoken)
		{

			unsigned int headData{ 0 };
			if (!this->read(stoken, &headData, sizeof(headData)))
			{
				return;
			}

			const auto needHead{ 0xFA'FB'FC'FD };
			if (strncmp(reinterpret_cast<const char*>(&headData), reinterpret_cast<const char*>(&needHead), sizeof(headData)) != 0)
			{
				return;
			}

			int pkgSize{ 0 };
			if (!this->read(stoken, &pkgSize, sizeof(pkgSize)))
			{
				return;
			}
			auto writeBuffer{ this->buffer.append(pkgSize) };
			assert(writeBuffer != nullptr);

			if (!this->read(stoken, writeBuffer, pkgSize))
			{
				return;
			}

			this->atNumOfPkgs++;
			iterId++;

		}

	private:


		fovere::Buffer::Ring<char, MAX_PACKAGES, Alloc> buffer;
//		UACE::RingBuffer<Alloc> buffer;

		SOCKET sock;
		WSAOVERLAPPED recvOverlapped;
		//asio::io_service ios;
		//asio::ip::tcp::socket sock;

		std::atomic_char8_t atNumOfPkgs{ 0 };

		std::jthread thProcess;

		int iterId = 0;

	};

};