/**
 * parser_defs.h
 * 
 * Shared definitions between the parser and tokenizer
 */

#ifndef PARSER_DEFS_H
#define PARSER_DEFS_H

#include "schema_types.h"

#define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE YYSTYPE;

/* Semantic value union - must match the %union in schema_parser.y */
union YYSTYPE {
    char *string;
    int number;
    DeonticOperator deontic;
    ComplianceType compliance;
};

/* Global yylval that will be set by the tokenizer and read by the parser */
extern YYSTYPE yylval;

#endif /* PARSER_DEFS_H */
