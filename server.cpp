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
    #include <unistd.h>
#endif

struct Message final {
    char text[256];
    int num;
    MSGPACK_DEFINE(text, num);
};

int main() {

    #ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "Error initializing Winsock: " << result << "\n";
            return 1;
        }
    #endif

    #ifdef _WIN32
        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
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
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "Server is listening on port 8080...\n";
    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        #ifdef _WIN32
            std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
            closesocket(serverSocket);
            WSACleanup();
        #else
            perror("Error connecting");
            close(clientSocket);
        #endif
        return 1;
    }
    std::cout << "Client connected.\n";

    while (true) {
        // Receive from client
        uint32_t len_net;
        int lenBytesRecv = recv(clientSocket, reinterpret_cast<char*>(&len_net), sizeof(len_net), MSG_WAITALL);
        if (lenBytesRecv <= 0) {
            std::cerr << "Recv failed: " << WSAGetLastError() << "\n";
            break;
        }
        uint32_t recvLen = ntohl(len_net);

        std::vector<char> recvBuffer(recvLen);
        int bytesRecv = recv(clientSocket, recvBuffer.data(), recvLen, 0);
        if (bytesRecv > 0) {
            msgpack::object_handle oh = msgpack::unpack(recvBuffer.data(), bytesRecv);
            Message receivedMsg = oh.get().as<Message>();
            std::cout << "msgpack from client:\n";
            std::cout << "Num: " << receivedMsg.num << "\n";
            std::cout << "Text: " << receivedMsg.text << "\n";
        } else {
            std::cerr << "Recv failed: " << WSAGetLastError() << "\n";
        }
        
        
        // Send reply
        Message replyMsg;
        std::cout << "Enter a number: ";
        while (!(std::cin >> replyMsg.num)) {  // Keep asking until the user enters a valid number
            std::cout << "Invalid input. Try again: ";
            std::cin.clear(); // Reset input errors
            std::cin.ignore(10000, '\n'); // Remove bad input
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::string text;
        do {
            std::cout << "Enter a text: ";
            std::getline(std::cin, text);
        } while (text.empty());
        strncpy(replyMsg.text, text.c_str(), sizeof(replyMsg.text) - 1);
        replyMsg.text[sizeof(replyMsg.text) - 1] = '\0';
        
        msgpack::sbuffer sbuf;
        msgpack::pack(sbuf, replyMsg);

        uint32_t sendLen = htonl(static_cast<uint32_t>(sbuf.size()));
        int lenBytesSent = send(clientSocket, reinterpret_cast<char*>(&sendLen), sizeof(sendLen), 0);

        // Send raw bytes
        int bytesSent = send(clientSocket, sbuf.data(), sbuf.size(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << "\n";
        }
    }

    #ifdef _WIN32
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
    #else
        close(clientSocket);
        close(serverSocket);
    #endif

    return 0;
}