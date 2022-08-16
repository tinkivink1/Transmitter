#include "Transmitter.h"


#define DEFAULT_SOURCE_IP			"127.0.0.1"
#define DEFAULT_DESTINATION_IP		"127.0.0.1"
//#define DEFAULT_SOURCE_IP			"192.168.28.110"
//#define DEFAULT_DESTINATION_IP		"192.168.28.107"

#define DEFAULT_SOURCE_TCP_PORT			52423
#define DEFAULT_SOURCE_UDP_PORT			55554
#define DEFAULT_DESTINATION_UDP_PORT	55555
#define DEFAULT_DESTINATION_TCP_PORT	55552


int main() {
	WSADATA wsaData;
	SOCKET	udp_socket	= INVALID_SOCKET,
			tcp_socket	= INVALID_SOCKET;
	u_long	ioMode		= 1; //Socket mode (blocking/non-blocking)
	fd_set	master_set	= fd_set(),
			working_set = fd_set();
	timeval timeout;
	timeout.tv_sec = 3 * 60;
	timeout.tv_usec = 0;
	char	buffer[80];
	
	int		ready_desc	= 0, 
			max_desc	= 0, 
			new_desc	= 0, 
			end_server	= FALSE, 
			close_conn	= FALSE,
			err			= WSAStartup(MAKEWORD(2, 2), &wsaData),
			len			= 0;

	if (err != 0) {
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}

	try {
		udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		ioctlsocket(udp_socket, FIONBIO, &ioMode);
		ioctlsocket(tcp_socket, FIONBIO, &ioMode);
	}
	catch (std::exception e) {

	};

	sockaddr_in udpLocal	= SockAddrInit(DEFAULT_SOURCE_IP, DEFAULT_SOURCE_UDP_PORT);
	sockaddr_in tcpLocal	= SockAddrInit(DEFAULT_SOURCE_IP, DEFAULT_SOURCE_TCP_PORT);
	sockaddr_in udpRemote	= SockAddrInit(DEFAULT_DESTINATION_IP, DEFAULT_DESTINATION_UDP_PORT);
	//sockaddr_in tcpRemote	= SockAddrInit(DEFAULT_DESTINATION_IP, DEFAULT_DESTINATION_TCP_PORT);


	err = bind(tcp_socket, (sockaddr*)&tcpLocal, sizeof(tcpLocal));
	IsSocketExcept(err);
	

	/*err = connect(tcp_socket, (sockaddr*)&tcpRemote, sizeof(tcpRemote));
	IsSocketExcept(err);*/

	err = listen(tcp_socket, 32);
	IsSocketExcept(err);
	FD_SET(tcp_socket, &master_set);
	max_desc = tcp_socket; 


	do {
		for (int i = 0; i < max_desc; i++) {
			err = select(max_desc + 1, &master_set, NULL, NULL, &timeout);
			IsSocketExcept(err);

			if (i == tcp_socket) {
				new_desc = accept(tcp_socket, NULL, NULL);
				FD_SET(new_desc, &master_set);
				if (max_desc < new_desc)
					max_desc = new_desc;
			}
			else {
				err = recv(i, buffer, sizeof(buffer), 0);
				if (err < 0)
				{
					if (errno != EWOULDBLOCK)
					{
						perror("  recv() failed");
						close_conn = TRUE;
					}
					break;
				}

				if (err == 0)
				{
					printf("  Connection closed\n");
					close_conn = TRUE;
					break;
				}

				printf("%s", std::string(buffer, sizeof(buffer)).c_str());
				len = err;
				printf("  %d bytes received\n", len);

			}
			printf("%d", FD_ISSET(tcp_socket, &master_set));
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