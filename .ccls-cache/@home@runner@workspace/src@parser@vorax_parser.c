#include "vorax_parser.h"
#include "../debug/memory_tracker.h"  // NOUVEAU: Pour TRACKED_MALLOC/FREE
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY/STRCAT
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Lexer implementation
void vorax_lexer_init(vorax_parser_context_t* ctx, const char* input) {
    ctx->input = input;
    ctx->position = 0;
    ctx->line = 1;
    ctx->column = 1;
    ctx->has_error = false;
    ctx->error_message[0] = '\0';
}

void vorax_lexer_skip_whitespace(vorax_parser_context_t* ctx) {
    while (ctx->input[ctx->position] && isspace(ctx->input[ctx->position])) {
        if (ctx->input[ctx->position] == '\n') {
            ctx->line++;
            ctx->column = 1;
        } else {
            ctx->column++;
        }
        ctx->position++;
    }
}

vorax_token_t vorax_lexer_next_token(vorax_parser_context_t* ctx) {
    vorax_token_t token = {TOKEN_UNKNOWN, "", ctx->line, ctx->column};
    
    vorax_lexer_skip_whitespace(ctx);
    
    if (!ctx->input[ctx->position]) {
        token.type = TOKEN_EOF;
        return token;
    }
    
    char c = ctx->input[ctx->position];
    
    // Single character tokens
    switch (c) {
        case '(':
            token.type = TOKEN_GROUP_START;
            token.value[0] = c;
            token.value[1] = '\0';
            ctx->position++;
            ctx->column++;
            return token;
        case ')':
            token.type = TOKEN_GROUP_END;
            token.value[0] = c;
            token.value[1] = '\0';
            ctx->position++;
            ctx->column++;
            return token;
        case ';':
            token.type = TOKEN_SEMICOLON;
            token.value[0] = c;
            token.value[1] = '\0';
            ctx->position++;
            ctx->column++;
            return token;
        case ',':
            token.type = TOKEN_COMMA;
            token.value[0] = c;
            token.value[1] = '\0';
            ctx->position++;
            ctx->column++;
            return token;
        case '+':
            token.type = TOKEN_PLUS;
            token.value[0] = c;
            token.value[1] = '\0';
            ctx->position++;
            ctx->column++;
            return token;
        case '%':
            token.type = TOKEN_PERCENT;
            token.value[0] = c;
            token.value[1] = '\0';
            ctx->position++;
            ctx->column++;
            return token;
        case '.':  // Use '.' instead of Unicode bullet
            token.type = TOKEN_LUM_SYMBOL;
            token.value[0] = c;
            token.value[1] = '\0';
            ctx->position++;
            ctx->column++;
            return token;
    }
    
    // Multi-character tokens
    if (c == '-' && ctx->input[ctx->position + 1] == '>') {
        token.type = TOKEN_ARROW;
        SAFE_STRCPY(token.value, "->", sizeof(token.value));
        ctx->position += 2;
        ctx->column += 2;
        return token;
    }
    
    if (c == ':' && ctx->input[ctx->position + 1] == '=') {
        token.type = TOKEN_ASSIGN;
        SAFE_STRCPY(token.value, ":=", sizeof(token.value));
        ctx->position += 2;
        ctx->column += 2;
        return token;
    }
    
    // Memory reference (#alpha, #beta, etc.)
    if (c == '#') {
        token.type = TOKEN_MEMORY_REF;
        size_t start = ctx->position;
        ctx->position++; // Skip #
        ctx->column++;
        
        while (ctx->input[ctx->position] && vorax_is_identifier_char(ctx->input[ctx->position])) {
            ctx->position++;
            ctx->column++;
        }
        
        size_t len = ctx->position - start;
        if (len < sizeof(token.value)) {
            strncpy(token.value, &ctx->input[start], len);
            token.value[len] = '\0';
        }
        return token;
    }
    
    // Numbers
    if (vorax_is_digit(c)) {
        token.type = TOKEN_NUMBER;
        size_t start = ctx->position;
        
        while (ctx->input[ctx->position] && vorax_is_digit(ctx->input[ctx->position])) {
            ctx->position++;
            ctx->column++;
        }
        
        size_t len = ctx->position - start;
        if (len < sizeof(token.value)) {
            strncpy(token.value, &ctx->input[start], len);
            token.value[len] = '\0';
        }
        return token;
    }
    
    // Identifiers and keywords
    if (vorax_is_identifier_char(c)) {
        size_t start = ctx->position;
        
        while (ctx->input[ctx->position] && vorax_is_identifier_char(ctx->input[ctx->position])) {
            ctx->position++;
            ctx->column++;
        }
        
        size_t len = ctx->position - start;
        if (len < sizeof(token.value)) {
            strncpy(token.value, &ctx->input[start], len);
            token.value[len] = '\0';
        }
        
        // Check for keywords
        if (strcmp(token.value, "zone") == 0) token.type = TOKEN_ZONE;
        else if (strcmp(token.value, "mem") == 0) token.type = TOKEN_MEM;
        else if (strcmp(token.value, "emit") == 0) token.type = TOKEN_EMIT;
        else if (strcmp(token.value, "split") == 0) token.type = TOKEN_SPLIT;
        else if (strcmp(token.value, "move") == 0) token.type = TOKEN_MOVE;
        else if (strcmp(token.value, "store") == 0) token.type = TOKEN_STORE;
        else if (strcmp(token.value, "retrieve") == 0) token.type = TOKEN_RETRIEVE;
        else if (strcmp(token.value, "cycle") == 0) token.type = TOKEN_CYCLE;
        else if (strcmp(token.value, "fuse") == 0) token.type = TOKEN_FUSE;
        else if (strcmp(token.value, "on") == 0) token.type = TOKEN_ON;
        else if (strcmp(token.value, "empty") == 0) token.type = TOKEN_EMPTY;
        else if (strcmp(token.value, "not") == 0) token.type = TOKEN_NOT;
        else if (strcmp(token.value, "if") == 0) token.type = TOKEN_IF;
        else if (strcmp(token.value, "every") == 0) token.type = TOKEN_EVERY;
        else if (strcmp(token.value, "ms") == 0) token.type = TOKEN_MS;
        else if (strcmp(token.value, "all") == 0) token.type = TOKEN_ALL;
        else token.type = TOKEN_IDENTIFIER;
        
        return token;
    }
    
    // Unknown character
    token.value[0] = c;
    token.value[1] = '\0';
    ctx->position++;
    ctx->column++;
    return token;
}

// AST utility functions
vorax_ast_node_t* vorax_ast_create_node(vorax_ast_node_type_e type, const char* data) {
    vorax_ast_node_t* node = TRACKED_MALLOC(sizeof(vorax_ast_node_t));
    if (!node) return NULL;
    
    node->type = type;
    if (data) {
        strncpy(node->data, data, sizeof(node->data) - 1);
        node->data[sizeof(node->data) - 1] = '\0';
    } else {
        node->data[0] = '\0';
    }
    
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    
    return node;
}

void vorax_ast_add_child(vorax_ast_node_t* parent, vorax_ast_node_t* child) {
    if (!parent || !child) return;
    
    if (parent->child_count >= parent->child_capacity) {
        size_t new_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        vorax_ast_node_t** new_children = TRACKED_REALLOC(parent->children, 
                                                  sizeof(vorax_ast_node_t*) * new_capacity);
        if (!new_children) return;
        
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    parent->children[parent->child_count] = child;
    parent->child_count++;
}

void vorax_ast_destroy(vorax_ast_node_t* node) {
    if (node) {
        for (size_t i = 0; i < node->child_count; i++) {
            vorax_ast_destroy(node->children[i]);
        }
        TRACKED_FREE(node->children);
        TRACKED_FREE(node);
    }
}

void vorax_ast_print(const vorax_ast_node_t* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    const char* type_names[] = {
        "ZONE_DECLARATION", "MEMORY_DECLARATION", "MEMORY_ASSIGNMENT",
        "EMIT_STATEMENT", "SPLIT_STATEMENT", "MOVE_STATEMENT",
        "STORE_STATEMENT", "RETRIEVE_STATEMENT", "CYCLE_STATEMENT",
        "FUSE_STATEMENT", "ON_STATEMENT", "EVERY_STATEMENT", "IF_STATEMENT"
    };
    
    printf("%s: %s\n", type_names[node->type], node->data);
    
    for (size_t i = 0; i < node->child_count; i++) {
        vorax_ast_print(node->children[i], indent + 1);
    }
}

// Parser implementation
vorax_ast_node_t* vorax_parse(const char* input) {
    vorax_parser_context_t ctx;
    vorax_lexer_init(&ctx, input);
    ctx.current_token = vorax_lexer_next_token(&ctx);
    
    return vorax_parse_program(&ctx);
}

vorax_ast_node_t* vorax_parse_program(vorax_parser_context_t* ctx) {
    vorax_ast_node_t* program = vorax_ast_create_node(AST_ZONE_DECLARATION, "program");
    if (!program) return NULL;
    
    while (ctx->current_token.type != TOKEN_EOF && !ctx->has_error) {
        vorax_ast_node_t* stmt = vorax_parse_statement(ctx);
        if (stmt) {
            vorax_ast_add_child(program, stmt);
        }
    }
    
    return program;
}

vorax_ast_node_t* vorax_parse_statement(vorax_parser_context_t* ctx) {
    switch (ctx->current_token.type) {
        case TOKEN_ZONE:
            return vorax_parse_zone_declaration(ctx);
        case TOKEN_MEM:
            return vorax_parse_memory_declaration(ctx);
        case TOKEN_EMIT:
            return vorax_parse_emit_statement(ctx);
        case TOKEN_SPLIT:
            return vorax_parse_split_statement(ctx);
        case TOKEN_MOVE:
            return vorax_parse_move_statement(ctx);
        default:
            ctx->current_token = vorax_lexer_next_token(ctx);
            return NULL;
    }
}

vorax_ast_node_t* vorax_parse_zone_declaration(vorax_parser_context_t* ctx) {
    vorax_ast_node_t* node = vorax_ast_create_node(AST_ZONE_DECLARATION, "");
    if (!node) return NULL;
    
    ctx->current_token = vorax_lexer_next_token(ctx); // Skip 'zone'
    
    while (ctx->current_token.type == TOKEN_IDENTIFIER) {
        vorax_ast_node_t* zone_name = vorax_ast_create_node(AST_ZONE_DECLARATION, ctx->current_token.value);
        if (zone_name) {
            vorax_ast_add_child(node, zone_name);
        }
        
        ctx->current_token = vorax_lexer_next_token(ctx);
        
        if (ctx->current_token.type == TOKEN_COMMA) {
            ctx->current_token = vorax_lexer_next_token(ctx);
        } else {
            break;
        }
    }
    
    if (ctx->current_token.type == TOKEN_SEMICOLON) {
        ctx->current_token = vorax_lexer_next_token(ctx);
    }
    
    return node;
}

vorax_ast_node_t* vorax_parse_memory_declaration(vorax_parser_context_t* ctx) {
    vorax_ast_node_t* node = vorax_ast_create_node(AST_MEMORY_DECLARATION, "");
    if (!node) return NULL;
    
    ctx->current_token = vorax_lexer_next_token(ctx); // Skip 'mem'
    
    while (ctx->current_token.type == TOKEN_IDENTIFIER) {
        vorax_ast_node_t* mem_name = vorax_ast_create_node(AST_MEMORY_DECLARATION, ctx->current_token.value);
        if (mem_name) {
            vorax_ast_add_child(node, mem_name);
        }
        
        ctx->current_token = vorax_lexer_next_token(ctx);
        
        if (ctx->current_token.type == TOKEN_COMMA) {
            ctx->current_token = vorax_lexer_next_token(ctx);
        } else {
            break;
        }
    }
    
    if (ctx->current_token.type == TOKEN_SEMICOLON) {
        ctx->current_token = vorax_lexer_next_token(ctx);
    }
    
    return node;
}

vorax_ast_node_t* vorax_parse_emit_statement(vorax_parser_context_t* ctx) {
    vorax_ast_node_t* node = vorax_ast_create_node(AST_EMIT_STATEMENT, "");
    if (!node) return NULL;
    
    ctx->current_token = vorax_lexer_next_token(ctx); // Skip 'emit'
    
    if (ctx->current_token.type == TOKEN_IDENTIFIER) {
        // SÉCURITÉ BUFFER: Vérifier taille avant strcat
        size_t data_len = strlen(node->data);
        size_t value_len = strlen(ctx->current_token.value);
        if (data_len + value_len < sizeof(node->data) - 1) {
            strncat(node->data, ctx->current_token.value, sizeof(node->data) - data_len - 1);
        }
        ctx->current_token = vorax_lexer_next_token(ctx);
        
        if (ctx->current_token.type == TOKEN_PLUS && 
            ctx->input[ctx->position] == '=') {
            ctx->position++;
            ctx->column++;
            ctx->current_token = vorax_lexer_next_token(ctx);
            
            if (ctx->current_token.type == TOKEN_NUMBER) {
                // SÉCURITÉ BUFFER: Utiliser strncat sécurisé
                size_t data_len = strlen(node->data);
                size_t remaining = sizeof(node->data) - data_len - 1;
                if (remaining > 1) {
                    strncat(node->data, " ", remaining);
                    remaining = sizeof(node->data) - strlen(node->data) - 1;
                    if (remaining > 0) {
                        strncat(node->data, ctx->current_token.value, remaining);
                    }
                }
                ctx->current_token = vorax_lexer_next_token(ctx);
            }
        }
    }
    
    if (ctx->current_token.type == TOKEN_SEMICOLON) {
        ctx->current_token = vorax_lexer_next_token(ctx);
    }
    
    return node;
}

vorax_ast_node_t* vorax_parse_split_statement(vorax_parser_context_t* ctx) {
    vorax_ast_node_t* node = vorax_ast_create_node(AST_SPLIT_STATEMENT, "");
    if (!node) return NULL;
    
    ctx->current_token = vorax_lexer_next_token(ctx); // Skip 'split'
    
    if (ctx->current_token.type == TOKEN_IDENTIFIER) {
        // SÉCURITÉ BUFFER: Vérifier avant strcat split
        size_t data_len = strlen(node->data);
        size_t value_len = strlen(ctx->current_token.value);
        if (data_len + value_len < sizeof(node->data) - 1) {
            strncat(node->data, ctx->current_token.value, sizeof(node->data) - strlen(node->data) - 1);
        }
        ctx->current_token = vorax_lexer_next_token(ctx);
        
        if (ctx->current_token.type == TOKEN_ARROW) {
            ctx->current_token = vorax_lexer_next_token(ctx);
            // Parse target zones
        }
    }
    
    if (ctx->current_token.type == TOKEN_SEMICOLON) {
        ctx->current_token = vorax_lexer_next_token(ctx);
    }
    
    return node;
}

vorax_ast_node_t* vorax_parse_move_statement(vorax_parser_context_t* ctx) {
    vorax_ast_node_t* node = vorax_ast_create_node(AST_MOVE_STATEMENT, "");
    if (!node) return NULL;
    
    ctx->current_token = vorax_lexer_next_token(ctx); // Skip 'move'
    
    if (ctx->current_token.type == TOKEN_IDENTIFIER) {
        // SÉCURITÉ BUFFER: Vérifier avant strcat move source
        size_t data_len = strlen(node->data);
        size_t value_len = strlen(ctx->current_token.value);
        if (data_len + value_len < sizeof(node->data) - 1) {
            strncat(node->data, ctx->current_token.value, sizeof(node->data) - strlen(node->data) - 1);
        }
        ctx->current_token = vorax_lexer_next_token(ctx);
        
        if (ctx->current_token.type == TOKEN_ARROW) {
            ctx->current_token = vorax_lexer_next_token(ctx);
            
            if (ctx->current_token.type == TOKEN_IDENTIFIER) {
                // SÉCURITÉ BUFFER: Vérifier avant strcat arrow + target
                size_t data_len = strlen(node->data);
                const char* arrow = " -> ";
                size_t arrow_len = strlen(arrow);
                size_t value_len = strlen(ctx->current_token.value);
                if (data_len + arrow_len + value_len < sizeof(node->data) - 1) {
                    SAFE_STRCAT(node->data, arrow, sizeof(node->data));
                    strncat(node->data, ctx->current_token.value, sizeof(node->data) - strlen(node->data) - 1);
                }
                ctx->current_token = vorax_lexer_next_token(ctx);
            }
        }
    }
    
    if (ctx->current_token.type == TOKEN_SEMICOLON) {
        ctx->current_token = vorax_lexer_next_token(ctx);
    }
    
    return node;
}

// Execution context implementation
vorax_execution_context_t* vorax_execution_context_create(void) {
    vorax_execution_context_t* ctx = TRACKED_MALLOC(sizeof(vorax_execution_context_t));
    if (!ctx) return NULL;
    
    ctx->zones = TRACKED_MALLOC(sizeof(lum_zone_t*) * 10);
    ctx->memories = TRACKED_MALLOC(sizeof(lum_memory_t*) * 10);
    
    if (!ctx->zones || !ctx->memories) {
        TRACKED_FREE(ctx->zones);
        TRACKED_FREE(ctx->memories);
        TRACKED_FREE(ctx);
        return NULL;
    }
    
    ctx->zone_count = 0;
    ctx->zone_capacity = 10;
    ctx->memory_count = 0;
    ctx->memory_capacity = 10;
    ctx->has_error = false;
    ctx->error_message[0] = '\0';
    
    return ctx;
}

void vorax_execution_context_destroy(vorax_execution_context_t* ctx) {
    if (ctx) {
        for (size_t i = 0; i < ctx->zone_count; i++) {
            lum_zone_destroy(ctx->zones[i]);
        }
        for (size_t i = 0; i < ctx->memory_count; i++) {
            lum_memory_destroy(ctx->memories[i]);
        }
        TRACKED_FREE(ctx->zones);
        TRACKED_FREE(ctx->memories);
        TRACKED_FREE(ctx);
    }
}

bool vorax_execute(vorax_execution_context_t* ctx, const vorax_ast_node_t* ast) {
    if (!ctx || !ast) return false;
    
    for (size_t i = 0; i < ast->child_count; i++) {
        if (!vorax_execute_statement(ctx, ast->children[i])) {
            return false;
        }
    }
    
    return true;
}

bool vorax_execute_statement(vorax_execution_context_t* ctx, const vorax_ast_node_t* node) {
    if (!ctx || !node) return false;
    
    switch (node->type) {
        case AST_ZONE_DECLARATION:
            for (size_t i = 0; i < node->child_count; i++) {
                vorax_context_add_zone(ctx, node->children[i]->data);
            }
            return true;
            
        case AST_MEMORY_DECLARATION:
            for (size_t i = 0; i < node->child_count; i++) {
                vorax_context_add_memory(ctx, node->children[i]->data);
            }
            return true;
            
        default:
            return true;
    }
}

lum_zone_t* vorax_context_find_zone(vorax_execution_context_t* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    
    for (size_t i = 0; i < ctx->zone_count; i++) {
        if (strcmp(ctx->zones[i]->name, name) == 0) {
            return ctx->zones[i];
        }
    }
    return NULL;
}

lum_memory_t* vorax_context_find_memory(vorax_execution_context_t* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    
    for (size_t i = 0; i < ctx->memory_count; i++) {
        if (strcmp(ctx->memories[i]->name, name) == 0) {
            return ctx->memories[i];
        }
    }
    return NULL;
}

bool vorax_context_add_zone(vorax_execution_context_t* ctx, const char* name) {
    if (!ctx || !name) return false;
    
    if (ctx->zone_count >= ctx->zone_capacity) {
        size_t new_capacity = ctx->zone_capacity * 2;
        lum_zone_t** new_zones = TRACKED_REALLOC(ctx->zones, sizeof(lum_zone_t*) * new_capacity);
        if (!new_zones) return false;
        
        ctx->zones = new_zones;
        ctx->zone_capacity = new_capacity;
    }
    
    lum_zone_t* zone = lum_zone_create(name);
    if (!zone) return false;
    
    ctx->zones[ctx->zone_count] = zone;
    ctx->zone_count++;
    
    return true;
}

bool vorax_context_add_memory(vorax_execution_context_t* ctx, const char* name) {
    if (!ctx || !name) return false;
    
    if (ctx->memory_count >= ctx->memory_capacity) {
        size_t new_capacity = ctx->memory_capacity * 2;
        lum_memory_t** new_memories = TRACKED_REALLOC(ctx->memories, sizeof(lum_memory_t*) * new_capacity);
        if (!new_memories) return false;
        
        ctx->memories = new_memories;
        ctx->memory_capacity = new_capacity;
    }
    
    lum_memory_t* memory = lum_memory_create(name);
    if (!memory) return false;
    
    ctx->memories[ctx->memory_count] = memory;
    ctx->memory_count++;
    
    return true;
}

// Utility functions
bool vorax_is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

bool vorax_is_digit(char c) {
    return isdigit(c);
}

int vorax_count_lum_symbols(const char* str) {
    if (!str) return 0;
    int count = 0;
    for (size_t i = 0; str[i]; i++) {
        if (str[i] == '.') count++;  // Use '.' instead of Unicode bullet
    }
    return count;
}