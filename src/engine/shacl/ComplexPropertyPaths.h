#ifndef QLEVER_ENGINE_SHACL_COMPLEXPROPERTYPATHS_H
#define QLEVER_ENGINE_SHACL_COMPLEXPROPERTYPATHS_H

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace shacl {

// Forward declarations for recursive path types
class PropertyPath;

// Path type enumeration for easy type checking
enum class PathType {
  Simple,        // Direct property IRI
  Inverse,       // ^property (reverse direction)
  Sequence,      // property1/property2 (follow path then next)
  Alternative,   // property1|property2 (match either)
  ZeroOrMore,    // property* (transitive closure, including zero hops)
  OneOrMore,     // property+ (transitive closure, at least one hop)
  ZeroOrOne,     // property? (optional path)
  Wildcard       // Matches any property
};

// Simple property path - just a property IRI
struct SimplePath {
  std::string propertyIri;

  SimplePath() = default;
  explicit SimplePath(const std::string& iri) : propertyIri(iri) {}

  bool operator==(const SimplePath& other) const {
    return propertyIri == other.propertyIri;
  }
};

// Inverse property path - ^property (follows property in reverse)
struct InversePath {
  std::shared_ptr<PropertyPath> innerPath;

  InversePath() = default;
  explicit InversePath(std::shared_ptr<PropertyPath> path)
      : innerPath(std::move(path)) {}

  bool operator==(const InversePath& other) const;
};

// Sequence path - property1/property2 (follow first, then second)
struct SequencePath {
  std::vector<std::shared_ptr<PropertyPath>> paths;

  SequencePath() = default;
  explicit SequencePath(std::vector<std::shared_ptr<PropertyPath>> p)
      : paths(std::move(p)) {}

  bool operator==(const SequencePath& other) const;
};

// Alternative path - property1|property2 (match any of the paths)
struct AlternativePath {
  std::vector<std::shared_ptr<PropertyPath>> paths;

  AlternativePath() = default;
  explicit AlternativePath(std::vector<std::shared_ptr<PropertyPath>> p)
      : paths(std::move(p)) {}

  bool operator==(const AlternativePath& other) const;
};

// Zero or more path - property* (transitive closure, including zero hops)
struct ZeroOrMorePath {
  std::shared_ptr<PropertyPath> innerPath;

  ZeroOrMorePath() = default;
  explicit ZeroOrMorePath(std::shared_ptr<PropertyPath> path)
      : innerPath(std::move(path)) {}

  bool operator==(const ZeroOrMorePath& other) const;
};

// One or more path - property+ (transitive closure, at least one hop)
struct OneOrMorePath {
  std::shared_ptr<PropertyPath> innerPath;

  OneOrMorePath() = default;
  explicit OneOrMorePath(std::shared_ptr<PropertyPath> path)
      : innerPath(std::move(path)) {}

  bool operator==(const OneOrMorePath& other) const;
};

// Zero or one path - property? (optional path)
struct ZeroOrOnePath {
  std::shared_ptr<PropertyPath> innerPath;

  ZeroOrOnePath() = default;
  explicit ZeroOrOnePath(std::shared_ptr<PropertyPath> path)
      : innerPath(std::move(path)) {}

  bool operator==(const ZeroOrOnePath& other) const;
};

// Wildcard path - matches any property
struct WildcardPath {
  bool operator==(const WildcardPath&) const { return true; }
};

// Main PropertyPath class using std::variant for type-safe path representation
class PropertyPath {
 public:
  using PathVariant = std::variant<SimplePath, InversePath, SequencePath,
                                   AlternativePath, ZeroOrMorePath,
                                   OneOrMorePath, ZeroOrOnePath, WildcardPath>;

  PropertyPath() : path_(SimplePath()) {}
  explicit PropertyPath(PathVariant p) : path_(std::move(p)) {}

  // Factory methods for creating different path types
  static PropertyPath simple(const std::string& iri);
  static PropertyPath inverse(PropertyPath innerPath);
  static PropertyPath sequence(std::vector<PropertyPath> paths);
  static PropertyPath alternative(std::vector<PropertyPath> paths);
  static PropertyPath zeroOrMore(PropertyPath innerPath);
  static PropertyPath oneOrMore(PropertyPath innerPath);
  static PropertyPath zeroOrOne(PropertyPath innerPath);
  static PropertyPath wildcard();

  // Parse path expression from string (e.g., "^ex:parent", "ex:p1/ex:p2")
  static PropertyPath parse(const std::string& pathExpr);

  // Get the path type
  PathType getType() const;

  // Get the variant (for visitor pattern)
  const PathVariant& getVariant() const { return path_; }
  PathVariant& getVariant() { return path_; }

  // Check if this is a simple path
  bool isSimple() const;

  // Get simple path IRI (only valid for simple paths)
  std::string getSimpleIri() const;

  // Convert path to string representation (for debugging/display)
  std::string toString() const;

  // Equality comparison
  bool operator==(const PropertyPath& other) const {
    return path_ == other.path_;
  }

  bool operator!=(const PropertyPath& other) const {
    return !(*this == other);
  }

 private:
  PathVariant path_;
};

// Helper functions for path manipulation

// Check if a path is transitive (*, +, or ?)
bool isTransitivePath(const PropertyPath& path);

// Check if a path requires recursive evaluation
bool requiresRecursion(const PropertyPath& path);

// Simplify a path (e.g., remove redundant sequence/alternative wrappers)
PropertyPath simplifyPath(const PropertyPath& path);

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_COMPLEXPROPERTYPATHS_H
