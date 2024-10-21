#include "FileManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>

/**
 * @brief Construtor da classe FileManager.
 */
FileManager::FileManager(const std::string& peer_id) : peer_id(peer_id) {
    directory = "./src/" + peer_id;
}

/**
 * @brief Carrega os chunks locais disponíveis.
 */
void FileManager::loadLocalChunks() {
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
            int chunk_number = std::stoi(filename.substr(pos + 3));
            local_chunks[file_name].insert(chunk_number);
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
