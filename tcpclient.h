#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <filesystem>

#include <common.h>

constexpr int MAX_CONN_ATTEMPTS = 5;
const std::filesystem::path IMG_RECV_PATH = "/media/sf_Condivisa2/OSMData/requested/";

class TcpClient
{
public:
    TcpClient(const unsigned int p_port, const std::string& p_serverAddrStr);
    bool init();
//    bool run();
    bool getTile(const double p_lat, const double p_lon, const uint32_t p_zoom);
//    bool sendHeader(const tcp_utils::Header& p_requestHeader);

private:
    unsigned int    m_port;
    std::string     m_serverAddrStr;
    int             m_socket;
    sockaddr_in     m_socketAddr;

    template <typename T>
    inline sockaddr* toSockAddr(T* p_addr)
    {
        return reinterpret_cast<sockaddr*>(p_addr);
    }

    bool sendData(const uint8_t* p_data, const unsigned int p_size);
    std::optional<tcp_utils::Header> recvHeader(int p_clientSocket);
    std::optional<std::vector<uint8_t>> recvData(int p_clientSocket, const uint32_t p_size);

};

#endif // TCPCLIENT_H
