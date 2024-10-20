#ifndef PEER_H
#define PEER_H

#include <string>
#include <map>
#include <vector>
#include "FileManager.h"
#include "ConfigManager.h"
#include "UDPServer.h"
#include "TCPServer.h"

/**
 * @brief Classe que representa um peer na rede P2P.
 */
class Peer {
private:
    int id;                                                             ///< Identificador do peer.
    std::string ip;                                                     ///< Endereço IP do peer.
    int udp_port;                                                       ///< Porta UDP para descoberta de arquivos.
    int tcp_port;                                                       ///< Porta TCP para transferência de chunks.
    int transfer_speed;                                                 ///< Capacidade de transferência em bytes/s.
    std::vector<std::tuple<std::string, int>> neighbors;                ///< Informações sobre sua vizinhança.
    FileManager fileManager;                                            ///< Gerenciador de arquivos do peer.
    UDPServer udpServer;                                                ///< Servidor UDP para descoberta de arquivos.
    TCPServer tcpServer;                                                ///< Servidor TCP para transferência de chunks.

public:
    /**
     * @brief Construtor da classe Peer.
     * @param id Identificador do peer.
     * @param ip Endereço IP do peer.
     * @param udp_port Porta UDP para descoberta de arquivos.
     * @param tcp_port Porta TCP para transferência de chunks.
     * @param transfer_speed Capacidade de transferência em bytes/s.
     * @param transfer_speed Informações dos vizinhos.
     */
    Peer(int id, const std::string& ip, int udp_port, 
     int tcp_port, int transfer_speed, 
     std::vector<std::tuple<std::string, int>> neighbors);

    /**
     * @brief // Método que configura informações para conexões UDP
     */
    void loadUDPConnections();

    /**
     * @brief Inicia os servidores UDP e TCP.
     */
    void start();

    /**
     * @brief Inicia a busca por um arquivo na rede.
     * @param metadata_file Nome do arquivo de metadados (.p2p).
     */
    void searchFile(const std::string& metadata_file);
};

#endif // PEER_H
