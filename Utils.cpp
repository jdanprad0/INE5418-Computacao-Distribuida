#include "Utils.h"
#include <regex>

/**
 * @brief Remove espaços em branco ao redor de uma string.
 */
std::string trim(const std::string& str) {
    return std::regex_replace(str, std::regex("^\\s+|\\s+$"), "");
}