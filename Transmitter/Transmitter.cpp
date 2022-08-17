#include "Transmitter.h"


#define DEFAULT_LOCALHOST_IP		"127.0.0.1"
#define DEFAULT_STATION_IP			"127.0.0.1"
#define DEFAULT_SOURCE_IP			"192.168.28.110"	// Стандартный ip 
#define DEFAULT_DESTINATION_IP		"192.168.28.111"	// ip "Приемника" 

#define DEFAULT_LOCALHOST_UDP_PORT		55553	// Порт для приема данных со станции
#define DEFAULT_LOCALHOST_TCP_PORT		52423	// Переделать
#define DEFAULT_STATION_UDP_PORT		55552	// Порт станции
#define DEFAULT_STATION_TCP_PORT		55550   // Порт станции

#define DEFAULT_SOURCE_UDP_PORT			55554	// Порт передатчика
#define DEFAULT_DESTINATION_UDP_PORT	55555	// Порт приемника


int main() {
	WSADATA wsaData;
	SOCKET	udp_socket			= INVALID_SOCKET, // Сокет для отправки данных на передатчик
			udp_station_socket	= INVALID_SOCKET, // Сокет для приема данных со станции
			tcp_socket			= INVALID_SOCKET; // Сокет для отправки сообщений на станцию
	u_long	ioMode				= 1;			  // Переменная для переключения режима блокировки сокетов
	fd_set	master_set			= fd_set();       // Набор рабочих дескрипторов сокетов
	timeval timeout				= {3 * 60, 0};	  // Таймаут приема данных от станции

	char	buffer[65535]; // Буфер для данных
	
	int		new_desc	= 0, 
			end_server	= FALSE, 
			close_conn	= FALSE,
			err			= WSAStartup(MAKEWORD(2, 2), &wsaData),
			len			= 0;

	if (err != 0) {
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}

	try {
		udp_socket			= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		udp_station_socket	= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		tcp_socket			= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		ioctlsocket(udp_station_socket, FIONBIO, &ioMode);
		ioctlsocket(udp_socket, FIONBIO, &ioMode);
		ioctlsocket(tcp_socket, FIONBIO, &ioMode);
	}
	catch (std::exception e) {

	};

	
	sockaddr_in udpLocalhost	= SockAddrInit(DEFAULT_LOCALHOST_IP,	DEFAULT_LOCALHOST_UDP_PORT);	// для приема со станции
	sockaddr_in tcpLocalhost	= SockAddrInit(DEFAULT_LOCALHOST_IP,	DEFAULT_LOCALHOST_TCP_PORT);	// для отправки на станцию
	sockaddr_in udpStation		= SockAddrInit(DEFAULT_STATION_IP,		DEFAULT_STATION_UDP_PORT);		// для приема со станции
	sockaddr_in tcpStation		= SockAddrInit(DEFAULT_STATION_IP,		DEFAULT_STATION_TCP_PORT);		// для отправки на станцию
	sockaddr_in udpSource		= SockAddrInit(DEFAULT_SOURCE_IP,		DEFAULT_SOURCE_UDP_PORT);		// для отправки на приемник
	sockaddr_in udpRemote		= SockAddrInit(DEFAULT_DESTINATION_IP,	DEFAULT_DESTINATION_UDP_PORT);	// для отправки на приемник


	err = bind(tcp_socket, (sockaddr*)&tcpLocalhost, sizeof(tcpLocalhost));
	IsSocketExcept(err);
	err = connect(tcp_socket, (sockaddr*)&tcpStation, sizeof(tcpStation));
	IsSocketExcept(err);

	err = bind(udp_station_socket, (sockaddr*)&udpLocalhost, sizeof(udpLocalhost));
	IsSocketExcept(err);
	err = connect(udp_station_socket, (sockaddr*)&udpStation, sizeof(udpStation));
	IsSocketExcept(err);

	err = bind(udp_socket, (sockaddr*)&udpSource, sizeof(udpSource));
	IsSocketExcept(err);
	err = connect(udp_socket, (sockaddr*)&udpRemote, sizeof(udpRemote));
	IsSocketExcept(err);

	FD_SET(tcp_socket, &master_set);
	FD_SET(udp_station_socket, &master_set);

	do {
		FD_SET(tcp_socket, &master_set);
		memset(&buffer, 0, sizeof(buffer));
		for (int i = 0; i <= master_set.fd_count; i++) {
			err = select(master_set.fd_count, &master_set, NULL, NULL, &timeout);
			IsSocketExcept(err);

			if (master_set.fd_array[i] == tcp_socket) {
				new_desc = accept(tcp_socket, NULL, NULL);
				if(new_desc>0)
					FD_SET(new_desc, &master_set);
			}
			else {
				err = recv(master_set.fd_array[i], buffer, sizeof(buffer), 0);
				if (err < 0)
				{
					if (errno != EWOULDBLOCK)
					{
						printf("%lu: nothing to receive\n", master_set.fd_array[i]);
						close_conn = TRUE;
						FD_CLR(master_set.fd_array[i], &master_set);
					}
					continue;
				}

				if (err == 0)
				{
					printf("%lu: Connection closed\n", master_set.fd_array[i]);
					close_conn = TRUE;
					FD_CLR(master_set.fd_array[i], &master_set);
					continue;
				}

				printf("%lu: %s", master_set.fd_array[i], std::string(buffer, err).c_str());
				len = err;
				printf("\t%d bytes received\n", len);

				err = sendto(udp_socket, buffer, err, NULL, (sockaddr*)&udpRemote, sizeof(udpRemote));
				printf("\t%d bytes sent\n", err);
				IsSocketExcept(err);
			}
		}

	} while (true);
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

//do {
//	memcpy(&working_set, &master_set, sizeof(master_set));
//	printf("Waiting on select()...\n");
//	err = select(max_desc + 1, &working_set, NULL, NULL, &timeout);
//
//	if (err < 0) {
//		perror("select() failed.\n");
//		break;
//	}
//	if (err == 0)
//	{
//		printf("select() timed out.\n");
//		break;
//	}
//
//	ready_desc = err;
//	for (int i = 0; i <= max_desc && ready_desc > 0; ++i) {
//		if (FD_ISSET(i, &working_set))
//		{
//			ready_desc--;
//			if (i == tcp_socket)
//			{
//				printf("Listening socket is readable\n");
//
//				do
//				{
//					new_desc = accept(tcp_socket, NULL, NULL);
//					if (new_desc < 0)
//					{
//						if (errno != EWOULDBLOCK)
//						{
//							perror("accept() failed");
//							end_server = TRUE;
//						}
//						break;
//					}
//
//
//					printf("  New incoming connection - %d\n", new_desc);
//					FD_SET(new_desc, &master_set);
//					if (new_desc > max_desc)
//						max_desc = new_desc;
//
//
//
//				} while (new_desc != -1);
//			}
//
//			else
//			{
//				printf("  Descriptor %d is readable\n", i);
//				close_conn = FALSE;
//				do
//				{
//					err = recv(i, buffer, sizeof(buffer), 0);
//					if (err < 0)
//					{
//						if (errno != EWOULDBLOCK)
//						{
//							perror("  recv() failed");
//							close_conn = TRUE;
//						}
//						break;
//					}
//
//					if (err == 0)
//					{
//						printf("  Connection closed\n");
//						close_conn = TRUE;
//						break;
//					}
//					len = err;
//					printf("  %d bytes received\n", len);
//
//					err = send(i, buffer, len, 0);
//					if (err < 0)
//					{
//						perror("  send() failed");
//						close_conn = TRUE;
//						break;
//					}
//
//				} while (TRUE);
//
//
//				if (close_conn)
//				{
//					FD_CLR(i, &master_set);
//					if (i == max_desc)
//					{
//						while (FD_ISSET(max_desc, &master_set) == FALSE)
//							max_desc -= 1;
//					}
//				}
//			}
//		}
//
//	}
//} while (end_server == FALSE);