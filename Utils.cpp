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
void logMessage(const std::string& type, const std::string& message) {
    std::cout << RESET;
    
    if (type == "ERROR") {
        std::cout << RED << "[" << type << "] " << message;
    } else if (type == "INFO") {
        std::cout << BLUE << "[" << type << "] " << message;
    } else if (type == "DISCOVERY_RECEIVED") {
        std::cout << YELLOW << "[" << type << "] " << message;
    } else if (type == "DISCOVERY_SENT") {
        std::cout << MAGENTA << "[" << type << "] " << message;
    } else if (type == "RESPONSE") {
        std::cout << GREEN << "[" << type << "] " << message;
    } else {
        std::cout << CIANO << "[" << type << "] " << message;
    }

    std::cout << RESET << std::endl;
}
