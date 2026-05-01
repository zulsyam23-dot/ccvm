/**
 * @brief Error handler interface
 */

#ifndef CCVM_FRONTEND_ERROR_HANDLER_H
#define CCVM_FRONTEND_ERROR_HANDLER_H

#include <string>

namespace ccvm {
namespace frontend {

class ErrorHandler {
public:
    void report(const std::string& message);
};

} // namespace frontend
} // namespace ccvm

#endif
