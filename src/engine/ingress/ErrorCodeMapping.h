// Error code to description mapping (cold-path only)
// Used for logging and diagnostics outside the hot execution path

#ifndef QLEVER_ENGINE_INGRESS_ERROR_CODE_MAPPING_H
#define QLEVER_ENGINE_INGRESS_ERROR_CODE_MAPPING_H

#include "ErrorCodes.h"
#include <string_view>

namespace qlever::ingress {

// Get human-readable description for error code (cold-path function)
inline std::string_view get_error_description(IngressErrorCode code) noexcept {
  return error_description(code);
}

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_ERROR_CODE_MAPPING_H
