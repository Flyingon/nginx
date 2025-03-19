#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <chrono>
#include <thread>

#define SERVER_IP "127.0.0.1"
#define PORT 80
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 创建 socket 文件描述符
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error");
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 将 IPv4 地址从文本转换为二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return EXIT_FAILURE;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return EXIT_FAILURE;
    }

    std::cout << "Connected to server." << std::endl;

    // 发送一个简单的 HTTP 请求
    const char *http_request = "GET /test502 HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    send(sock, http_request, strlen(http_request), 0);
    std::cout << "Request sent." << std::endl;

    // 等待 10 秒
    std::this_thread::sleep_for(std::chrono::milliseconds (300));

    // 主动断开连接
    close(sock);
    std::cout << "Connection closed." << std::endl;

    return 0;
}