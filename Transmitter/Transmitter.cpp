#include "Transmitter.h"


#define DEFAULT_LOCALHOST_IP		"127.0.0.1"
#define DEFAULT_STATION_IP			"127.0.0.1"
#define DEFAULT_SOURCE_IP			"192.168.28.110"	// Стандартный ip 
#define DEFAULT_DESTINATION_IP		"192.168.28.111"	// ip "Приемника" 

#define DEFAULT_LOCALHOST_UDP_PORT		55553	// Порт для приема данных со станции
#define DEFAULT_LOCALHOST_TCP_PORT		55520	// Переделать
#define DEFAULT_STATION_UDP_PORT		55552	// Порт станции
#define DEFAULT_STATION_TCP_PORT		55551   // Порт станции

#define DEFAULT_SOURCE_UDP_PORT			55554	// Порт передатчика
#define DEFAULT_DESTINATION_UDP_PORT	55555	// Порт приемника

#define FREQ							48000

WSADATA wsaData;
int err1 = WSAStartup(MAKEWORD(2, 2), &wsaData);

int main() {
	std::mutex locker;
	std::vector<std::vector<char>> buffer;


	std::thread receiver = std::thread(StationReceiver, std::ref(buffer), std::ref(locker));
	std::thread transmitter = std::thread(Transmitter, std::ref(buffer), std::ref(locker));


	receiver.detach();
	transmitter.detach();


	char a = 0;
	do {
		a = getchar();
	} while (a != 'Q');

	return 0;
}


int Transmitter(std::vector<std::vector<char>>& buffer, std::mutex& locker) {
	SOCKET		udp_socket = INVALID_SOCKET;
	u_long		ioMode = 1;
	int			err = 0;
	char		tempBuffer[PACKET_SIZE];

	std::ofstream file("C:\\Users\\Admin\\Desktop\\test2.wav", std::ios::binary);
	sockaddr_in udpSource = SockAddrInit(DEFAULT_SOURCE_IP, DEFAULT_SOURCE_UDP_PORT);		// для отправки на приемник
	sockaddr_in udpRemote = SockAddrInit(DEFAULT_DESTINATION_IP, DEFAULT_DESTINATION_UDP_PORT);	// для отправки на приемник

	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	err = bind(udp_socket, (sockaddr*)&udpSource, sizeof(udpSource));
	IsSocketExcept(err);
	err = connect(udp_socket, (sockaddr*)&udpRemote, sizeof(udpRemote));
	IsSocketExcept(err);

	ioctlsocket(udp_socket, FIONBIO, &ioMode);

	int counter = 0;
	do {
		while (buffer.size() > 0) {
			locker.lock();
			std::copy(buffer[0].begin(), buffer[0].end(), tempBuffer);
			err = send(udp_socket, tempBuffer, sizeof(tempBuffer), NULL);
			IsSocketExcept(err);
			file.write(tempBuffer, sizeof(tempBuffer));
			buffer.erase(buffer.begin());
			locker.unlock();
			Sleep(1000 / 94);
		}
	} while (true);
}

int StationReceiver(std::vector<std::vector<char>>& buffer, std::mutex& locker) {
	SOCKET		udp_station_socket	= INVALID_SOCKET,
				tcp_socket			= INVALID_SOCKET;
	int			err			= 0, 
				_start		= 0x02,
				_continue	= 0x03,
				_pause		= 0x04,
				_end		= 0x05;
	u_long		ioMode		= 1;
	char		signal;
	bool		is_ended	= false;
	char tempBuffer[PACKET_SIZE];
	std::vector<char> tempererBuffer;

	udp_station_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in udpLocalhost = SockAddrInit(DEFAULT_LOCALHOST_IP, DEFAULT_LOCALHOST_UDP_PORT);	// udp source
	sockaddr_in tcpLocalhost = SockAddrInit(DEFAULT_LOCALHOST_IP, DEFAULT_LOCALHOST_TCP_PORT);	// tcp source
	sockaddr_in udpStation = SockAddrInit(DEFAULT_STATION_IP, DEFAULT_STATION_UDP_PORT);		// udp dest
	sockaddr_in tcpStation = SockAddrInit(DEFAULT_STATION_IP, DEFAULT_STATION_TCP_PORT);		// tcp dest

	err = bind(tcp_socket, (sockaddr*)&tcpLocalhost, sizeof(tcpLocalhost));
	IsSocketExcept(err);
	err = connect(tcp_socket, (sockaddr*)&tcpStation, sizeof(tcpStation));
	IsSocketExcept(err);

	err = bind(udp_station_socket, (sockaddr*)&udpLocalhost, sizeof(udpLocalhost));
	IsSocketExcept(err);
	err = connect(udp_station_socket, (sockaddr*)&udpStation, sizeof(udpStation));
	IsSocketExcept(err);

	ioctlsocket(udp_station_socket, FIONBIO, &ioMode);
	ioctlsocket(tcp_socket, FIONBIO, &ioMode);

	int rcvbufsz(FREQ * 10), newrcvbufsz(0);
	int nSizer(sizeof(int));
	int nretValue(setsockopt(udp_station_socket, SOL_SOCKET, SO_RCVBUF, (char*)(&rcvbufsz), sizeof(int)));
	if (nretValue != 0)
	{
		printf("Error in setsockopt(SO_RCVBUF) (returned value=%d) with errorcode(WSAGetLastError)=%lu.", nretValue, ::WSAGetLastError());
		return false;
	}

	signal = GetBytes(_start)[0];
	err = send(tcp_socket, &signal, 1, NULL);

	int receivedBytes; 

	do {
		while (buffer.size() * PACKET_SIZE < FREQ / 4) {
			locker.lock();

			receivedBytes = err = recv(udp_station_socket, tempBuffer, sizeof(tempBuffer), NULL);

			if (err == -1) {
				signal = GetBytes(_continue)[0];
				err = send(tcp_socket, &signal, sizeof(signal), NULL);
			}

			if (receivedBytes > 0) {
				std::cout << WSAGetLastError() << std::endl;

				tempererBuffer = std::vector<char>(tempBuffer, tempBuffer + sizeof(tempBuffer));
				buffer.push_back(tempererBuffer);
			}
			locker.unlock();

			if (is_ended && buffer.size() == 0) {
				closesocket(tcp_socket);
				closesocket(udp_station_socket);
				exit(0);
			}
		}

		err = recv(tcp_socket, &signal, 1, NULL);


		signal = GetBytes(_pause)[0];
		err = send(tcp_socket, &signal, sizeof(signal), NULL);
		//std::cout << "send err: " << err << std::endl;

	} while (true);
}

template <typename T>
char* GetBytes(T value) {
	return static_cast<char*>(static_cast<void*>(&value));
}
void IsSocketExcept(int result) {
	if (result == SOCKET_ERROR) {
		printf("Socket action failed with error: %d\n", WSAGetLastError());
	}
}

sockaddr_in SockAddrInit(std::string ip_addr, u_short port) {
	sockaddr_in new_sock_addr;
	try {
		new_sock_addr.sin_family		= AF_INET;
		new_sock_addr.sin_addr.s_addr	= inet_addr(ip_addr.c_str());
		new_sock_addr.sin_port			= htons(port);   //bytes to litle endian
	}
	catch (std::exception e) {
		printf("Socket initialization exception occured (%s:%hu)", ip_addr.c_str(), port);
	}

	return new_sock_addr;
}

template <typename T>
T SwapEndian(T u)
{
	static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

	union
	{
		T u;
		unsigned char u8[sizeof(T)];
	} source, dest;

	source.u = u;

	for (size_t k = 0; k < sizeof(T); k++)
		dest.u8[k] = source.u8[sizeof(T) - k - 1];

	return dest.u;
}
