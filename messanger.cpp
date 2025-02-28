#include <winsock2.h>
#include <ws2tcpip.h>  
#include <windows.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")


#define _CRT_SECURE_NO_WARNINGS

// Глобальные переменные для элементов пользовательского интерфейса
HWND hEditMessage, hSendButton, hMessageList,hStaticImage;
SOCKET clientSocket;
HICON hIcon; 
// Прототипы функций
void InitializeWinsock();
SOCKET ConnectToServer(const char* serverIp, int port);
void SendMessageToServer(SOCKET sock, const char* message);
DWORD WINAPI ReceiveMessagesThread(LPVOID param);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
     
    InitializeWinsock();

    
    clientSocket = ConnectToServer("127.0.0.1", 54000); 
    if (clientSocket == INVALID_SOCKET) {
        MessageBox(NULL, L"Could not connect to server", L"Error", MB_OK);
        return 1;
    }

    
    const wchar_t CLASS_NAME[] = L"MessengerApp";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Messenger App", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 800, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        return 0;
    }
    
    HICON hIconSmall = (HICON)LoadImage(NULL, L"C:\\Users\\ayoub\\source\\repos\\messanger\\conversation_chat_deal_agreement_icon_124665.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    HICON hIconBig = (HICON)LoadImage(NULL, L"C:\\Users\\ayoub\\source\\repos\\messanger\\conversation_chat_deal_agreement_icon_124665.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);

    
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);  
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);     
    ShowWindow(hwnd, nCmdShow);

    
    CreateThread(NULL, 0, ReceiveMessagesThread, &clientSocket, 0, NULL);

    
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}


void InitializeWinsock() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        MessageBox(NULL, L"WSAStartup failed", L"Error", MB_OK);
        exit(1);
    }
}


SOCKET ConnectToServer(const char* serverIp, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        MessageBox(NULL, L"Socket creation failed", L"Error", MB_OK);
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    int result = connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        closesocket(sock);
        MessageBox(NULL, L"Connection to server failed", L"Error", MB_OK);
        return INVALID_SOCKET;
    }

    return sock;
}

// Отправить сообщение на сервер
void SendMessageToServer(SOCKET sock, const char* message) {
    send(sock, message, strlen(message), 0);
}

// Поток для получения сообщений
DWORD WINAPI ReceiveMessagesThread(LPVOID param) {
    SOCKET sock = *(SOCKET*)param;
    char buffer[512];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            
            wchar_t wBuffer[512];
            size_t convertedChars = 0;
            mbstowcs_s(&convertedChars, wBuffer, sizeof(wBuffer) / sizeof(wBuffer[0]), buffer, _TRUNCATE);
            SendMessage(hMessageList, LB_ADDSTRING, 0, (LPARAM)wBuffer);
        }
    }

    return 0;
}

// Оконная процедура для обработки событий пользовательского интерфейса
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        hEditMessage = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 10, 10, 300, 20, hwnd, NULL, NULL, NULL);
        hSendButton = CreateWindow(L"BUTTON", L"Имя Пользователя", WS_CHILD | WS_VISIBLE, 320, 10, 400, 20, hwnd, (HMENU)1, NULL, NULL);
        hMessageList = CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL, 10, 40, 370, 300, hwnd, NULL, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { 
            wchar_t message[256];
            GetWindowText(hEditMessage, message, 256);

            
            char messageToSend[256];
            size_t convertedChars = 0;
            wcstombs_s(&convertedChars, messageToSend, sizeof(messageToSend), message, _TRUNCATE);
            bool isFirstMessageSent = false;

            if (!isFirstMessageSent) {
               
                SendMessageToServer(clientSocket, messageToSend);
                isFirstMessageSent = true;  

                
                SetWindowText(hSendButton, L"Отправить");
            }
            else {
               
                SendMessageToServer(clientSocket, messageToSend);
            }

            
            SendMessage(hMessageList, LB_ADDSTRING, 0, (LPARAM)message);
            SetWindowText(hEditMessage, L"");
        }
        
        break;

    case WM_DESTROY:
        
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

