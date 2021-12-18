#pragma once
#include <thread>

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <queue>
#include <type_traits>

#include "../UACEViewerUtils/asio_includes.h"

using namespace std::chrono_literals;

class BasicTCPServer
{
	using atcp = asio::ip::tcp;
	std::condition_variable cv;
	std::mutex mtcv;
	std::mutex mtSending;

	std::atomic_flag afConnected;

public:
	BasicTCPServer(int port)
		:sock(ios), acceptor(ios, atcp::endpoint(atcp::v4(), port))
	{
		this->sThread = std::jthread([this](std::stop_token stoken)
			{

				this->acceptor.async_accept(this->sock, [&](const asio::error_code& error)
					{
						this->afConnected.test_and_set();
						this->afConnected.notify_all();
					});

				auto iosThread{ std::jthread([this](std::stop_token stoken)
					{
						while (!stoken.stop_requested())
						{
							this->ios.run();
						}
					}) };

				while (!stoken.stop_requested()) {}

			});

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
		while (freeSenderId == -1)
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
				asio::write(this->sock, asio::buffer(&headerData, sizeof(headerData)));
				asio::write(this->sock, asio::buffer(&pkgLen, sizeof(pkgLen)));
				asio::write(this->sock, asio::buffer(container));

				this->aFreeSendersId[freeSenderId] = 1;

			});

		afStarted.wait(false);

	}

private:
	asio::io_service ios;
	atcp::socket sock;
	atcp::acceptor acceptor;
	std::array<char, 255> buffer{};

	std::jthread sThread;

	std::array<std::jthread, 4> aSenders{};
	std::array<int, 4> aFreeSendersId{ 1, 1, 1, 1 };

};

static void wait(const auto& client, int numOfPkgs = 1)
{
	for (int itersLeft = 100; (client.getNumOfPkgs() < numOfPkgs) && (itersLeft > 0); itersLeft--)
	{
		std::this_thread::sleep_for(10ms);
	}
}