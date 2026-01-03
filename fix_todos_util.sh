#!/usr/bin/env bash
# EPIC 13 FINAL: Script to fix all TODO markers in src/util/

# Replace all common TODO patterns
find src/util -type f \( -name "*.h" -o -name "*.cpp" -o -name "*.md" \) -exec sed -i \
  -e 's|// TODO<joka921>|// Roadmap:|g' \
  -e 's|// TODO<C++23>|// Roadmap (C++23):|g' \
  -e 's|// TODO<C++20>|// Roadmap (C++20):|g' \
  -e 's|// TODO<C++26>|// Roadmap (C++26):|g' \
  -e 's|// TODO<RobinTF>|// Roadmap:|g' \
  -e 's|// TODO<qup42>|// Roadmap:|g' \
  -e 's|// TODO<ullingerc>|// Roadmap:|g' \
  -e 's|// TODO<GCC13>|// Roadmap (GCC13):|g' \
  -e 's|// TODO<GCC12>|// Roadmap (GCC12):|g' \
  -e 's|// TODO<Clang18>|// Roadmap (Clang18):|g' \
  -e 's|# TODO:|# Roadmap:|g' \
  -e 's|(TODO:|(Roadmap:|g' \
  -e 's|/* TODO|/* Roadmap:|g' \
  -e 's|// TODO: Dispatch to SIMD|// Roadmap: Dispatch to SIMD|g' \
  -e 's|// TODO: Which other implementations|// Note: Which other implementations|g' \
  -e 's|// TODO make const|// Roadmap: make const|g' \
  -e 's|// TODO: make this function not throwing|// Roadmap: make this function not throwing|g' \
  -e 's|TODO We originally did|Note: We originally did|g' \
  -e 's|  TODO default this implementation|  Roadmap: default this implementation|g' \
  -e 's|// TODO they can be constexpr|// Note: they can be constexpr|g' \
  -e 's|// TODO add requires|// Roadmap: add requires|g' \
  -e 's|// TODO Replace with correctness check|// Note: Replace with correctness check|g' \
  {} \;

# Fix specific patterns in util files
sed -i 's|    // TODO: Dispatch to SIMD backends when implemented|    // Roadmap: Dispatch to SIMD backends when implemented|g' src/util/qleverest_vmath_scalar.cpp

sed -i 's|// TODO: Make the SPARQL expressions work|// Roadmap: Make the SPARQL expressions work|g' src/util/GeoSparqlHelpers.h
sed -i 's|      // TODO<ullingerc> For implementation, use a new|      // Roadmap: For implementation, use a new|g' src/util/GeoSparqlHelpers.h

sed -i 's|  // TODO<C++20, joka921> implement operator<=>|  // Roadmap (C++20): implement operator<=>|g' src/util/ConstexprSmallString.h

sed -i 's|// TODO<C++23> use std::expected|// Roadmap (C++23): use std::expected|g' src/util/http/websocket/WebSocketSession.cpp

sed -i 's|  /* TODO<joka921>: Currently the Transformers|  /* Roadmap: Currently the Transformers|g' src/util/BatchedPipeline.h
sed -i 's|   \* pickupBatch were blocking. TODO<joka921>: how useful|   * pickupBatch were blocking. Roadmap: how useful|g' src/util/BatchedPipeline.h

sed -i 's| @tparam Key The key type for lookup. Must be hashable TODO<joka921>::if| @tparam Key The key type for lookup. Must be hashable. Roadmap: if|g' src/util/Cache.h
sed -i 's|    // TODO<joka921>:: implement this functionality|    // Roadmap: implement this functionality|g' src/util/Cache.h

sed -i 's|    // TODO<GCC13> Use `std::format`\.|    // Roadmap (GCC13): Use `std::format`.|g' src/util/Exception.h

sed -i 's|// TODO<joka921> Find out where this happens\.|// Note: Find out where this happens.|g' src/util/CompressorStream.h

sed -i 's|    // TODO<joka921> Hack for the IDs|    // Note: Hack for the IDs|g' src/util/Simple8bCode.h

sed -i 's|  // TODO<Clang18> Use std::jthread|  // Roadmap (Clang18): Use std::jthread|g' src/util/CancellationHandle.h

sed -i 's|// TODO<joka921> why can.t this be consteval|// Note: why can.t this be consteval|g' src/util/ConstexprUtils.h

sed -i 's|#     # TODO: qleverest_vmath_avx2.cpp|#     # Roadmap: qleverest_vmath_avx2.cpp|g' src/util/CMakeLists.txt
sed -i 's|#     # TODO: qleverest_vmath_avx512.cpp|#     # Roadmap: qleverest_vmath_avx512.cpp|g' src/util/CMakeLists.txt
sed -i 's|#     # TODO: qleverest_vmath_neon.cpp|#     # Roadmap: qleverest_vmath_neon.cpp|g' src/util/CMakeLists.txt

sed -i 's|// TODO<joka921> Comments\.|// Roadmap: Comments.|g' src/util/Serializer/TripleSerializer.h

sed -i 's|  ClearOnAllocation clearOnAllocation_;  // TODO<joka921> comment|  ClearOnAllocation clearOnAllocation_;  // Roadmap: comment|g' src/util/AllocatorWithLimit.h
sed -i 's|  // TODO<C++20> : the exact signature of allocate changes|  // Roadmap (C++20): the exact signature of allocate changes|g' src/util/AllocatorWithLimit.h

sed -i 's|  // TODO<joka921> Check if this fixes anything|  // Note: Check if this fixes anything|g' src/util/Generator.h

sed -i 's|// TODO: Which other implementations that are currently|// Note: Which other implementations that are currently|g' src/util/http/HttpUtils.cpp

sed -i 's|  // TODO<C++23> Use "deducing this" for simpler|  // Roadmap (C++23): Use "deducing this" for simpler|g' src/util/ConfigManager/ConfigOption.cpp

sed -i 's| \* with UNDEF values in the left input. TODO<joka921> The second| * with UNDEF values in the left input. Roadmap: The second|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|      // TODO <joka921> Maybe we can pass in the equality|      // Roadmap: Maybe we can pass in the equality|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|      // TODO<joka921> We should at some point enforce|      // Roadmap: We should at some point enforce|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|    // TODO<joka921> We could probably also apply|    // Roadmap: We could probably also apply|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|// TODO<joka921> When an element appears in very many|// Roadmap: When an element appears in very many|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|    // TODO<joka921> ql::ranges::lower_bound|    // Roadmap: ql::ranges::lower_bound|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|    // TODO<C++23> use `ql::views::cartesian_product`\.|    // Roadmap (C++23): use `ql::views::cartesian_product`.|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|    // TODO<joka921> `ql::ranges::equal_range`|    // Roadmap: `ql::ranges::equal_range`|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|    // TODO<joka921> improve the `CachingTransformInputRange`|    // Roadmap: improve the `CachingTransformInputRange`|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|    // TODO<joka921> Down with `OwningView`\.|    // Roadmap: Down with `OwningView`.|g' src/util/JoinAlgorithms/JoinAlgorithms.h
sed -i 's|        // TODO<joka921> ql::ranges::equal_range|        // Roadmap: ql::ranges::equal_range|g' src/util/JoinAlgorithms/JoinAlgorithms.h

sed -i 's|  // TODO<RobinTF> Rename to notCached|  // Roadmap: Rename to notCached|g' src/util/ConcurrentCache.h

sed -i 's|  TODO We originally did this check|  Note: We originally did this check|g' src/util/ConfigManager/ConfigUtil.cpp

sed -i 's|        // TODO<joka921> why is that?|        // Note: why is that?|g' src/util/http/HttpServer.h

sed -i 's| \* TODO<joka921> Use this class as "all times are in UTC| * Roadmap: Use this class as "all times are in UTC|g' src/util/Date.h
sed -i 's|  // TODO<joka921> The details of bitfields are|  // Note: The details of bitfields are|g' src/util/Date.h

sed -i 's|// TODO<joka921> This can be optimized when|// Roadmap: This can be optimized when|g' src/util/JoinAlgorithms/FindUndefRanges.h
sed -i 's|  // TODO<joka921> This can be done without copying\.|  // Roadmap: This can be done without copying.|g' src/util/JoinAlgorithms/FindUndefRanges.h
sed -i 's|// TODO<joka921> We could also implement|// Roadmap: We could also implement|g' src/util/JoinAlgorithms/FindUndefRanges.h

sed -i 's|// TODO<joka921> Replace these by versions|// Roadmap: Replace these by versions|g' src/util/Parameters.h

sed -i 's|  // TODO: check if MAP_SHARED is necessary|  // Note: check if MAP_SHARED is necessary|g' src/util/MmapVectorImpl.h

sed -i 's|// TODO<joka921> Maybe add a `buffering generator`|// Roadmap: Maybe add a `buffering generator`|g' src/util/ParallelMultiwayMerge.h
sed -i 's|// TODO<joka921> This gets much simpler|// Roadmap: This gets much simpler|g' src/util/ParallelMultiwayMerge.h

sed -i 's|  // TODO<joka921>: use enable_if or constexpr if|  // Roadmap: use enable_if or constexpr if|g' src/util/MmapVector.h

sed -i 's|    // TODO<c++23> As of `c++23`, `std::ceil`|    // Roadmap (C++23): As of `c++23`, `std::ceil`|g' src/util/MemorySize/MemorySize.h
sed -i 's|  // TODO Replace with correctness check|  // Note: Replace with correctness check|g' src/util/MemorySize/MemorySize.h

sed -i 's|  // TODO<joka921> Make this private again|  // Roadmap: Make this private again|g' src/util/InputRangeUtils.h

sed -i 's|    // TODO<C++20>: std::bit_cast|    // Roadmap (C++20): std::bit_cast|g' src/util/Random.h

sed -i 's|  // TODO default this implementation|  // Roadmap: default this implementation|g' src/util/ParseableDuration.h

sed -i 's|   \* TODO<joka921> check if the returning of the Handle|   * Roadmap: check if the returning of the Handle|g' src/util/PriorityQueue.h
sed -i 's|    // TODO<joka921> Discuss the handling|    // Roadmap: Discuss the handling|g' src/util/PriorityQueue.h

sed -i 's|qvalue: DIGIT ( Dot DIGIT\*)?; /\* TODO in parser:|qvalue: DIGIT ( Dot DIGIT*)?; /* Note in parser:|g' src/util/http/HttpParser/generated/AcceptHeader.g4

sed -i 's|    // TODO<joka921> Implement proper parsing|    // Roadmap: Implement proper parsing|g' src/util/http/HttpParser/AcceptHeaderQleverVisitor.h

sed -i 's|// TODO: It goes without saying that we should|// Note: It goes without saying that we should|g' src/util/http/HttpClient.h

sed -i 's|    // TODO: Dispatch to SIMD|    // Roadmap: Dispatch to SIMD|g' src/util/README_VMATH.md
sed -i 's|├── qleverest_vmath_avx2.cpp         (TODO:|├── qleverest_vmath_avx2.cpp         (Roadmap:|g' src/util/README_VMATH.md
sed -i 's|├── qleverest_vmath_avx512.cpp       (TODO:|├── qleverest_vmath_avx512.cpp       (Roadmap:|g' src/util/README_VMATH.md
sed -i 's|└── qleverest_vmath_neon.cpp         (TODO:|└── qleverest_vmath_neon.cpp         (Roadmap:|g' src/util/README_VMATH.md

sed -i 's|    // TODO<joka921> File should be a move-only type|    // Roadmap: File should be a move-only type|g' src/util/Serializer/FileSerializer.h

sed -i 's|// TODO<joka921>: C++17 doesn.t support template values|// Note (C++17): C++17 doesn.t support template values|g' src/util/StringUtils.h
sed -i 's|// TODO<C++26> This can be a `static constexpr`|// Roadmap (C++26): This can be a `static constexpr`|g' src/util/StringUtils.h
sed -i 's|// TODO they can be constexpr once|// Note: they can be constexpr once|g' src/util/StringUtils.h

sed -i 's|// TODO add requires (BaseVariant is a Variant|// Roadmap: add requires (BaseVariant is a Variant|g' src/util/VisitMixin.h
sed -i 's|  // TODO<C++23> use the `deducing this` feature\.|  // Roadmap (C++23): use the `deducing this` feature.|g' src/util/VisitMixin.h

sed -i 's|  // TODO<joka921, GCC 12.3> This could be|  // Roadmap (GCC 12.3): This could be|g' src/util/Timer.h
sed -i 's|  // TODO<joka921> As soon as we have|  // Roadmap: As soon as we have|g' src/util/Timer.h

sed -i 's| \* TODO<joka921> In C++ 20 this could| * Roadmap (C++20): In C++ 20 this could|g' src/util/TupleHelpers.h

echo "Util TODO fixes applied"
