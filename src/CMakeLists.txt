# Create a library for the J interpreter components
add_library(j_interpreter_lib)

# Add sources from subdirectories
# We'll use target_sources to add files from subdirectories
# or define them as separate object libraries.
# For simplicity here, we'll collect sources.

target_sources(j_interpreter_lib PUBLIC
    # common
    common/source_location.hpp # Header-only for now

    # AST
    ast/ast_nodes.hpp
    ast/ast_nodes.cpp

    # Lexer
    lexer/token.hpp
    lexer/token.cpp
    lexer/lexer.hpp
    lexer/lexer.cpp

    # Parser
    parser/parser.hpp
    parser/parser.cpp

    # Interpreter
    interpreter/interpreter.hpp
    interpreter/interpreter.cpp
    interpreter/tf_operations.hpp
    interpreter/tf_operations.cpp

    # Optional public umbrella header
    j_interpreter_lib.hpp
)

target_include_directories(j_interpreter_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} # So includes like "ast/ast_nodes.hpp" work
)

# Public compile features (if any, e.g., for C++20)
target_compile_features(j_interpreter_lib PUBLIC cxx_std_20)

# Add TensorFlow if available
if(TENSORFLOW_FOUND)
    target_include_directories(j_interpreter_lib PUBLIC ${TENSORFLOW_INCLUDE_DIR})
    target_link_directories(j_interpreter_lib PUBLIC ${TENSORFLOW_LIB_DIR})
    target_link_libraries(j_interpreter_lib PUBLIC tensorflow_cc tensorflow_framework)
    target_compile_definitions(j_interpreter_lib PUBLIC TENSORFLOW_ENABLED)
    message(STATUS "Linking with TensorFlow libraries")
else()
    message(STATUS "Using TensorFlow stubs")
endif()
