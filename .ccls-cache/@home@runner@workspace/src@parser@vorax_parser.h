#ifndef VORAX_PARSER_H
#define VORAX_PARSER_H

#include "../lum/lum_core.h"
#include "../vorax/vorax_operations.h"

// Token types for VORAX language
typedef enum {
    TOKEN_ZONE,           // zone
    TOKEN_MEM,            // mem
    TOKEN_EMIT,           // emit
    TOKEN_SPLIT,          // split
    TOKEN_MOVE,           // move
    TOKEN_STORE,          // store
    TOKEN_RETRIEVE,       // retrieve
    TOKEN_CYCLE,          // cycle
    TOKEN_FUSE,           // ⧉ or fuse
    TOKEN_ARROW,          // -> or →
    TOKEN_ASSIGN,         // :=
    TOKEN_MEMORY_REF,     // #alpha, #beta, etc.
    TOKEN_LUM_SYMBOL,     // •
    TOKEN_GROUP_START,    // (
    TOKEN_GROUP_END,      // )
    TOKEN_IDENTIFIER,     // A, B, C, etc.
    TOKEN_NUMBER,         // 1, 2, 3, etc.
    TOKEN_SEMICOLON,      // ;
    TOKEN_COMMA,          // ,
    TOKEN_PLUS,           // +
    TOKEN_PERCENT,        // %
    TOKEN_ON,             // on
    TOKEN_EMPTY,          // empty
    TOKEN_NOT,            // not
    TOKEN_IF,             // if
    TOKEN_EVERY,          // every
    TOKEN_MS,             // ms
    TOKEN_ALL,            // all
    TOKEN_CALCULATE,      // calculate
    TOKEN_TRANSFORM,      // transform  
    TOKEN_MATRIX,         // matrix
    TOKEN_VECTOR,         // vector
    TOKEN_DERIVATIVE,     // derivative
    TOKEN_INTEGRAL,       // integral
    TOKEN_SIN,            // sin
    TOKEN_COS,            // cos
    TOKEN_LOG,            // log
    TOKEN_EXP,            // exp
    TOKEN_SQRT,           // sqrt
    TOKEN_POW,            // pow
    TOKEN_COMPLEX,        // complex
    TOKEN_REAL,           // real
    TOKEN_IMAG,           // imag
    TOKEN_DOT,            // .
    TOKEN_MULTIPLY,       // *
    TOKEN_DIVIDE,         // /
    TOKEN_POWER,          // ^
    TOKEN_EQUALS,         // =
    TOKEN_EOF,            // End of file
    TOKEN_UNKNOWN         // Unknown token
} vorax_token_type_e;

// Token structure
typedef struct {
    vorax_token_type_e type;
    char value[64];
    size_t line;
    size_t column;
} vorax_token_t;

// AST node types
typedef enum {
    AST_PROGRAM,
    AST_ZONE_DECLARATION,
    AST_MEMORY_DECLARATION,
    AST_MEMORY_ASSIGNMENT,
    AST_EMIT_STATEMENT,
    AST_SPLIT_STATEMENT,
    AST_MOVE_STATEMENT,
    AST_STORE_STATEMENT,
    AST_RETRIEVE_STATEMENT,
    AST_CYCLE_STATEMENT,
    AST_FUSE_STATEMENT,
    AST_ON_STATEMENT,
    AST_EVERY_STATEMENT,
    AST_IF_STATEMENT,
    AST_CALCULATE_STATEMENT,
    AST_TRANSFORM_STATEMENT,
    AST_MATRIX_OPERATION,
    AST_VECTOR_OPERATION,
    AST_COMPLEX_OPERATION,
    AST_MATHEMATICAL_FUNCTION,
    AST_DERIVATIVE_OPERATION,
    AST_INTEGRAL_OPERATION
} vorax_ast_node_type_e;

// AST node structure
typedef struct vorax_ast_node {
    vorax_ast_node_type_e type;
    char data[256];
    struct vorax_ast_node** children;
    size_t child_count;
    size_t child_capacity;
} vorax_ast_node_t;

// Parser context
typedef struct {
    const char* input;
    size_t position;
    size_t line;
    size_t column;
    vorax_token_t current_token;
    bool has_error;
    char error_message[256];
} vorax_parser_context_t;

// Execution context for running VORAX programs
typedef struct {
    lum_zone_t** zones;
    size_t zone_count;
    size_t zone_capacity;
    lum_memory_t** memories;
    size_t memory_count;
    size_t memory_capacity;
    bool has_error;
    char error_message[256];
} vorax_execution_context_t;

// Lexer functions
void vorax_lexer_init(vorax_parser_context_t* ctx, const char* input);
vorax_token_t vorax_lexer_next_token(vorax_parser_context_t* ctx);
void vorax_lexer_skip_whitespace(vorax_parser_context_t* ctx);

// Parser functions
vorax_ast_node_t* vorax_parse(const char* input);
vorax_ast_node_t* vorax_parse_program(vorax_parser_context_t* ctx);
vorax_ast_node_t* vorax_parse_statement(vorax_parser_context_t* ctx);
vorax_ast_node_t* vorax_parse_zone_declaration(vorax_parser_context_t* ctx);
vorax_ast_node_t* vorax_parse_memory_declaration(vorax_parser_context_t* ctx);
vorax_ast_node_t* vorax_parse_emit_statement(vorax_parser_context_t* ctx);
vorax_ast_node_t* vorax_parse_split_statement(vorax_parser_context_t* ctx);
vorax_ast_node_t* vorax_parse_move_statement(vorax_parser_context_t* ctx);

// AST utility functions
vorax_ast_node_t* vorax_ast_create_node(vorax_ast_node_type_e type, const char* data);
void vorax_ast_add_child(vorax_ast_node_t* parent, vorax_ast_node_t* child);
void vorax_ast_destroy(vorax_ast_node_t* node);
void vorax_ast_print(const vorax_ast_node_t* node, int indent);

// Execution functions
vorax_execution_context_t* vorax_execution_context_create(void);
void vorax_execution_context_destroy(vorax_execution_context_t* ctx);
bool vorax_execute(vorax_execution_context_t* ctx, const vorax_ast_node_t* ast);
bool vorax_execute_statement(vorax_execution_context_t* ctx, const vorax_ast_node_t* node);

// Context management
lum_zone_t* vorax_context_find_zone(vorax_execution_context_t* ctx, const char* name);
lum_memory_t* vorax_context_find_memory(vorax_execution_context_t* ctx, const char* name);
bool vorax_context_add_zone(vorax_execution_context_t* ctx, const char* name);
bool vorax_context_add_memory(vorax_execution_context_t* ctx, const char* name);

// Utility functions
bool vorax_is_identifier_char(char c);
bool vorax_is_digit(char c);
int vorax_count_lum_symbols(const char* str);

#endif // VORAX_PARSER_H