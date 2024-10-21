#include "UDPServer.h"
#include "Utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

/**
 * @brief Construtor da classe UDPServer.
 * 
 * Inicializa o servidor UDP, criando o socket e vinculando-o à porta especificada.
 * Em caso de erro, o programa é encerrado.
 */
UDPServer::UDPServer(const std::string& ip, int port, int peer_id, FileManager& fileManager)
    : ip(ip), port(port), peer_id(peer_id), fileManager(fileManager) {
    
    // Criação do socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do endereço para o bind
    struct sockaddr_in addr;               // Declara a estrutura para armazenar o endereço do socket.
    addr.sin_family = AF_INET;             // Define o tipo de endereço como IPv4.
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Configura para aceitar conexões de qualquer endereço IP.
    addr.sin_port = htons(port);           // Converte e define a porta no formato de rede.


    // Vincula o socket à porta especificada
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao fazer bind no socket UDP");
    }

    logMessage("INFO", "Servidor UDP inicializado em " + ip + ":" + std::to_string(port));
}

/**
 * @brief Define os vizinhos para o peer atual.
 * 
 * Adiciona os vizinhos (peers) com quem este peer se comunicará diretamente via UDP.
 */
void UDPServer::setUDPNeighbors(const std::vector<std::tuple<std::string, int>>& neighbors) {
    for (const auto& neighbor : neighbors) {
        std::string neighbor_ip = std::get<0>(neighbor);
        int neighbor_port = std::get<1>(neighbor);

        udpNeighbors.emplace_back(neighbor_ip, neighbor_port);
    }
    logMessage("INFO", "Vizinhos configurados para o servidor UDP.");
}

/**
 * @brief Inicia o servidor UDP para receber mensagens.
 * 
 * Esta função entra em um loop infinito que recebe mensagens UDP e as encaminha para o processamento
 * em uma nova thread.
 */
void UDPServer::run() {
    char buffer[1024];
    struct sockaddr_in sender_addr{};
    socklen_t addr_len = sizeof(sender_addr);

    logMessage("INFO", "Servidor UDP em execução... Aguardando mensagens...");

    while (true) {
        // Recebe a mensagem UDP
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                          (struct sockaddr*)&sender_addr, &addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            // Identifica o IP do peer que enviou a mensagem
            char direct_sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sender_addr.sin_addr), direct_sender_ip, INET_ADDRSTRLEN);

            // Obtém a porta do peer
            int direct_sender_port = ntohs(sender_addr.sin_port);

            // Cria uma instância de PeerInfo para armazenar o IP e a porta do remetente
            PeerInfo direct_sender_info(std::string(direct_sender_ip), direct_sender_port);

            // Cria uma nova thread para processar a mensagem recebida
            std::thread(&UDPServer::processMessage, this, message, direct_sender_info).detach();
        }
    }
}

/**
 * @brief Processa uma mensagem recebida de outro peer.
 * 
 * A mensagem é analisada e encaminhada para o processamento adequado com base no comando (DISCOVERY ou RESPONSE).
 */
void UDPServer::processMessage(const std::string& message, const PeerInfo& direct_sender_info) {
    std::stringstream ss(message);
    std::string command;
    ss >> command;

    if (command == "DISCOVERY") {
        processDiscoveryMessage(ss, direct_sender_info);
    } else if (command == "RESPONSE") {
        processResponseMessage(ss, direct_sender_info);
    } else {
        logMessage("ERROR", "Comando desconhecido recebido: " + command);
    }
}

/**
 * @brief Processa a mensagem DISCOVERY.
 * 
 * Extrai as informações da mensagem DISCOVERY, verifica se o peer atual possui chunks
 * do arquivo solicitado e, se sim, envia uma resposta. Caso contrário, propaga a mensagem.
 */
void UDPServer::processDiscoveryMessage(std::stringstream& message, const PeerInfo& direct_sender_info) {
    std::string file_name, chunk_requester_ip_port, chunk_requester_ip;
    int total_chunks, ttl, chunk_requester_port;
    size_t colon_pos;

    // Extrai os dados da mensagem DISCOVERY
    message >> file_name >> total_chunks >> ttl >> chunk_requester_ip_port;

    // Separa o IP e a porta do peer original
    colon_pos = chunk_requester_ip_port.find(':');
    chunk_requester_ip = chunk_requester_ip_port.substr(0, colon_pos);
    chunk_requester_port = std::stoi(chunk_requester_ip_port.substr(colon_pos + 1));

    // Só manda mensagem de descoberta de mensagens que não foi o próprio peer que enviou
    if (chunk_requester_ip != ip) {
        logMessage("DISCOVERY_RECEIVED",
                "Recebido pedido de descoberta do arquivo '" + file_name + "' com TTL " + std::to_string(ttl) +
                " do Peer " + direct_sender_info.ip + ":" + std::to_string(direct_sender_info.port) +
                ". Resposta será enviada para o Peer " + chunk_requester_ip + ":" + std::to_string(chunk_requester_port));

        // Monta um Peer Info do solicitante dos chuncks do arquivo
        PeerInfo chunk_requester_info(std::string(chunk_requester_ip), chunk_requester_port);

        // Verifica se possui chunks do arquivo e envia a resposta
        sendChunkResponse(file_name, chunk_requester_info);

        // Propaga a mensagem para os vizinhos se o TTL for maior que zero
        if (ttl > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Atraso de 1 segundo
            sendDiscoveryMessage(file_name, total_chunks, ttl - 1, chunk_requester_info);
        }
    }
}

/**
 * @brief Processa a mensagem RESPONSE.
 * 
 * Exibe os chunks disponíveis recebidos na mensagem de resposta.
 */
void UDPServer::processResponseMessage(std::stringstream& message, const PeerInfo& direct_sender_info) {
    std::string file_name;
    std::vector<int> chunks_received;

    // Extrai o nome do arquivo e os chunks disponíveis
    message >> file_name;
    int chunk;
    while (message >> chunk) {
        chunks_received.push_back(chunk);
    }

    std::stringstream chunks_ss;
    for (const int& chunk : chunks_received) {
        chunks_ss << chunk << " ";
    }

    logMessage("RESPONSE",
               "Recebida resposta do Peer " + direct_sender_info.ip + ":" + std::to_string(direct_sender_info.port) +
               " para o arquivo '" + file_name + "'. Chunks disponíveis: " + chunks_ss.str());
}

/**
 * @brief Envia uma mensagem de descoberta para todos os vizinhos.
 */
void UDPServer::sendDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info) {
    std::string message = buildDiscoveryMessage(file_name, total_chunks, ttl, chunk_requester_info);

    for (const auto& [neighbor_ip, neighbor_port] : udpNeighbors) {
        struct sockaddr_in neighbor_addr;

        // Configura o tipo de endereço (IPv4), endereço IP e porta do vizinho para o envio da mensagem UDP.
        neighbor_addr.sin_family = AF_INET;
        neighbor_addr.sin_addr.s_addr = inet_addr(neighbor_ip.c_str());
        neighbor_addr.sin_port = htons(neighbor_port);

        // Envia a mensagem UDP para o vizinho especificado.
        ssize_t bytes_sent = sendto(sockfd, message.c_str(), message.size(), 0,
                        (struct sockaddr*)&neighbor_addr, sizeof(neighbor_addr));

        if (bytes_sent < 0) {
            perror("Erro ao enviar mensagem UDP");
        } else {
            logMessage("DISCOVERY_SENT",
                       "Mensagem de descoberta enviada para Peer " + neighbor_ip + ":" + std::to_string(neighbor_port) +
                       " -> " + message);
        }
    }
}

/**
 * @brief Envia uma resposta com os chunks disponíveis de um arquivo solicitado.
 * 
 * Verifica se o peer possui chunks do arquivo solicitado e envia uma mensagem de resposta.
 */
bool UDPServer::sendChunkResponse(const std::string& file_name, const PeerInfo& chunk_requester_info) {
    std::vector<int> chunks_available = fileManager.getAvailableChunks(file_name);

    if (!chunks_available.empty()) {
        std::string response_message = buildChunkResponseMessage(file_name, chunks_available);

        struct sockaddr_in requester_addr{};

        // Configura o tipo de endereço (IPv4), endereço IP e porta do requester dos chuncks para o envio da mensagem UDP.
        requester_addr.sin_family = AF_INET;
        requester_addr.sin_addr.s_addr = inet_addr(chunk_requester_info.ip.c_str());
        requester_addr.sin_port = htons(chunk_requester_info.port);

        // Envia a mensagem UDP para o requester dos chuncks.
        ssize_t bytes_sent = sendto(sockfd, response_message.c_str(), response_message.size(), 0,
                                    (struct sockaddr*)&requester_addr, sizeof(requester_addr));

        if (bytes_sent < 0) {
            perror("Erro ao enviar resposta UDP com chunks disponíveis.");
            return false;
        }

        std::stringstream chunks_ss;
        for (const int& chunk : chunks_available) {
            chunks_ss << chunk << " ";
        }

        logMessage("INFO",
                   "Enviada resposta para o Peer " + chunk_requester_info.ip + ":" + std::to_string(chunk_requester_info.port) +
                   " com chunks disponíveis do arquivo '" + file_name + "': " + chunks_ss.str());
        return true;
    }

    logMessage("INFO", "Nenhum chunk disponível para o arquivo '" + file_name + "'");
    return false;
}

/**
 * @brief Monta a mensagem de descoberta de um arquivo.
 */
std::string UDPServer::buildDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info) const {
    std::stringstream ss;
    ss << "DISCOVERY " << file_name << " " << total_chunks << " " << ttl << " " << chunk_requester_info.ip << ":" << chunk_requester_info.port;
    return ss.str();
}

/**
 * @brief Monta a mensagem de resposta com os chunks disponíveis.
 */
std::string UDPServer::buildChunkResponseMessage(const std::string& file_name, const std::vector<int>& chunks_available) const {
    std::stringstream ss;
    ss << "RESPONSE " << file_name << " ";  // Inicia a mensagem com "RESPONSE" e o nome do arquivo
    
    for (const int& chunk : chunks_available) {
        ss << chunk << " ";  // Adiciona o ID de cada chunk disponível
    }

    return ss.str();
}
