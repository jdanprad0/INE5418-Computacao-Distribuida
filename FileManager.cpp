#include "FileManager.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>

/**
 * @brief Construtor da classe FileManager.
 */
FileManager::FileManager(const std::string& peer_id) : peer_id(peer_id) {}

/**
 * @brief Carrega os chunks locais disponíveis.
 */
void FileManager::loadLocalChunks() {
    // Setando o diretório dos arquivos
    directory = Constants::BASE_PATH + peer_id;

    namespace fs = std::filesystem;

    if (!fs::exists(directory)) {
        fs::create_directory(directory);
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        std::string filename = entry.path().filename().string();

        // Formato esperado: <nome>.ch<chunk>
        size_t pos = filename.find(".ch");
        if (pos != std::string::npos) {
            std::string file_name = filename.substr(0, pos);
            int chunk_id = std::stoi(filename.substr(pos + 3));
            local_chunks[file_name].insert(chunk_id);
        }
    }
}

/**
 * @brief Verifica se possui um chunk específico de um arquivo.
 */
bool FileManager::hasChunk(const std::string& file_name, int chunk) {
    return local_chunks[file_name].find(chunk) != local_chunks[file_name].end();
}

/**
 * @brief Retorna o caminho do chunk solicitado.
 */
std::string FileManager::getChunkPath(const std::string& file_name, int chunk) {
    return directory + "/" + file_name + ".ch" + std::to_string(chunk);
}

/**
 * @brief Salva um chunk recebido no diretório do peer.
 */
void FileManager::saveChunk(const std::string& file_name, int chunk, const char* data, size_t size) {
    std::string path = getChunkPath(file_name, chunk);
    std::ofstream outfile(path, std::ios::binary);
    outfile.write(data, size);
    outfile.close();

    local_chunks[file_name].insert(chunk);
}

/**
 * @brief Verifica se todos os chunks de um arquivo foram recebidos.
 */
bool FileManager::hasAllChunks(const std::string& file_name, int total_chunks) {
    return local_chunks[file_name].size() == static_cast<size_t>(total_chunks);
}

/**
 * @brief Retorna os chunks disponíveis para um arquivo específico.
 */
std::vector<int> FileManager::getAvailableChunks(const std::string& file_name) {
    std::vector<int> available_chunks;

    // Verifica se o arquivo está no mapa de chunks locais
    if (local_chunks.find(file_name) != local_chunks.end()) {
        // Copia os chunks disponíveis para o vetor
        available_chunks.assign(local_chunks[file_name].begin(), local_chunks[file_name].end());
    }

    return available_chunks;
}

/**
 * @brief Concatena todos os chunks para formar o arquivo completo.
 */
void FileManager::assembleFile(const std::string& file_name, int total_chunks) {
    std::string output_path = directory + "/" + file_name;
    std::ofstream output_file(output_path, std::ios::binary);

    for (int i = 0; i < total_chunks; ++i) {
        std::string chunk_path = getChunkPath(file_name, i);
        std::ifstream chunk_file(chunk_path, std::ios::binary);

        if (!chunk_file.is_open()) {
            std::cerr << "Erro ao abrir o chunk: " << chunk_path << std::endl;
            return;
        }

        output_file << chunk_file.rdbuf();
        chunk_file.close();
    }

    output_file.close();
    std::cout << "Arquivo " << file_name << " montado com sucesso!" << "\n" << std::endl;
}

/**
 * @brief Inicializa a estrutura para armazenar informações sobre onde encontrar cada chunk.
 */
void FileManager::initializeChunkLocationInfo(const std::string& file_name, int total_chunks) {
    // Verifica se já existe uma entrada para o file_name
    if (chunk_location_info.find(file_name) == chunk_location_info.end()) {
        // Inicializa a lista de chunks, onde cada chunk_id contém um vetor vazio de ChunkLocationInfo
        chunk_location_info[file_name].resize(total_chunks); // Inicializa com vetores vazios para cada chunk
    }
}

/**
 * @brief Inicializa o vetor de mutexes para cada chunk de um arquivo.
 */
void FileManager::initializeChunkMutexes(const std::string& file_name, int total_chunks) {
    // Verifica se o file_name já existe no mapa
    if (chunk_location_info_mutex.find(file_name) == chunk_location_info_mutex.end()) {
        // Se não existir, inicializa um vetor de mutexes com o número total de chunks
        chunk_location_info_mutex[file_name] = std::vector<std::mutex>(total_chunks);
    }
}


/**
 * @brief Armazena informações de chunks recebidos para um arquivo específico.
 */
void FileManager::storeChunkLocationInfo(const std::string& file_name, const std::vector<int>& chunk_ids, const std::string& ip, int port, int transfer_speed) {
    for (const int chunk_id : chunk_ids) {
        // Verifica se o chunk_id está dentro do intervalo (tamanho da estrutura)
        if (static_cast<size_t>(chunk_id) < chunk_location_info[file_name].size()) {
            {
                // Bloqueia o acesso ao mapa para evitar condições de corrida
                std::lock_guard<std::mutex> lock(chunk_location_info_mutex[file_name][chunk_id]);

                // Verifica se o peer já existe na lista de ChunkLocationInfo
                auto& chunk_list = chunk_location_info[file_name][chunk_id];
                bool peer_exists = std::any_of(chunk_list.begin(), chunk_list.end(), 
                                            [&](const ChunkLocationInfo& cli) {
                                                return cli.ip == ip && cli.port == port;
                                            });

                // Se o peer não existir, adiciona à lista
                if (!peer_exists) {
                    chunk_list.emplace_back(ip, port, transfer_speed);
                }
            }
        } else {
            logMessage(LogType::ERROR, "chunk_id " + std::to_string(chunk_id) + " está fora do intervalo para o arquivo: " + file_name);
        }
    }
}

/**
 * @brief Seleciona os peers de onde os chunks serão baixados.
 */
std::unordered_map<std::string, std::vector<int>> FileManager::selectPeersForChunkDownload(const std::string& file_name) {
    std::unordered_map<std::string, std::vector<int>> chunks_by_peer;

    std::size_t total_chunks = chunk_location_info[file_name].size();

    // Itera pelos chunks que precisam ser baixados
    for (std::size_t chunk = 0; chunk < total_chunks; ++chunk) {
        // Verifica se há peers que têm o chunk atual
        if (chunk < total_chunks) {
            const std::vector<ChunkLocationInfo>& responses = chunk_location_info[file_name][chunk];

            // Se houver ao menos uma resposta, selecione o primeiro peer
            if (!responses.empty()) {
                const ChunkLocationInfo& selected_peer = responses[0]; // Seleciona o primeiro peer da lista
                
                // Cria a chave no formato "IP:Port"
                std::ostringstream peer_key;
                peer_key << selected_peer.ip << ":" << selected_peer.port;
                
                // Verifica se o peer já existe no mapa chunks_by_peer
                if (chunks_by_peer.find(peer_key.str()) != chunks_by_peer.end()) {
                    // Se já existir, apenas adiciona o chunk à lista de chunks desse peer
                    chunks_by_peer[peer_key.str()].push_back(static_cast<int>(chunk));
                } else {
                    // Se não existir, cria uma nova entrada para esse peer
                    chunks_by_peer[peer_key.str()] = std::vector<int>{static_cast<int>(chunk)};
                }
            }
        }
    }
    return chunks_by_peer;
}