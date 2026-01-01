#include "engine/shacl/ComplexPropertyPaths.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace shacl {

// Equality operators for recursive path types

bool InversePath::operator==(const InversePath& other) const {
  if (!innerPath || !other.innerPath) {
    return innerPath == other.innerPath;
  }
  return *innerPath == *other.innerPath;
}

bool SequencePath::operator==(const SequencePath& other) const {
  if (paths.size() != other.paths.size()) {
    return false;
  }
  for (size_t i = 0; i < paths.size(); ++i) {
    if (!paths[i] || !other.paths[i]) {
      if (paths[i] != other.paths[i]) return false;
    } else if (*paths[i] != *other.paths[i]) {
      return false;
    }
  }
  return true;
}

bool AlternativePath::operator==(const AlternativePath& other) const {
  if (paths.size() != other.paths.size()) {
    return false;
  }
  for (size_t i = 0; i < paths.size(); ++i) {
    if (!paths[i] || !other.paths[i]) {
      if (paths[i] != other.paths[i]) return false;
    } else if (*paths[i] != *other.paths[i]) {
      return false;
    }
  }
  return true;
}

bool ZeroOrMorePath::operator==(const ZeroOrMorePath& other) const {
  if (!innerPath || !other.innerPath) {
    return innerPath == other.innerPath;
  }
  return *innerPath == *other.innerPath;
}

bool OneOrMorePath::operator==(const OneOrMorePath& other) const {
  if (!innerPath || !other.innerPath) {
    return innerPath == other.innerPath;
  }
  return *innerPath == *other.innerPath;
}

bool ZeroOrOnePath::operator==(const ZeroOrOnePath& other) const {
  if (!innerPath || !other.innerPath) {
    return innerPath == other.innerPath;
  }
  return *innerPath == *other.innerPath;
}

// PropertyPath factory methods

PropertyPath PropertyPath::simple(const std::string& iri) {
  return PropertyPath(SimplePath(iri));
}

PropertyPath PropertyPath::inverse(PropertyPath innerPath) {
  return PropertyPath(
      InversePath(std::make_shared<PropertyPath>(std::move(innerPath))));
}

PropertyPath PropertyPath::sequence(std::vector<PropertyPath> paths) {
  std::vector<std::shared_ptr<PropertyPath>> sharedPaths;
  sharedPaths.reserve(paths.size());
  for (auto& p : paths) {
    sharedPaths.push_back(std::make_shared<PropertyPath>(std::move(p)));
  }
  return PropertyPath(SequencePath(std::move(sharedPaths)));
}

PropertyPath PropertyPath::alternative(std::vector<PropertyPath> paths) {
  std::vector<std::shared_ptr<PropertyPath>> sharedPaths;
  sharedPaths.reserve(paths.size());
  for (auto& p : paths) {
    sharedPaths.push_back(std::make_shared<PropertyPath>(std::move(p)));
  }
  return PropertyPath(AlternativePath(std::move(sharedPaths)));
}

PropertyPath PropertyPath::zeroOrMore(PropertyPath innerPath) {
  return PropertyPath(
      ZeroOrMorePath(std::make_shared<PropertyPath>(std::move(innerPath))));
}

PropertyPath PropertyPath::oneOrMore(PropertyPath innerPath) {
  return PropertyPath(
      OneOrMorePath(std::make_shared<PropertyPath>(std::move(innerPath))));
}

PropertyPath PropertyPath::zeroOrOne(PropertyPath innerPath) {
  return PropertyPath(
      ZeroOrOnePath(std::make_shared<PropertyPath>(std::move(innerPath))));
}

PropertyPath PropertyPath::wildcard() { return PropertyPath(WildcardPath()); }

// Path type detection

PathType PropertyPath::getType() const {
  return std::visit(
      [](const auto& p) -> PathType {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, SimplePath>) {
          return PathType::Simple;
        } else if constexpr (std::is_same_v<T, InversePath>) {
          return PathType::Inverse;
        } else if constexpr (std::is_same_v<T, SequencePath>) {
          return PathType::Sequence;
        } else if constexpr (std::is_same_v<T, AlternativePath>) {
          return PathType::Alternative;
        } else if constexpr (std::is_same_v<T, ZeroOrMorePath>) {
          return PathType::ZeroOrMore;
        } else if constexpr (std::is_same_v<T, OneOrMorePath>) {
          return PathType::OneOrMore;
        } else if constexpr (std::is_same_v<T, ZeroOrOnePath>) {
          return PathType::ZeroOrOne;
        } else {
          return PathType::Wildcard;
        }
      },
      path_);
}

bool PropertyPath::isSimple() const { return getType() == PathType::Simple; }

std::string PropertyPath::getSimpleIri() const {
  if (const auto* simple = std::get_if<SimplePath>(&path_)) {
    return simple->propertyIri;
  }
  throw std::runtime_error("Path is not a simple path");
}

// Convert path to string representation

std::string PropertyPath::toString() const {
  return std::visit(
      [](const auto& p) -> std::string {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, SimplePath>) {
          return p.propertyIri;
        } else if constexpr (std::is_same_v<T, InversePath>) {
          return "^(" + (p.innerPath ? p.innerPath->toString() : "null") + ")";
        } else if constexpr (std::is_same_v<T, SequencePath>) {
          std::ostringstream oss;
          for (size_t i = 0; i < p.paths.size(); ++i) {
            if (i > 0) oss << "/";
            oss << (p.paths[i] ? p.paths[i]->toString() : "null");
          }
          return oss.str();
        } else if constexpr (std::is_same_v<T, AlternativePath>) {
          std::ostringstream oss;
          oss << "(";
          for (size_t i = 0; i < p.paths.size(); ++i) {
            if (i > 0) oss << "|";
            oss << (p.paths[i] ? p.paths[i]->toString() : "null");
          }
          oss << ")";
          return oss.str();
        } else if constexpr (std::is_same_v<T, ZeroOrMorePath>) {
          return "(" + (p.innerPath ? p.innerPath->toString() : "null") + ")*";
        } else if constexpr (std::is_same_v<T, OneOrMorePath>) {
          return "(" + (p.innerPath ? p.innerPath->toString() : "null") + ")+";
        } else if constexpr (std::is_same_v<T, ZeroOrOnePath>) {
          return "(" + (p.innerPath ? p.innerPath->toString() : "null") + ")?";
        } else {
          return "*";  // Wildcard
        }
      },
      path_);
}

// Simple path parser (basic implementation - can be extended)
PropertyPath PropertyPath::parse(const std::string& pathExpr) {
  if (pathExpr.empty()) {
    throw std::invalid_argument("Empty path expression");
  }

  // Handle wildcard
  if (pathExpr == "*") {
    return wildcard();
  }

  // Handle inverse path (^property)
  if (pathExpr[0] == '^') {
    std::string innerExpr = pathExpr.substr(1);
    return inverse(parse(innerExpr));
  }

  // Handle suffix operators (*, +, ?)
  if (pathExpr.back() == '*' && pathExpr.size() > 1) {
    std::string innerExpr = pathExpr.substr(0, pathExpr.size() - 1);
    return zeroOrMore(parse(innerExpr));
  }
  if (pathExpr.back() == '+' && pathExpr.size() > 1) {
    std::string innerExpr = pathExpr.substr(0, pathExpr.size() - 1);
    return oneOrMore(parse(innerExpr));
  }
  if (pathExpr.back() == '?' && pathExpr.size() > 1) {
    std::string innerExpr = pathExpr.substr(0, pathExpr.size() - 1);
    return zeroOrOne(parse(innerExpr));
  }

  // Handle sequence (property1/property2)
  size_t slashPos = pathExpr.find('/');
  if (slashPos != std::string::npos) {
    std::vector<PropertyPath> paths;
    size_t start = 0;
    while (start < pathExpr.size()) {
      size_t end = pathExpr.find('/', start);
      if (end == std::string::npos) {
        end = pathExpr.size();
      }
      std::string segment = pathExpr.substr(start, end - start);
      if (!segment.empty()) {
        paths.push_back(parse(segment));
      }
      start = end + 1;
    }
    return sequence(std::move(paths));
  }

  // Handle alternative (property1|property2)
  size_t pipePos = pathExpr.find('|');
  if (pipePos != std::string::npos) {
    std::vector<PropertyPath> paths;
    size_t start = 0;
    while (start < pathExpr.size()) {
      size_t end = pathExpr.find('|', start);
      if (end == std::string::npos) {
        end = pathExpr.size();
      }
      std::string segment = pathExpr.substr(start, end - start);
      if (!segment.empty()) {
        paths.push_back(parse(segment));
      }
      start = end + 1;
    }
    return alternative(std::move(paths));
  }

  // Default: simple path
  return simple(pathExpr);
}

// Helper functions

bool isTransitivePath(const PropertyPath& path) {
  PathType type = path.getType();
  return type == PathType::ZeroOrMore || type == PathType::OneOrMore ||
         type == PathType::ZeroOrOne;
}

bool requiresRecursion(const PropertyPath& path) {
  PathType type = path.getType();
  return type == PathType::ZeroOrMore || type == PathType::OneOrMore ||
         type == PathType::Sequence || type == PathType::Alternative;
}

PropertyPath simplifyPath(const PropertyPath& path) {
  return std::visit(
      [](const auto& p) -> PropertyPath {
        using T = std::decay_t<decltype(p)>;

        // Simplify sequences with single element
        if constexpr (std::is_same_v<T, SequencePath>) {
          if (p.paths.size() == 1 && p.paths[0]) {
            return simplifyPath(*p.paths[0]);
          }
        }

        // Simplify alternatives with single element
        if constexpr (std::is_same_v<T, AlternativePath>) {
          if (p.paths.size() == 1 && p.paths[0]) {
            return simplifyPath(*p.paths[0]);
          }
        }

        // For other types, return as-is
        return PropertyPath(p);
      },
      path.getVariant());
}

}  // namespace shacl
