#include <iostream>
#include <limits>
#include <msgpack.hpp>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>   // for close()
#endif

struct Message {
    std::string text;
    int num;
    MSGPACK_DEFINE(text, num);
};

int main() {
    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "Error initializing Winsock: " << result << "\n";
            return 1;
        }
    #endif

    // Create socket
    #ifdef _WIN32
        SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
            WSACleanup();
            return 1;
        }
    #else
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            perror("Error creating socket");
            return 1;
        }
    #endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        #ifdef _WIN32
                std::cerr << "Error connecting: " << WSAGetLastError() << "\n";
                closesocket(clientSocket);
                WSACleanup();
        #else
                perror("Error connecting");
                close(clientSocket);
        #endif
        return 1;
    }

    std::cout << "Connected to server!\n";

    std::cout << "Connected to server!\n";

    while (true) {
        Message replyMsg;
        std::cout << "Enter a number: ";
        while (!(std::cin >> replyMsg.num)) {
            std::cout << "Invalid input. Try again: ";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::cout << "Enter a text: ";
        std::getline(std::cin, replyMsg.text);

        // Serialize with MessagePack
        msgpack::sbuffer sbuf;
        msgpack::pack(sbuf, replyMsg);

        //Send length of serialized data
        uint32_t sendLen = htonl(static_cast<uint32_t>(sbuf.size()));   
        int lenBytesSent = send(clientSocket, reinterpret_cast<char*>(&sendLen), sizeof(sendLen), 0);

        // Send serialized data
        int bytesSent = send(clientSocket, sbuf.data(), sbuf.size(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << "\n";
            break;
        }

        // Receive length of serialized data
        uint32_t len_net;
        int lenBytesRecv = recv(clientSocket, reinterpret_cast<char*>(&len_net), sizeof(len_net), MSG_WAITALL);
        if (lenBytesRecv <= 0) {
            std::cerr << "Recv failed: " << WSAGetLastError() << "\n";
            break;
        }
        uint32_t recvLen = ntohl(len_net);

        // Receive serialized data
        std::vector<char> buffer(recvLen);
        int bytesRecv = recv(clientSocket, buffer.data(), recvLen, MSG_WAITALL);
        if (bytesRecv > 0) {
            // Deserialize
            msgpack::object_handle oh = msgpack::unpack(buffer.data(), bytesRecv);
            Message receivedMsg = oh.get().as<Message>();

            std::cout << "msgpack from server:\n";
            std::cout << "Num: " << receivedMsg.num << "\n";
            std::cout << "Text: " << receivedMsg.text << "\n";
        } else {
            std::cerr << "Recv failed: " << WSAGetLastError() << "\n";
            break;
        }
    }   

    #ifdef _WIN32
        closesocket(clientSocket);
        WSACleanup();
    #else
        close(clientSocket);
    #endif

    return 0;
}
