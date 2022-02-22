module;
#include <thread>

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <queue>
#include <type_traits>
#include <string>
#include <variant>
#include <cassert>
#include <array>
#include <chrono>

#include "../UACEViewerUtils/WinsockTCP.h"

export module BasicTCPServer;
import UACEJsonCoder;

export class BasicTCPServer
{

	std::condition_variable cv;
	std::mutex mtcv;
	std::mutex mtSending;

	std::atomic_flag afConnected;

public:
	BasicTCPServer(int port)
	{
		this->sThread = std::jthread([this, port](std::stop_token stoken)
			{
				
				WSADATA wsaData;
				WORD DllVersion = MAKEWORD(2, 2);
				if (auto result{ WSAStartup(DllVersion, &wsaData) }; result != 0)
				{
					assert(result == 0);
				}

				//Create a socket
				if ((this->serverSock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
				{
					const auto err{ WSAGetLastError() };
					assert(false);
				}

				//Prepare the sockaddr_in structure
				sockaddr_in server;
				server.sin_family = AF_INET;
				server.sin_addr.s_addr = INADDR_ANY;
				server.sin_port = htons(port);

				//Bind
				if (bind(this->serverSock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
				{
					const auto err{ WSAGetLastError() };
					assert(false);
				}

				listen(this->serverSock, 1);

				this->sock = accept(this->serverSock, nullptr, nullptr);
				if (this->sock == INVALID_SOCKET)
				{
					const auto err{ WSAGetLastError() };
					assert(false);
				}

				const auto jpkg{ this->recvStartPkg() };

				constexpr char awaitString[]{ "\
					{\"ClientType\": \"CEngine\",\
					\"ClientName\" : \"NameEngine0\",\
					\"LastDeviationId\" : -1}\
				" };

				assert(jpkg.compare(awaitString) == 0);

				this->afConnected.test_and_set();
				this->afConnected.notify_all();

				while (!stoken.stop_requested()) {}

				closesocket(this->serverSock);
				WSACleanup();

			});

	}

	std::string recvStartPkg()
	{

		int recvHeader{ 0 };
		const auto headerLen{ recv(this->sock, reinterpret_cast<char*>(&recvHeader), sizeof(recvHeader), MSG_WAITALL) };
		if (recvHeader != 0xFA'FB'FC'FD)
			return "";

		int recvPkgLen{ 0 };
		const auto pkgLenBytes{ recv(this->sock, reinterpret_cast<char*>(&recvPkgLen), sizeof(recvPkgLen), MSG_WAITALL) };
		if (recvPkgLen == 0)
			return "";

		std::string outStr;
		char data[1024];
		const auto recvLen{ recv(this->sock, data, recvPkgLen, MSG_WAITALL)};
		outStr.append(data);

		return outStr;

	}

	void sendData(const auto& container)
	{

		[[nodiscard]] const auto getFreeSenderId = [this]()
		{
			int id{ 0 };
			for (const auto bFree : this->aFreeSendersId)
			{
				if (bFree)
				{
					return id;
				}
				id++;
			}
			return -1;
		};

		int freeSenderId{ -1 };
		while (freeSenderId < 0 || freeSenderId >= 4)
		{
			freeSenderId = getFreeSenderId();
		}
		this->aFreeSendersId[freeSenderId] = 0;

		std::atomic_flag afStarted{};

		this->aSenders[freeSenderId] = std::jthread([&, freeSenderId]()
			{

				std::lock_guard lg(this->mtSending);
				afStarted.test_and_set();
				afStarted.notify_one();

				this->afConnected.wait(false);

				const unsigned int headerData{ 0xFA'FB'FC'FD };
				

				const auto pkgLen{ container.size() * sizeof(std::decay_t<decltype(container)>::value_type) };
				send(this->sock, reinterpret_cast<const char*>(&headerData), sizeof(headerData), 0);
				send(this->sock, reinterpret_cast<const char*>(&pkgLen), sizeof(pkgLen), 0);
				if (pkgLen != 0)
					send(this->sock, reinterpret_cast<const char*>(container.data()), pkgLen, 0);

				this->aFreeSendersId[freeSenderId] = 1;

			});

		afStarted.wait(false);

	}

	bool waitForDeviation(int devId)
	{

		int recvHeader{ 0 };
		const auto headerLen{ recv(this->sock, reinterpret_cast<char*>(&recvHeader), sizeof(recvHeader), MSG_WAITALL) };
		if (recvHeader != 0xFA'FB'FC'FD)
			return "";

		int recvPkgLen{ 0 };
		const auto pkgLenBytes{ recv(this->sock, reinterpret_cast<char*>(&recvPkgLen), sizeof(recvPkgLen), MSG_WAITALL) };
		if (recvPkgLen == 0)
			return "";

		char data[1024];
		const auto recvLen{ recv(this->sock, data, recvPkgLen, MSG_WAITALL) };
		
		const auto pkg{ UACE::JsonCoder::decode(data) };
		if (!std::holds_alternative<UACE::JsonCoder::PkgLastDeviationId>(pkg))
			return false;

		return std::get<UACE::JsonCoder::PkgLastDeviationId>(pkg).lastDeviationId == devId;

	}

private:

	SOCKET serverSock;
	SOCKET sock;
	std::array<char, 255> buffer{};

	std::jthread sThread;

	std::array<std::jthread, 4> aSenders{};
	std::array<int, 4> aFreeSendersId{ 1, 1, 1, 1 };

};

export static void wait(const auto& client, int numOfPkgs = 1)
{

	using namespace std::chrono_literals;
	for (int itersLeft = 100; (client.getNumOfPkgs() < numOfPkgs) && (itersLeft > 0); itersLeft--)
	{
		std::this_thread::sleep_for(10ms);
	}
}