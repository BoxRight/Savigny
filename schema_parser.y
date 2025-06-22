%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "schema_types.h"
#include "config_validator.h"
#include "custom_tokenizer.h"

/* Function declarations */
void yyerror(const char *s);
extern int yylex();
extern int tokenizer_get_line();
extern const char* tokenizer_get_text();

/* Global schema being built */
Schema* current_schema;
%}

%union {
    char *string;
    int number;
    int deontic;
    int compliance;
}

%token INSTITUTION
%token <number> NUMBER
%token <string> NOMBRE_INSTITUCION
%token <string> TIPO_INSTITUCION
%token <string> MULTIPLICIDAD
%token <string> DOMINIO_LEGAL
%token REGLA
%token <string> ROL
%token DEBE
%token NO_DEBE
%token PUEDE
%token TIENE_DERECHO
%token EN_CASO_QUE
%token Y
%token <string> STRING
%token ACTUA_SOBRE
%token VIOLACION
%token ENTONCES
%token HECHO
%token EVIDENCIA
%token BUSCA_ACTO
%token ESTABLEZCA
%token CUMPLIMIENTO
%token INCUMPLIMIENTO
%token ADJUDIQUE
%token LO_ESENCIAL
%token LO_SIGUIENTE

%type <deontic> deontic_op
%type <string> action
%type <string> condition
%type <string> condition_list
%type <string> scope
%type <compliance> compliance_type

%%

schema
    : institution_decl norm_list violation_list fact_list agenda_list
    {
        /* Schema parsing successful */
        printf("Successfully parsed schema\n");
    }
    ;

institution_decl
    : INSTITUTION NOMBRE_INSTITUCION TIPO_INSTITUCION MULTIPLICIDAD DOMINIO_LEGAL
    {
        /* Set institution in the schema */
        InstitutionType type = config_get_institution_type($3);
        Multiplicity mult = config_get_multiplicity($4);
        
        /* Validate tokens using config */
        if (!config_is_valid_institution($2)) {
            const char* suggestion = config_suggest_institution($2);
            if (suggestion) {
                fprintf(stderr, "Warning: Unknown institution '%s', did you mean '%s'?\n", 
                        $2, suggestion);
                free($2);
                $2 = strdup(suggestion);
            } else {
                fprintf(stderr, "Warning: Unknown institution '%s'\n", $2);
            }
        }
        
        if (!config_is_valid_type($3)) {
            fprintf(stderr, "Warning: Unknown institution type '%s'\n", $3);
        }
        
        if (!config_is_valid_domain($5)) {
            fprintf(stderr, "Warning: Unknown legal domain '%s'\n", $5);
        }
        
        /* Set institution and save context */
        set_institution(current_schema, $2, type, mult, $5);
        config_set_current_institution($2);
        
        free($2); free($3); free($4); free($5);
    }
    ;

norm_list
    : /* empty */
    | norm_list norm
    ;

norm
    : NUMBER ROL deontic_op action scope_opt
    {
        /* Validate role using config */
        if (!config_is_valid_role($2)) {
            const char* suggestion = config_suggest_role($2);
            if (suggestion) {
                fprintf(stderr, "Warning: Role '%s' may not be valid for this institution, did you mean '%s'?\n", 
                        $2, suggestion);
                free($2);
                $2 = strdup(suggestion);
            }
        }
        
        /* Create and add norm to schema */
        Norm* norm = create_norm($1, $2, $3, $4);
        add_norm_to_schema(current_schema, norm);
        
        free($2); free($4);
    }
    | NUMBER EN_CASO_QUE condition_list ROL deontic_op action scope_opt
    {
        /* Validate role */
        if (!config_is_valid_role($4)) {
            const char* suggestion = config_suggest_role($4);
            if (suggestion) {
                fprintf(stderr, "Warning: Role '%s' may not be valid for this institution, did you mean '%s'?\n", 
                        $4, suggestion);
                free($4);
                $4 = strdup(suggestion);
            }
        }
        
        /* Create norm */
        Norm* norm = create_norm($1, $4, $5, $6);
        
        /* Add condition */
        add_condition_to_norm(norm, $3);
        
        /* Add to schema */
        add_norm_to_schema(current_schema, norm);
        
        free($3); free($4); free($6);
    }
    | NUMBER EN_CASO_QUE REGLA NUMBER ROL deontic_op action scope_opt
    {
        /* Validate role */
        if (!config_is_valid_role($5)) {
            const char* suggestion = config_suggest_role($5);
            if (suggestion) {
                fprintf(stderr, "Warning: Role '%s' may not be valid for this institution, did you mean '%s'?\n", 
                        $5, suggestion);
                free($5);
                $5 = strdup(suggestion);
            }
        }
        
        /* Create norm */
        Norm* norm = create_norm($1, $5, $6, $7);
        
        /* Add reference to other norm as condition */
        char condition_text[64];
        snprintf(condition_text, sizeof(condition_text), "NORM_REFERENCE:%d", $4);
        add_condition_to_norm(norm, strdup(condition_text));
        
        /* Add to schema */
        add_norm_to_schema(current_schema, norm);
        
        free($5); free($7);
    }
    ;

deontic_op
    : DEBE { $$ = DEONTIC_OBLIGATION; }
    | NO_DEBE { $$ = DEONTIC_PROHIBITION; }
    | PUEDE { $$ = DEONTIC_PRIVILEGE; }
    | TIENE_DERECHO { $$ = DEONTIC_CLAIM_RIGHT; }
    ;

action
    : STRING { $$ = $1; }
    | STRING Y STRING
    {
        /* Create a composite action by joining both actions */
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "%s y %s", $1, $3);
        
        free($1); free($3);
        $$ = strdup(buffer);
    }
    ;

condition
    : STRING { $$ = $1; }
    | REGLA NUMBER 
    {
        /* Create a norm reference condition */
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "NORM_REFERENCE:%d", $2);
        $$ = strdup(buffer);
    }
    ;

condition_list
    : condition { $$ = $1; }
    | condition Y condition
    {
        /* Create a composite condition by joining both conditions */
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "%s y %s", $1, $3);
        
        free($1); free($3);
        $$ = strdup(buffer);
    }
    ;

scope_opt
    : /* empty */ 
    | ACTUA_SOBRE scope
    {
        /* Get the last norm added and add scope to it */
        Norm* last_norm = current_schema->norms;
        if (last_norm != NULL) {
            while (last_norm->next != NULL) {
                last_norm = last_norm->next;
            }
            add_scope_to_norm(last_norm, $2);
        }
        free($2);
    }
    ;

scope
    : STRING { $$ = $1; }
    ;

violation_list
    : /* empty */
    | violation_list violation
    ;

violation
    : VIOLACION NUMBER ENTONCES ROL deontic_op STRING
    {
        /* Validate role */
        if (!config_is_valid_role($4)) {
            const char* suggestion = config_suggest_role($4);
            if (suggestion) {
                fprintf(stderr, "Warning: Role '%s' may not be valid for this institution, did you mean '%s'?\n", 
                        $4, suggestion);
                free($4);
                $4 = strdup(suggestion);
            }
        }
        
        /* Create and add violation to schema */
        Violation* viol = create_violation($2, $4, $5, $6);
        add_violation_to_schema(current_schema, viol);
        
        free($4); free($6);
    }
    | VIOLACION NUMBER Y VIOLACION NUMBER ENTONCES ROL deontic_op STRING
    {
        /* Validate role */
        if (!config_is_valid_role($7)) {
            const char* suggestion = config_suggest_role($7);
            if (suggestion) {
                fprintf(stderr, "Warning: Role '%s' may not be valid for this institution, did you mean '%s'?\n", 
                        $7, suggestion);
                free($7);
                $7 = strdup(suggestion);
            }
        }
        
        /* Create and add compound violation to schema */
        Violation* viol = create_compound_violation($2, $5, $7, $8, $9);
        add_violation_to_schema(current_schema, viol);
        
        free($7); free($9);
    }
    ;

fact_list
    : /* empty */
    | fact_list fact
    ;

fact
    : HECHO STRING EVIDENCIA STRING
    {
        /* Create and add legal fact to schema */
        LegalFact* fact = create_legal_fact($2, $4);
        add_fact_to_schema(current_schema, fact);
        
        free($2); free($4);
    }
    | HECHO STRING STRING
    {
        /* Create and add legal fact to schema */
        LegalFact* fact = create_legal_fact($2, $3);
        add_fact_to_schema(current_schema, fact);
        
        free($2); free($3);
    }
    ;

agenda_list
    : /* empty */
    | agenda_list agenda
    ;

agenda
    : ROL BUSCA_ACTO ESTABLEZCA compliance_type NOMBRE_INSTITUCION ADJUDIQUE ROL LO_ESENCIAL
    {
        /* Validate roles */
        if (!config_is_valid_role($1)) {
            fprintf(stderr, "Warning: Role '%s' may not be valid for this institution\n", $1);
        }
        
        if (!config_is_valid_role($7)) {
            fprintf(stderr, "Warning: Role '%s' may not be valid for this institution\n", $7);
        }
        
        /* Create agenda */
        Agenda* agenda = create_agenda($1, $4, $5, $7);
        set_agenda_essential(agenda, true);
        add_agenda_to_schema(current_schema, agenda);
        
        free($1); free($5); free($7);
    }
    | ROL BUSCA_ACTO ESTABLEZCA compliance_type NOMBRE_INSTITUCION ADJUDIQUE ROL LO_SIGUIENTE STRING
    {
        /* Validate roles */
        if (!config_is_valid_role($1)) {
            fprintf(stderr, "Warning: Role '%s' may not be valid for this institution\n", $1);
        }
        
        if (!config_is_valid_role($7)) {
            fprintf(stderr, "Warning: Role '%s' may not be valid for this institution\n", $7);
        }
        
        /* Create agenda */
        Agenda* agenda = create_agenda($1, $4, $5, $7);
        set_agenda_essential(agenda, false);
        
        /* Add remedy */
        add_norm_remedy_to_agenda(agenda, $9);
        
        add_agenda_to_schema(current_schema, agenda);
        
        free($1); free($5); free($7); free($9);
    }
    ;

compliance_type
    : CUMPLIMIENTO { $$ = COMPLIANCE_FULFILLED; }
    | INCUMPLIMIENTO { $$ = COMPLIANCE_BREACHED; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error at line %d: %s\n", tokenizer_get_line(), s);
}

/* Main function to parse a schema */
Schema* parse_schema(FILE* input) {
    /* Initialize tokenizer */
    if (!tokenizer_init(input)) {
        fprintf(stderr, "Failed to initialize tokenizer\n");
        return NULL;
    }
    
    /* Initialize schema */
    current_schema = create_schema();
    if (!current_schema) {
        fprintf(stderr, "Failed to create schema\n");
        tokenizer_cleanup();
        return NULL;
    }
    
    /* Parse schema */
    int result = yyparse();
    
    /* Clean up tokenizer */
    tokenizer_cleanup();
    
    /* Handle parse result */
    if (result != 0) {
        free_schema(current_schema);
        return NULL;
    }
    
    return current_schema;
}
