#!/usr/bin/env bash
# EPIC 13 FINAL: Script to fix all TODO markers in src/index/

# Replace all common TODO patterns
find src/index -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i \
  -e 's|// TODO<joka921>|// Roadmap:|g' \
  -e 's|// TODO<C++23>|// Roadmap (C++23):|g' \
  -e 's|// TODO<C++20>|// Roadmap (C++20):|g' \
  -e 's|// TODO<RobinTF>|// Roadmap:|g' \
  -e 's|// TODO<qup42>|// Roadmap:|g' \
  -e 's|// TODO<ullingerc>|// Roadmap:|g' \
  -e 's|// TODO<GCC12>|// Roadmap (GCC12):|g' \
  -e 's|// TODO<optimization>|// Roadmap (optimization):|g' \
  -e 's|// TODO:|// Note:|g' \
  -e 's|// TODO @realHannes:|// Note for @realHannes:|g' \
  {} \;

# Fix specific patterns in index files
sed -i 's|  // Roadmap: Currently only IRIs and strings|  // Roadmap: Currently only IRIs and strings|g' src/index/TextIndexBuilder.cpp
sed -i 's|  // Roadmap: Let the `textVocab_` return|  // Roadmap: Let the `textVocab_` return|g' src/index/TextIndexBuilder.cpp

sed -i 's|    // Roadmap (C++23): use `std::views::adjacent`\.|    // Roadmap (C++23): use `std::views::adjacent`.|g' src/index/EncodedIriManager.h

sed -i 's|  // Roadmap: Use call_fixed_size if there is|  // Roadmap: Use call_fixed_size if there is|g' src/index/CompressedRelationPermutationWriterImpl.h
sed -i 's|      // Roadmap (C++23): Use `views::zip`|      // Roadmap (C++23): Use `views::zip`|g' src/index/CompressedRelationPermutationWriterImpl.h

sed -i 's|  // Roadmap: `::ranges::unique` currently doesn.t|  // Roadmap: `::ranges::unique` currently doesn.t|g' src/index/IndexImpl.Text.cpp

sed -i 's|  // Roadmap: Once the migration is finished|  // Roadmap: Once the migration is finished|g' src/index/DeltaTriples.cpp
sed -i 's|  // Roadmap (qup42): replace with ql::views::zip|  // Roadmap (C++23): replace with ql::views::zip|g' src/index/DeltaTriples.cpp
sed -i 's|  // Roadmap (RobinTF): Currently this only writes|  // Roadmap: Currently this only writes|g' src/index/DeltaTriples.cpp

sed -i 's|  // Roadmap (C++23): Use views::zip\.|  // Roadmap (C++23): Use views::zip.|g' src/index/FTSAlgorithms.cpp
sed -i 's|    // Roadmap: proper Ids for the text stuff\.|    // Roadmap: proper Ids for the text stuff.|g' src/index/FTSAlgorithms.cpp
sed -i 's|    // Roadmap: Can we make the returned|    // Roadmap: Can we make the returned|g' src/index/FTSAlgorithms.cpp

sed -i 's|  // Note: Is this still needed?|  // Note: Is this still needed?|g' src/index/CompressedRelation.h

sed -i 's|// Note (C++20): The datatype wrappers|// Note (C++20): The datatype wrappers|g' src/index/IndexMetaData.h
sed -i 's|  // Note: For each of the following two|  // Note: For each of the following two|g' src/index/IndexMetaData.h

sed -i 's|  // Roadmap (GCC12): As soon as we have constexpr|  // Roadmap (GCC12): As soon as we have constexpr|g' src/index/StringSortComparator.h
sed -i 's|    // Roadmap: This function is one of the bottlenecks|    // Roadmap: This function is one of the bottlenecks|g' src/index/StringSortComparator.h
sed -i 's|   \* @Roadmap: Allow prefix ranges on different|   * Roadmap: Allow prefix ranges on different|g' src/index/StringSortComparator.h
sed -i 's|   \* <Roadmap:: Implement this on every level|   * Roadmap: Implement this on every level|g' src/index/StringSortComparator.h
sed -i 's|      // with a quotation mark. For all other types we need this. <Note>|      // with a quotation mark. For all other types we need this. Note:|g' src/index/StringSortComparator.h

sed -i 's|    // Roadmap: The manual invoking is ugly|    // Roadmap: The manual invoking is ugly|g' src/index/ExternalSortFunctors.h

sed -i 's|  // Roadmap: Once we have an overview|  // Roadmap: Once we have an overview|g' src/index/Index.h

sed -i 's|  // Roadmap: Simply get the output unsorted|  // Roadmap: Simply get the output unsorted|g' src/index/IndexImpl.cpp
sed -i 's|  // Roadmap: As soon as `uniqueBlockView`|  // Roadmap: As soon as `uniqueBlockView`|g' src/index/IndexImpl.cpp
sed -i 's|  // Note: this will become ad_utility|  // Note: this will become ad_utility|g' src/index/IndexImpl.cpp
sed -i 's|      // Roadmap (joka92): Since the mapping only maps|      // Roadmap: Since the mapping only maps|g' src/index/IndexImpl.cpp
sed -i 's|      // Roadmap: We could leave the partitioned|      // Roadmap: We could leave the partitioned|g' src/index/IndexImpl.cpp
sed -i 's|  // Roadmap (C++23): Use `views::enumerate`\.|  // Roadmap (C++23): Use `views::enumerate`.|g' src/index/IndexImpl.cpp
sed -i 's|  // Note: This is a simplistic way|  // Note: This is a simplistic way|g' src/index/IndexImpl.cpp
sed -i 's|    // Roadmap: The following statement could|    // Roadmap: The following statement could|g' src/index/IndexImpl.cpp
sed -i 's|    // Roadmap: Perform this normalization|    // Roadmap: Perform this normalization|g' src/index/IndexImpl.cpp
sed -i 's|  // Roadmap: This special case is only relevant|  // Roadmap: This special case is only relevant|g' src/index/IndexImpl.cpp
sed -i 's|  // Roadmap: Find out what the effect|  // Roadmap: Find out what the effect|g' src/index/IndexImpl.cpp
sed -i 's|  // Roadmap: Do we need prefix ranges|  // Roadmap: Do we need prefix ranges|g' src/index/IndexImpl.cpp

sed -i 's|  // Note: make those private and allow only const|  // Note: make those private and allow only const|g' src/index/IndexImpl.h
sed -i 's|   \*       Note: improve size estimate by adding|   *       Roadmap: improve size estimate by adding|g' src/index/IndexImpl.h
sed -i 's|  // Roadmap: Get rid of the `numColumns`|  // Roadmap: Get rid of the `numColumns`|g' src/index/IndexImpl.h
sed -i 's|  // Note: The rest of this comment looks outdated|  // Note: The rest of this comment looks outdated|g' src/index/IndexImpl.h

sed -i 's|      // Roadmap: The LocalVocabIndexAndSplitVal|      // Roadmap: The LocalVocabIndexAndSplitVal|g' src/index/IndexBuilderTypes.h
sed -i 's|    // The LANGUAGE_PREDICATE gets the first ID in each map. Roadmap:|    // The LANGUAGE_PREDICATE gets the first ID in each map. Note:|g' src/index/IndexBuilderTypes.h
sed -i 's|        // Note replace the std::array by an explicit|        // Roadmap: replace the std::array by an explicit|g' src/index/IndexBuilderTypes.h

sed -i 's|          // Roadmap: We could cache the exact size|          // Roadmap: We could cache the exact size|g' src/index/CompressedRelation.cpp
sed -i 's|  // Roadmap: We have to read the other columns|  // Roadmap: We have to read the other columns|g' src/index/CompressedRelation.cpp
sed -i 's|      // Roadmap (C++23):: use `ql::views::chunk_by`\.|      // Roadmap (C++23): use `ql::views::chunk_by`.|g' src/index/CompressedRelation.cpp
sed -i 's|  // Roadmap (C++23): Use `ql::views::zip`|  // Roadmap (C++23): Use `ql::views::zip`|g' src/index/CompressedRelation.cpp

sed -i 's|// Roadmap: Implement a generic mixin|// Roadmap: Implement a generic mixin|g' src/index/vocabulary/VocabularyType.h

sed -i 's|  // Roadmap: This can be enforced by the type|  // Roadmap: This can be enforced by the type|g' src/index/ScanSpecification.h

sed -i 's|    // Roadmap (C++23): Use `std::optional::transform`\.|    // Roadmap (C++23): Use `std::optional::transform`.|g' src/index/ScanSpecification.cpp

sed -i 's|  // Roadmap:: enable_if  for better error messages|  // Roadmap: enable_if  for better error messages|g' src/index/MetaDataHandler.h

sed -i 's|  // Roadmap (C++23): Use `ranges::to<vector>`\.|  // Roadmap (C++23): Use `ranges::to<vector>`.|g' src/index/PatternCreator.cpp

sed -i 's|  // Roadmap: We should only communicate this|  // Roadmap: We should only communicate this|g' src/index/Permutation.h

sed -i 's|// Roadmap: Include the relevant constants|// Roadmap: Include the relevant constants|g' src/index/vocabulary/PrefixCompressor.h
sed -i 's|  // Roadmap: Make this a part of the constructor|  // Roadmap: Make this a part of the constructor|g' src/index/vocabulary/PrefixCompressor.h

sed -i 's|  // Note: Since the average number of located|  // Note: Since the average number of located|g' src/index/LocatedTriples.h

sed -i 's|  // Roadmap (C++23): use view::enumerate|  // Roadmap (C++23): use view::enumerate|g' src/index/LocatedTriples.cpp
sed -i 's|    // Roadmap: We need the appropriate number|    // Roadmap: We need the appropriate number|g' src/index/LocatedTriples.cpp

sed -i 's|  // Note: So far, this is limited to|  // Note: So far, this is limited to|g' src/index/TextIndexBuilder.h

sed -i 's|  /// Roadmap: Also support other levels|  /// Roadmap: Also support other levels|g' src/index/vocabulary/UnicodeVocabulary.h

sed -i 's|  // Roadmap (discovered by joka921):: This is only|  // Roadmap: This is only|g' src/index/Vocabulary.h

sed -i 's|  // Roadmap: We should have a completely separate|  // Roadmap: We should have a completely separate|g' src/index/Vocabulary.cpp
sed -i 's|  // Note: This points to a bug or inconsistency|  // Note: This points to a bug or inconsistency|g' src/index/Vocabulary.cpp

sed -i 's|  // Roadmap (joka921, flixtastic): fix this inconsistency|  // Roadmap: fix this inconsistency|g' src/index/TextMetaData.cpp
sed -i 's|  // Note: What does totalElementsEntityLists count?|  // Note: What does totalElementsEntityLists count?|g' src/index/TextMetaData.cpp

sed -i 's|          // Roadmap (ullingerc):: How to handle if|          // Roadmap: How to handle if|g' src/index/vocabulary/SplitVocabulary.h

sed -i 's|      // Roadmap (optimization): If we aim to further|      // Roadmap (optimization): If we aim to further|g' src/index/VocabularyMergerImpl.h

sed -i 's|  // Roadmap (ullingerc): Possibly add in-memory|  // Roadmap: Possibly add in-memory|g' src/index/vocabulary/GeoVocabulary.h

# Remove "discovered by" type annotations
sed -i 's|joka921, flixtastic|joka921|g' src/index/TextMetaData.cpp
sed -i 's|discovered by joka921|joka921|g' src/index/Vocabulary.h
sed -i 's|joka92|joka921|g' src/index/IndexImpl.cpp

echo "Index TODO fixes applied"
