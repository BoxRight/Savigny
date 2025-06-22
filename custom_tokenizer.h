/**
 * custom_tokenizer.h
 * 
 * Custom tokenizer that handles noise tolerance for the Kelsen schema transpiler
 */

#ifndef CUSTOM_TOKENIZER_H
#define CUSTOM_TOKENIZER_H

#include <stdio.h>
#include <stdbool.h>

/**
 * Initialize the tokenizer with an input file
 * 
 * @param input_file File to tokenize
 * @return true if initialization succeeded, false otherwise
 */
bool tokenizer_init(FILE* input_file);

/**
 * Clean up resources used by the tokenizer
 */
void tokenizer_cleanup(void);

/**
 * Get the next token from the input stream
 * 
 * This function is designed to be used as yylex() by Bison
 * 
 * @return The type of the next token
 */
int yylex(void);

/**
 * Get current line number
 * 
 * @return Current line number
 */
int tokenizer_get_line(void);

/**
 * Get current column number
 * 
 * @return Current column number
 */
int tokenizer_get_column(void);

/**
 * Get the text of the current token
 * 
 * @return Text of the current token
 */
const char* tokenizer_get_text(void);

/**
 * Check if a word is a noise word (to be ignored)
 * 
 * @param word Word to check
 * @return true if it's a noise word, false otherwise
 */
bool is_noise_word(const char* word);

#endif /* CUSTOM_TOKENIZER_H */
