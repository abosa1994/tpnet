#include <iostream>
#include <vector>
#include <tcpclient.h>
#include <tcpserver.h>

using namespace std;

std::vector<std::string> parse_args(int argc, char* argv[]) {
    if (argc < 2)
        throw std::invalid_argument("Usage: " + std::string(argv[0]) + " <arg1> <arg2> ...");

    return std::vector<std::string>(argv + 1, argv + argc);
}

int main(int argc, char** argv)
{
    std::vector<std::string> args = parse_args(argc, argv);
    std::string mode = args[0];
    std::string host;
    unsigned int port;
    if ( argc > 3)
    {
        host = args[1];
        port = std::stoi(args[2]);
    }
    else
    {
        port = std::stoi(args[1]);
    }

    if ( mode == "-c" )
    {
        TcpClient client(port, host);
        if ( !client.init() ) return -1;
        client.getTile(45.0704900, 7.6868200, 10);
    }
    else if ( mode == "-s" )
    {
        TcpServer server(port);
        if ( !server.init() ) return -1;
        server.run();
    }


    return 0;
}
