#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <unordered_map>
#include <set>
#include <map>

std::mutex mtx;  // Shared mutex for synchronization
#define PORT 8080

std::unordered_map<std::string, std::string> mp;  // User credentials
std::unordered_map<std::string, std::multiset<std::string>> g;  // Group members
std::multimap<std::string, int> online;  // Online users mapping (username -> socket)
void inform(std::string username,std::multimap<std::string, int> online,int flag)
{
    std::string s;
    if (flag==1){ 
      s=" has joined the chat.";
    }
    else{
       s=" has left the chat.";
    }

        
    const char* group_f_msg = username.append(s).c_str();
     for (auto it : online)
     {
        send(it.second, group_f_msg, strlen(group_f_msg), 0);
     }
     
}
std::pair<std::string, std::string> tparse(std::string message) {
    std::string group_name;
    std::string group_msg;
    size_t space1 = message.find(' ');
    size_t space2 = message.find(' ', space1 + 1);
    if (space1 != std::string::npos && space2 != std::string::npos) {
        group_name = message.substr(space1 + 1, space2 - space1 - 1);
        group_msg = message.substr(space2 + 1);
    }
    return {group_name, group_msg};
}

std::string oparse(std::string message) {
    size_t space1 = message.find(' ');
    if (space1 != std::string::npos) {
        return message.substr(space1 + 1);  // Get everything after the first space
    }
    return "";  // Return empty string if no space is found
}

void parse() {
    std::ifstream file("users.txt");
    std::string str;
    
    while (std::getline(file, str)) {
        std::string username;
        std::string password;
        bool found_colon = false;
        
        for (char c : str) {
            if (c == ':') {
                found_colon = true;
                continue;
            }
            if (found_colon) {
                password += c;
            } else {
                username += c;
            }
        }
        
        mp[username] = password;  // Store user credentials
    }
}

int valid(const char* user, const char* password) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = mp.find(user);
    if (it != mp.end() && password == it->second) {
        return 1;  // Valid credentials
    }
    return -1;  // Invalid credentials
}

void handle_client(int new_socket) {
    char buffer[1024] = {0};
    
    // Send welcome message
    const char* hello = "Connected to the server";
    send(new_socket, hello, strlen(hello), 0);
    
    // Receive username
    memset(buffer, 0, sizeof(buffer));
    read(new_socket, buffer, sizeof(buffer));
    std::string username = buffer;
    
    // Receive password
    memset(buffer, 0, sizeof(buffer));
    read(new_socket, buffer, sizeof(buffer));
    std::string password = buffer;
    
    // Validate credentials
    if (valid(username.c_str(), password.c_str()) == 1) {
        inform(username,online,1);
        online.insert({username, new_socket});
        const char* welcome_msg = "Welcome to the chat server !";
        send(new_socket, welcome_msg, strlen(welcome_msg), 0);
    } else {
        const char* fail_msg = "Authentication failed";
        send(new_socket, fail_msg, strlen(fail_msg), 0);
        close(new_socket);  // Close the connection
        return;
    }

    std::string prompt = "";

    while (prompt != "/exit") {
        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));
        prompt = buffer;

        if (std::string(buffer).find("/create_group") != std::string::npos) {
            std::string group_name = oparse(std::string(buffer));
            std::multiset<std::string> tmp;
            tmp.insert(username);
            mtx.lock();
            g.insert({group_name, tmp});
            mtx.unlock();
            std::string group_f_msg_str = "Group " + group_name + " created";
            const char* group_f_msg = group_f_msg_str.c_str();
            send(new_socket, group_f_msg, strlen(group_f_msg), 0);
        }
        else if (std::string(buffer).find("/leave_group") != std::string::npos) {
            std::string group_name = oparse(buffer);
            if (g.find(group_name) != g.end()) {
                mtx.lock();
                g[group_name].erase(username);  // Removes one occurrence of the username
                std::string group_f_msg_str = "Left Group " + group_name ;
                 const char* group_f_msg = group_f_msg_str.c_str();
                send(new_socket, group_f_msg, strlen(group_f_msg), 0);
                mtx.unlock();
            } else {
                const char* group_f_msg = "Failed to leave group (group not found)";
                send(new_socket, group_f_msg, strlen(group_f_msg), 0);
            }
        }
        else if (std::string(buffer).find("/join_group") != std::string::npos) {
            std::string group_name = oparse(buffer);
            if (g.find(group_name) != g.end()) {
                mtx.lock();
                g[group_name].insert(username);  // Adds the user to the group
                std::string group_f_msg_str = "Joined Group " + group_name ;
                const char* group_f_msg = group_f_msg_str.c_str();
                send(new_socket, group_f_msg, strlen(group_f_msg), 0);
                mtx.unlock();
            } else {
                const char* group_f_msg = "Failed to join group (group not found)";
                send(new_socket, group_f_msg, strlen(group_f_msg), 0);
            }
        }
        else if (std::string(buffer).find("/broadcast") != std::string::npos) {
            std::string sandesh;
            sandesh.append("["+username+"] : ");
            sandesh.append(oparse(buffer));
            for (auto it : online) {
                if(it.first!=username){
                send(it.second, sandesh.c_str(), sandesh.size(), 0);}
            }
        }
        else if (std::string(buffer).find("/msg") != std::string::npos) {
            std::string susername = tparse(buffer).first;
            std::string sandesh;
            sandesh.append("["+username+"] : ");
            sandesh.append(tparse(buffer).second);
           
            if (online.find(susername) != online.end()) {
                int user_socket = online.find(susername)->second;
                send(user_socket, sandesh.c_str(), sandesh.size(), 0);
            } else {
                const char* user_not_found = "User not found";
                send(new_socket, user_not_found, strlen(user_not_found), 0);
            }
        }
        else if (std::string(buffer).find("/group_msg") != std::string::npos) {
            std::string group = tparse(buffer).first;
            std::string sandesh;
            sandesh.append("["+group+"] : ");
            sandesh.append(tparse(buffer).second);


            if (g.find(group) != g.end()) {
                if(g.find(group)->second.count(username)!=0){
                for (const std::string& usern : g[group]) {
                    if(usern != username){
                    if (online.find(usern) != online.end()) {
                        int user_socket = online.find(usern)->second;
                        send(user_socket, sandesh.c_str(), sandesh.size(), 0);
                    }}
                }
                const char* group_msg_sent = "Group message sent";
                send(new_socket, group_msg_sent, strlen(group_msg_sent), 0);}
                else{
                    const char* group_msg_sent = "You are not member of this group";
                send(new_socket, group_msg_sent, strlen(group_msg_sent), 0);
                }
            } else {
                const char* group_not_found = "Group not found";
                send(new_socket, group_not_found, strlen(group_not_found), 0);
            }
        }
        else {
            const char* invalid_command = "Invalid command";
            send(new_socket, invalid_command, strlen(invalid_command), 0);
        }
        if(prompt=="/exit")
        {
           mtx.lock();
           online.erase(username);
           
           mtx.unlock();
           inform(username,online,-1);
        }
    }
    close(new_socket);  // Close the connection
}

int main() {
    parse();  // Parse users.txt to populate user credentials
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Server is listening on port " << PORT << std::endl;
    
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept");
            exit(EXIT_FAILURE);
        }
        
        std::thread(handle_client, new_socket).detach();  // Handle client in a new thread
    }
    
    
    close(server_fd);  // Close the server socket
    return 0;
}