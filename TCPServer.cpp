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
TCPServer::TCPServer(const std::string& ip, int port, int peer_id, int transfer_speed, FileManager& file_manager)
    : ip(ip), port(port), peer_id(peer_id), transfer_speed(transfer_speed), file_manager(file_manager) {
    
    // Cria um socket TCP para escuta
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Estrutura para armazenar informações do endereço do socket
    struct sockaddr_in addr = createSockAddr(ip.c_str(), port);

    // Faz o bind do socket à porta especificada
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao fazer bind no socket TCP");
        exit(EXIT_FAILURE);
    }

    // Coloca o socket em modo de escuta com até 5 por vez
    if (listen(sockfd, 5) < 0) {
        perror("Erro ao escutar no socket TCP");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Inicia o servidor TCP para aceitar conexões.
 */
void TCPServer::run() {
    while (true) {
        // Informações para socket do cliente
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        
        int client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sock >= 0) {
            // Thread para receber o chunk
            std::thread(&TCPServer::receiveChunk, this, client_sock).detach();
        } else {
            perror("Erro ao aceitar conexão TCP");
        }
    }
}

/**
 * @brief Recebe um chunk do cliente e o salva.
 */
void TCPServer::receiveChunk(int client_sock) {
    // Inicializa um buffer para armazenar o chunk recebido
    char* file_buffer = nullptr;
    size_t bytes_received;

    // Recebe a informação que vai começar a transferência (esperando algo como "PUT <file_name> <chunk_number> <transfer_speed>")
    char request_buffer[256];
    ssize_t request_size = recv(client_sock, request_buffer, sizeof(request_buffer), 0);
    
    if (request_size <= 0) {
        if (request_size == 0) {
            logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
        } else {
            perror("Erro ao receber dados via TCP");
        }
        close(client_sock);
        return;
    }

    request_buffer[request_size] = '\0';  // Finaliza a string recebida
    std::string request(request_buffer);

    std::stringstream ss(request);
    std::string command, file_name;
    int chunk_number;
    int transfer_speed;

    ss >> command >> file_name >> chunk_number >> transfer_speed;

    if (command == "PUT") {
        // Cria um buffer com o tamanho da velocidade de transferência do cliente
        file_buffer = new char[transfer_speed];  // Buffer dinâmico com o tamanho da velocidade

        // Abre o arquivo em modo binário para escrita
        std::string directory = Constants::BASE_PATH + std::to_string(peer_id);  // Corrigido para usar peer_id diretamente
        std::ofstream chunk_file(directory, std::ios::binary);

        if (!chunk_file.is_open()) {
            logMessage(LogType::ERROR, "Não foi possível criar o arquivo para o chunk.");
            close(client_sock);
            delete[] file_buffer;
            return;
        }

        // Recebe o chunk em blocos
        while ((bytes_received = recv(client_sock, file_buffer, transfer_speed, 0)) > 0) {
            chunk_file.write(file_buffer, bytes_received);  // Grava os bytes recebidos no arquivo
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Espera 1 segundo para respeitar a velocidade de transferência
        }

        if (bytes_received < 0) {
            perror("Erro ao receber o chunk.");
        }

        chunk_file.close(); // Fecha o arquivo após a transferência
    }

    close(client_sock);
    delete[] file_buffer;
}

/**
 * @brief Envia um ou mais chunks para o peer solicitante.
 */
void TCPServer::sendChunk(const std::string& file_name, const std::vector<int>& requested_chunks, const PeerInfo& direct_sender_info) {
    // Cria um buffer com base na minha velocidade
    char* file_buffer = new char[transfer_speed];

    // Cria um novo socket para a conexão
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Erro ao criar socket.");
    }

    // Estrutura para armazenar informações do endereço do socket
    struct sockaddr_in peer_addr = createSockAddr(direct_sender_info.ip.c_str(), direct_sender_info.port);

    // Lógica para enviar os chunks solicitados
    for (int chunk_id : requested_chunks) {
        // Obtém o caminho do chunk
        std::string chunk_path = file_manager.getChunkPath(file_name, chunk_id);
        std::ifstream chunk_file(chunk_path, std::ios::binary); // Abre o arquivo em modo binário

        if (!chunk_file.is_open()) {
            logMessage(LogType::ERROR, "Chunk solicitado não encontrado.");
            continue; // Se não encontrar o chunk, continue para o próximo
        }

        // Conecta ao peer solicitante
        if (connect(client_sock, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
            perror("Erro ao conectar ao peer.");
            close(client_sock); // Fecha o socket se a conexão falhar
            continue; // Continua para o próximo chunk
        }

        // Envia o chunk em blocos, respeitando a velocidade de transferência
        while (chunk_file) {
            // Lê uma quantidade de bytes do chunk
            chunk_file.read(file_buffer, transfer_speed);  // Lê até o tamanho da minha velocidade

            // Obtém o número de bytes lidos
            size_t bytes_to_send = chunk_file.gcount(); // Número real de bytes lidos

            // Envia os bytes lidos
            ssize_t bytes_sent = send(client_sock, file_buffer, bytes_to_send, 0);
            if (bytes_sent < 0) {
                perror("Erro ao enviar o chunk.");
                break; // Interrompe se houver um erro no envio
            }

            // Espera por 1 segundo para dar o efeito de velocidade
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        // Fecha o arquivo após o envio
        chunk_file.close();
    }

    close(client_sock);
    delete[] file_buffer;
}
