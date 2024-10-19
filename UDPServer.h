#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <tuple>
#include "FileManager.h"

/**
 * @brief Classe responsável pela comunicação UDP para descoberta de arquivos.
 */
class UDPServer {
private:
    std::string ip;                                                     ///< Endereço IP do peer.
    int port;                                                           ///< Porta UDP para descoberta.
    int peer_id;                                                        ///< ID do peer.
    int sockfd;                                                         ///< Socket UDP.
    FileManager& fileManager;                                           ///< Referência ao gerenciador de arquivos. 
    std::vector<std::tuple<int, std::string, int, int>> connections;    ///< Informações de peers para realizar conexões.
    std::map<std::string, std::set<std::string>> received_messages;     ///< Mapeia mensagens e IPs de origem.

public:
    /**
     * @brief Construtor da classe UDPServer.
     * @param ip Endereço IP do peer.
     * @param port Porta UDP para descoberta.
     * @param peer_id ID do peer.
     * @param fileManager Referência ao gerenciador de arquivos.
     */
    UDPServer(const std::string& ip, int port, int peer_id, FileManager& fileManager);

    /**
     * @brief Define informações para as conexões a serem feitas
     * @param neighbor_info Vizinhos do peer.
     */
    void setConnections(const std::vector<std::tuple<int, std::string, int, int>>& neighbor_info);

    /**
     * @brief Inicia o servidor UDP para receber mensagens de descoberta.
     */
    void run();

    /**
     * @brief Inicia o processo de descoberta de um arquivo.
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     * @param ttl Time-to-live para o flooding.
     */
    void initiateDiscovery(const std::string& file_name, int total_chunks, int ttl);

    /**
     * @brief Envia uma mensagem de descoberta para os vizinhos.
     * @param message Mensagem a ser enviada.
     * @param ttl Time-to-live para o flooding.
     * @param sender_ip IP do peer de origem.
     */
    void sendDiscoveryMessage(const std::string& message, int ttl, const std::string& sender_ip);
};

#endif // UDPSERVER_H
