#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>

#define PORT 8080

int sock = 0;
struct sockaddr_in serv_addr;

void listen_to_server() {
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread > 0) {
            std::cout << buffer << std::endl;
        }
        else if (valread == 0) {
            std::cout << "Server disconnected!" << std::endl;
            break;
        }
        else {
            std::cerr << "Error in receiving message" << std::endl;
            break;
        }
    }
}

int main() {
    char buffer[1024] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // Read welcome message
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, sizeof(buffer));
    std::cout  << buffer << std::endl;

    // Prompt for username
    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    send(sock, username.c_str(), username.size(), 0);

    // Prompt for password
    std::string password;
    std::cout << "Enter your password: ";
    std::getline(std::cin, password);
    send(sock, password.c_str(), password.size(), 0);
    
    // Read authentication result from server
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, sizeof(buffer));
    std::cout  << buffer << std::endl;
    if (std::string(buffer).find("Authentication failed") != std::string::npos) {
        close(sock);
        return 1;
    }

    // Start listening for messages from the server in a separate thread
    std::thread listener(listen_to_server);
    listener.detach();  // Detach the thread so it runs independently

    // Now, you can send commands to the server while the listener thread receives messages
    std::string prompt = "";
    
    while (prompt != "/exit") {
        std::getline(std::cin, prompt);
        send(sock, prompt.c_str(), prompt.size(), 0);

        // After sending, you may also want to print server messages as they arrive
        // This is handled by the listener thread
    }

    close(sock);
    return 0;
}
