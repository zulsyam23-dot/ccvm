/**
 * @brief Error reporting and diagnostics
 */

#include "ccvm/frontend/error_handler.h"
#include <iostream>

namespace ccvm {
namespace frontend {

void ErrorHandler::report(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
}

} // namespace frontend
} // namespace ccvm
