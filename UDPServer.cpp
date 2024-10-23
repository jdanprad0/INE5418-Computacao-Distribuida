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
UDPServer::UDPServer(const std::string& ip, int port, int peer_id, int transfer_speed, FileManager& file_manager, TCPServer& tcp_server)
    : ip(ip), port(port), peer_id(peer_id), transfer_speed(transfer_speed), file_manager(file_manager), tcp_server(tcp_server) {}

/**
 * @brief Função para criar e configurar o socket UDP.
 */
void UDPServer::initializeUDPSocket() {
    // Criação do socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do endereço para o bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                  // Define o tipo de endereço como IPv4.
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // Configura para aceitar conexões de qualquer endereço IP.
    addr.sin_port = htons(port);                // Converte e define a porta no formato de rede.

    // Vincula o socket à porta especificada
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao fazer bind no socket UDP");
        exit(EXIT_FAILURE);
    }

    logMessage(LogType::INFO, "Servidor UDP inicializado em " + ip + ":" + std::to_string(port));
}

/**
 * @brief Inicia o servidor UDP para receber mensagens.
 */
void UDPServer::run() {
    char buffer[1024];
    struct sockaddr_in sender_addr{};
    socklen_t addr_len = sizeof(sender_addr);

    initializeUDPSocket();

    logMessage(LogType::INFO, "Servidor UDP em execução... Aguardando mensagens...");

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
 */
void UDPServer::processMessage(const std::string& message, const PeerInfo& direct_sender_info) {
    std::stringstream ss(message);
    std::string command, file_name;
    ss >> command;

    if (command == "DISCOVERY") {
         processChunkDiscoveryMessage(ss, direct_sender_info);
    } else if (command == "RESPONSE") {
        {
            std::streampos pos_before_file_name = ss.tellg(); // Salva a posição antes de ler o file_name
            ss >> file_name;
            std::lock_guard<std::mutex> lock(processing_mutex);
            if (processing_active_map[file_name]) {
                ss.clear(); // Limpa qualquer flag de erro no stream
                ss.seekg(pos_before_file_name); // Volta para a posição antes de ler o file_name
                processChunkResponseMessage(ss, direct_sender_info);
            } else {
                logMessage(LogType::OTHER, "Mensagem RESPONSE recebida para " + file_name + ", mas o processamento está desativado.");
            }
        }
    }
    else if (command == "REQUEST") {
        processChunkRequestMessage(ss, direct_sender_info);
    }
    else {
        logMessage(LogType::ERROR, "Comando desconhecido recebido: " + command);
    }
}

/**
 * @brief Processa a mensagem DISCOVERY.
 */
void UDPServer::processChunkDiscoveryMessage(std::stringstream& message, const PeerInfo& direct_sender_info) {
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
        logMessage(LogType::DISCOVERY_RECEIVED,
                "Recebido pedido de descoberta do arquivo '" + file_name + "' com TTL " + std::to_string(ttl) +
                " do Peer " + direct_sender_info.ip + ":" + std::to_string(direct_sender_info.port) +
                ". Resposta será enviada para o Peer " + chunk_requester_ip + ":" + std::to_string(chunk_requester_port));

        // Monta um Peer Info do solicitante dos chunks do arquivo
        PeerInfo chunk_requester_info(std::string(chunk_requester_ip), chunk_requester_port);

        // Verifica se possui chunks do arquivo e envia a resposta
        sendChunkResponseMessage(file_name, chunk_requester_info);

        // Propaga a mensagem para os vizinhos se o TTL for maior que zero
        if (ttl > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Atraso de 1 segundo
            sendChunkDiscoveryMessage(file_name, total_chunks, ttl - 1, chunk_requester_info);
        }
    }
}

/**
 * @brief Processa a mensagem RESPONSE.
 */
void UDPServer::processChunkResponseMessage(std::stringstream& message, const PeerInfo& direct_sender_info) {
    std::string file_name;
    int transfer_speed;
    std::vector<int> chunks_received;

    // Extrai o nome do arquivo e os chunks disponíveis
    message >> file_name >> transfer_speed;

    int chunk;
    while (message >> chunk) {
        chunks_received.push_back(chunk);
    }

    std::stringstream chunks_ss;
    for (const int& chunk : chunks_received) {
        chunks_ss << chunk << " ";
    }

    // Armazena as respostas recebidas no mapa
    file_manager.storeChunkLocationInfo(file_name, chunks_received, direct_sender_info.ip, direct_sender_info.port, transfer_speed);

    logMessage(LogType::RESPONSE,
               "Recebida resposta do Peer " + direct_sender_info.ip + ":" + std::to_string(direct_sender_info.port) +
               " para o arquivo '" + file_name + "'. Chunks disponíveis: " + chunks_ss.str());
}

/**
 * @brief Processa a requisição de chunks recebida de outro peer.
 */
void UDPServer::processChunkRequestMessage(std::stringstream& message, const PeerInfo& direct_sender_info) {
    std::string file_name;
    std::vector<int> requested_chunks;
    int chunk_id;

    // Extrai o nome do arquivo
    message >> file_name;

    // Extrai os IDs dos chunks solicitados
    while (message >> chunk_id) {
        requested_chunks.push_back(chunk_id);
    }

    // Log da requisição recebida
    logMessage(LogType::INFO,
               "Recebida requisição de chunks do Peer " + direct_sender_info.ip + ":" + std::to_string(direct_sender_info.port) +
               " para o arquivo '" + file_name + "'. Chunks solicitados: " + std::to_string(requested_chunks.size()));

    // Inicia a transferência dos chunks solicitados via TCP
    tcp_server.sendChunk(file_name, requested_chunks, direct_sender_info);
}

/**
 * @brief Envia uma mensagem de descoberta para todos os vizinhos.
 */
void UDPServer::sendChunkDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info) {
    std::string message = buildChunkDiscoveryMessage(file_name, total_chunks, ttl, chunk_requester_info);

    for (const auto& [neighbor_ip, neighbor_port] : udpNeighbors) {
        // Usa a função sendUDPMessage para enviar a mensagem
        ssize_t bytes_sent = sendUDPMessage(neighbor_ip, neighbor_port, message);

        if (bytes_sent < 0) {
            perror("Erro ao enviar mensagem UDP");
        } else {
            logMessage(LogType::DISCOVERY_SENT,
                       "Mensagem de descoberta enviada para Peer " + neighbor_ip + ":" + std::to_string(neighbor_port) +
                       " -> " + message);
        }
    }
}

/**
 * @brief Envia uma resposta com os chunks disponíveis de um arquivo solicitado.
 */
bool UDPServer::sendChunkResponseMessage(const std::string& file_name, const PeerInfo& chunk_requester_info) {
    std::vector<int> chunks_available = file_manager.getAvailableChunks(file_name);

    if (!chunks_available.empty()) {
        std::string response_message = buildChunkResponseMessage(file_name, chunks_available);

        // Usa a função sendUDPMessage para enviar a mensagem
        ssize_t bytes_sent = sendUDPMessage(chunk_requester_info.ip, chunk_requester_info.port, response_message);

        if (bytes_sent < 0) {
            perror("Erro ao enviar resposta UDP com chunks disponíveis.");
            return false;
        }

        std::stringstream chunks_ss;
        for (const int& chunk : chunks_available) {
            chunks_ss << chunk << " ";
        }

        logMessage(LogType::INFO,
                   "Enviada resposta para o Peer " + chunk_requester_info.ip + ":" + std::to_string(chunk_requester_info.port) +
                   " com chunks disponíveis do arquivo '" + file_name + "': " + chunks_ss.str());
        return true;
    }

    logMessage(LogType::INFO, "Nenhum chunk disponível para o arquivo '" + file_name + "'");
    return false;
}

/**
 * @brief Envia uma mensagem REQUEST para pedir chunks específicos de um arquivo a cada peer.
 */
void UDPServer::sendChunkRequestMessage(const std::string& file_name) {
    auto chunks_by_peer = file_manager.selectPeersForChunkDownload(file_name);

    for (const auto& [peer_ip, peer_info] : chunks_by_peer) {
        int peer_port = peer_info.first;
        const std::vector<int>& chunks = peer_info.second;

        // Monta a mensagem REQUEST para pedir chunks específicos
        std::string request_message = buildChunkRequestMessage(file_name, chunks);

        // Usa a função sendUDPMessage para enviar a mensagem REQUEST
        ssize_t bytes_sent = sendUDPMessage(peer_ip, peer_port, request_message);

        if (bytes_sent < 0) {
            perror("Erro ao enviar mensagem UDP REQUEST de chunks");
        } else {
            logMessage(LogType::INFO, "Mensagem REQUEST enviada para " + peer_ip + ":" + std::to_string(peer_port) +
                       " -> " + request_message);
        }
    }
}

/**
 * @brief Monta a mensagem de descoberta de um arquivo.
 */
std::string UDPServer::buildChunkDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info) const {
    std::stringstream ss;
    ss << "DISCOVERY " << file_name << " " << total_chunks << " " << ttl << " " << chunk_requester_info.ip << ":" << chunk_requester_info.port;
    return ss.str();
}

/**
 * @brief Monta a mensagem de resposta com os chunks disponíveis.
 */
std::string UDPServer::buildChunkResponseMessage(const std::string& file_name, const std::vector<int>& chunks_available) const {
    std::stringstream ss;
    ss << "RESPONSE " << file_name << " " << transfer_speed << " ";  // Inicia a mensagem com LogType::RESPONSE e o nome do arquivo
    
    for (const int& chunk : chunks_available) {
        ss << chunk << " ";  // Adiciona o ID de cada chunk disponível
    }

    return ss.str();
}

/**
 * @brief Monta a mensagem REQUEST para pedir chunks específicos de um arquivo.
 */
std::string UDPServer::buildChunkRequestMessage(const std::string& file_name, const std::vector<int>& chunks) const {
    std::stringstream ss;
    ss << "REQUEST " << file_name << " ";
    
    for (const int& chunk : chunks) {
        ss << chunk << " ";
    }

    return ss.str();
}

/**
 * @brief Espera por um tempo determinado pelas respostas e então desativa o processamento de respostas.
 */
void UDPServer::waitForResponses(const std::string& file_name) {
    {
        std::lock_guard<std::mutex> lock(processing_mutex);
        processing_active_map[file_name] = true; // Marca como ativo antes de iniciar o timer
    }

    std::this_thread::sleep_for(std::chrono::seconds(Constants::RESPONSE_TIMEOUT_SECONDS)); // Aguarda o tempo de resposta

    {
        std::lock_guard<std::mutex> lock(processing_mutex);
        processing_active_map[file_name] = false; // Desativa o processamento para o file_name após o timeout
    }

    logMessage(LogType::INFO, "Processamento de mensagens RESPONSE desativado para o arquivo: " + file_name);
}

/**
 * @brief Função auxiliar que configura o endereço IP e porta e envia uma mensagem UDP.
 */
ssize_t UDPServer::sendUDPMessage(const std::string& ip, int port, const std::string& message) {    
    // Estrutura para armazenar informações do endereço do socket
    struct sockaddr_in peer_addr = createSockAddr(ip.c_str(), port);

    // Envia a mensagem UDP para o peer
    ssize_t bytes_sent = sendto(sockfd, message.c_str(), message.size(), 0,
                                (struct sockaddr*)&peer_addr, sizeof(peer_addr));

    return bytes_sent; // Retorna o número de bytes enviados ou -1 em caso de erro
}

/**
 * @brief Define os vizinhos para o peer atual.
 */
void UDPServer::setUDPNeighbors(const std::vector<std::tuple<std::string, int>>& neighbors) {
    for (const auto& neighbor : neighbors) {
        std::string neighbor_ip = std::get<0>(neighbor);
        int neighbor_port = std::get<1>(neighbor);

        udpNeighbors.emplace_back(neighbor_ip, neighbor_port);
    }
    logMessage(LogType::INFO, "Vizinhos configurados para o servidor UDP.");
}