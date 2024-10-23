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
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Estrutura para armazenar informações do meu endereço do socket
    struct sockaddr_in my_addr = createSockAddr(ip.c_str(), port);

    // Faz o bind do socket à porta especificada
    if (bind(server_sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("Erro ao fazer bind no socket TCP");
        exit(EXIT_FAILURE);
    }

    // Coloca o socket em modo de escuta com até 5 por vez
    if (listen(server_sockfd, 10) < 0) {
        perror("Erro ao escutar no socket TCP");
        exit(EXIT_FAILURE);
    }

    logMessage(LogType::INFO, "Servidor TCP inicializado em " + ip + ":" + std::to_string(port));
}

/**
 * @brief Inicia o servidor TCP para aceitar conexões.
 */
void TCPServer::run() {
    while (true) {
        // Informações para socket do cliente
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        
        int client_sockfdfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sockfdfd >= 0) {
            // Thread para receber o chunk
            std::thread(&TCPServer::receiveChunk, this, client_sockfdfd).detach();
        } else {
            perror("Erro ao aceitar conexão TCP");
        }
    }
}

/**
 * @brief Recebe um chunk do cliente e o salva.
 */
void TCPServer::receiveChunk(int client_sockfd) {
    char* file_buffer = nullptr; // Inicializa um buffer para armazenar o chunk recebido
    size_t bytes_received; // Armazena o número real de bytes recebidos ao fazer a transferência do chunk

    // Recebe a informação que vai começar a transferência (esperando algo como "PUT <file_name> <chunk_id> <transfer_speed>")
    char request_buffer[256];
    ssize_t request_size = recv(client_sockfd, request_buffer, sizeof(request_buffer), 0);
    
    if (request_size <= 0) {
        if (request_size == 0) {
            logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
        } else {
            perror("Erro ao receber dados via TCP");
        }
        close(client_sockfd);
        return;
    }

    request_buffer[request_size] = '\0';  // Finaliza a string recebida (mensagem de controle)
    std::string request(request_buffer);

    std::stringstream ss(request);
    std::string command, file_name;
    int chunk_id;
    int transfer_speed;

    ss >> command >> file_name >> chunk_id >> transfer_speed;

    // Variáveis para armazenar o endereço IP e a porta do cliente
    struct sockaddr_in source_addr;
    socklen_t source_addr_len = sizeof(source_addr);
    
    // Obtém o IP e a porta do cliente
    if (getpeername(client_sockfd, (struct sockaddr*)&source_addr, &source_addr_len) == -1) {
        perror("Erro ao obter informações do cliente");
        close(client_sockfd);
        return;
    }

    // Identifica o IP do peer que enviou a mensagem
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(source_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    // Obtém a porta do peer
    int client_port = ntohs(source_addr.sin_port);

    if (command == "PUT") {
        // Cria um buffer com o tamanho da velocidade de transferência do cliente
        file_buffer = new char[transfer_speed];

        // Abre o arquivo em modo binário para escrita
        std::ofstream chunk_file(file_manager.getChunkPath(file_name, chunk_id), std::ios::binary);

        if (!chunk_file.is_open()) {
            logMessage(LogType::ERROR, "Não foi possível criar o arquivo para o chunk " + std::to_string(chunk_id));
            close(client_sockfd);
            delete[] file_buffer;
            return;
        }

        // Recebe o chunk em blocos
        while ((bytes_received = recv(client_sockfd, file_buffer, transfer_speed, 0)) > 0) {
            chunk_file.write(file_buffer, bytes_received);  // Grava os bytes recebidos no arquivo
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Espera 1 segundo para respeitar a velocidade de transferência
            logMessage(LogType::INFO, "Recebido " + std::to_string(bytes_received) + " bytes do chunk " + std::to_string(chunk_id) + " do arquivo " + file_name + " de " + client_ip + ":" + std::to_string(client_port));
        }

        if (bytes_received < 0) {
            perror("Erro ao receber o chunk.");
        } else {
            logMessage(LogType::SUCCESS, "SUCESSO AO RECEBER O CHUNK " + std::to_string(chunk_id) + " DO ARQUIVO " + file_name + " DE " + client_ip + ":" + std::to_string(client_port));
        }

        chunk_file.close(); // Fecha o arquivo após a transferência
    }

    close(client_sockfd);
    delete[] file_buffer;
}

/**
 * @brief Envia um ou mais chunks para o peer solicitante.
 */
void TCPServer::sendChunk(const std::string& file_name, const std::vector<int>& requested_chunks, const PeerInfo& destination_info) {
    // Cria um buffer com base na minha velocidade
    char* file_buffer = new char[transfer_speed];

    // Cria um novo socket para a conexão
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0) {
        perror("Erro ao criar socket.");
        return; // Retorna se houver erro na criação do socket
    }

    // Estrutura para armazenar informações do endereço do destinatário
    struct sockaddr_in destination_addr = createSockAddr(destination_info.ip.c_str(), destination_info.port + 1000);

    // Tenta se conectar ao destinatário
    if (connect(client_sockfd, (struct sockaddr*)&destination_addr, sizeof(destination_addr)) < 0) {
        perror("Erro ao conectar ao peer.");
        close(client_sockfd);
        return; // Retorna se não conseguir conectar
    }

    // Envia a mensagem de controle antes de transferir o chunk
    for (int chunk_id : requested_chunks) {
        std::stringstream ss;
        ss << "PUT " << file_name << " " << chunk_id << " " << transfer_speed; // Formato da mensagem

        std::string control_message = ss.str();
        ssize_t bytes_sent = send(client_sockfd, control_message.c_str(), control_message.size(), 0); // Envia a mensagem
        if (bytes_sent < 0) {
            perror("Erro ao enviar a mensagem de controle para envio de chunk.");
            close(client_sockfd);
            delete[] file_buffer;
            return; // Retorna se houver erro ao enviar a mensagem de controle
        }

        // Obtém o caminho do chunk
        std::string chunk_path = file_manager.getChunkPath(file_name, chunk_id);
        std::ifstream chunk_file(chunk_path, std::ios::binary); // Abre o arquivo em modo binário

        if (!chunk_file.is_open()) {
            logMessage(LogType::ERROR, "Chunk " + std::to_string(chunk_id) + "não encontrado.");
            continue; // Se não encontrar o chunk, continua para o próximo
        }

        // Envia o chunk em blocos, respeitando a minha velocidade de transferência
        while (chunk_file) {
            // Lê uma quantidade de bytes do chunk
            chunk_file.read(file_buffer, transfer_speed);  // Lê até o tamanho da minha velocidade

            // Obtém o número de bytes lidos
            size_t bytes_to_send = chunk_file.gcount(); // Número real de bytes lidos

            // Envia os bytes lidos
            ssize_t bytes_sent = send(client_sockfd, file_buffer, bytes_to_send, 0);
            if (bytes_sent < 0) {
                perror("Erro ao enviar o chunk");;
                logMessage(LogType::ERROR, "Um erro ocorreu ao tentar enviar o chunk " + std::to_string(chunk_id));
                break; // Interrompe se houver um erro no envio
            }

            logMessage(LogType::INFO, "Enviado " + std::to_string(bytes_sent) + " bytes do chunk " + std::to_string(chunk_id) + " do arquivo " + file_name + " para o IP " + destination_info.ip.c_str() + ":" + std::to_string(destination_info.port + 1000));

            // Espera por 1 segundo para dar o efeito de velocidade
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        logMessage(LogType::SUCCESS, "SUCESSO AO ENVIAR O CHUNK " + std::to_string(chunk_id) + " DO ARQUIVO " + file_name + " PARA " + destination_info.ip.c_str() + ":" + std::to_string(destination_info.port + 1000));

        // Fecha o arquivo após o envio
        chunk_file.close();
    }

    close(client_sockfd);
    delete[] file_buffer; // Libera a memória alocada para o buffer
}

