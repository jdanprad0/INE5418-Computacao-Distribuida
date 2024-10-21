#include "TCPServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>

/**
 * @brief Construtor da classe TCPServer.
 */
TCPServer::TCPServer(const std::string& ip, int port, int speed, FileManager& fileManager)
    : ip(ip), port(port), speed(speed), fileManager(fileManager) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao fazer bind no socket TCP");
    }

    if (listen(sockfd, 5) < 0) {
        perror("Erro ao escutar no socket TCP");
    }
}

/**
 * @brief Inicia o servidor TCP para aceitar conexões.
 */
void TCPServer::run() {
    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sock >= 0) {
            std::thread(&TCPServer::transferChunk, this, client_sock).detach();
        } else {
            perror("Erro ao aceitar conexão TCP");
        }
    }
}

/**
 * @brief Transfere um chunk para o peer solicitante.
 */
void TCPServer::transferChunk(int client_sock) {
    // Cria um buffer com o tamanho da velocidade (limite de transferência em bytes por segundo)
    char *file_buffer = new char[speed];  // Buffer dinâmico com o tamanho da velocidade
    size_t bytes_read;

    // Recebe o pedido do cliente (esperando algo como "GET <file_name> <chunk_number>")
    char request_buffer[256];
    ssize_t bytes_received = recv(client_sock, request_buffer, sizeof(request_buffer), 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            std::cout << "Conexão fechada pelo cliente." << "\n" << std::endl;
        } else {
            perror("Erro ao receber dados via TCP");
        }
        close(client_sock);
        delete[] file_buffer;  // Libera a memória alocada para o buffer
        return;
    }

    request_buffer[bytes_received] = '\0';  // Finaliza a string recebida
    std::string request(request_buffer);

    // Espera que o request seja no formato "GET <file_name> <chunk_number>"
    std::stringstream ss(request);
    std::string command, file_name;
    int chunk_number;
    ss >> command >> file_name >> chunk_number;

    if (command == "GET") {
        // Obtém o caminho para o chunk solicitado
        std::string chunk_path = fileManager.getChunkPath(file_name, chunk_number);
        std::ifstream chunk_file(chunk_path, std::ios::binary);  // Abre o arquivo em modo binário

        if (!chunk_file.is_open()) {
            std::cerr << "Chunk solicitado não encontrado." << std::endl;
            close(client_sock);
            delete[] file_buffer;  // Libera a memória alocada para o buffer
            return;
        }

        // Envia o chunk em blocos, respeitando a velocidade de transferência (tamanho do buffer)
        while ((bytes_read = chunk_file.readsome(file_buffer, speed)) > 0) {
            send(client_sock, file_buffer, bytes_read, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Dorme 1 segundo para respeitar a velocidade
        }

        chunk_file.close();
    }

    // Fecha o socket após a transferência
    close(client_sock);
    delete[] file_buffer;  // Libera a memória alocada para o buffer
}

