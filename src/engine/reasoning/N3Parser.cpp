// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include "engine/reasoning/N3Parser.h"

#include <algorithm>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "parser/ParsedQuery.h"
#include "util/Log.h"

namespace reasoning {

std::vector<std::shared_ptr<Rule>> N3Parser::parseRulesFromSparql(
    const std::string& sparqlQuery) {
  std::vector<std::shared_ptr<Rule>> rules;

  if (!isN3Rule(sparqlQuery)) {
    LOG(WARN) << "Input does not contain N3 rule syntax";
    return rules;
  }

  // Find all implication operators and extract rules
  std::string query = sparqlQuery;
  size_t pos = 0;
  size_t ruleId = 0;

  while ((pos = query.find("=>", pos)) != std::string::npos ||
         (pos = query.find(":implies", pos)) != std::string::npos ||
         (pos = query.find("log:implies", pos)) != std::string::npos) {
    // For simplicity in this initial implementation, parse one rule at a time
    std::string ruleSubstring =
        query.substr(std::max(0, static_cast<int>(pos) - 100), 200);

    try {
      auto rule = parseImplicationRule(ruleSubstring);
      if (rule) {
        rules.push_back(rule);
        ruleId++;
      }
    } catch (const std::exception& e) {
      LOG(WARN) << "Failed to parse N3 rule: " << e.what();
    }

    pos++;
  }

  return rules;
}

std::shared_ptr<Rule> N3Parser::parseImplicationRule(
    const std::string& ruleText) {
  // Extract body and head from "{ body } => { head }"
  auto body = extractRuleBody(ruleText);
  auto head = extractRuleHead(ruleText);

  if (body.empty() || head.empty()) {
    throw std::runtime_error("Invalid N3 rule format");
  }

  // Rule IDs are assigned sequentially
  static size_t nextRuleId = 0;
  return std::make_shared<Rule>(body, head, nextRuleId++);
}

std::vector<SparqlTriple> N3Parser::extractRuleBody(
    const std::string& ruleText) {
  std::vector<SparqlTriple> triples;

  // Find the first { }
  size_t pos = 0;
  auto [bodyStr, endPos] = extractBracedContent(ruleText, 0);

  if (bodyStr.empty()) {
    return triples;
  }

  // Split by periods to get individual triples
  std::istringstream iss(bodyStr);
  std::string tripleStr;

  while (std::getline(iss, tripleStr, '.')) {
    // Trim whitespace
    tripleStr.erase(0, tripleStr.find_first_not_of(" \t\n\r"));
    tripleStr.erase(tripleStr.find_last_not_of(" \t\n\r") + 1);

    if (!tripleStr.empty()) {
      try {
        triples.push_back(parseTriple(tripleStr));
      } catch (const std::exception& e) {
        LOG(WARN) << "Failed to parse triple: " << tripleStr << " - "
                  << e.what();
      }
    }
  }

  return triples;
}

std::vector<SparqlTriple> N3Parser::extractRuleHead(
    const std::string& ruleText) {
  std::vector<SparqlTriple> triples;

  // Find the second { } (after the =>)
  size_t implPos = findImplicationOperator(ruleText);
  if (implPos == std::string::npos) {
    return triples;
  }

  // Skip past the implication operator
  size_t headStart = ruleText.find('{', implPos);
  if (headStart == std::string::npos) {
    return triples;
  }

  auto [headStr, endPos] = extractBracedContent(ruleText, headStart);

  if (headStr.empty()) {
    return triples;
  }

  // Split by periods to get individual triples
  std::istringstream iss(headStr);
  std::string tripleStr;

  while (std::getline(iss, tripleStr, '.')) {
    // Trim whitespace
    tripleStr.erase(0, tripleStr.find_first_not_of(" \t\n\r"));
    tripleStr.erase(tripleStr.find_last_not_of(" \t\n\r") + 1);

    if (!tripleStr.empty()) {
      try {
        triples.push_back(parseTriple(tripleStr));
      } catch (const std::exception& e) {
        LOG(WARN) << "Failed to parse triple: " << tripleStr << " - "
                  << e.what();
      }
    }
  }

  return triples;
}

std::string N3Parser::n3ToSparql(const std::string& n3Rule) {
  auto body = extractRuleBody(n3Rule);
  auto head = extractRuleHead(n3Rule);

  if (body.empty() || head.empty()) {
    return "";
  }

  std::string result = "INSERT { ";

  // Add head triples
  for (size_t i = 0; i < head.size(); ++i) {
    result += head[i].asString();
    if (i < head.size() - 1) {
      result += " . ";
    }
  }

  result += " } WHERE { ";

  // Add body triples
  for (size_t i = 0; i < body.size(); ++i) {
    result += body[i].asString();
    if (i < body.size() - 1) {
      result += " . ";
    }
  }

  result += " }";

  return result;
}

bool N3Parser::isN3Rule(const std::string& sparqlQuery) {
  return containsImplication(sparqlQuery) && containsN3Quotes(sparqlQuery);
}

bool N3Parser::containsN3Quotes(const std::string& sparqlQuery) {
  return sparqlQuery.find('{') != std::string::npos &&
         sparqlQuery.find('}') != std::string::npos;
}

std::vector<std::string> N3Parser::extractQuotedPatterns(
    const std::string& text) {
  std::vector<std::string> patterns;

  size_t pos = 0;
  while ((pos = text.find('{', pos)) != std::string::npos) {
    try {
      auto [content, endPos] = extractBracedContent(text, pos);
      patterns.push_back(content);
      pos = endPos;
    } catch (const std::exception& e) {
      LOG(WARN) << "Failed to extract quoted pattern: " << e.what();
      pos++;
    }
  }

  return patterns;
}

bool N3Parser::containsImplication(const std::string& text) {
  for (const auto* op : IMPLICATION_OPERATORS) {
    if (text.find(op) != std::string::npos) {
      return true;
    }
  }
  return false;
}

size_t N3Parser::findImplicationOperator(const std::string& text) {
  size_t minPos = std::string::npos;
  for (const auto* op : IMPLICATION_OPERATORS) {
    size_t pos = text.find(op);
    if (pos != std::string::npos && pos < minPos) {
      minPos = pos;
    }
  }
  return minPos;
}

std::pair<std::string, size_t> N3Parser::extractBracedContent(
    const std::string& text, size_t startPos) {
  if (startPos >= text.length() || text[startPos] != '{') {
    throw std::runtime_error("Expected '{' at position " +
                             std::to_string(startPos));
  }

  size_t depth = 0;
  size_t pos = startPos;

  while (pos < text.length()) {
    if (text[pos] == '{') {
      depth++;
    } else if (text[pos] == '}') {
      depth--;
      if (depth == 0) {
        return {text.substr(startPos + 1, pos - startPos - 1), pos + 1};
      }
    }
    pos++;
  }

  throw std::runtime_error("Unmatched braces");
}

SparqlTriple N3Parser::parseTriple(const std::string& tripleText) {
  auto parts = splitTriple(tripleText);

  if (parts.size() != 3) {
    throw std::runtime_error("Invalid triple format");
  }

  // Parse subject, predicate, object as LiteralOrIri or Variable
  // This is simplified; a full implementation would use the SPARQL parser
  LiteralOrIri subject;
  LiteralOrIri predicate;
  LiteralOrIri object;

  // Check if each part is a variable (starts with ?)
  if (parts[0][0] == '?') {
    subject = Variable{parts[0]};
  } else {
    subject = LiteralOrIri{parts[0]};
  }

  if (parts[1][0] == '?') {
    predicate = Variable{parts[1]};
  } else {
    predicate = LiteralOrIri{parts[1]};
  }

  if (parts[2][0] == '?') {
    object = Variable{parts[2]};
  } else {
    object = LiteralOrIri{parts[2]};
  }

  return SparqlTriple{subject, predicate, object};
}

std::vector<std::string> N3Parser::splitTriple(const std::string& tripleText) {
  std::vector<std::string> parts;
  std::istringstream iss(tripleText);
  std::string part;

  while (iss >> part) {
    // Trim parentheses and other characters
    part.erase(0, part.find_first_not_of("()<>"));
    part.erase(part.find_last_not_of("()<>;") + 1);

    if (!part.empty()) {
      parts.push_back(part);
    }
  }

  return parts;
}

}  // namespace reasoning
