#include <iostream>
#include <WinSock2.h>
#include <thread>
#include <vector>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

using namespace std;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999
#define BUFFER_SIZE 255

vector<SOCKET> clients;
mutex clients_mutex;

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int recv_size;
    string nickname;

    // Receive nickname from client
    recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (recv_size <= 0) {
        cout << "Failed to receive nickname from client. Error: " << WSAGetLastError() << endl;
        closesocket(client_socket);
        return;
    }
    buffer[recv_size] = '\0'; // Ensure null-terminated string
    nickname = buffer;
    cout << "Nickname client: " << nickname << endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (recv_size <= 0) {
            cout << "Client disconnected. Error: " << WSAGetLastError() << endl;
            closesocket(client_socket);
            clients_mutex.lock();
            clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end());
            clients_mutex.unlock();
            break;
        }
        buffer[recv_size] = '\0'; // Ensure null-terminated string
        cout << "Received from client " << nickname << ": " << buffer << endl;

        clients_mutex.lock();
        for (SOCKET s : clients) {
            send(s, (nickname + ": " + buffer).c_str(), nickname.length() + 2 + recv_size, 0);
        }
        clients_mutex.unlock();
    }
}

int main() {
    WSADATA ws;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
        cerr << "Failed to initialize Winsock. Error: " << WSAGetLastError() << endl;
        return EXIT_FAILURE;
    }
    cout << "Berhasil inisialisasi" << endl;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        cerr << "Failed to create socket. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        return EXIT_FAILURE;
    }
    cout << "Berhasil membuat socket" << endl;

    // Setup server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // Menggunakan IP address yang spesifik
    server_addr.sin_port = htons(SERVER_PORT);
    memset(&server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind failed. Error: " << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        return EXIT_FAILURE;
    }
    cout << "Berhasil melakukan bind" << endl;

    // Listen for incoming connections
    if (listen(server_socket, 1) == SOCKET_ERROR) {
        cerr << "Listen failed. Error: " << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    cout << "Menunggu koneksi..." << endl;

    while (true) {
        // Accept incoming connection   
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Accept failed. Error: " << WSAGetLastError() << endl;
            closesocket(server_socket);
            WSACleanup();
            return EXIT_FAILURE;
        }

        clients_mutex.lock();
        clients.push_back(client_socket);
        clients_mutex.unlock();

        thread t(handle_client, client_socket);
        t.detach();

        char* client_ip = inet_ntoa(client_addr.sin_addr);
        cout << "Client connected! IP: " << client_ip << endl;
    }

    // Cleanup
    closesocket(server_socket);
    WSACleanup();

    cout << "Press Enter to exit...";
    cin.get();

    return 0;
}
