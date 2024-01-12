#pragma once

#include "Logger.hpp"
#include "Parser.hpp"

namespace CTinyC {
    struct CodeMetadata {
        struct FuncMetadata {
            std::string name;
            size_t offset;
        };

        std::vector<FuncMetadata> func_meta;
    };

    struct CodeGenerator {
        CodeGenerator(Logger* logger) : m_logger(logger) {}

        std::pair<std::vector<uint8_t>, CodeMetadata> ast_to_code(ASTN const& root_node, int start_offset);

    private:
        Logger* m_logger;
    };
}
