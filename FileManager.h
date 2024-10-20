#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <set>

/**
 * @brief Classe responsável por gerenciar os arquivos e chunks do peer.
 */
class FileManager {
private:
    std::string peer_id;                                    ///< ID do peer (usado para diretório).
    std::map<std::string, std::set<int>> local_chunks;      ///< Chunks locais disponíveis.
    std::string directory;                                  ///< Diretório dos arquivos do peer.

public:
    /**
     * @brief Construtor da classe FileManager.
     * @param peer_id ID do peer.
     */
    FileManager(const std::string& peer_id);

    /**
     * @brief Carrega os chunks locais disponíveis.
     */
    void loadLocalChunks();

    /**
     * @brief Verifica se possui um chunk específico de um arquivo.
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return true se possuir o chunk, false caso contrário.
     */
    bool hasChunk(const std::string& file_name, int chunk);

    /**
     * @brief Retorna o caminho do chunk solicitado.
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @return Caminho completo do chunk.
     */
    std::string getChunkPath(const std::string& file_name, int chunk);

    /**
     * @brief Salva um chunk recebido no diretório do peer.
     * @param file_name Nome do arquivo.
     * @param chunk Número do chunk.
     * @param data Dados do chunk.
     * @param size Tamanho dos dados.
     */
    void saveChunk(const std::string& file_name, int chunk, const char* data, size_t size);

    /**
     * @brief Verifica se todos os chunks de um arquivo foram recebidos.
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     * @return true se todos os chunks foram recebidos, false caso contrário.
     */
    bool hasAllChunks(const std::string& file_name, int total_chunks);
    
    /**
     * @brief Retorna os chunks disponíveis para um arquivo específico.
     * @param file_name Nome do arquivo.
     * @return Vetor contendo os chunks disponíveis localmente.
     */
    std::vector<int> getAvailableChunks(const std::string& file_name);

    /**
     * @brief Concatena todos os chunks para formar o arquivo completo.
     * @param file_name Nome do arquivo.
     * @param total_chunks Total de chunks do arquivo.
     */
    void assembleFile(const std::string& file_name, int total_chunks);
};

#endif // FILEMANAGER_H
