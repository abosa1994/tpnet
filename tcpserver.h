#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <optional>
#include <vector>
#include <string>
#include <atomic>
#include <filesystem>

#include <common.h>
#include "tileimage.h"


const std::filesystem::path IMG_STORAGE_PATH = "/media/sf_Condivisa2/OSMData/tiles/";

class TcpServer
{
public:
    TcpServer(const unsigned int port);
    bool init();
    bool run();

private:
    unsigned int        m_port;
    int                 m_socket;
    sockaddr_in         m_socketAddr;
    std::atomic<bool>   m_isRunning;

    template <typename T>
    inline sockaddr* toSockAddr(T* p_addr)
    {
        return reinterpret_cast<sockaddr*>(p_addr);
    }

    std::optional<tcp_utils::Header> recvHeader(int p_clientSocket);
    std::optional<std::vector<uint8_t>> recvData(int p_clientSocket, const uint32_t p_size);
    bool handleTileRequest(const int p_clientSocket, const tcp_utils::TileRequest& p_request);
    bool sendImage(const int p_destSocket, TileImage& p_tileImage);
    bool sendData(const int p_destSocket, const uint8_t *p_data, const unsigned int p_size);
};

#endif // TCPSERVER_H
