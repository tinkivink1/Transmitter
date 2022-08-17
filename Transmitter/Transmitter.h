#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")


int main();
sockaddr_in SockAddrInit(std::string ip_addr, u_short port);
template <typename T>
T SwapEndian(T u);
void IsSocketExcept(int result);