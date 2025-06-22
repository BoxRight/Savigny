/**
 * schema_types.h
 * 
 * Data structures for the abstract syntax tree (AST) of the Kelsen schema transpiler
 */

#ifndef SCHEMA_TYPES_H
#define SCHEMA_TYPES_H

#include <stdlib.h>
#include <stdbool.h>

/**
 * Enum for deontic operators
 */
typedef enum {
    DEONTIC_OBLIGATION,    // debe
    DEONTIC_PROHIBITION,   // no-debe
    DEONTIC_PRIVILEGE,     // puede
    DEONTIC_CLAIM_RIGHT    // tiene-derecho-a
} DeonticOperator;

/**
 * Enum for institution types
 */
typedef enum {
    INST_CONTRACT,         // contrato
    INST_PROCEDURE,        // procedimiento
    INST_LEGAL_ACT,        // acto jurídico
    INST_LEGAL_FACT        // hecho jurídico
} InstitutionType;

/**
 * Enum for multiplicity
 */
typedef enum {
    MULT_MULTIPLE,         // múltiples
    MULT_SINGLE            // una
} Multiplicity;

/**
 * Enum for compliance types
 */
typedef enum {
    COMPLIANCE_FULFILLED,  // cumplimiento
    COMPLIANCE_BREACHED    // incumplimiento
} ComplianceType;

/**
 * Structure for a condition in a conditional norm
 */
typedef struct condition {
    char* description;     // Description of the condition
    struct condition* next; // Next condition for compound conditions
} Condition;

/**
 * Structure for a scope descriptor
 */
typedef struct scope {
    char* description;     // Description of what the norm acts upon
} Scope;

/**
 * Structure for a norm
 */
typedef struct norm {
    int number;                 // Norm number
    char* role;                 // Role (e.g., "comprador", "vendedor")
    DeonticOperator deontic;    // Deontic operator
    char* action;               // Action description
    Scope* scope;               // Optional scope descriptor
    Condition* condition;       // Optional condition (for conditional norms)
    struct norm* next;          // Next norm in the list
} Norm;

/**
 * Structure for a violation reference
 */
typedef struct violation_ref {
    int norm_number;            // Referenced norm number
    struct violation_ref* next; // Next violation in a compound
} ViolationRef;

/**
 * Structure for a violation
 */
typedef struct violation {
    ViolationRef* violated_norms; // Referenced norm(s)
    char* role;                   // Role subject to consequence
    DeonticOperator deontic;      // Deontic operator for consequence
    char* consequence;            // Description of consequence
    struct violation* next;       // Next violation in the list
} Violation;

/**
 * Structure for a legal fact
 */
typedef struct legal_fact {
    char* description;          // Description of the fact
    char* evidence;             // Description of the evidence
    struct legal_fact* next;    // Next fact in the list
} LegalFact;

/**
 * Structure for a norm/remedy reference in an agenda
 */
typedef struct norm_remedy {
    char* description;           // Description of norm or remedy
    struct norm_remedy* next;    // Next norm/remedy in the list
} NormRemedy;

/**
 * Structure for an agenda
 */
typedef struct agenda {
    char* requesting_role;       // Role seeking the legal act
    ComplianceType compliance;   // Compliance type sought
    char* institution;           // Referenced institution
    char* beneficiary_role;      // Role to receive adjudication
    bool is_essential;           // Whether "lo esencial" is used
    NormRemedy* norm_remedies;   // List of norm/remedies (if not essential)
    struct agenda* next;         // Next agenda in the list
} Agenda;

/**
 * Structure for a legal institution
 */
typedef struct institution {
    char* name;                 // Institution name
    InstitutionType type;       // Institution type
    Multiplicity multiplicity;  // Multiplicity
    char* legal_domain;         // Legal domain
} Institution;

/**
 * Structure for the complete schema
 */
typedef struct schema {
    Institution institution;    // Institution declaration
    Norm* norms;                // List of norms
    Violation* violations;      // List of violations
    LegalFact* facts;           // List of legal facts
    Agenda* agendas;            // List of agendas
} Schema;

/**
 * Function prototypes for schema manipulation
 */
Schema* create_schema();
void free_schema(Schema* schema);

Norm* create_norm(int number, char* role, DeonticOperator deontic, char* action);
void add_scope_to_norm(Norm* norm, char* description);
void add_condition_to_norm(Norm* norm, char* description);
void add_norm_to_schema(Schema* schema, Norm* norm);

Violation* create_violation(int norm_number, char* role, DeonticOperator deontic, char* consequence);
Violation* create_compound_violation(int norm1, int norm2, char* role, DeonticOperator deontic, char* consequence);
void add_violation_to_schema(Schema* schema, Violation* violation);

LegalFact* create_legal_fact(char* description, char* evidence);
void add_fact_to_schema(Schema* schema, LegalFact* fact);

Agenda* create_agenda(char* requesting_role, ComplianceType compliance, char* institution, char* beneficiary_role);
void set_agenda_essential(Agenda* agenda, bool essential);
void add_norm_remedy_to_agenda(Agenda* agenda, char* description);
void add_agenda_to_schema(Schema* schema, Agenda* agenda);

void set_institution(Schema* schema, char* name, InstitutionType type, Multiplicity multiplicity, char* legal_domain);

/**
 * Function prototype for Kelsen code generation
 */
static void initialize_string_names(Schema* schema);
char* generate_kelsen_code(Schema* schema);
char* generate_kelsen_code_with_context(Schema* schema);
static void generate_distinctive_string_name(char* dest, const char* action, int norm_index);
#endif /* SCHEMA_TYPES_H */
