add_library(shex ShapeSchemaManager.cpp ShapeValidationOperation.cpp)
qlever_target_link_libraries(shex engine rdfTypes util nlohmann_json::nlohmann_json)
