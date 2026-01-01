#include "engine/shacl/PathResolver.h"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <unordered_map>

namespace shacl {

// InMemoryRdfData implementation

void InMemoryRdfData::addTriple(const std::string& subject,
                                const std::string& predicate,
                                const std::string& object) {
  triples_.emplace_back(subject, predicate, object);
}

void InMemoryRdfData::addTriples(const std::vector<Triple>& triples) {
  triples_.insert(triples_.end(), triples.begin(), triples.end());
}

void InMemoryRdfData::clear() { triples_.clear(); }

std::vector<std::string> InMemoryRdfData::getObjects(
    const std::string& subject, const std::string& predicate) const {
  std::vector<std::string> objects;
  for (const auto& triple : triples_) {
    if (triple.subject == subject && triple.predicate == predicate) {
      objects.push_back(triple.object);
    }
  }
  return objects;
}

std::vector<std::string> InMemoryRdfData::getSubjects(
    const std::string& predicate, const std::string& object) const {
  std::vector<std::string> subjects;
  for (const auto& triple : triples_) {
    if (triple.predicate == predicate && triple.object == object) {
      subjects.push_back(triple.subject);
    }
  }
  return subjects;
}

std::vector<std::string> InMemoryRdfData::getProperties(
    const std::string& subject) const {
  std::vector<std::string> properties;
  for (const auto& triple : triples_) {
    if (triple.subject == subject) {
      properties.push_back(triple.predicate);
    }
  }
  return properties;
}

std::vector<Triple> InMemoryRdfData::getAllTriples() const { return triples_; }

// PathResolver implementation

std::vector<std::string> PathResolver::resolve(const std::string& startNode,
                                               const PropertyPath& path) const {
  return std::visit(
      [this, &startNode](const auto& p) -> std::vector<std::string> {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, SimplePath>) {
          return resolveSimple(startNode, p);
        } else if constexpr (std::is_same_v<T, InversePath>) {
          return resolveInverse(startNode, p);
        } else if constexpr (std::is_same_v<T, SequencePath>) {
          return resolveSequence(startNode, p);
        } else if constexpr (std::is_same_v<T, AlternativePath>) {
          return resolveAlternative(startNode, p);
        } else if constexpr (std::is_same_v<T, ZeroOrMorePath>) {
          return resolveZeroOrMore(startNode, p);
        } else if constexpr (std::is_same_v<T, OneOrMorePath>) {
          return resolveOneOrMore(startNode, p);
        } else if constexpr (std::is_same_v<T, ZeroOrOnePath>) {
          return resolveZeroOrOne(startNode, p);
        } else {
          return resolveWildcard(startNode, p);
        }
      },
      path.getVariant());
}

std::set<std::string> PathResolver::resolveUnique(
    const std::string& startNode, const PropertyPath& path) const {
  auto values = resolve(startNode, path);
  return std::set<std::string>(values.begin(), values.end());
}

bool PathResolver::pathExists(const std::string& startNode,
                              const std::string& targetNode,
                              const PropertyPath& path) const {
  auto values = resolveUnique(startNode, path);
  return values.find(targetNode) != values.end();
}

size_t PathResolver::countValues(const std::string& startNode,
                                 const PropertyPath& path) const {
  return resolveUnique(startNode, path).size();
}

// Path type specific resolution methods

std::vector<std::string> PathResolver::resolveSimple(
    const std::string& startNode, const SimplePath& path) const {
  return dataProvider_->getObjects(startNode, path.propertyIri);
}

std::vector<std::string> PathResolver::resolveInverse(
    const std::string& startNode, const InversePath& path) const {
  if (!path.innerPath) {
    return {};
  }

  // For inverse paths, we need to find all nodes that reach startNode
  // via the inner path. This is equivalent to resolving the inner path
  // in reverse direction.

  // If inner path is simple, we can optimize
  if (path.innerPath->isSimple()) {
    std::string propertyIri = path.innerPath->getSimpleIri();
    return dataProvider_->getSubjects(propertyIri, startNode);
  }

  // For complex inner paths, we need to check all possible starting nodes
  // This is expensive, but correct
  std::vector<std::string> results;
  auto allTriples = dataProvider_->getAllTriples();

  // Collect all unique subjects
  std::unordered_set<std::string> allSubjects;
  for (const auto& triple : allTriples) {
    allSubjects.insert(triple.subject);
  }

  // For each potential subject, check if it reaches startNode via inner path
  for (const auto& subject : allSubjects) {
    if (pathExists(subject, startNode, *path.innerPath)) {
      results.push_back(subject);
    }
  }

  return results;
}

std::vector<std::string> PathResolver::resolveSequence(
    const std::string& startNode, const SequencePath& path) const {
  if (path.paths.empty()) {
    return {};
  }

  // Start with the initial node
  std::vector<std::string> currentNodes = {startNode};

  // Apply each path in sequence
  for (const auto& subPath : path.paths) {
    if (!subPath) continue;

    std::vector<std::string> nextNodes;
    for (const auto& node : currentNodes) {
      auto resolved = resolve(node, *subPath);
      nextNodes.insert(nextNodes.end(), resolved.begin(), resolved.end());
    }
    currentNodes = std::move(nextNodes);
  }

  return currentNodes;
}

std::vector<std::string> PathResolver::resolveAlternative(
    const std::string& startNode, const AlternativePath& path) const {
  std::vector<std::string> results;

  // Resolve each alternative path and collect all results
  for (const auto& subPath : path.paths) {
    if (!subPath) continue;
    auto resolved = resolve(startNode, *subPath);
    results.insert(results.end(), resolved.begin(), resolved.end());
  }

  return results;
}

std::vector<std::string> PathResolver::resolveZeroOrMore(
    const std::string& startNode, const ZeroOrMorePath& path) const {
  if (!path.innerPath) {
    return {startNode};  // Zero hops
  }

  return computeTransitiveClosure(startNode, *path.innerPath, true);
}

std::vector<std::string> PathResolver::resolveOneOrMore(
    const std::string& startNode, const OneOrMorePath& path) const {
  if (!path.innerPath) {
    return {};
  }

  return computeTransitiveClosure(startNode, *path.innerPath, false);
}

std::vector<std::string> PathResolver::resolveZeroOrOne(
    const std::string& startNode, const ZeroOrOnePath& path) const {
  std::vector<std::string> results = {startNode};  // Zero hops

  if (path.innerPath) {
    auto oneHop = resolve(startNode, *path.innerPath);
    results.insert(results.end(), oneHop.begin(), oneHop.end());
  }

  return results;
}

std::vector<std::string> PathResolver::resolveWildcard(
    const std::string& startNode, const WildcardPath& /*path*/) const {
  // Wildcard matches any property from startNode
  std::vector<std::string> results;
  auto allTriples = dataProvider_->getAllTriples();

  for (const auto& triple : allTriples) {
    if (triple.subject == startNode) {
      results.push_back(triple.object);
    }
  }

  return results;
}

// Transitive closure computation using BFS

std::vector<std::string> PathResolver::computeTransitiveClosure(
    const std::string& startNode, const PropertyPath& path,
    bool includeZeroHops) const {
  std::unordered_set<std::string> visited;
  std::unordered_set<std::string> results;
  std::queue<std::pair<std::string, size_t>> queue;

  // Add start node
  queue.push({startNode, 0});
  visited.insert(startNode);

  if (includeZeroHops) {
    results.insert(startNode);
  }

  while (!queue.empty()) {
    auto [currentNode, depth] = queue.front();
    queue.pop();

    // Check depth limit
    if (depth >= maxDepth_) {
      continue;
    }

    // Resolve one step of the path
    auto nextNodes = resolve(currentNode, path);

    for (const auto& nextNode : nextNodes) {
      results.insert(nextNode);

      // Add to queue if not visited
      if (visited.find(nextNode) == visited.end()) {
        visited.insert(nextNode);
        queue.push({nextNode, depth + 1});
      }
    }
  }

  return std::vector<std::string>(results.begin(), results.end());
}

// Recursive resolution helper

void PathResolver::resolveRecursive(
    const std::string& currentNode, const PropertyPath& path,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& results, size_t depth) const {
  // Check depth limit
  if (depth >= maxDepth_) {
    return;
  }

  // Mark as visited
  if (visited.find(currentNode) != visited.end()) {
    return;
  }
  visited.insert(currentNode);

  // Resolve one step
  auto nextNodes = resolve(currentNode, path);

  for (const auto& nextNode : nextNodes) {
    results.insert(nextNode);
    resolveRecursive(nextNode, path, visited, results, depth + 1);
  }
}

// Utility functions

std::vector<std::string> extractNodes(
    const std::vector<std::string>& resolvedValues) {
  // In this simple implementation, values are already nodes
  return resolvedValues;
}

std::vector<std::string> deduplicateNodes(
    const std::vector<std::string>& nodes) {
  std::unordered_set<std::string> uniqueNodes(nodes.begin(), nodes.end());
  return std::vector<std::string>(uniqueNodes.begin(), uniqueNodes.end());
}

}  // namespace shacl
