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

/**
 * @brief Construtor da classe UDPServer.
 */
UDPServer::UDPServer(const std::string& ip, int port, int peer_id, FileManager& fileManager)
    : ip(ip), port(port), peer_id(peer_id), fileManager(fileManager) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao fazer bind no socket UDP");
    }
}

/**
 * @brief Define informações para as conexões a serem feitas
 */
void UDPServer::setConnections(const std::vector<std::tuple<int, std::string, int, int>>& neighbor_info) {
    this->connections = neighbor_info;
}

/**
 * @brief Inicia o servidor UDP para receber mensagens de descoberta.
 */
void UDPServer::run() {
    char buffer[1024];
    struct sockaddr_in sender_addr{};
    socklen_t addr_len = sizeof(sender_addr);

    while (true) {
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                          (struct sockaddr*)&sender_addr, &addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            // Identifica o peer de origem usando apenas o IP
            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sender_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);

            std::string sender_ip_str(sender_ip);

            // Verifica se a mensagem já foi recebida do mesmo peer de origem (baseado no IP)
            if (received_messages.find(message) != received_messages.end()) {
                if (received_messages[message].find(sender_ip_str) != received_messages[message].end()) {
                    continue;  // Ignora a mensagem se já foi recebida deste IP de origem
                }
            }

            // Adiciona o IP de origem ao conjunto de peers que enviaram essa mensagem
            received_messages[message].insert(sender_ip_str);

            // Processa a mensagem
            std::stringstream ss(message);
            std::string command;
            ss >> command;

            if (command == "DISCOVERY") {
                std::string file_name;
                int total_chunks;
                int ttl;
                ss >> file_name >> total_chunks >> ttl;

                std::cout << "Peer " << peer_id << " recebeu pedido de descoberta do arquivo " << file_name << " com TTL " << ttl
                          << " de Peer " << sender_ip_str << std::endl;

                // Verifica se possui algum chunk do arquivo
                if (fileManager.hasChunk(file_name, 0)) { // Simplificado para chunk 0
                    std::cout << "Peer " << peer_id << " possui o arquivo " << file_name << "." << std::endl;
                    // Criar a conexão
                }

                // Propaga a mensagem para os vizinhos se o TTL for maior que zero
                if (ttl > 0) {
                    std::this_thread::sleep_for(std::chrono::seconds(1)); // Atraso de 1 segundo
                    sendDiscoveryMessage(message, ttl - 1, sender_ip_str);  // Agora passamos apenas o IP do peer de origem
                }
            }
        }
    }
}

/**
 * @brief Inicia o processo de descoberta de um arquivo.
 */
void UDPServer::initiateDiscovery(const std::string& file_name, int total_chunks, int ttl) {
    std::stringstream ss;
    ss << "DISCOVERY " << file_name << " " << total_chunks << " " << ttl;
    std::string message = ss.str();

    // Envia a mensagem para os vizinhos
    sendDiscoveryMessage(message, ttl, ip);
}

/**
 * @brief Envia uma mensagem de descoberta para os vizinhos.
 */
void UDPServer::sendDiscoveryMessage(const std::string& message, int ttl, const std::string& sender_ip) {
    for (const auto& [neighbor_id, neighbor_ip, neighbor_port, _] : connections) {
        struct sockaddr_in neighbor_addr{};
        neighbor_addr.sin_family = AF_INET;
        neighbor_addr.sin_addr.s_addr = inet_addr(neighbor_ip.c_str());
        neighbor_addr.sin_port = htons(neighbor_port);

        ssize_t bytes_sent = sendto(sockfd, message.c_str(), message.size(), 0,
                                    (struct sockaddr*)&neighbor_addr, sizeof(neighbor_addr));

        if (bytes_sent < 0) {
            perror("Erro ao enviar mensagem UDP");
        } else {
            std::cout << "Peer " << peer_id << " enviou mensagem para Peer " << neighbor_id << " de IP: " << neighbor_ip << std::endl;
        }
    }
}
