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

    // Coloca o socket em modo de escuta
    if (listen(server_sockfd, Constants::TCP_MAX_PENDING_CONNECTIONS) < 0) {
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
        
        // Aceita a conexão do cliente
        int client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sockfd >= 0) {
            // Cria uma thread para lidar com o recebimento dos chunks
            std::thread(&TCPServer::receiveChunks, this, client_sockfd).detach();
        } else {
            perror("Erro ao aceitar conexão TCP");
        }
    }
}

/**
 * @brief Recebe um chunk do cliente e o salva.
 */
void TCPServer::receiveChunks(int client_sockfd) {
    if (!setSocketTimeout(client_sockfd, Constants::TCP_TIMEOUT_SECONDS)) {
        logMessage(LogType::INFO, "Não foi possível configurar o timeout no socket.");
    }

    // Obtém o IP e porta do cliente
    auto [client_ip, client_port] = getClientAddressInfo(client_sockfd);
    
    // Continua a leitura até o cliente fechar a conexão
    while (true) {
        // Armazena a mensagem de controle recebida
        std::string control_message = "";

        // Controle de quantos bytes da mensagem de controle foram recebidos
        size_t control_message_total_bytes_received = 0;

        // Quantidade de bytes recebidos ao esperar a totalidade da mensagem de controle
        ssize_t control_message_size = 0;

        // Recebe a mensagem de controle em pedaços
        do {
            // Buffer para armazenar os dados da mensagem de controle
            char control_message_buffer[Constants::TCP_CONTROL_MESSAGE_MAX_SIZE] = {0};

            // Recebe os dados
            control_message_size = recv(client_sockfd, control_message_buffer, Constants::TCP_CONTROL_MESSAGE_MAX_SIZE, 0);

            // Verifica se houve erro, timeout ou o cliente fechou a conexão
            if (control_message_size < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    logMessage(LogType::INFO, "Timeout ao aguardar a mensagem de controle.");
                } else {
                    perror("Erro ao receber a mensagem de controle");
                }
                close(client_sockfd);
                return;
            } else if (control_message_size == 0) {
                logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
                close(client_sockfd);
                return;
            }

            // Adiciona os bytes recebidos à mensagem de controle
            control_message.append(control_message_buffer, control_message_size);
            control_message_total_bytes_received += control_message_size;

        } while (control_message_total_bytes_received < Constants::TCP_CONTROL_MESSAGE_MAX_SIZE); // Continua recebendo até que a mensagem de controle esteja completa

        logMessage(LogType::INFO, "Mensagem de controle '" + control_message + "' recebida de " + client_ip + ":" + std::to_string(client_port));
        
        // Transforma a string da mensagem de controle em um stream para extração
        std::stringstream control_message_stream(control_message);

        // Variáveis para armazenar os valores da mensagem de controle
        std::string command, file_name;
        int chunk_id, transfer_speed;
        size_t chunk_size;

        // Extrai os valores da mensagem de controle
        control_message_stream >> command >> file_name >> chunk_id >> transfer_speed >> chunk_size;

        // Verifica se o comando é "PUT", que indica recebimento de chunk de arquivo
        if (command == "PUT") {
            // Cria um buffer para armazenar o chunk completo
            std::vector<char> chunk_buffer(chunk_size, 0);

            // Controle de quantos bytes do chunk foram recebidos
            size_t chunk_total_bytes_received = 0;

            // Continua recebendo o chunk até alcançar o tamanho esperado
            while (chunk_total_bytes_received < chunk_size) {
                // Quantidade de bytes recebidos ao esperar a totalidade do chunk
                ssize_t chunk_bytes_received = 0;

                // Buffer temporário para armazenar os dados recebidos
                std::vector<char> chunk_temp_buffer(transfer_speed, 0);

                // Recebe os dados do chunk
                chunk_bytes_received = recv(client_sockfd, chunk_temp_buffer.data(), transfer_speed, 0);

                // Verifica se houve erro, timeout ou o cliente fechou a conexão
                if (chunk_bytes_received < 0) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        logMessage(LogType::INFO, "Timeout ao aguardar o chunk " + std::to_string(chunk_id) + ".");
                    } else {
                        perror("Erro ao receber o chunk.");
                    }
                    close(client_sockfd);
                    return;
                } else if (chunk_bytes_received == 0) {
                    logMessage(LogType::INFO, "Conexão fechada pelo cliente.");
                    close(client_sockfd);
                    return;
                }

                if (chunk_bytes_received > 0) {
                    // Copia os dados recebidos para o buffer principal do chunk
                    std::copy(chunk_temp_buffer.begin(), chunk_temp_buffer.begin() + chunk_bytes_received, chunk_buffer.begin() + chunk_total_bytes_received);

                    // Atualiza o total de bytes recebidos
                    chunk_total_bytes_received += chunk_bytes_received;

                    logMessage(LogType::INFO, "Recebido " + std::to_string(chunk_bytes_received) + " bytes do chunk " + std::to_string(chunk_id) + " de " + client_ip + ":" + std::to_string(client_port) + " (" + std::to_string(chunk_total_bytes_received) + "/" + std::to_string(chunk_size) + " bytes).");
                }
            }

            // Verifica se todos os bytes esperados foram recebidos
            if (chunk_total_bytes_received >= chunk_size) {
                logMessage(LogType::SUCCESS, "SUCESSO AO RECEBER O CHUNK " + std::to_string(chunk_id) + " DO ARQUIVO " + file_name + " de " + client_ip + ":" + std::to_string(client_port));

                // Obtém o caminho/nome para salvar o chunk
                std::string directory = file_manager.getChunkPath(file_name, chunk_id);

                // Cria o arquivo em modo binário
                std::ofstream chunk_file(directory, std::ios::binary);
                
                // Verifica se o arquivo foi aberto corretamente
                if (!chunk_file.is_open()) {
                    logMessage(LogType::ERROR, "Não foi possível criar o arquivo para o chunk " + std::to_string(chunk_id));
                    break;
                }

                // Escreve o conteúdo do buffer no arquivo
                chunk_file.write(chunk_buffer.data(), chunk_buffer.size());

                // Fecha o arquivo após a escrita
                chunk_file.close();
            } else {
                logMessage(LogType::ERROR, "Falha ao receber o chunk " + std::to_string(chunk_id) + " de " + client_ip + ":" + std::to_string(client_port) + ". Bytes esperados: " + std::to_string(chunk_size) + ", recebidos: " + std::to_string(chunk_total_bytes_received));
            }
        }
    }

    // Fecha o socket após terminar
    close(client_sockfd);
}

/**
 * @brief Envia um ou mais chunks para o peer solicitante.
 */
void TCPServer::sendChunks(const std::string& file_name, const std::vector<int>& chunks, const PeerInfo& destination_info) {
    // Cria um novo socket para a conexão
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0) {
        perror("Erro ao criar socket.");
        return;
    }

    // Estrutura para armazenar informações do endereço do destinatário
    struct sockaddr_in destination_addr = createSockAddr(destination_info.ip.c_str(), destination_info.port);

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
        
        // Cria um buffer em memória para armazenar todos os bytes do arquivo
        std::vector<char> file_buffer(chunk_size);
        
        // Lê o arquivo inteiro para o buffer
        chunk_file.read(file_buffer.data(), chunk_size);

        // Fecha o arquivo após a leitura
        chunk_file.close();

        // Cria a mensagem de controle
        std::stringstream ss;
        ss << "PUT " << file_name << " " << chunk << " " << transfer_speed << " " << chunk_size;

        std::string control_message = ss.str();

        // Cria um buffer de tamanho fixo e preenche com 0s
        char control_message_buffer[Constants::TCP_CONTROL_MESSAGE_MAX_SIZE] = {0};

        // Copia a mensagem de controle para o buffer fixo
        std::memcpy(control_message_buffer, control_message.c_str(), std::min(control_message.size(), sizeof(control_message_buffer) - 1));

        // Adiciona um caractere nulo no final da mensagem para garantir o fim da string
        control_message_buffer[control_message.size()] = '\0';

        // Envia a mensagem de controle de tamanho fixo
        ssize_t bytes_sent = send(client_sockfd, control_message_buffer, Constants::TCP_CONTROL_MESSAGE_MAX_SIZE, 0); 

        if (bytes_sent < 0) {
            perror("Erro ao enviar a mensagem de controle.");
            break;
        }

        // Variáveis para o controle de envio
        size_t total_bytes_sent = 0;
        size_t bytes_to_send;

        // Envia o chunk em blocos, respeitando a velocidade de transferência
        while (total_bytes_sent < chunk_size) {
            // Calcula quantos bytes enviar no próximo bloco
            bytes_to_send = std::min(static_cast<size_t>(transfer_speed), chunk_size - total_bytes_sent);

            // Envia os bytes da estrutura em memória (file_buffer)
            ssize_t bytes_sent = send(client_sockfd, file_buffer.data() + total_bytes_sent, bytes_to_send, 0);

            if (bytes_sent < 0) {
                perror("Erro ao enviar o chunk.");
                break;
            }

            total_bytes_sent += bytes_sent;

            logMessage(LogType::CHUNK_SENT, "Enviado " + std::to_string(bytes_sent) + " bytes do chunk " + std::to_string(chunk) + " do arquivo " + file_name + " para " + destination_info.ip + ":" + std::to_string(destination_info.port));

            // Simula a velocidade de transferência
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        logMessage(LogType::SUCCESS, "SUCESSO AO ENVIAR O CHUNK " + std::to_string(chunk) + " DO ARQUIVO " + file_name + " para " + destination_info.ip + ":" + std::to_string(destination_info.port));
    }

    // Fecha o socket após enviar todos os chunks
    close(client_sockfd);
}

/**
 * @brief Obtém o endereço IP e a porta do cliente conectado via socket.
 */
std::tuple<std::string, int> TCPServer::getClientAddressInfo(int client_sockfd) {
    // Declara uma struct para armazenar o endereço do cliente
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Usa getpeername() para obter as informações do cliente conectado
    if (getpeername(client_sockfd, (struct sockaddr*)&client_addr, &addr_len) == 0) {
        // Buffer para armazenar o endereço IP do cliente
        char client_ip[INET_ADDRSTRLEN];

        // Converte o IP para uma string legível
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        // Obtém a porta do cliente
        int client_port = ntohs(client_addr.sin_port);

        // Retorna a tupla contendo o IP e a porta
        return std::make_tuple(std::string(client_ip), client_port);
    } else {
        // Se houver um erro ao obter as informações, retorna uma tupla com valores padrão
        perror("Erro ao obter IP e porta do cliente");
        return std::make_tuple("Erro", -1);
    }
}

/**
 * @brief Configura o timeout para operações de recebimento no socket.
 */
bool TCPServer::setSocketTimeout(int sockfd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("Erro ao configurar o timeout do socket");
        return false;
    }
    return true;
}