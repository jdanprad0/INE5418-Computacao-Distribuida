#ifndef UDPSERVER_H
#define UDPSERVER_H

#include "FileManager.h"
#include "TCPServer.h"
#include "Utils.h"
#include "Constants.h"
#include <string>
#include <map>
#include <vector>
#include <set>
#include <tuple>
#include <unordered_map>
#include <mutex>

/**
 * @brief Classe responsável por gerenciar a comunicação UDP para descoberta de arquivos em uma rede P2P.
 * 
 * Esta classe implementa as funcionalidades de envio e recebimento de mensagens UDP 
 * relacionadas à descoberta de arquivos, assim como o processamento dessas mensagens. 
 * Ela interage com a classe FileManager para verificar e enviar os chunks de arquivos 
 * que o peer possui, bem como para descobrir arquivos na rede.
 */
class UDPServer {
private:
    const std::string ip;                                   ///< Endereço IP do peer atual.
    const int port;                                         ///< Porta UDP que o peer está utilizando para a comunicação.
    const int peer_id;                                      ///< Identificador único (ID) do peer.
    const int transfer_speed;                               ///< Velocidade de transferência de dados em bytes/segundo.
    int sockfd;                                             ///< Descriptor do socket UDP utilizado para a comunicação.
    std::vector<std::tuple<std::string, int>> udpNeighbors; ///< Lista contendo os vizinhos diretos do peer (endereços IP e portas UDP).
    std::map<std::string, bool> processing_active_map;      ///< Mapa para controlar o estado de processamento de cada arquivo. Mapeia file_name para processing_active.
    std::mutex processing_mutex;                            ///< Mutex para proteger o acesso ao mapa
    FileManager& file_manager;                              ///< Referência ao gerenciador de arquivos (armazenamento e chunks).
    TCPServer& tcp_server;                                  ///< Referência ao servidor TCP.

public:
    /**
     * @brief Construtor da classe UDPServer.
     * 
     * Inicializa o servidor UDP com o endereço IP, a porta e o ID do peer, e 
     * associa-o a um gerenciador de arquivos (`FileManager`), que é responsável por 
     * armazenar e gerenciar os arquivos e seus chunks.
     * 
     * @param ip Endereço IP do peer.
     * @param port Porta UDP usada para a comunicação.
     * @param peer_id ID do peer na rede P2P.
     * @param transfer_speed Velocidade de transferência de dados em bytes/segundo do peer na rede P2P.
     * @param file_manager Referência ao gerenciador de arquivos do peer.
     * @param tcp_server Referência ao servidor TCP do peer.
     */
    UDPServer(const std::string& ip, int port, int peer_id, int transfer_speed, FileManager& file_manager, TCPServer& tcp_server);

    /**
     * @brief Função para criar e configurar o socket UDP.
     * 
     * Esta função cria um socket UDP, configura o endereço e vincula o socket à porta especificada.
     * 
     */
    void initializeUDPSocket();

    /**
     * @brief Inicia o servidor UDP, permitindo que o peer receba e envie mensagens.
     * 
     * Essa função ativa o loop principal para o recebimento de mensagens UDP e 
     * encaminha as mensagens recebidas para o processamento adequado.
     */
    void run();

    /**
     * @brief Inicializa o recebimento de respostas para chunks de um arquivo específico.
     * 
     * Esta função configura o sistema para processar respostas de peers na rede P2P
     * que possuem chunks do arquivo identificado por file_name. Ao ser chamada, indica
     * que as respostas dos peers em detrimento as mensagens de descoberta devem ser
     * processadas. Quando desativada, o sistema interrompe o processamento dessas
     * respostas.
     * 
     * @param file_name O nome do arquivo para o qual as respostas dos peers serão processadas.
     */
    void initializeProcessingActive(std::string file_name);

    /**
     * @brief Processa uma mensagem recebida de outro peer.
     * 
     * A mensagem recebida será analisada e processada em uma nova thread para 
     * melhorar o desempenho e permitir a recepção simultânea de várias mensagens.
     * 
     * @param message A mensagem recebida.
     * @param direct_sender_info Informações sobre o peer que enviou diretamente a mensagem, incluindo seu endereço IP e porta UDP.
     */
    void processMessage(const std::string& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Processa uma mensagem de descoberta (DISCOVERY).
     * 
     * Esta função é responsável por processar mensagens DISCOVERY, que são enviadas 
     * por peers que estão buscando um arquivo na rede. A função extrai as informações 
     * da mensagem, verifica se o peer atual possui os chunks do arquivo solicitado e, 
     * caso positivo, envia uma resposta. Caso o TTL (Time-to-Live) ainda esteja válido, 
     * a mensagem é propagada para os vizinhos.
     * 
     * @param message Stream com os dados da mensagem DISCOVERY.
     * @param direct_sender_info Informações sobre o peer que enviou diretamente a mensagem, incluindo seu endereço IP e porta UDP.
     */
    void processChunkDiscoveryMessage(std::stringstream& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Processa uma mensagem de resposta (RESPONSE).
     * 
     * Esta função é responsável por processar as respostas recebidas após um peer enviar 
     * uma solicitação de descoberta de arquivo. Ela extrai as informações do peer que 
     * enviou a resposta positiva a sua mensagem de descoberta.
     * 
     * @param message Stream com os dados da mensagem RESPONSE.
     * @param direct_sender_info Informações sobre o peer que enviou diretamente a mensagem, incluindo seu endereço IP e porta UDP.
     */
    void processChunkResponseMessage(std::stringstream& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Processa a requisição de chunks recebida de outro peer.
     * 
     * Este método analisa a mensagem de requisição de chunks e inicia a transferência
     * dos chunks solicitados usando o servidor TCP associado.
     * 
     * @param message Stream com os dados da mensagem de requisição.
     * @param direct_sender_info Informações sobre o peer que enviou a requisição, incluindo seu endereço IP e porta UDP.
     */
    void processChunkRequestMessage(std::stringstream& message, const PeerInfo& direct_sender_info);

    /**
     * @brief Envia uma mensagem de descoberta (DISCOVERY) para todos os vizinhos.
     * 
     * Essa mensagem será usada para solicitar a localização de um arquivo específico na rede.
     * 
     * @param file_name Nome do arquivo que o peer deseja localizar.
     * @param total_chunks Número total de chunks que compõem o arquivo.
     * @param ttl Time-to-live para limitar o alcance do flooding.
     * @param chunk_requester_info Informações sobre o peer que solicitou os chuncks do arquivo, como seu endereço IP e porta UDP.
     * @param initial_discovery Indica se é a mensagem de descoberta inicial.
     */
    void sendChunkDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info);
    
    /**
     * @brief Envia uma resposta (RESPONSE) contendo os chunks disponíveis para um arquivo.
     * 
     * Após receber uma solicitação de descoberta, essa função envia uma resposta 
     * para o peer solicitante informando quais chunks estão disponíveis.
     * 
     * @param file_name Nome do arquivo solicitado.
     * @param chunk_requester_info Informações sobre o peer que solicitou os chuncks do arquivo, como seu endereço IP e porta UDP.
     */
    void sendChunkResponseMessage(const std::string& file_name, const PeerInfo& chunk_requester_info);

    /**
     * @brief Envia uma mensagem REQUEST para pedir chunks específicos de um arquivo a cada peer.
     * 
     * Esta função percorre o mapa chunks_by_peer e, para cada peer, envia uma mensagem UDP
     * no formato: "REQUEST file_name chunk1 chunk2 ... chunkn" para o endereço IP e porta do peer.
     * 
     * @param file_name O nome do arquivo cujos chunks estão sendo solicitados.
     * @param chunks_by_peer Mapa que associa cada peer (IP) a um par contendo a porta e os chunks que eles possuem.
     */
    void sendChunkRequestMessage(const std::string& file_name);

    /**
     * @brief Monta a mensagem de descoberta (DISCOVERY) para envio.
     * 
     * Constrói uma string formatada contendo as informações da mensagem DISCOVERY que será enviada 
     * aos vizinhos para a busca de um arquivo.
     * 
     * @param file_name Nome do arquivo a ser descoberto.
     * @param total_chunks Número total de chunks do arquivo.
     * @param ttl Time-to-live da mensagem DISCOVERY.
     * @param chunk_requester_info Informações sobre o peer que solicitou os chuncks do arquivo, como seu endereço IP e porta UDP.
     * @return String contendo a mensagem DISCOVERY formatada.
     */
    std::string buildChunkDiscoveryMessage(const std::string& file_name, int total_chunks, int ttl, const PeerInfo& chunk_requester_info) const;

    /**
     * @brief Constrói uma mensagem de resposta (RESPONSE) contendo os chunks disponíveis.
     * 
     * Cria uma string formatada com as informações de quais chunks estão disponíveis 
     * para o arquivo solicitado pelo peer.
     * 
     * @param file_name Nome do arquivo solicitado.
     * @param chunks_available Vetor com os IDs dos chunks disponíveis.
     * @return String contendo a mensagem RESPONSE formatada.
     */
    std::string buildChunkResponseMessage(const std::string& file_name, const std::vector<int>& chunks_available) const;

    /**
     * @brief Monta a mensagem REQUEST para chunks específicos de um arquivo.
     * 
     * Esta função cria a mensagem REQUEST no formato "REQUEST file_name chunk1 chunk2 ...".
     * 
     * @param file_name O nome do arquivo cujos chunks estão sendo solicitados.
     * @param chunks Lista de IDs dos chunks que estão sendo solicitados.
     * @return A string contendo a mensagem REQUEST montada.
     */
    std::string buildChunkRequestMessage(const std::string& file_name, const std::vector<int>& chunks) const;

    /**
     * @brief Espera por um tempo determinado pelas respostas e então desativa o processamento de respostas.
     * 
     * @param file_name Nome do arquivo para o qual as respostas estão sendo aguardadas.
     */
    void waitForResponses(const std::string& file_name);

    /**
     * @brief Função auxiliar que configura o endereço IP e porta e envia uma mensagem UDP.
     * 
     * Esta função configura o endereço (IP e porta) e envia uma mensagem UDP para o peer especificado.
     * 
     * @param ip O endereço IP do peer para o qual a mensagem será enviada.
     * @param port A porta UDP do peer para o qual a mensagem será enviada.
     * @param message A mensagem que será enviada.
     * @return O número de bytes enviados, ou um valor negativo em caso de erro.
     */
    ssize_t sendUDPMessage(const std::string& ip, int port, const std::string& message);

    /**
     * @brief Define os vizinhos para o peer atual.
     * 
     * Esta função é usada para configurar os peers vizinhos com quem este peer 
     * pode se comunicar diretamente via UDP.
     * 
     * @param neighbors Vizinhos do peer (IP e Porta).
     */
    void setUDPNeighbors(const std::vector<std::tuple<std::string, int>>& neighbors);

    /**
     * @brief Obtém o endereço IP e a porta do peer a partir de uma estrutura sockaddr_in.
     * 
     * Esta função recebe a estrutura `sockaddr_in` contendo as informações do peer (IP e porta)
     * e retorna uma tupla contendo o endereço IP e a porta.
     * 
     * @param sender_addr Estrutura sockaddr_in contendo as informações do peer.
     * @return std::tuple<std::string, int> Tupla contendo o endereço IP (string) e a porta (int).
     */
    std::tuple<std::string, int> getSenderAddressInfo(const sockaddr_in& sender_addr);
};

#endif // UDPSERVER_H
