#include "Utils.h"
#include <regex>
#include <mutex>
#include <arpa/inet.h>
#include <cstring>

// Mutex para proteger a saída do console
std::mutex cout_mutex;

/**
 * @brief Remove espaços em branco ao redor de uma string.
 */
std::string trim(const std::string& str) {
    return std::regex_replace(str, std::regex("^\\s+|\\s+$"), "");
}

/**
 * @brief Função auxiliar para formatar e exibir mensagens de log de forma consistente, com cores.
 */
void logMessage(LogType type, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(cout_mutex); // Bloqueia o acesso à saída do console
        switch (type) {
            case LogType::DISCOVERY_RECEIVED:
                std::cout << Constants::YELLOW << "[DISCOVERY_RECEIVED] " << message;
                break;
            case LogType::DISCOVERY_SENT:
                std::cout << Constants::MAGENTA << "[DISCOVERY_SENT] " << message;
                break;
            case LogType::RESPONSE_RECEIVED:
                std::cout << Constants::CIANO << "[RESPONSE_RECEIVED] " << message;
                break;
            case LogType::RESPONSE_SENT:
                std::cout << Constants::GRAY << "[RESPONSE_SENT] " << message;
                break;
            case LogType::REQUEST_RECEIVED:
                std::cout << Constants::ORANGE << "[REQUEST_RECEIVED] " << message;
                break;
            case LogType::REQUEST_SENT:
                std::cout << Constants::PINK << "[REQUEST_SENT] " << message;
                break;
            case LogType::SUCCESS:
                std::cout << Constants::GREEN << "[SUCCESS] " << message;
                break;
            case LogType::INFO:
                std::cout << Constants::BLUE << "[INFO] " << message;
                break;
            case LogType::ERROR:
                std::cout << Constants::RED << "[ERROR] " << message;
                break;
            default:
                std::cout << Constants::ORANGE << "[OTHER] " << message;
                break;
        }
        std::cout << Constants::RESET << std::endl; // Reseta a cor do texto e finaliza a linha
    }
}

struct sockaddr_in createSockAddr(const std::string& ip, int port) {
    // Estrutura para armazenar informações do endereço do socket
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET; // Define a família de endereços como IPv4
    addr.sin_addr.s_addr = inet_addr(ip.c_str()); // Converte o endereço IP da string para o formato de rede
    addr.sin_port = htons(port); // Converte a porta para o formato de rede

    return addr; // Retorna a estrutura configurada
}