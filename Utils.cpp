#include "Utils.h"
#include <regex>

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
    switch (type) {
        case LogType::ERROR:
            std::cout << RED << "[ERROR] " << message << RESET << std::endl << std::flush;
            break;
        case LogType::INFO:
            std::cout << BLUE << "[INFO] " << message << RESET << std::endl << std::flush;
            break;
        case LogType::DISCOVERY_RECEIVED:
            std::cout << YELLOW << "[DISCOVERY_RECEIVED] " << message << RESET << std::endl << std::flush;
            break;
        case LogType::DISCOVERY_SENT:
            std::cout << MAGENTA << "[DISCOVERY_SENT] " << message << RESET << std::endl << std::flush;
            break;
        case LogType::RESPONSE:
            std::cout << GREEN << "[RESPONSE] " << message << RESET << std::endl << std::flush;
            break;
        default:
            std::cout << CIANO << "[OTHER] " << message << RESET << std::endl << std::flush;
            break;
    }
}