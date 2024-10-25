#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "FileManager.h"
#include "Utils.h"
#include <string>

/**
 * @brief Definição da struct PeerInfo.
 * 
 * Esta struct armazena as informações de um peer, especificamente seu endereço IP
 * e a porta UDP utilizada para comunicação.
 */
struct PeerInfo {
    std::string ip;  ///< Endereço IP do peer.
    int port;        ///< Porta UDP do peer.

    PeerInfo(const std::string& ip, int port) : ip(ip), port(port) {}
};

/**
 * @brief Classe responsável pela transferência de chunks via TCP.
 * 
 * Esta classe gerencia as operações de transferência de dados de chunks de arquivos
 * entre peers em uma rede P2P utilizando o protocolo TCP. Ela é responsável por 
 * aceitar conexões de clientes, bem como enviar e receber chunks de arquivos.
 */
class TCPServer {
private:
    const std::string ip;                                   ///< Endereço IP do peer.
    const int port;                                         ///< Porta TCP para transferência.
    const int peer_id;                                      ///< Identificador único (ID) do peer.
    const int transfer_speed;                               ///< Capacidade de transferência.
    int server_sockfd;                                      ///< Socket TCP para aceitar conexões.
    FileManager& file_manager;                              ///< Referência ao gerenciador de arquivos.

public:
    /**
     * @brief Construtor da classe TCPServer.
     * 
     * Inicializa um servidor TCP com as informações do peer, incluindo o endereço IP,
     * porta, capacidade de transferência e uma referência ao gerenciador de arquivos.
     * 
     * @param ip Endereço IP do peer.
     * @param port Porta TCP para transferência.
     * @param peer_id ID do peer na rede P2P.
     * @param transfer_speed Capacidade de transferência.
     * @param file_manager Referência ao gerenciador de arquivos para acessar os chunks disponíveis.
     */
    TCPServer(const std::string& ip, int port, int peer_id, int transfer_speed, FileManager& file_manager);

    /**
     * @brief Inicia o servidor TCP para aceitar conexões.
     * 
     * Este método cria um socket TCP e aguarda conexões de peers que desejam
     * transferir chunks. As transferências são gerenciadas em threads separadas
     * para permitir múltiplas transferências simultâneas.
     */
    void run();

    /**
     * @brief Recebe um chunk enviado por um peer.
     * 
     * Este método recebe dados de um chunk de um cliente que está conectado ao servidor.
     * Ele armazena o chunk no diretório designado do peer.
     * 
     * @param client_sockfd Socket do cliente conectado.
     */
    void receiveChunks(int client_sockfd);

    /**
     * @brief Transfere um ou mais chunks para o peer solicitante.
     * 
     * Este método é responsável por enviar chunks específicos de um arquivo para um cliente que
     * estabeleceu uma conexão com o servidor. Os chunks são recuperados do gerenciador de arquivos
     * e enviados através do socket do cliente.
     * 
     * @param file_name Nome do arquivo cujos chunks estão sendo solicitados.
     * @param chunks Lista com os IDs dos chunks que devem ser transferidos.
     * @param destination_info Informações sobre o peer que está solicitando os chunks, incluindo seu endereço IP e porta UDP (Porta TCP = Porta UDP + 1000).
     */
    void sendChunks(const std::string& file_name, const std::vector<int>& chunks, const PeerInfo& destination_info);

    /**
     * @brief Obtém o endereço IP e a porta do cliente conectado via socket.
     * 
     * Esta função utiliza o descritor de socket do cliente para obter o endereço IP e a porta
     * através de `getpeername()`. Ela retorna essas informações em uma tupla contendo o IP 
     * como `std::string` e a porta como `int`.
     * 
     * @param client_sockfd Descritor de socket do cliente conectado.
     * @return std::tuple<std::string, int> Tupla contendo o endereço IP (string) e a porta (int).
     */
    std::tuple<std::string, int> getClientAddressInfo(int client_sockfd);

    /**
     * @brief Configura o timeout para operações de recebimento no socket.
     * 
     * @param sockfd Descritor do socket.
     * @param seconds Número de segundos para o timeout.
     * @return true Se a configuração foi bem-sucedida.
     * @return false Se houve um erro ao configurar o timeout.
     */
    bool setSocketTimeout(int sockfd, int seconds);
};

#endif // TCPSERVER_H
