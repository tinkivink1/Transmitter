#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <fstream>
#include <mutex>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

#define PACKET_SIZE	512

int main();
int StationReceiver(std::vector<std::vector<char>>& buffer, std::mutex& locker);
int Transmitter(std::vector<std::vector<char>>& buffer, std::mutex& locker);
sockaddr_in SockAddrInit(std::string ip_addr, u_short port);
template <typename T>
T SwapEndian(T u);
template <typename T>
char* GetBytes(T value);
void IsSocketExcept(int result);
