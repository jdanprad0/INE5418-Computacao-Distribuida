#include "Utils.h"
#include <regex>
#include <mutex>

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
            case LogType::ERROR:
                std::cout << Constants::RED << "[ERROR] " << message;
                break;
            case LogType::INFO:
                std::cout << Constants::BLUE << "[INFO] " << message;
                break;
            case LogType::DISCOVERY_RECEIVED:
                std::cout << Constants::YELLOW << "[DISCOVERY_RECEIVED] " << message;
                break;
            case LogType::DISCOVERY_SENT:
                std::cout << Constants::MAGENTA << "[DISCOVERY_SENT] " << message;
                break;
            case LogType::RESPONSE:
                std::cout << Constants::GREEN << "[RESPONSE] " << message;
                break;
            default:
                std::cout << Constants::CIANO << "[OTHER] " << message;
                break;
        }
        std::cout << Constants::RESET << std::endl; // Reseta a cor do texto e finaliza a linha
    }
}
