// Phase 2A: Advanced Value Constraints Implementation
// This file contains implementations for NumericRangeConstraint, PatternConstraint,
// LanguageTagConstraint, LengthConstraint, and DatatypeFacetConstraint

#include "ShEx.h"
#include <sstream>
#include <regex>
#include <cmath>
#include <limits>

namespace shex {

// ============================================================================
// Phase 2A: Helper Functions
// ============================================================================

namespace {

// Parse numeric literal from string, handling integer, decimal, and double
struct NumericValue {
  double value;
  int totalDigits;
  int fractionDigits;
  bool isInteger;
  bool isValid;

  NumericValue() : value(0.0), totalDigits(0), fractionDigits(0),
                   isInteger(false), isValid(false) {}
};

NumericValue parseNumericLiteral(const std::string& str) {
  NumericValue result;

  // Handle special values
  if (str == "INF" || str == "+INF") {
    result.value = std::numeric_limits<double>::infinity();
    result.isValid = true;
    result.isInteger = false;
    return result;
  }
  if (str == "-INF") {
    result.value = -std::numeric_limits<double>::infinity();
    result.isValid = true;
    result.isInteger = false;
    return result;
  }
  if (str == "NaN") {
    result.value = std::numeric_limits<double>::quiet_NaN();
    result.isValid = true;
    result.isInteger = false;
    return result;
  }

  // Try to parse as double
  try {
    size_t pos;
    result.value = std::stod(str, &pos);
    if (pos != str.length()) {
      return result;  // Invalid: didn't consume entire string
    }
    result.isValid = true;

    // Count digits
    std::string numStr = str;
    bool negative = !numStr.empty() && (numStr[0] == '-' || numStr[0] == '+');
    if (negative) numStr = numStr.substr(1);

    size_t dotPos = numStr.find('.');
    if (dotPos == std::string::npos) {
      // Integer or scientific notation without decimal point
      size_t ePos = numStr.find_first_of("eE");
      if (ePos != std::string::npos) {
        numStr = numStr.substr(0, ePos);
      }
      // Remove leading zeros
      size_t firstNonZero = numStr.find_first_not_of('0');
      if (firstNonZero == std::string::npos) {
        result.totalDigits = 1;  // It's zero
      } else {
        result.totalDigits = static_cast<int>(numStr.length() - firstNonZero);
      }
      result.fractionDigits = 0;
      result.isInteger = (str.find('.') == std::string::npos &&
                         str.find_first_of("eE") == std::string::npos);
    } else {
      // Has decimal point
      std::string intPart = numStr.substr(0, dotPos);
      std::string fracPart = numStr.substr(dotPos + 1);

      // Remove scientific notation part
      size_t ePos = fracPart.find_first_of("eE");
      if (ePos != std::string::npos) {
        fracPart = fracPart.substr(0, ePos);
      }

      // Remove leading zeros from integer part
      size_t firstNonZero = intPart.find_first_not_of('0');
      int intDigits = 0;
      if (firstNonZero != std::string::npos) {
        intDigits = static_cast<int>(intPart.length() - firstNonZero);
      }

      result.totalDigits = intDigits + static_cast<int>(fracPart.length());
      result.fractionDigits = static_cast<int>(fracPart.length());
      result.isInteger = false;
    }

  } catch (...) {
    return result;  // isValid = false
  }

  return result;
}

// Count UTF-8 characters (not bytes)
size_t countUtf8CharsInternal(const std::string& str) {
  size_t count = 0;
  for (size_t i = 0; i < str.length(); ) {
    unsigned char c = static_cast<unsigned char>(str[i]);
    if (c < 0x80) {
      // Single byte (ASCII)
      i += 1;
    } else if ((c & 0xE0) == 0xC0) {
      // Two bytes
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {
      // Three bytes
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {
      // Four bytes
      i += 4;
    } else {
      // Invalid UTF-8, skip this byte
      i += 1;
    }
    count++;
  }
  return count;
}

// Validate BCP47 language tag
bool isValidBCP47Internal(const std::string& tag) {
  if (tag.empty()) return false;

  // BCP47 language tags consist of subtags separated by hyphens
  // Simple validation: check format with regex
  static const std::regex bcp47Regex(
      "^([a-z]{2,3}|[a-z]{4}|[a-z]{5,8})"           // Primary language
      "(-[A-Z][a-z]{3})?"                           // Optional script
      "(-([A-Z]{2}|[0-9]{3}))?"                     // Optional region
      "(-([a-z0-9]{5,8}|[0-9][a-z0-9]{3}))*$"       // Optional variants
  );

  return std::regex_match(tag, bcp47Regex);
}

// Parse and validate ISO 8601 date (YYYY-MM-DD)
bool parseDate(const std::string& dateStr) {
  if (dateStr.length() != 10) return false;
  if (dateStr[4] != '-' || dateStr[7] != '-') return false;

  try {
    int year = std::stoi(dateStr.substr(0, 4));
    int month = std::stoi(dateStr.substr(5, 2));
    int day = std::stoi(dateStr.substr(8, 2));

    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;

    // Check days in month
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];

    // Leap year check for February
    if (month == 2) {
      bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
      if (isLeap) maxDay = 29;
    }

    if (day > maxDay) return false;

    return true;
  } catch (...) {
    return false;
  }
}

// Parse and validate ISO 8601 dateTime
bool parseDateTime(const std::string& dateTimeStr) {
  if (dateTimeStr.length() < 19) return false;

  std::string datePart = dateTimeStr.substr(0, 10);
  if (!parseDate(datePart)) return false;

  if (dateTimeStr[10] != 'T' && dateTimeStr[10] != 't') return false;

  std::string timePart = dateTimeStr.substr(11);

  size_t timeEndPos = timePart.find_first_of("Z+-");
  std::string time = (timeEndPos != std::string::npos) ?
                     timePart.substr(0, timeEndPos) : timePart;

  if (time.length() < 8) return false;
  if (time[2] != ':' || time[5] != ':') return false;

  try {
    int hour = std::stoi(time.substr(0, 2));
    int minute = std::stoi(time.substr(3, 2));
    int second = std::stoi(time.substr(6, 2));

    if (hour < 0 || hour > 23) return false;
    if (minute < 0 || minute > 59) return false;
    if (second < 0 || second > 59) return false;

    return true;
  } catch (...) {
    return false;
  }
}

}  // anonymous namespace

// ============================================================================
// Phase 2A: NumericRangeConstraint Implementation
// ============================================================================

bool NumericRangeConstraint::validate(const std::string& value,
                                      XsdDatatype datatype) const {
  NumericValue parsed = parseNumericLiteral(value);
  if (!parsed.isValid) return false;

  // Check datatype compatibility
  if (datatype == XsdDatatype::INTEGER && !parsed.isInteger) {
    return false;
  }

  // Check range constraints
  if (minInclusive.has_value() && parsed.value < minInclusive.value()) {
    return false;
  }
  if (maxInclusive.has_value() && parsed.value > maxInclusive.value()) {
    return false;
  }
  if (minExclusive.has_value() && parsed.value <= minExclusive.value()) {
    return false;
  }
  if (maxExclusive.has_value() && parsed.value >= maxExclusive.value()) {
    return false;
  }

  // Check digit constraints
  if (totalDigits.has_value() && parsed.totalDigits > totalDigits.value()) {
    return false;
  }
  if (fractionDigits.has_value() && parsed.fractionDigits > fractionDigits.value()) {
    return false;
  }

  return true;
}

// ============================================================================
// Phase 2A: PatternConstraint Implementation
// ============================================================================

void PatternConstraint::ensureCompiled() const {
  if (!compiled) {
    compiledRegex = std::make_unique<RE2>(pattern);
    compiled = true;
  }
}

bool PatternConstraint::validate(const std::string& value) const {
  ensureCompiled();

  if (!compiledRegex || !compiledRegex->ok()) {
    return false;  // Invalid regex
  }

  return RE2::FullMatch(value, *compiledRegex);
}

// ============================================================================
// Phase 2A: LanguageTagConstraint Implementation
// ============================================================================

bool LanguageTagConstraint::isValidBCP47(const std::string& tag) {
  return isValidBCP47Internal(tag);
}

bool LanguageTagConstraint::validate(const std::string& tag) const {
  if (!isValidBCP47(tag)) {
    return false;
  }

  // Exact match if specified
  if (languageTag.has_value()) {
    return tag == languageTag.value();
  }

  // Pattern match if specified (e.g., "en-*" matches "en-US", "en-GB")
  if (languagePattern.has_value()) {
    const std::string& pattern = languagePattern.value();

    // Simple wildcard matching
    if (!pattern.empty() && pattern.back() == '*') {
      std::string prefix = pattern.substr(0, pattern.length() - 1);
      return tag.substr(0, prefix.length()) == prefix;
    }

    // Exact match for non-wildcard patterns
    return tag == pattern;
  }

  // No specific constraint, just validate BCP47
  return true;
}

// ============================================================================
// Phase 2A: LengthConstraint Implementation
// ============================================================================

size_t LengthConstraint::countUtf8Chars(const std::string& str) {
  return countUtf8CharsInternal(str);
}

bool LengthConstraint::validate(const std::string& value) const {
  size_t len = countUtf8Chars(value);

  if (exactLength.has_value()) {
    return len == exactLength.value();
  }

  if (minLength.has_value() && len < minLength.value()) {
    return false;
  }

  if (maxLength.has_value() && len > maxLength.value()) {
    return false;
  }

  return true;
}

// ============================================================================
// Phase 2A: DatatypeFacetConstraint Implementation
// ============================================================================

bool DatatypeFacetConstraint::validateInteger(const std::string& value) {
  NumericValue parsed = parseNumericLiteral(value);
  return parsed.isValid && parsed.isInteger;
}

bool DatatypeFacetConstraint::validateDecimal(const std::string& value) {
  NumericValue parsed = parseNumericLiteral(value);
  return parsed.isValid;
}

bool DatatypeFacetConstraint::validateDouble(const std::string& value) {
  NumericValue parsed = parseNumericLiteral(value);
  return parsed.isValid;
}

bool DatatypeFacetConstraint::validateBoolean(const std::string& value) {
  return value == "true" || value == "false" ||
         value == "1" || value == "0";
}

bool DatatypeFacetConstraint::validateDate(const std::string& value) {
  return parseDate(value);
}

bool DatatypeFacetConstraint::validateDateTime(const std::string& value) {
  return parseDateTime(value);
}

bool DatatypeFacetConstraint::validate(const std::string& value) const {
  switch (datatype) {
    case XsdDatatype::INTEGER:
      return validateInteger(value);
    case XsdDatatype::DECIMAL:
      return validateDecimal(value);
    case XsdDatatype::DOUBLE:
    case XsdDatatype::FLOAT:
      return validateDouble(value);
    case XsdDatatype::BOOLEAN:
      return validateBoolean(value);
    case XsdDatatype::DATE:
      return validateDate(value);
    case XsdDatatype::DATETIME:
      return validateDateTime(value);
    case XsdDatatype::STRING:
      return true;
  case XsdDatatype::TIME:
    case XsdDatatype::UNKNOWN:
    default:
      return true;
  }
}

}  // namespace shex
