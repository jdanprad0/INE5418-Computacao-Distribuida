#include "UDPServer.h"
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
 */
UDPServer::UDPServer(const std::string& ip, int port, int peer_id, FileManager& fileManager)
    : ip(ip), port(port), peer_id(peer_id), fileManager(fileManager) {
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao fazer bind no socket UDP");
    }
}

/**
 * @brief Define informações para as conexões a serem feitas
 */
void UDPServer::setUDPNeighbors(const std::vector<std::tuple<std::string, int>>& neighbors) {
    // Itera sobre os neighbors e extrai o IP e a porta
    for (const auto& neighbor : neighbors) {
        // Pega o primeiro elemento da tupla, que é o IP
        std::string neighbor_ip = std::get<0>(neighbor);

        // Pega o segundo elemento da tupla, que é a porta 
        int neighbor_port = std::get<1>(neighbor);

        // Adiciona o IP e a porta à lista de udpNeighbors
        this->udpNeighbors.emplace_back(neighbor_ip, neighbor_port);
    }
}

/**
 * @brief Inicia o servidor UDP para receber mensagens de descoberta.
 */
void UDPServer::run() {
    char buffer[1024];
    struct sockaddr_in sender_addr{};
    socklen_t addr_len = sizeof(sender_addr);

    while (true) {
        // Recebe a mensagem do socket UDP
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                          (struct sockaddr*)&sender_addr, &addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            // Identifica o IP do peer que enviou a mensagem
            char direct_sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sender_addr.sin_addr), direct_sender_ip, INET_ADDRSTRLEN);

            // Obtém a porta do peer que enviou a mensagem
            int direct_sender_port = ntohs(sender_addr.sin_port);

            // Converte o endereço IP para uma string
            PeerInfo direct_sender_info(std::string(direct_sender_ip), direct_sender_port);

            // Cria uma nova thread para processar a mensagem recebida
            std::thread(&UDPServer::processMessage, this, message, direct_sender_info).detach();
        }
    }
}

/**
 * @brief Processa a mensagem recebida em uma nova thread.
 */
void UDPServer::processMessage(const std::string& message, const PeerInfo& direct_sender_info) {
    // Processa a mensagem
    std::stringstream ss(message);
    std::string command;
    ss >> command;

    if (command == "DISCOVERY") {
        processDiscoveryMessage(ss, direct_sender_info);
    } else if (command == "RESPONSE") {
        processResponseMessage(ss, direct_sender_info);
    } else {
        std::cout << "Comando desconhecido recebido: " << command << "\n" << std::endl;
    }
}

/**
 * @brief Processa a mensagem DISCOVERY.
 */
void UDPServer::processDiscoveryMessage(std::stringstream& message, const PeerInfo& direct_sender_info) {
    std::string file_name, original_sender_ip_port, original_sender_ip;
    int total_chunks, ttl, original_sender_UDP_port;
    size_t colon_pos;

    // Extrai os dados da mensagem DISCOVERY
    message >> file_name >> total_chunks >> ttl >> original_sender_ip_port;

    // Separar o IP e a porta
    colon_pos = original_sender_ip_port.find(':');
    original_sender_ip = original_sender_ip_port.substr(0, colon_pos);
    original_sender_UDP_port = std::stoi(original_sender_ip_port.substr(colon_pos + 1));

    std::cout << "Eu " << ip << ":" << port << " recebi pedido de descoberta do arquivo " << file_name
              << " com TTL " << ttl << " do Peer " << direct_sender_info.ip << ":" << direct_sender_info.port
              << ". A resposta deve ser feita para o Peer " << original_sender_ip << ":" << original_sender_UDP_port << "\n" << std::endl;

    // Verifica se possui algum chunk do arquivo
    sendChunkResponse(file_name, original_sender_ip, original_sender_UDP_port);

    // Propaga a mensagem para os vizinhos se o TTL for maior que zero
    if (ttl > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Atraso de 1 segundo
        sendDiscoveryMessage(file_name, total_chunks, ttl - 1, original_sender_ip, original_sender_UDP_port);
    }
}

/**
 * @brief Processa a mensagem RESPONSE.
 */
void UDPServer::processResponseMessage(std::stringstream& ss, const PeerInfo& direct_sender_info) {
    std::string file_name;
    std::vector<int> chunks_received;

    // Extrai o nome do arquivo da mensagem RESPONSE
    ss >> file_name;

    // Extrai os chunks disponíveis da mensagem RESPONSE
    int chunk;
    while (ss >> chunk) {
        chunks_received.push_back(chunk);
    }

    // Exibe os chunks recebidos na mensagem de resposta
    std::cout << "Eu " << ip << ":" << port << " recebi a resposta do Peer " << direct_sender_info.ip << ":" << direct_sender_info.port << " para o arquivo " << file_name
              << ". Chunks disponíveis: ";

    for (const int& chunk : chunks_received) {
        std::cout << chunk << " ";
    }

    std::cout << "\n" << std::endl;
}

bool UDPServer::sendChunkResponse(const std::string& file_name, const std::string& requester_ip, int requester_UDP_port) {
    // Verifica quais chunks do arquivo o peer atual possui
    std::vector<int> chunks_available = fileManager.getAvailableChunks(file_name);  // Supõe-se que o FileManager tenha um método para obter os chunks disponíveis

    if (!chunks_available.empty()) {
        // Constroi a mensagem de resposta com os chunks disponíveis
        std::string response_message = buildChunkResponseMessage(file_name, chunks_available);

        // Envia a mensagem de resposta de volta para o peer solicitante
        struct sockaddr_in requester_addr{};
        requester_addr.sin_family = AF_INET;
        requester_addr.sin_addr.s_addr = inet_addr(requester_ip.c_str());
        requester_addr.sin_port = htons(requester_UDP_port);

        ssize_t bytes_sent = sendto(sockfd, response_message.c_str(), response_message.size(), 0,
                                    (struct sockaddr*)&requester_addr, sizeof(requester_addr));

        if (bytes_sent < 0) {
            perror("Erro ao enviar resposta UDP com chunks disponíveis.");

            return true;
        } else {
            // Imprime uma mensagem informando quais chunks foram enviados
            std::cout << "Eu " << ip << ":" << port << ", enviei a resposta para o Peer " << requester_ip << ":" << requester_UDP_port
                      << " com os chunks disponíveis do arquivo " << file_name << ": ";

            // Listando os chunks disponíveis
            for (const int& chunk : chunks_available) {
                std::cout << chunk << " ";
            }

            std::cout << "\n" << std::endl;
            return true;
        }
    } else {
        std::cout << "Eu " << ip << ":" << port << " não possuo chunks do arquivo " << file_name << "\n" << std::endl;
        return false;
    }
}

/**
 * @brief Monta a mensagem de descoberta de um arquivo.
 */
std::string UDPServer::buildDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, std::string original_sender_ip, int original_sender_UDP_port) const {
    std::stringstream ss;
    ss << "DISCOVERY " << file_name << " " << total_chunks << " " << ttl << " " << original_sender_ip << ":" << original_sender_UDP_port;
    return ss.str();
}

std::string UDPServer::buildChunkResponseMessage(const std::string& file_name, const std::vector<int>& chunks_available) const {
    std::stringstream ss;
    ss << "RESPONSE " << file_name << " ";  // Inicia a mensagem com "RESPONSE" e o nome do arquivo
    
    // Adiciona os chunks disponíveis na mensagem
    for (const int& chunk : chunks_available) {
        ss << chunk << " ";  // Adiciona o ID de cada chunk disponível
    }

    return ss.str();
}

/**
 * @brief Envia uma mensagem de descoberta para os vizinhos.
 */
void UDPServer::sendDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, std::string original_sender_ip, int original_sender_UDP_port) {
    std::string message = buildDiscoveryMessage(file_name, total_chunks, ttl, original_sender_ip, original_sender_UDP_port);

    for (const auto& [neighbor_ip, neighbor_port] : udpNeighbors) {
        struct sockaddr_in neighbor_addr;
        neighbor_addr.sin_family = AF_INET;
        neighbor_addr.sin_addr.s_addr = inet_addr(neighbor_ip.c_str());
        neighbor_addr.sin_port = htons(neighbor_port);

        ssize_t bytes_sent = sendto(sockfd, message.c_str(), message.size(), 0,
                        (struct sockaddr*)&neighbor_addr, sizeof(neighbor_addr));

        if (bytes_sent < 0) {
            perror("Erro ao enviar mensagem UDP");
        } else {
            std::cout << "Eu " << ip << ":" << port << " enviei a mensagem de descoberta <<" << message << ">>" << " para o Peer " << neighbor_ip << ":" << neighbor_port << "\n" << std::endl;
        }
        
    }
}
