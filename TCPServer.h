#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include "FileManager.h"

/**
 * @brief Classe responsável pela transferência de chunks via TCP.
 */
class TCPServer {
private:
    std::string ip;                                         ///< Endereço IP do peer.
    int port;                                               ///< Porta TCP para transferência.
    int speed;                                              ///< Capacidade de transferência (bytes/s).
    int sockfd;                                             ///< Socket TCP.
    FileManager& fileManager;                               ///< Referência ao gerenciador de arquivos.

public:
    /**
     * @brief Construtor da classe TCPServer.
     * @param ip Endereço IP do peer.
     * @param port Porta TCP para transferência.
     * @param speed Capacidade de transferência (bytes/s).
     * @param fileManager Referência ao gerenciador de arquivos.
     */
    TCPServer(const std::string& ip, int port, int speed, FileManager& fileManager);

    /**
     * @brief Inicia o servidor TCP para aceitar conexões.
     */
    void run();

    /**
     * @brief Transfere um chunk para o peer solicitante.
     * @param client_sock Socket do cliente.
     */
    void transferChunk(int client_sock);
};

#endif // TCPSERVER_H
