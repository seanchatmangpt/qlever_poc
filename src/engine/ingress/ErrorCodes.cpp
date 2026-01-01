// ErrorCodes.cpp - Implementation stub
// Error code definitions and utilities

#include "ErrorCodes.h"

namespace qlever::ingress {

// Error code validation function (verifies enum completeness)
bool is_valid_error_code(uint16_t code) noexcept {
  // Check if code is within valid range
  return code < 700;
}

}  // namespace qlever::ingress
