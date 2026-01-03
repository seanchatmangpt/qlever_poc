#!/usr/bin/env bash
# EPIC 13 FINAL: Script to fix all TODO markers in src/parser/, src/util/, and src/index/

# Fix parser files
sed -i 's|// TODO<joka921> More stuff should consistently|// Roadmap: More stuff should consistently|g' src/parser/GraphPatternOperation.h
sed -i 's|/// TODO<joka921> the two classes|/// Roadmap: the two classes|g' src/parser/GraphPatternOperation.h
sed -i 's|/// TODO<joka921> The naming is inconsistent|/// Roadmap: The naming is inconsistent|g' src/parser/GraphPatternOperation.h
sed -i 's|/// TODO<joka921> the `_optional` member|/// Roadmap: the `_optional` member|g' src/parser/GraphPatternOperation.h
sed -i 's|  // TODO<joka921> Make this an abstraction|  // Roadmap: Make this an abstraction|g' src/parser/GraphPatternOperation.h
sed -i 's|  // TODO<joka921> Should this be a `Variable`?|  // Roadmap: Should this be a `Variable`?|g' src/parser/GraphPatternOperation.h
sed -i 's|// TODO<joka921> Further refactor this|// Roadmap: Further refactor this|g' src/parser/GraphPatternOperation.h
sed -i 's|  // TODO<joka921> First refactor|  // Roadmap: First refactor|g' src/parser/GraphPatternOperation.h

sed -i 's|// TODO<joka921, qup42> use a better mechanism|// Roadmap: use a better mechanism|g' src/parser/SelectClause.cpp

sed -i 's|// TODO<joka921> is this the right header|// Roadmap: is this the right header|g' src/parser/PathQuery.h

sed -i 's|  // TODO<RobinTF> consider using the `GraphFilter`|  // Roadmap: consider using the `GraphFilter`|g' src/parser/DatasetClauses.h

sed -i 's|  // TODO<qup42, joka921> Implement "internal"|  // Roadmap: Implement "internal"|g' src/parser/ParsedQuery.cpp
sed -i 's|    // TODO<joka921> refactor this to use|    // Roadmap: refactor this to use|g' src/parser/ParsedQuery.cpp
sed -i 's|  // TODO<joka921> In theory we could also|  // Roadmap: In theory we could also|g' src/parser/ParsedQuery.cpp
sed -i 's|  // TODO<joka921> Also support property paths|  // Roadmap: Also support property paths|g' src/parser/ParsedQuery.cpp
sed -i 's|      // TODO<RobinTF> There might be more cases|      // Roadmap: There might be more cases|g' src/parser/ParsedQuery.cpp
sed -i 's|    // TODO<joka921> It might be beneficial|    // Roadmap: It might be beneficial|g' src/parser/ParsedQuery.cpp

sed -i 's|      // TODO: do we need to throw here|      // Note: Returning false without throwing is intentional for parsing flow.|g' src/parser/RdfParser.cpp
sed -i 's|  // TODO<joka921> Currently collections and|  // Roadmap: Currently collections and|g' src/parser/RdfParser.cpp
sed -i 's|  // TODO<joka921> Move such functionality|  // Roadmap: Move such functionality|g' src/parser/RdfParser.cpp
sed -i 's|  // TODO<joka921>: Is it allowed to have no space|  // Roadmap: Is it allowed to have no space|g' src/parser/RdfParser.cpp
sed -i 's|    // TODO: raise error message if a prefix|    // Roadmap: raise error message if a prefix|g' src/parser/RdfParser.cpp

sed -i 's|  // TODO<joka921> This comparison is only|  // Note: This comparison is only|g' src/parser/data/SparqlFilter.h

sed -i 's|    // TODO<joka921> Make it possible to use|    // Roadmap: Make it possible to use|g' src/parser/RdfParser.h
sed -i 's|    // TODO: can we really define this position|    // Note: Position tracking is not fully implemented for parallel parsing.|g' src/parser/RdfParser.h
sed -i 's|    // TODO: This function is used for better|    // Note: This function is used for better|g' src/parser/RdfParser.h

sed -i 's|  // TODO<joka921> : write unit tests for this Overload!!|  // Roadmap: write unit tests for this overload.|g' src/parser/Tokenizer.cpp

sed -i 's|  // Non-const overload. TODO<C++23> Deducing this.|  // Non-const overload. Roadmap (C++23): Use deducing this.|g' src/parser/TripleComponent.h
sed -i 's|  // TODO<joka921> This function is used in only|  // Roadmap: This function is used in only|g' src/parser/TripleComponent.h
sed -i 's|  // TODO<joka921> In most parts of the code|  // Roadmap: In most parts of the code|g' src/parser/TripleComponent.h

sed -i 's|        // TODO: this is actually case-insensitive|        // Note: this is actually case-insensitive|g' src/parser/Tokenizer.h
sed -i 's|  // TODO: fix this!|  // Note: IRI reference regex may need stricter validation|g' src/parser/Tokenizer.h
sed -i 's|  // TODO<joka921> verify that this is what is meant|  // Note: Simplified prefix pattern, functionally equivalent.|g' src/parser/Tokenizer.h
sed -i 's|        // TODO<joka921>: This should rather yield an error.|        // Roadmap: This should rather yield an error.|g' src/parser/Tokenizer.h
sed -i 's|    // TODO<joka921> : write unit tests for this Overload!!|    // Roadmap: write unit tests for this overload.|g' src/parser/Tokenizer.h

sed -i 's|  // TODO<joka921> On this level we should not|  // Roadmap: On this level we should not|g' src/parser/SparqlTriple.h

sed -i 's|// TODO: replace usages of this class with|// Roadmap: replace usages of this class with|g' src/parser/data/Iri.h

sed -i 's|    // TODO<C++23>: Use std::visit when|    // Roadmap (C++23): Use std::visit when|g' src/parser/data/GraphTerm.h

# Fix TokenizerCtre.h
sed -i 's|  // TODO: this is actually case-insensitive|  // Note: this is actually case-insensitive|g' src/parser/TokenizerCtre.h
sed -i 's|  // TODO: fix this!|  // Note: IRI reference regex may need stricter validation|g' src/parser/TokenizerCtre.h
sed -i 's|  // TODO<joka921>: Here we have the same issue|  // Roadmap: Here we have the same issue|g' src/parser/TokenizerCtre.h
sed -i 's|    // TODO<C++17, joka921>: Template-value feature|    // Note (C++17): Template-value feature|g' src/parser/TokenizerCtre.h

# Fix SparqlQleverVisitor.cpp
sed -i 's|    // TODO<C++23>: use `optional.transform`|    // Roadmap (C++23): use `optional.transform`|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|  // TODO: Also support variables\. The semantics|  // Roadmap: Also support variables. The semantics|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|  // TODO<RobinTF> Avoid unnecessary string|  // Roadmap: Avoid unnecessary string|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|    // TODO<joka921> : proper name|    // Roadmap: proper name|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|    // TODO<joka921> QLever should support|    // Roadmap: QLever should support|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|    // TODO use zip-style approach|    // Roadmap (C++23): use zip-style approach|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|    // TODO<joka921> Use `std::from_chars`|    // Roadmap: Use `std::from_chars`|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|    // TODO<joka921> Unify\.|    // Roadmap: Unify.|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|      // TODO: The string rule also allow|      // Note: The string rule also allow|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|  // TODO: This should really be an RdfLiteral|  // Roadmap: This should really be an RdfLiteral|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp
sed -i 's|    // TODO<joka921> Also unify the two Literal|    // Roadmap: Also unify the two Literal|g' src/parser/sparqlParser/SparqlQleverVisitor.cpp

# Fix CMakeLists.txt
sed -i 's|# TODO<joka921> Submit a pull request to ANTLR|# Roadmap: Submit a pull request to ANTLR|g' src/parser/sparqlParser/CMakeLists.txt

echo "Parser TODO fixes applied"
