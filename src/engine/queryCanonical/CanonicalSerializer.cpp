// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation

#include "engine/queryCanonical/CanonicalSerializer.h"

#include <sstream>
#include <variant>

#include "parser/GraphPatternOperation.h"
#include "parser/SparqlTriple.h"
#include "util/Exception.h"

namespace queryCanonical {

// QSHAPE format version 1 header
constexpr char QSHAPE_HEADER[] = "QSHAPE\x01";
constexpr size_t QSHAPE_HEADER_SIZE = 7;

// ____________________________________________________________________________
CanonicalSerializer::CanonicalSerializer(const ParsedQuery& query)
    : query_(query) {}

// ____________________________________________________________________________
std::vector<uint8_t> CanonicalSerializer::serializeToCanonicalForm() const {
  std::vector<uint8_t> buffer;
  buffer.reserve(1024);  // Reasonable initial capacity

  // 1. Write version header ("QSHAPE\x01")
  buffer.insert(buffer.end(), QSHAPE_HEADER,
                QSHAPE_HEADER + QSHAPE_HEADER_SIZE);

  // 2. Compute and write feature flags
  FeatureFlags flags = computeFeatureFlags();
  writeUInt16(buffer, static_cast<uint16_t>(flags));

  // 3. Serialize query header (SELECT, CONSTRUCT, ASK, etc.)
  serializeQueryHeader(buffer);

  // 4. Serialize operator tree (depth-first traversal)
  serializeOperatorTree(buffer);

  // 5. Serialize solution modifiers (ORDER BY, LIMIT, OFFSET, GROUP BY, HAVING)
  serializeSolutionModifiers(buffer);

  return buffer;
}

// ____________________________________________________________________________
std::string CanonicalSerializer::serializeToDebugString() const {
  std::ostringstream oss;

  // Header
  oss << "=== QSHAPE v1 Debug Output ===\n";

  // Feature flags
  FeatureFlags flags = computeFeatureFlags();
  oss << "Feature Flags: 0x" << std::hex << static_cast<uint16_t>(flags)
      << std::dec << "\n";

  // Query type
  if (query_.hasSelectClause()) {
    const auto& selectClause = query_.selectClause();
    oss << "Query Type: SELECT";
    if (selectClause.distinct_) {
      oss << " DISTINCT";
    }
    if (selectClause.reduced_) {
      oss << " REDUCED";
    }
    oss << "\n";
  } else if (query_.hasConstructClause()) {
    oss << "Query Type: CONSTRUCT\n";
  } else if (query_.hasAskClause()) {
    oss << "Query Type: ASK\n";
  } else if (query_.hasUpdateClause()) {
    oss << "Query Type: UPDATE\n";
  }

  // Operator tree
  oss << "\nOperator Tree:\n";
  oss << debugOperatorTree();

  // Solution modifiers
  if (!query_._orderBy.empty()) {
    oss << "\nORDER BY: " << query_._orderBy.size() << " keys\n";
  }
  if (query_._limitOffset.hasLimit()) {
    oss << "LIMIT: " << query_._limitOffset.limitOrDefault() << "\n";
  }
  if (query_._limitOffset.hasOffset()) {
    oss << "OFFSET: " << query_._limitOffset.offsetOrDefault() << "\n";
  }
  if (!query_._groupByVariables.empty()) {
    oss << "GROUP BY: " << query_._groupByVariables.size() << " variables\n";
  }
  if (!query_._havingClauses.empty()) {
    oss << "HAVING: " << query_._havingClauses.size() << " clauses\n";
  }

  return oss.str();
}

// ____________________________________________________________________________
FeatureFlags CanonicalSerializer::computeFeatureFlags() const {
  FeatureFlags flags = FeatureFlags::NONE;

  // Check SELECT clause flags
  if (query_.hasSelectClause()) {
    const auto& selectClause = query_.selectClause();
    if (selectClause.distinct_) {
      flags |= FeatureFlags::DISTINCT;
    }
    if (selectClause.reduced_) {
      flags |= FeatureFlags::REDUCED;
    }
  }

  // Check CONSTRUCT clause
  if (query_.hasConstructClause()) {
    flags |= FeatureFlags::IS_CONSTRUCT;
  }

  // Check solution modifiers
  if (!query_._orderBy.empty()) {
    flags |= FeatureFlags::HAS_ORDER_BY;
  }
  if (query_._limitOffset.hasLimit()) {
    flags |= FeatureFlags::HAS_LIMIT;
  }
  if (query_._limitOffset.hasOffset()) {
    flags |= FeatureFlags::HAS_OFFSET;
  }
  if (!query_._groupByVariables.empty()) {
    flags |= FeatureFlags::HAS_GROUP_BY;
  }
  if (!query_._havingClauses.empty()) {
    flags |= FeatureFlags::HAS_HAVING;
  }

  // Recursively check for specific operators in the graph pattern
  auto checkGraphPattern = [&flags](const parsedQuery::GraphPattern& pattern,
                                     auto& self) -> void {
    if (!pattern._filters.empty()) {
      flags |= FeatureFlags::HAS_FILTER;
    }

    for (const auto& op : pattern._graphPatterns) {
      std::visit(
          ad_utility::OverloadCallOperator{
              [&](const parsedQuery::Optional& opt) {
                flags |= FeatureFlags::HAS_OPTIONAL;
                self(opt._child, self);
              },
              [&](const parsedQuery::Union& u) {
                flags |= FeatureFlags::HAS_UNION;
                self(u._child1, self);
                self(u._child2, self);
              },
              [&](const parsedQuery::Minus& m) {
                flags |= FeatureFlags::HAS_MINUS;
                self(m._child, self);
              },
              [&](const parsedQuery::Subquery&) {
                flags |= FeatureFlags::HAS_SUBQUERY;
              },
              [&](const parsedQuery::Service&) {
                flags |= FeatureFlags::HAS_SERVICE;
              },
              [&](const parsedQuery::Bind&) { flags |= FeatureFlags::HAS_BIND; },
              [&](const parsedQuery::Values&) {
                flags |= FeatureFlags::HAS_VALUES;
              },
              [&](const parsedQuery::GroupGraphPattern& ggp) {
                self(ggp._child, self);
              },
              [&](const auto&) {
                // Other operators don't affect feature flags
              }},
          op);
    }
  };

  checkGraphPattern(query_._rootGraphPattern, checkGraphPattern);

  return flags;
}

// ____________________________________________________________________________
void CanonicalSerializer::serializeQueryHeader(
    std::vector<uint8_t>& buffer) const {
  if (query_.hasSelectClause()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::SELECT));

    const auto& selectClause = query_.selectClause();
    // Write number of selected variables
    const auto& selectedVars = selectClause.getSelectedVariables();
    writeUInt32(buffer, static_cast<uint32_t>(selectedVars.size()));

    // Write each selected variable
    for (const auto& var : selectedVars) {
      writeVariable(buffer, var);
    }

    // Write number of aliases
    const auto& aliases = selectClause.getAliases();
    writeUInt32(buffer, static_cast<uint32_t>(aliases.size()));

    // Note: We don't serialize the full alias expressions here as they
    // would have been normalized in previous steps. We just track the count.

  } else if (query_.hasConstructClause()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::CONSTRUCT));
    // CONSTRUCT clause serialization would go here
    // For now, just mark the type

  } else if (query_.hasAskClause()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::ASK));

  } else if (query_.hasUpdateClause()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::UPDATE));

  } else {
    // Unknown query type - should not happen with valid ParsedQuery
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::UNKNOWN));
  }
}

// ____________________________________________________________________________
void CanonicalSerializer::serializeOperatorTree(
    std::vector<uint8_t>& buffer) const {
  // Serialize the root graph pattern
  serializeGraphPattern(query_._rootGraphPattern, buffer);
}

// ____________________________________________________________________________
void CanonicalSerializer::serializeGraphPattern(
    const parsedQuery::GraphPattern& pattern,
    std::vector<uint8_t>& buffer) const {
  // Write number of filters
  writeUInt32(buffer, static_cast<uint32_t>(pattern._filters.size()));

  // Serialize each filter (simplified - just count for now)
  // Full filter expression serialization would be complex and is not
  // strictly necessary for query shape if filters have been normalized

  // Write number of graph pattern operations
  writeUInt32(buffer, static_cast<uint32_t>(pattern._graphPatterns.size()));

  // Serialize each graph pattern operation in order (deterministic)
  for (const auto& op : pattern._graphPatterns) {
    serializeGraphPatternOperation(op, buffer);
  }
}

// ____________________________________________________________________________
void CanonicalSerializer::serializeGraphPatternOperation(
    const parsedQuery::GraphPatternOperation& op,
    std::vector<uint8_t>& buffer) const {
  std::visit(
      ad_utility::OverloadCallOperator{
          [&](const parsedQuery::BasicGraphPattern& bgp) {
            writeUInt8(buffer,
                       static_cast<uint8_t>(OperatorType::BASIC_GRAPH_PATTERN));
            // Write number of triples
            writeUInt32(buffer, static_cast<uint32_t>(bgp._triples.size()));

            // Serialize each triple (simplified)
            // In a normalized query, triples would have placeholders for
            // constants
            for (const auto& triple : bgp._triples) {
              writeUInt8(buffer,
                         static_cast<uint8_t>(OperatorType::TRIPLE_PATTERN));
              // Write subject, predicate, object indicators
              // For simplicity, we track whether each component is a variable
              writeUInt8(buffer, triple.s_.isVariable() ? 1 : 0);
              if (triple.s_.isVariable()) {
                writeVariable(buffer, triple.s_.getVariable());
              }

              // Predicate (can be variable or property path)
              if (std::holds_alternative<Variable>(triple.p_)) {
                writeUInt8(buffer, 1);  // Variable
                writeVariable(buffer, std::get<Variable>(triple.p_));
              } else {
                writeUInt8(buffer, 0);  // Not a variable (property path or IRI)
              }

              writeUInt8(buffer, triple.o_.isVariable() ? 1 : 0);
              if (triple.o_.isVariable()) {
                writeVariable(buffer, triple.o_.getVariable());
              }
            }
          },
          [&](const parsedQuery::Optional& opt) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::OPTIONAL));
            serializeGraphPattern(opt._child, buffer);
          },
          [&](const parsedQuery::Union& u) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::UNION));
            serializeGraphPattern(u._child1, buffer);
            serializeGraphPattern(u._child2, buffer);
          },
          [&](const parsedQuery::Minus& m) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::MINUS));
            serializeGraphPattern(m._child, buffer);
          },
          [&](const parsedQuery::Subquery& sq) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::SUBQUERY));
            // For subqueries, we would recursively serialize the nested query
            // For now, just mark the operator type
          },
          [&](const parsedQuery::Service& svc) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::SERVICE));
            // Serialize service endpoint (would be normalized)
            writeUInt8(buffer, svc.silent_ ? 1 : 0);
          },
          [&](const parsedQuery::Bind& bind) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::BIND));
            writeVariable(buffer, bind._target);
            // Expression would be serialized here (complex)
          },
          [&](const parsedQuery::Values& vals) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::VALUES));
            // Serialize number of variables
            writeUInt32(buffer, static_cast<uint32_t>(
                                    vals._inlineValues._variables.size()));
            // Serialize number of value rows
            writeUInt32(
                buffer,
                static_cast<uint32_t>(vals._inlineValues._values.size()));
          },
          [&](const parsedQuery::GroupGraphPattern& ggp) {
            writeUInt8(buffer,
                       static_cast<uint8_t>(OperatorType::GROUP_GRAPH_PATTERN));
            // Serialize GRAPH clause type if present
            std::visit(
                ad_utility::OverloadCallOperator{
                    [&](std::monostate) { writeUInt8(buffer, 0); },
                    [&](const TripleComponent::Iri&) { writeUInt8(buffer, 1); },
                    [&](const std::pair<Variable,
                                        parsedQuery::GroupGraphPattern::
                                            GraphVariableBehaviour>&) {
                      writeUInt8(buffer, 2);
                    }},
                ggp.graphSpec_);
            serializeGraphPattern(ggp._child, buffer);
          },
          [&](const parsedQuery::TransPath& tp) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::TRANS_PATH));
            // Serialize min/max bounds
            writeUInt64(buffer, tp._min);
            writeUInt64(buffer, tp._max);
            serializeGraphPattern(tp._childGraphPattern, buffer);
          },
          [&](const parsedQuery::PathQuery&) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::PATH_QUERY));
          },
          [&](const parsedQuery::SpatialQuery&) {
            writeUInt8(buffer,
                       static_cast<uint8_t>(OperatorType::SPATIAL_QUERY));
          },
          [&](const parsedQuery::TextSearchQuery&) {
            writeUInt8(buffer,
                       static_cast<uint8_t>(OperatorType::TEXT_SEARCH_QUERY));
          },
          [&](const parsedQuery::Describe&) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::DESCRIBE));
          },
          [&](const parsedQuery::Load&) {
            writeUInt8(buffer, static_cast<uint8_t>(OperatorType::LOAD));
          },
          [&](const parsedQuery::NamedCachedResult&) {
            writeUInt8(buffer,
                       static_cast<uint8_t>(OperatorType::NAMED_CACHED_RESULT));
          },
          [&](const parsedQuery::MaterializedViewQuery&) {
            writeUInt8(
                buffer,
                static_cast<uint8_t>(OperatorType::MATERIALIZED_VIEW_QUERY));
          }},
      op);
}

// ____________________________________________________________________________
void CanonicalSerializer::serializeSolutionModifiers(
    std::vector<uint8_t>& buffer) const {
  // Serialize GROUP BY
  if (!query_._groupByVariables.empty()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::GROUP_BY));
    writeUInt32(buffer, static_cast<uint32_t>(query_._groupByVariables.size()));
    for (const auto& var : query_._groupByVariables) {
      writeVariable(buffer, var);
    }
  }

  // Serialize HAVING
  if (!query_._havingClauses.empty()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::HAVING));
    writeUInt32(buffer, static_cast<uint32_t>(query_._havingClauses.size()));
  }

  // Serialize ORDER BY
  if (!query_._orderBy.empty()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::ORDER_BY));
    writeUInt32(buffer, static_cast<uint32_t>(query_._orderBy.size()));
    for (const auto& orderKey : query_._orderBy) {
      writeVariable(buffer, orderKey.variable());
      writeUInt8(buffer, orderKey.isDescending() ? 1 : 0);
    }
  }

  // Serialize LIMIT
  if (query_._limitOffset.hasLimit()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::LIMIT));
    writeUInt64(buffer, query_._limitOffset.limitOrDefault());
  }

  // Serialize OFFSET
  if (query_._limitOffset.hasOffset()) {
    writeUInt8(buffer, static_cast<uint8_t>(OperatorType::OFFSET));
    writeUInt64(buffer, query_._limitOffset.offsetOrDefault());
  }
}

// ____________________________________________________________________________
void CanonicalSerializer::writeString(std::vector<uint8_t>& buffer,
                                      const std::string& str) {
  // Write length-prefixed string
  writeUInt32(buffer, static_cast<uint32_t>(str.size()));
  buffer.insert(buffer.end(), str.begin(), str.end());
}

// ____________________________________________________________________________
void CanonicalSerializer::writeVariable(std::vector<uint8_t>& buffer,
                                        const Variable& var) {
  writeString(buffer, var.name());
}

// ____________________________________________________________________________
void CanonicalSerializer::writeUInt8(std::vector<uint8_t>& buffer,
                                     uint8_t value) {
  buffer.push_back(value);
}

// ____________________________________________________________________________
void CanonicalSerializer::writeUInt16(std::vector<uint8_t>& buffer,
                                      uint16_t value) {
  // Little-endian encoding
  buffer.push_back(static_cast<uint8_t>(value & 0xFF));
  buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

// ____________________________________________________________________________
void CanonicalSerializer::writeUInt32(std::vector<uint8_t>& buffer,
                                      uint32_t value) {
  // Little-endian encoding
  buffer.push_back(static_cast<uint8_t>(value & 0xFF));
  buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

// ____________________________________________________________________________
void CanonicalSerializer::writeUInt64(std::vector<uint8_t>& buffer,
                                      uint64_t value) {
  // Little-endian encoding
  for (int i = 0; i < 8; ++i) {
    buffer.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
  }
}

// ____________________________________________________________________________
std::string CanonicalSerializer::debugOperatorTree() const {
  return debugGraphPattern(query_._rootGraphPattern, 0);
}

// ____________________________________________________________________________
std::string CanonicalSerializer::debugGraphPattern(
    const parsedQuery::GraphPattern& pattern, int indent) const {
  std::ostringstream oss;
  std::string indentStr(indent * 2, ' ');

  if (!pattern._filters.empty()) {
    oss << indentStr << "FILTER: " << pattern._filters.size() << " clause(s)\n";
  }

  for (const auto& op : pattern._graphPatterns) {
    oss << debugGraphPatternOperation(op, indent);
  }

  return oss.str();
}

// ____________________________________________________________________________
std::string CanonicalSerializer::debugGraphPatternOperation(
    const parsedQuery::GraphPatternOperation& op, int indent) const {
  std::ostringstream oss;
  std::string indentStr(indent * 2, ' ');

  std::visit(
      ad_utility::OverloadCallOperator{
          [&](const parsedQuery::BasicGraphPattern& bgp) {
            oss << indentStr << "BasicGraphPattern (" << bgp._triples.size()
                << " triples)\n";
          },
          [&](const parsedQuery::Optional& opt) {
            oss << indentStr << "OPTIONAL:\n";
            oss << debugGraphPattern(opt._child, indent + 1);
          },
          [&](const parsedQuery::Union& u) {
            oss << indentStr << "UNION:\n";
            oss << indentStr << "  Left:\n";
            oss << debugGraphPattern(u._child1, indent + 2);
            oss << indentStr << "  Right:\n";
            oss << debugGraphPattern(u._child2, indent + 2);
          },
          [&](const parsedQuery::Minus& m) {
            oss << indentStr << "MINUS:\n";
            oss << debugGraphPattern(m._child, indent + 1);
          },
          [&](const parsedQuery::Subquery&) {
            oss << indentStr << "SUBQUERY\n";
          },
          [&](const parsedQuery::Service& svc) {
            oss << indentStr << "SERVICE" << (svc.silent_ ? " SILENT" : "")
                << "\n";
          },
          [&](const parsedQuery::Bind& bind) {
            oss << indentStr << "BIND -> " << bind._target.name() << "\n";
          },
          [&](const parsedQuery::Values& vals) {
            oss << indentStr << "VALUES ("
                << vals._inlineValues._variables.size() << " vars, "
                << vals._inlineValues._values.size() << " rows)\n";
          },
          [&](const parsedQuery::GroupGraphPattern& ggp) {
            oss << indentStr << "GroupGraphPattern:\n";
            oss << debugGraphPattern(ggp._child, indent + 1);
          },
          [&](const parsedQuery::TransPath& tp) {
            oss << indentStr << "TransPath [" << tp._min << ", " << tp._max
                << "]\n";
            oss << debugGraphPattern(tp._childGraphPattern, indent + 1);
          },
          [&](const parsedQuery::PathQuery&) {
            oss << indentStr << "PathQuery\n";
          },
          [&](const parsedQuery::SpatialQuery&) {
            oss << indentStr << "SpatialQuery\n";
          },
          [&](const parsedQuery::TextSearchQuery&) {
            oss << indentStr << "TextSearchQuery\n";
          },
          [&](const parsedQuery::Describe&) {
            oss << indentStr << "DESCRIBE\n";
          },
          [&](const parsedQuery::Load&) { oss << indentStr << "LOAD\n"; },
          [&](const parsedQuery::NamedCachedResult&) {
            oss << indentStr << "NamedCachedResult\n";
          },
          [&](const parsedQuery::MaterializedViewQuery&) {
            oss << indentStr << "MaterializedViewQuery\n";
          }},
      op);

  return oss.str();
}

}  // namespace queryCanonical
