add_library(nexusflow_decision STATIC
    rule_engine/rule_engine.cpp
    risk_engine/risk_engine.cpp
    decision_engine/decision_engine.cpp

)

target_include_directories(
    nexusflow_decision
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_compile_features(
    nexusflow_decision
    PUBLIC
        cxx_std_20
)



target_sources(nexusflow_decision
    PRIVATE
        decision_engine/ml_decision_engine.cpp
)

# ML integration dependency
target_link_libraries(nexusflow_decision
    PUBLIC
        nexusflow_ml
)

target_sources(nexusflow_decision
    PRIVATE
        decision_engine/unified_decision_engine.cpp
)
