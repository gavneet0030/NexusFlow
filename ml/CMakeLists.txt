add_library(nexusflow_ml
    feature_engineering/feature_extractor.cpp
    training/risk_model.cpp
    training/trained_risk_model.cpp
    inference_service/inference_service.cpp
)

target_include_directories(nexusflow_ml
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/feature_engineering
        ${CMAKE_CURRENT_SOURCE_DIR}/training
        ${CMAKE_CURRENT_SOURCE_DIR}/inference_service
)

target_compile_features(nexusflow_ml PUBLIC cxx_std_20)
