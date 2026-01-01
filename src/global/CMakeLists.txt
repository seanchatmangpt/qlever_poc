add_library(global RuntimeParameters.cpp Epoch.cpp EpochMetrics.cpp EpochCacheInvalidationHook.cpp)
qlever_target_link_libraries(global)
