#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#endif

std::vector<uint8_t> parseMACAddress(const std::string& macAddress) {
    std::vector<uint8_t> bytes;
    std::istringstream is(macAddress);
    uint16_t byte;
    while (is >> std::hex >> byte) {
        bytes.push_back(static_cast<uint8_t>(byte));
    }
    return bytes;
}

std::vector<uint8_t> createMagicPacket(const std::vector<uint8_t>& macAddress) {
    const int numRepeats = 16;
    std::vector<uint8_t> magicPacket;
    magicPacket.insert(magicPacket.end(), 6, 0xFF);
    for (int i = 0; i < numRepeats; ++i) {
        magicPacket.insert(magicPacket.end(), macAddress.begin(), macAddress.end());
    }
    return magicPacket;
}

int main() {
    //change mac add
    const std::string targetMacAddress = "00:11:22:33:44:55";
    std::vector<uint8_t> macAddressBytes = parseMACAddress(targetMacAddress);
    std::vector<uint8_t> magicPacket = createMagicPacket(macAddressBytes);

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. Error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    BOOL broadcastOption = TRUE;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastOption, sizeof(broadcastOption)) == SOCKET_ERROR) {
        std::cerr << "Failed to set broadcast option for the socket. Error code: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(9);
    targetAddr.sin_addr.s_addr = INADDR_BROADCAST;
    if (sendto(sock, (char*)magicPacket.data(), magicPacket.size(), 0, (sockaddr*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to send magic packet. Error code: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Magic packet sent to wake up the target PC." << std::endl;
    }
    closesocket(sock);
    WSACleanup();

#else
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        std::cerr << "Failed to create socket. Error: " << strerror(errno) << std::endl;
        return 1;
    }
    int broadcastOption = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastOption, sizeof(broadcastOption)) < 0) {
        std::cerr << "Failed to set broadcast option for the socket. Error: " << strerror(errno) << std::endl;
        close(sock);
        return 1;
    }
    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(9);
    inet_pton(AF_INET, "255.255.255.255", &(targetAddr.sin_addr));
    if (sendto(sock, magicPacket.data(), magicPacket.size(), 0, (sockaddr*)&targetAddr, sizeof(targetAddr)) < 0) {
        std::cerr << "Failed to send magic packet. Error: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Magic packet sent to wake up the target PC." << std::endl;
    }
    close(sock);
#endif
    return 0;
}


