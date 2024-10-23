#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "Constants.h"
#include <string>
#include <map>
#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>

/**
 * @brief Estrutura que armazena as informações sobre um peer que possui um chunk específico.
 * 
 * A estrutura `ChunkLocationInfo` guarda os dados essenciais para localizar um peer que possui um chunk específico,
 * como endereço IP, porta de comunicação e a velocidade de transferência oferecida. Esses dados são usados 
 * para otimizar o download dos chunks.
 */
struct ChunkLocationInfo {
    std::string ip;          ///< Endereço IP do peer que possui o chunk.
    int port;                ///< Porta do peer que possui o chunk.
    int transfer_speed;      ///< Velocidade de transferência do peer.

    /**
     * @brief Construtor da estrutura ChunkLocationInfo.
     * 
     * Inicializa os membros da estrutura com os valores fornecidos ou, caso não sejam fornecidos, 
     * usa valores padrão.
     * 
     * @param ip Endereço IP do peer que possui o chunk (padrão: string vazia).
     * @param port Porta do peer que possui o chunk (padrão: 0).
     * @param transfer_speed Velocidade de transferência do peer (padrão: 0).
     */
    ChunkLocationInfo(const std::string& ip = "", int port = 0, int transfer_speed = 0)
        : ip(ip), port(port), transfer_speed(transfer_speed) {}
};

/**
 * @brief A classe `FileManager` é responsável pela gestão dos arquivos e chunks disponíveis para um peer em uma rede P2P.
 * 
 * Esta classe oferece funcionalidades para armazenar e gerenciar chunks de arquivos locais, permitindo que o peer 
 * verifique rapidamente quais chunks estão disponíveis e quais precisam ser baixados. Além disso, a `FileManager`
 * mantém informações detalhadas sobre a localização de chunks em outros peers, facilitando o processo de 
 * download. A classe também possui métodos para selecionar de forma eficiente os peers de onde os chunks 
 * ausentes podem ser adquiridos, garantindo uma transferência de dados otimizada.
 */
class FileManager {
private:
    std::string peer_id;  
    ///< ID do peer.

    std::map<std::string, std::set<int>> local_chunks;
    ///< Mapa que armazena os chunks locais disponíveis para cada arquivo.
    ///< A chave é o nome do arquivo (std::string).
    ///< O valor é um conjunto (std::set<int>) contendo os IDs dos chunks que o peer já possui para aquele arquivo.

    std::string directory;  
    ///< Diretório responsável pelo armazenamento dos arquivos do peer, incluindo o local onde novos chunks serão salvos.

    std::unordered_map<std::string, std::vector<std::vector<ChunkLocationInfo>>> chunk_location_info;
    ///< Mapa que armazena informações sobre os peers que possuem cada chunk de um arquivo.
    ///< A chave é o nome do arquivo (std::string).
    ///< O valor é um vetor onde cada índice representa um chunk do arquivo.
    ///< Cada índice contém um vetor de `ChunkLocationInfo`, onde cada `ChunkLocationInfo` descreve um peer
    ///< que possui o chunk, incluindo seu IP, porta e velocidade de transferência.

    std::unordered_map<std::string, std::vector<std::mutex>> chunk_location_info_mutex;
    ///< Mutex para garantir acesso seguro ao mapa de informações dos peers que possuem chunks de arquivos. Há um mutex para cada chunk.

public:
    /**
     * @brief Construtor da classe FileManager.
     * 
     * Inicializa um novo `FileManager` atribuindo um ID único ao peer e configurando o diretório
     * onde os chunks de arquivos serão armazenados localmente. O ID do peer é usado para montar o
     * diretório final.
     * 
     * @param peer_id ID do peer.
     */
    FileManager(const std::string& peer_id);

    /**
     * @brief Carrega os chunks locais disponíveis.
     * 
     * Essa função verifica o diretório do peer e escaneia os arquivos de chunks presentes.
     * A função atualiza a lista de chunks que o peer já possui localmente, facilitando o gerenciamento
     * e verificação dos chunks disponíveis.
     */
    void loadLocalChunks();

    /**
     * @brief Verifica se possui um chunk específico de um arquivo.
     * 
     * Essa função verifica se o peer já possui um chunk específico de um determinado arquivo
     * em seu armazenamento local.
     * 
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return true se possuir o chunk, false caso contrário.
     */
    bool hasChunk(const std::string& file_name, int chunk);

    /**
     * @brief Retorna o caminho do chunk solicitado.
     * 
     * Retorna o caminho absoluto no sistema de arquivos onde um chunk específico está armazenado.
     * Isso permite que o chunk seja localizado e transferido, se necessário.
     * 
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return Caminho completo do chunk.
     */
    std::string getChunkPath(const std::string& file_name, int chunk);

    /**
     * @brief Salva um chunk recebido no diretório do peer.
     * 
     * Salva os dados recebidos de um chunk no diretório designado do peer. O chunk é gravado
     * no sistema de arquivos para que o peer possa armazená-lo e acessá-lo mais tarde.
     * 
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @param data Dados do chunk.
     * @param size Tamanho dos dados.
     */
    void saveChunk(const std::string& file_name, int chunk, const char* data, size_t size);

    /**
     * @brief Verifica se todos os chunks de um arquivo foram recebidos.
     * 
     * Essa função verifica se o peer já baixou todos os chunks de um arquivo específico.
     * Isso é útil para saber quando o download de um arquivo foi totalmente concluído.
     * 
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     * @return true se todos os chunks foram recebidos, false caso contrário.
     */
    bool hasAllChunks(const std::string& file_name, int total_chunks);

    /**
     * @brief Retorna os chunks disponíveis para um arquivo específico.
     * 
     * Essa função retorna uma lista de chunks que já estão disponíveis localmente para um determinado arquivo,
     * permitindo que o peer verifique quais partes do arquivo já foram baixadas.
     * 
     * @param file_name Nome do arquivo.
     * @return Vetor contendo os chunks disponíveis localmente.
     */
    std::vector<int> getAvailableChunks(const std::string& file_name);

    /**
     * @brief Concatena todos os chunks para formar o arquivo completo.
     * 
     * Combina todos os chunks de um arquivo que foram baixados para formar o arquivo original.
     * Essa função é chamada após todos os chunks terem sido recebidos.
     * 
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     */
    void assembleFile(const std::string& file_name, int total_chunks);

    /**
     * @brief Inicializa a estrutura para armazenar informações sobre onde encontrar cada chunk.
     * 
     * Esta função prepara o `chunk_location_info` para armazenar as informações dos peers
     * que possuem cada chunk do arquivo. Ela cria uma entrada no mapa para cada chunk do arquivo,
     * onde as informações dos peers que possuem esse chunk serão armazenadas.
     * 
     * @param file_name O nome do arquivo ao qual o chunk pertence.
     * @param total_chunks O número total de chunks do arquivo que precisam ser inicializados.
     */
    void initializeChunkLocationInfo(const std::string& file_name, int total_chunks);

    /**
     * @brief Armazena informações de chunks recebidos para um arquivo específico.
     * 
     * Insere as informações de um chunk recebido no mapa `chunk_location_info`.
     * Essas informações incluem o IP, porta e velocidade de transferência do peer que possui o chunk.
     * A função usa mutexes para garantir que múltiplas threads possam acessar o mapa com segurança.
     * 
     * @param file_name O nome do arquivo associado aos chunks.
     * @param chunk_ids Uma lista de IDs dos chunks que foram recebidos.
     * @param ip O endereço IP do peer que enviou a resposta.
     * @param port A porta do peer que enviou a resposta.
     * @param transfer_speed A velocidade de transferência do peer.
     */
    void storeChunkLocationInfo(const std::string& file_name, const std::vector<int>& chunk_ids, const std::string& ip, int port, int transfer_speed);

    /**
     * @brief Seleciona os peers de onde os chunks serão baixados, mantendo a porta no valor.
     * 
     * Esta função escolhe quais peers (baseados no IP) serão usados para baixar os chunks do arquivo
     * especificado, e retorna um mapa onde a chave é o IP do peer e o valor é um par contendo a porta
     * do peer e a lista de chunks que serão baixados daquele peer.
     * 
     * @param file_name O nome do arquivo cujos chunks serão baixados.
     * @return Mapa onde a chave é o IP do peer e o valor é um par contendo a porta e a lista de chunks a serem baixados daquele peer.
     */
    std::unordered_map<std::string, std::pair<int, std::vector<int>>> selectPeersForChunkDownload(const std::string& file_name);
};

#endif // FILEMANAGER_H
