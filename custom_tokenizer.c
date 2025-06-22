/**
 * custom_tokenizer.c
 * 
 * Implementation of the custom tokenizer for the Kelsen schema transpiler
 */

#include "custom_tokenizer.h"
#include "schema_parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* Global variable for semantic values */
extern YYSTYPE yylval;

/* Tokenizer state */
static FILE* input_file = NULL;
static int current_line = 1;
static int current_column = 1;
static char* current_token_text = NULL;

/* Buffer for the current line */
#define MAX_LINE_LENGTH 4096
static char line_buffer[MAX_LINE_LENGTH];
static int line_position = 0;
static int line_length = 0;

/* Noise word list */
static const char* noise_words[] = {
    "comienza", "como", "un", "una", "que", "en", "personas", "establecen", 
    "dentro", "del", "dadas", "condiciones", "legales", "forma", "requerida", 
    "la", "norma", "si", "hay", "de", "please", "hello", "maybe", "a", "al", 
    "por", "con", "del", "esta", "incluye", "para", "su", "los", "las", 
    "es", "son", "está", "están", "ha", "han", "fue", "fueron", "será", "serán", "&","$", "el", "la", "Pero", "siguiente", "resolución", "lo", "proteger", "sus", "derechos", "e", "intereses"
};

/* Number of noise words */
static const int num_noise_words = sizeof(noise_words) / sizeof(noise_words[0]);

/**
 * Check if a character is a token separator
 */
static bool is_separator(char c) {
    return isspace(c) || c == '.' || c == ',' || c == ':' || c == ';' || c == '[' || c == ']';
}

/**
 * Check if a word is a noise word
 */
bool is_noise_word(const char* word) {
    for (int i = 0; i < num_noise_words; i++) {
        if (strcasecmp(word, noise_words[i]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Read a new line from input
 */
static bool read_line() {
    if (fgets(line_buffer, MAX_LINE_LENGTH, input_file) == NULL) {
        return false;
    }
    
    line_length = strlen(line_buffer);
    line_position = 0;
    current_line++;
    current_column = 1;
    
    return true;
}

/**
 * Skip whitespace and other separators
 */
static void skip_separators() {
    while (line_position < line_length) {
        if (is_separator(line_buffer[line_position])) {
            if (line_buffer[line_position] == '\n') {
                if (!read_line()) {
                    return;
                }
            } else {
                line_position++;
                current_column++;
            }
        } else {
            break;
        }
    }
}

/**
 * Free the current token text
 */
static void free_current_token() {
    if (current_token_text != NULL) {
        free(current_token_text);
        current_token_text = NULL;
    }
}

/**
 * Extract a word from the current position
 */
static char* extract_word() {
    int start = line_position;
    
    /* Find the end of the word */
    while (line_position < line_length && !is_separator(line_buffer[line_position])) {
        line_position++;
        current_column++;
    }
    
    /* Extract the word */
    int length = line_position - start;
    char* word = (char*)malloc(length + 1);
    if (word == NULL) {
        return NULL;
    }
    
    strncpy(word, line_buffer + start, length);
    word[length] = '\0';
    
    return word;
}

/**
 * Extract a quoted string
 */
static char* extract_quoted_string() {
    /* Skip the opening quote */
    line_position++;
    current_column++;
    
    int start = line_position;
    
    /* Find the end of the string */
    while (line_position < line_length && line_buffer[line_position] != '"') {
        line_position++;
        current_column++;
    }
    
    /* Extract the string */
    int length = line_position - start;
    char* string = (char*)malloc(length + 1);
    if (string == NULL) {
        return NULL;
    }
    
    strncpy(string, line_buffer + start, length);
    string[length] = '\0';
    
    /* Skip the closing quote */
    if (line_position < line_length && line_buffer[line_position] == '"') {
        line_position++;
        current_column++;
    }
    
    return string;
}

/**
 * Check if a word is a number
 */
static bool is_number(const char* word) {
    int len = strlen(word);
    
    /* Handle numbers like "1." */
    if (len > 0 && word[len-1] == '.') {
        len--;
    }
    
    for (int i = 0; i < len; i++) {
        if (!isdigit(word[i])) {
            return false;
        }
    }
    
    return len > 0;
}

static bool is_action_word(const char* word) {
    /* Words that commonly appear in actions */
    return strcasecmp(word, "pagar") == 0 ||
           strcasecmp(word, "entregar") == 0 ||
           strcasecmp(word, "proporcionar") == 0 ||
           strcasecmp(word, "reparar") == 0 ||
           strcasecmp(word, "compensar") == 0 ||
           strcasecmp(word, "cancelar") == 0 ||
           strcasecmp(word, "rescindir") == 0 ||
           strcasecmp(word, "exigir") == 0;
}

static bool is_amount(const char* word) {
    /* Check if it starts with a dollar sign or contains digits */
    return word[0] == '$' || strpbrk(word, "0123456789") != NULL;
}

/**
 * Convert a word to a number
 */
static int word_to_number(const char* word) {
    return atoi(word);
}

/**
 * Match a token with a keyword
 */
static bool match_keyword(const char* word, const char* keyword) {
    return strcasecmp(word, keyword) == 0;
}

/**
 * Check if a word starts with a prefix
 */
static bool starts_with(const char* word, const char* prefix) {
    return strncasecmp(word, prefix, strlen(prefix)) == 0;
}

/**
 * Initialize the tokenizer
 */
bool tokenizer_init(FILE* file) {
    if (file == NULL) {
        return false;
    }
    
    input_file = file;
    current_line = 0;
    current_column = 1;
    line_position = 0;
    line_length = 0;
    
    /* Read the first line */
    if (!read_line()) {
        return false;
    }
    
    return true;
}

/**
 * Clean up the tokenizer
 */
void tokenizer_cleanup() {
    input_file = NULL;
    free_current_token();
}

/**
 * Get the next token
 */
/**
 * Get the next token
 */
 
 /**
 * Custom token additions for norm references
 */

/* Add this to your tokenizer_init or similar initialization function */
void init_norm_reference_token() {
    /* Make sure REGLA token is defined in your token list */
    #define REGLA 264  /* Make sure this matches the value in your schema_parser.tab.h */
}

/* Add this to your yylex function, in the token recognition logic */
int recognize_norm_reference(const char* text) {
    /* Check if the token is "regla" */
    if (strcasecmp(text, "regla") == 0) {
        return REGLA;
    }
    return 0;  /* Not a norm reference */
}

int yylex() {
    free_current_token();
    
    /* Skip separators and noise words */
    bool found_token = false;
    while (!found_token) {
        skip_separators();
        
        /* Check for end of file */
        if (line_position >= line_length && feof(input_file)) {
            printf("DEBUG: Token = EOF\n");
            return 0;  /* Return 0 for EOF */
        }
        
        /* Check for quoted string */
        if (line_position < line_length && line_buffer[line_position] == '"') {
            char* quoted_string = extract_quoted_string();
            if (quoted_string != NULL) {
                current_token_text = quoted_string;
                yylval.string = strdup(quoted_string);
                printf("DEBUG: Token = STRING (quoted: %s)\n", quoted_string);
                return STRING;
            }
        }

        /* Extract a word */
        char* word = extract_word();
        if (word == NULL || strlen(word) == 0) {
            free(word);
            if (!read_line()) {
                printf("DEBUG: Token = EOF\n");
                return 0;  /* Return 0 for EOF */
            }
            continue;
        }
        
        /* Check if it's a noise word */
        if (is_noise_word(word)) {
            printf("DEBUG: Skipping noise word: %s\n", word);
            free(word);
            continue;
        }
        
        /* Found a real token */
        current_token_text = word;
        found_token = true;
    }
    
    printf("DEBUG: Processing token: %s\n", current_token_text);
    
    /* Check for institution marker */
    if (strcasecmp(current_token_text, "[Institution]") == 0) {
        printf("DEBUG: Token = INSTITUTION\n");
        return INSTITUTION;
    }
    /* Check for number */
    if (is_number(current_token_text)) {
        yylval.number = word_to_number(current_token_text);
        printf("DEBUG: Token = NUMBER (%d)\n", yylval.number);
        return NUMBER;
    }
    
	if (current_token_text != NULL && strcasecmp(current_token_text, "regla") == 0) {
		printf("DEBUG: Token = REGLA\n");
		return REGLA;  /* Make sure REGLA is defined in schema_parser.tab.h */
	}

    
/* Check for deontic operators */
    if (match_keyword(current_token_text, "debe")) {
        printf("DEBUG: Token = DEBE\n");
        return DEBE;
    } else if (match_keyword(current_token_text, "no-debe")) {
        printf("DEBUG: Token = NO_DEBE\n");
        return NO_DEBE;
    } else if (match_keyword(current_token_text, "puede")) {
        printf("DEBUG: Token = PUEDE\n");
        return PUEDE;
    } else if (match_keyword(current_token_text, "tiene-derecho-a")) {
        printf("DEBUG: Token = TIENE_DERECHO\n");
        return TIENE_DERECHO;
    }
    
    /* Check for conditional operator */
    if (match_keyword(current_token_text, "en-caso-que")) {
        return EN_CASO_QUE;
    }
    
    /* Check for conjunction */
    if (match_keyword(current_token_text, "y")) {
        return Y;
    }
    
/* Check for violation markers */
    if (match_keyword(current_token_text, "violación") || 
        starts_with(current_token_text, "violacion")) {
        printf("DEBUG: Token = VIOLACION\n");
        return VIOLACION;
    } else if (match_keyword(current_token_text, "entonces")) {
        printf("DEBUG: Token = ENTONCES\n");
        return ENTONCES;
    }
    
/* Check for fact markers */
    if (match_keyword(current_token_text, "hecho") || 
        starts_with(current_token_text, "hecho-juridico")) {
        printf("DEBUG: Token = HECHO\n");
        return HECHO;
    } else if (match_keyword(current_token_text, "evidencia")) {
        printf("DEBUG: Token = EVIDENCIA\n");
        return EVIDENCIA;
    }
    
    /* Check for agenda markers */
    if (match_keyword(current_token_text, "busca")) {
        return BUSCA_ACTO;
    } else if (match_keyword(current_token_text, "establezca")) {
        return ESTABLEZCA;
    } else if (match_keyword(current_token_text, "cumplimiento")) {
        return CUMPLIMIENTO;
    } else if (match_keyword(current_token_text, "incumplimiento")) {
        return INCUMPLIMIENTO;
    } else if (match_keyword(current_token_text, "adjudique")) {
        return ADJUDIQUE;
    } else if (starts_with(current_token_text, "lo-esencial") ||
               match_keyword(current_token_text, "esencial")) {
        return LO_ESENCIAL;
    } else if (starts_with(current_token_text, "lo-siguiente") ||
               match_keyword(current_token_text, "siguiente")) {
        return LO_SIGUIENTE;
    }
    
/* Check for scope marker */
    if (starts_with(current_token_text, "actua")) {
        printf("DEBUG: Token = ACTUA_SOBRE\n");
        return ACTUA_SOBRE;
    }
    
    /* Check for institution marker */
    if (strcasecmp(current_token_text, "[Institution]") == 0 || 
        strcasecmp(current_token_text, "Institution") == 0) {  // Accept without brackets too
        printf("DEBUG: Token = INSTITUTION\n");
        return INSTITUTION;
    }
    
/* Check for institution types */
    if (match_keyword(current_token_text, "contrato") || 
        match_keyword(current_token_text, "procedimiento") || 
        match_keyword(current_token_text, "acto-juridico") || 
        match_keyword(current_token_text, "hecho-juridico") ||
        match_keyword(current_token_text, "acto") || 
        match_keyword(current_token_text, "hecho")) {
        yylval.string = strdup(current_token_text);
        printf("DEBUG: Token = TIPO_INSTITUCION (%s)\n", current_token_text);
        return TIPO_INSTITUCION;
    }
    
    /* Check for multiplicity */
    if (match_keyword(current_token_text, "múltiples") || 
        match_keyword(current_token_text, "multiples") ||
        match_keyword(current_token_text, "multiple") ||
        match_keyword(current_token_text, "una") ||
        match_keyword(current_token_text, "single")) {
        yylval.string = strdup(current_token_text);
        printf("DEBUG: Token = MULTIPLICIDAD (%s)\n", current_token_text);
        return MULTIPLICIDAD;
    }
    
    /* Check for legal domain */
    if (starts_with(current_token_text, "derecho-")) {
        yylval.string = strdup(current_token_text);
        printf("DEBUG: Token = DOMINIO_LEGAL (%s)\n", current_token_text);
        return DOMINIO_LEGAL;
    }
    
    /* Check for institution name (starts with capital letter) */
    if (isupper(current_token_text[0])) {
        yylval.string = strdup(current_token_text);
        printf("DEBUG: Token = NOMBRE_INSTITUCION (%s)\n", current_token_text);
        return NOMBRE_INSTITUCION;
    }
    
    /* Check for role */
    if (starts_with(current_token_text, "el-") || 
        starts_with(current_token_text, "la-") ||
        match_keyword(current_token_text, "comprador") ||
        match_keyword(current_token_text, "vendedor") ||
        match_keyword(current_token_text, "arrendador") ||
        match_keyword(current_token_text, "arrendatario") ||
        match_keyword(current_token_text, "acreedor") ||
        match_keyword(current_token_text, "deudor") ||
        match_keyword(current_token_text, "juez") ||
        match_keyword(current_token_text, "quejoso") ||
        match_keyword(current_token_text, "autoridad") ||
        match_keyword(current_token_text, "trabajador") ||
        match_keyword(current_token_text, "empleador") ||
        match_keyword(current_token_text, "trabajador") ||
        match_keyword(current_token_text, "parte1") ||
        match_keyword(current_token_text, "parte2")) {
        
        yylval.string = strdup(current_token_text);
        printf("DEBUG: Token = ROL (%s)\n", current_token_text);
        return ROL;
    }
    
    /* Check for institution name (starts with capital letter) */
    if (isupper(current_token_text[0])) {
        yylval.string = strdup(current_token_text);
        return NOMBRE_INSTITUCION;
    }
    
/* Default: it's a string */
    yylval.string = strdup(current_token_text);
    printf("DEBUG: Token = STRING (%s)\n", current_token_text);
    return STRING;
}

/**
 * Get current line number
 */
int tokenizer_get_line() {
    return current_line;
}

/**
 * Get current column number
 */
int tokenizer_get_column() {
    return current_column;
}

/**
 * Get the text of the current token
 */
const char* tokenizer_get_text() {
    return current_token_text;
}
