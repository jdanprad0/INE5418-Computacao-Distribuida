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
            std::thread(&TCPServer::receiveChunks, this, client_sockfdfd).detach();
        } else {
            perror("Erro ao aceitar conexão TCP");
        }
    }
}

/**
 * @brief Recebe um chunk do cliente e o salva.
 */
void TCPServer::receiveChunks(int client_sockfd) {
    // Cria um buffer com ponteiro nulo para receber os chunks
    char* file_buffer = nullptr;

    // Cria uma variável que recebe quantos bytes eu recebi na comunicação TCP
    size_t bytes_received;
    
    // Cria um buffer para receber a mensagem de controle
    char request_buffer[256];

    // Continua a leitura até o cliente fechar a conexão
    while (true) {
        // Recebe a mensagem de controle
        ssize_t request_size = recv(client_sockfd, request_buffer, sizeof(request_buffer), 0);
        
        if (request_size <= 0) {
            if (request_size == 0) {
                logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
            } else {
                perror("Erro ao receber dados via TCP");
            }
            break; // Encerra o loop
        }
        
        // Coloca um marcador de final de string no final do conteúdo do buffer
        request_buffer[request_size] = '\0';

        // Passa o conteúdo do buffer para a string request
        std::string request(request_buffer);

        // Transforma a string request em um stream de string para leitura
        std::stringstream ss(request);

        // Variáveis para receber o conteúdo da string
        std::string command, file_name;
        int chunk_id, transfer_speed;
        size_t chunk_size;

        // Pega cada conteúdo da mensagem de controle atribuindo às variáveis
        ss >> command >> file_name >> chunk_id >> transfer_speed >> chunk_size;

        // Verifica se é uma mensagem que indica recebimento de chunk
        if (command == "PUT") {
            // Redimenciona o tamanho do buffer com base na velocidade de transferência do cliente
            file_buffer = new char[transfer_speed];

            // Cria e abre um arquivo para o chunk em modo binário para escrita
            std::ofstream chunk_file(file_manager.getChunkPath(file_name, chunk_id), std::ios::binary);

            // Verifica se o arquivo foi encontrado/aberto
            if (!chunk_file.is_open()) {
                logMessage(LogType::ERROR, "Não foi possível criar o arquivo para o chunk " + std::to_string(chunk_id));
                delete[] file_buffer;
                break;
            }

            // Controle de quantos bytes já foram recebidos
            size_t total_bytes_received = 0;

            // Recebe o chunk até que o número total de bytes seja alcançado
            while (total_bytes_received < chunk_size && (bytes_received = recv(client_sockfd, file_buffer, transfer_speed, 0)) > 0) {
                chunk_file.write(file_buffer, bytes_received);
                total_bytes_received += bytes_received;

                logMessage(LogType::INFO, "Recebido " + std::to_string(bytes_received) + " bytes do chunk " + std::to_string(chunk_id) + " (" + std::to_string(total_bytes_received) + "/" + std::to_string(chunk_size) + " bytes).");

                // Utilizado para simular a velocidade de transferência
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            if (total_bytes_received == chunk_size) {
                logMessage(LogType::SUCCESS, "SUCESSO AO RECEBER O CHUNK " + std::to_string(chunk_id) + " DO ARQUIVO " + file_name);
            } else {
                logMessage(LogType::ERROR, "Falha ao receber o chunk " + std::to_string(chunk_id) + ". Bytes esperados: " + std::to_string(chunk_size) + ", recebidos: " + std::to_string(total_bytes_received));
            }

            // Fecha o arquivo e libera o buffer
            chunk_file.close();
            delete[] file_buffer;
        }
    }

    // Fecha o socket do cliente
    close(client_sockfd);
}


/**
 * @brief Envia um ou mais chunks para o peer solicitante.
 */
void TCPServer::sendChunks(const std::string& file_name, const std::vector<int>& chunks, const PeerInfo& destination_info) {
    // Cria um buffer com base na minha velocidade
    char* file_buffer = new char[transfer_speed];

    // Cria um novo socket para a conexão
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0) {
        perror("Erro ao criar socket.");
        return;
    }

    // Estrutura para armazenar informações do endereço do destinatário
    struct sockaddr_in destination_addr = createSockAddr(destination_info.ip.c_str(), destination_info.port + 1000);

    // Tenta se conectar ao destinatário
    if (connect(client_sockfd, (struct sockaddr*)&destination_addr, sizeof(destination_addr)) < 0) {
        perror("Erro ao conectar ao peer.");
        close(client_sockfd);
        return;
    }

    // Itera sobre os chunks e envia um a um
    for (int chunk : chunks) {
        // Obtém o caminho do chunk
        std::string chunk_path = file_manager.getChunkPath(file_name, chunk);

        // Abre o arquivo em modo binário e posiciona o cursor no final para obter o tamanho
        std::ifstream chunk_file(chunk_path, std::ios::binary | std::ios::ate);
        
        // Verifica se o arquivo foi encontrado/aberto
        if (!chunk_file.is_open()) {
            logMessage(LogType::ERROR, "Chunk " + std::to_string(chunk) + " não encontrado.");
            continue;  // Pula para o próximo chunk
        }

        // Obtém o tamanho do chunk
        size_t chunk_size = chunk_file.tellg();

        // Volta para o início do arquivo
        chunk_file.seekg(0);

        // Envia a mensagem de controle antes de transferir o chunk
        std::stringstream ss;

        // Formato da mensagem
        // PUT <arquivo> <chunk> <velocidade> <tamanho do chunk>
        // Exemplo: PUT file.txt 0 200 10240000
        ss << "PUT " << file_name << " " << chunk << " " << transfer_speed << " " << chunk_size;

        // Transforma a mensagem em uma string
        std::string control_message = ss.str();

        // Envia a mensagem de controle
        ssize_t bytes_sent = send(client_sockfd, control_message.c_str(), control_message.size(), 0); // Envia a mensagem de controle
        
        if (bytes_sent < 0) {
            perror("Erro ao enviar a mensagem de controle.");
            break;
        }

        // Envia o chunk em blocos, respeitando a velocidade de transferência
        while (chunk_file) {
            chunk_file.read(file_buffer, transfer_speed);
            size_t bytes_to_send = chunk_file.gcount();

            bytes_sent = send(client_sockfd, file_buffer, bytes_to_send, 0);
            if (bytes_sent < 0) {
                perror("Erro ao enviar o chunk.");
                break;
            }

            logMessage(LogType::INFO, "Enviado " + std::to_string(bytes_sent) + " bytes do chunk " + std::to_string(chunk) + " do arquivo " + file_name);

            // Utilizado para simular a velocidade de transferência
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        chunk_file.close();
        logMessage(LogType::SUCCESS, "SUCESSO AO ENVIAR O CHUNK " + std::to_string(chunk) + " DO ARQUIVO " + file_name);
    }

    close(client_sockfd);
    delete[] file_buffer;
}
