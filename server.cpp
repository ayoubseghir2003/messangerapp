#include <iostream>
#include <winsock2.h>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>
#include <string>   
#include <cstring>  
#pragma comment(lib, "ws2_32.lib")


std::vector<SOCKET> allClients;
std::map<std::string, SOCKET> clientSockets;

// Функция сохранения сообщений в файл
void saveMessageToFile(const std::string& username, const std::string& message) {
    std::ofstream file("chat_history.txt", std::ios::app); 
    if (file.is_open()) {
        file << username << ": " << message << std::endl;
    }
    else {
        std::cerr << "Could not open chat history file!" << std::endl;
    }
}

// Функция отправки предыдущих сообщений новому клиенту
void sendHistoryToClient(SOCKET clientSocket) {
    std::ifstream file("chat_history.txt");
    std::string line;
    while (std::getline(file, line)) {
        
        int messageSize = static_cast<int>(line.size());
        if (messageSize > 0) {
            send(clientSocket, line.c_str(), messageSize + 1, 0); 
        }
    }
}

// Функция для управления общением с клиентом
void handleClient(SOCKET clientSocket) {
    char username[1024] = { 0 };
    int bytesReceived = recv(clientSocket, username, 1024, 0);
    if (bytesReceived > 0) {
        std::string usernameStr(username);
        std::cout << usernameStr << " connected." << std::endl;

        
        clientSockets[usernameStr] = clientSocket;
        allClients.push_back(clientSocket);

        
        sendHistoryToClient(clientSocket);

        
        char buffer[1024] = { 0 };

        while (true) {
            ZeroMemory(buffer, 1024);
            bytesReceived = recv(clientSocket, buffer, 1024, 0);

            if (bytesReceived == SOCKET_ERROR) {
                std::cerr << "Error in recv(). Quitting" << std::endl;
                break;
            }

            if (bytesReceived == 0) {
                std::cout << usernameStr << " disconnected." << std::endl;
                break;
            }

            //Немедленно распечатайте полученное сообщение
            std::cout << usernameStr << ": " << buffer << std::endl;

            
            saveMessageToFile(usernameStr, buffer);

            
            for (SOCKET s : allClients) {
                if (s != clientSocket) {
                    std::string msgWithUsername = usernameStr + ": " + buffer;
                    send(s, msgWithUsername.c_str(), static_cast<int>(msgWithUsername.size() + 1), 0);
                }
            }
        }

        // Удалить клиента из карты и списка при отключении
        clientSockets.erase(usernameStr);
        allClients.erase(std::remove(allClients.begin(), allClients.end(), clientSocket), allClients.end());
    }

    
    closesocket(clientSocket);
}

int main() {
    
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    int wsOk = WSAStartup(ver, &wsData);
    if (wsOk != 0) {
        std::cerr << "Can't initialize Winsock! Quitting" << std::endl;
        return -1;
    }

    
    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET) {
        std::cerr << "Can't create a socket! Quitting" << std::endl;
        WSACleanup();
        return -1;
    }

    
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);  
    hint.sin_addr.S_un.S_addr = INADDR_ANY;  

    bind(listening, (sockaddr*)&hint, sizeof(hint));

    
    listen(listening, SOMAXCONN);

    
    sockaddr_in client;
    int clientSize = sizeof(client);

    while (true) {
        SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Can't accept client! Quitting" << std::endl;
            continue;
        }

        
        std::thread(handleClient, clientSocket).detach();
    }

    
    WSACleanup();
    return 0;
}
