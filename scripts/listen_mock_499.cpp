#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define PORT 8080

// IP header structure
struct ipheader {
    unsigned char      iph_ihl:4, iph_ver:4;
    unsigned char      iph_tos;
    unsigned short int iph_len;
    unsigned short int iph_ident;
    unsigned short int iph_offset;
    unsigned char      iph_ttl;
    unsigned char      iph_protocol;
    unsigned short int iph_chksum;
    struct  in_addr    iph_sourceip;
    struct  in_addr    iph_destip;
};

// TCP header structure
struct tcpheader {
    unsigned short int tcph_srcport;
    unsigned short int tcph_destport;
    unsigned int       tcph_seqnum;
    unsigned int       tcph_acknum;
    unsigned char      tcph_reserved:4, tcph_offset:4;
    unsigned char      tcph_flags;
    unsigned short int tcph_win;
    unsigned short int tcph_chksum;
    unsigned short int tcph_urgptr;
};

int main() {
    int sock;
    char buffer[4096];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    // 创建一个原始套接字，用于捕获TCP数据包
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening for incoming SYN packets on port %d...\n", PORT);

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // 接收传入的数据包
        if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_addr, &addr_len) < 0) {
            perror("Packet receive failed");
            close(sock);
            exit(EXIT_FAILURE);
        }

        struct ipheader *ip = (struct ipheader *) buffer;
        struct tcpheader *tcp = (struct tcpheader *) (buffer + ip->iph_ihl * 4);

        // 检查接收到的包是否为TCP SYN包，并且目标端口是否为8080
        if (tcp->tcph_flags == TH_SYN && ntohs(tcp->tcph_destport) == PORT) {
            sleep(1000);
            printf("Received SYN packet from %s:%d\n",
                   inet_ntoa(ip->iph_sourceip),
                   ntohs(tcp->tcph_srcport));
            // 不发送SYN+ACK，直接丢弃包
        }
    }

    close(sock);
    return 0;
}
