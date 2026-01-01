#ifndef QLEVER_ENGINE_SHEX_SHEXVALIDATOR_H
#define QLEVER_ENGINE_SHEX_SHEXVALIDATOR_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ShExConformance.h"
#include "ShExTypes.h"

namespace shex {

// Forward declaration
class RDFGraph;

// ShEx Validator - validates RDF nodes against ShEx shapes
class ShExValidator {
 public:
  ShExValidator() = default;

  // Load shapes from ShapeMap
  void loadShapes(const ShapeMap& shapeMap);

  // Validate a single RDF node against a shape
  // Returns ValidationResult with violations
  ValidationResult validateNode(const std::string& nodeId,
                                 const std::string& shapeId,
                                 const RDFGraph& graph,
                                 const ValidationConfig& config =
                                     ValidationConfig());

  // Validate multiple nodes against a shape
  ValidationReport validateNodes(const std::vector<std::string>& nodeIds,
                                  const std::string& shapeId,
                                  const RDFGraph& graph,
                                  const ValidationConfig& config =
                                      ValidationConfig());

  // Get formatted violations and reports
  std::string formatViolations(const ValidationResult& result) const;

  std::string formatReport(const ValidationReport& report) const;

  // Get loaded shape IDs
  std::vector<std::string> getShapeIds() const;

  // Check if a shape exists
  bool hasShape(const std::string& shapeId) const;

  // Get validation statistics
  struct ValidationStats {
    size_t nodesValidated = 0;
    size_t constraintsEvaluated = 0;
    size_t violationsFound = 0;
    bool limitExceeded = false;
  };

  const ValidationStats& getStats() const { return stats_; }
  void resetStats();

 private:
  // Internal shape storage
  ShapeMap shapeMap_;

  // Validation statistics
  ValidationStats stats_;

  // Validate node against shape (internal)
  bool validateNodeInternal(const std::string& nodeId,
                             const std::shared_ptr<ShapeExpr>& shape,
                             const RDFGraph& graph,
                             ValidationResult& result,
                             const ValidationConfig& config);

  // Validate triple constraint
  bool validateTripleConstraint(const std::string& nodeId,
                                 const TripleConstraint& tc,
                                 const RDFGraph& graph,
                                 ValidationResult& result,
                                 const ValidationConfig& config);

  // Validate cardinality
  bool validateCardinality(const std::string& nodeId,
                            const TripleConstraint& tc,
                            size_t actualCount,
                            ValidationResult& result);

  // Validate datatype
  bool validateDatatype(const std::string& value,
                         const ValueType& expectedType,
                         ValidationResult& result,
                         const std::string& propertyPath);

  // Validate node kind
  bool validateNodeKind(const std::string& value,
                         ShExNodeKind expectedKind,
                         ValidationResult& result,
                         const std::string& propertyPath);

  // Validate CLOSED constraint
  bool validateClosed(const std::string& nodeId,
                       const std::shared_ptr<ShapeExpr>& shape,
                       const RDFGraph& graph,
                       ValidationResult& result);

  // Check validation limits
  bool checkLimits(const ValidationConfig& config,
                    ValidationResult& result);
};

// RDF Graph representation for validation
// Minimal interface for accessing RDF triples
class RDFGraph {
 public:
  struct Triple {
    std::string subject;
    std::string predicate;
    std::string object;

    Triple(std::string s, std::string p, std::string o)
        : subject(std::move(s)),
          predicate(std::move(p)),
          object(std::move(o)) {}
  };

  RDFGraph() = default;

  // Add a triple to the graph
  void addTriple(const std::string& subject, const std::string& predicate,
                 const std::string& object) {
    triples_.emplace_back(subject, predicate, object);

    // Index by subject for faster lookup
    subjectIndex_[subject].push_back(triples_.size() - 1);
  }

  // Get all triples with a given subject
  std::vector<Triple> getTriplesForSubject(const std::string& subject) const {
    std::vector<Triple> result;
    auto it = subjectIndex_.find(subject);
    if (it != subjectIndex_.end()) {
      for (size_t idx : it->second) {
        result.push_back(triples_[idx]);
      }
    }
    return result;
  }

  // Get all triples with a given subject and predicate
  std::vector<std::string> getObjects(const std::string& subject,
                                       const std::string& predicate) const {
    std::vector<std::string> objects;
    auto it = subjectIndex_.find(subject);
    if (it != subjectIndex_.end()) {
      for (size_t idx : it->second) {
        if (triples_[idx].predicate == predicate) {
          objects.push_back(triples_[idx].object);
        }
      }
    }
    return objects;
  }

  // Get all properties of a subject
  std::vector<std::string> getProperties(const std::string& subject) const {
    std::vector<std::string> props;
    auto it = subjectIndex_.find(subject);
    if (it != subjectIndex_.end()) {
      for (size_t idx : it->second) {
        props.push_back(triples_[idx].predicate);
      }
    }
    return props;
  }

  // Check if a value is an IRI
  static bool isIRI(const std::string& value) {
    return !value.empty() && (value[0] == '<' || value.find("http") == 0);
  }

  // Check if a value is a literal
  static bool isLiteral(const std::string& value) {
    return !value.empty() && (value[0] == '"' || value[0] == '\'');
  }

  // Check if a value is a blank node
  static bool isBlankNode(const std::string& value) {
    return !value.empty() && value.find("_:") == 0;
  }

  // Get datatype of a literal (if specified)
  static std::string getLiteralDatatype(const std::string& literal) {
    size_t pos = literal.find("^^");
    if (pos != std::string::npos) {
      return literal.substr(pos + 2);
    }
    return "";
  }

  // Clear all triples
  void clear() {
    triples_.clear();
    subjectIndex_.clear();
  }

  // Get total number of triples
  size_t size() const { return triples_.size(); }

 private:
  std::vector<Triple> triples_;
  std::unordered_map<std::string, std::vector<size_t>>
      subjectIndex_;  // subject -> triple indices
};

}  // namespace shex

#endif  // QLEVER_ENGINE_SHEX_SHEXVALIDATOR_H
