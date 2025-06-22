/**
 * schema_types.c
 * 
 * Implementation of schema manipulation functions
 */

#include "schema_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "context_manager.h"
#include "config_validator.h"
#include <cJSON.h> 
/**
 * Helper function for safe string duplication
 */
static char* safe_strdup(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    char* result = strdup(str);
    if (result == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    
    return result;
}

/**
 * Create a new schema
 */
Schema* create_schema() {
    Schema* schema = (Schema*)malloc(sizeof(Schema));
    if (schema == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    
    /* Initialize schema fields */
    schema->institution.name = NULL;
    schema->institution.legal_domain = NULL;
    schema->norms = NULL;
    schema->violations = NULL;
    schema->facts = NULL;
    schema->agendas = NULL;
    
    return schema;
}

/**
 * Free all resources used by a schema
 */
void free_schema(Schema* schema) {
    if (schema == NULL) {
        return;
    }
    
    /* Free institution fields */
    free(schema->institution.name);
    free(schema->institution.legal_domain);
    
    /* Free norms */
    Norm* norm = schema->norms;
    while (norm != NULL) {
        Norm* next_norm = norm->next;
        
        free(norm->role);
        free(norm->action);
        
        /* Free scope if present */
        if (norm->scope != NULL) {
            free(norm->scope->description);
            free(norm->scope);
        }
        
        /* Free condition if present */
        Condition* condition = norm->condition;
        while (condition != NULL) {
            Condition* next_condition = condition->next;
            free(condition->description);
            free(condition);
            condition = next_condition;
        }
        
        free(norm);
        norm = next_norm;
    }
    
    /* Free violations */
    Violation* violation = schema->violations;
    while (violation != NULL) {
        Violation* next_violation = violation->next;
        
        /* Free violation references */
        ViolationRef* vref = violation->violated_norms;
        while (vref != NULL) {
            ViolationRef* next_vref = vref->next;
            free(vref);
            vref = next_vref;
        }
        
        free(violation->role);
        free(violation->consequence);
        free(violation);
        violation = next_violation;
    }
    
    /* Free legal facts */
    LegalFact* fact = schema->facts;
    while (fact != NULL) {
        LegalFact* next_fact = fact->next;
        free(fact->description);
        free(fact->evidence);
        free(fact);
        fact = next_fact;
    }
    
    /* Free agendas */
    Agenda* agenda = schema->agendas;
    while (agenda != NULL) {
        Agenda* next_agenda = agenda->next;
        
        free(agenda->requesting_role);
        free(agenda->institution);
        free(agenda->beneficiary_role);
        
        /* Free norm remedies */
        NormRemedy* remedy = agenda->norm_remedies;
        while (remedy != NULL) {
            NormRemedy* next_remedy = remedy->next;
            free(remedy->description);
            free(remedy);
            remedy = next_remedy;
        }
        
        free(agenda);
        agenda = next_agenda;
    }
    
    /* Free schema itself */
    free(schema);
}

/**
 * Create a new norm
 */
Norm* create_norm(int number, char* role, DeonticOperator deontic, char* action) {
    Norm* norm = (Norm*)malloc(sizeof(Norm));
    if (norm == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    
    norm->number = number;
    norm->role = safe_strdup(role);
    norm->deontic = deontic;
    norm->action = safe_strdup(action);
    norm->scope = NULL;
    norm->condition = NULL;
    norm->next = NULL;
    
    return norm;
}

/**
 * Add a scope to a norm
 */
void add_scope_to_norm(Norm* norm, char* description) {
    if (norm == NULL) {
        return;
    }
    
    /* Create scope */
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    if (scope == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }
    
    scope->description = safe_strdup(description);
    
    /* Free previous scope if any */
    if (norm->scope != NULL) {
        free(norm->scope->description);
        free(norm->scope);
    }
    
    norm->scope = scope;
}

/**
 * Add a condition to a norm
 */
void add_condition_to_norm(Norm* norm, char* description) {
    if (norm == NULL) {
        return;
    }
    
    /* Create condition */
    Condition* condition = (Condition*)malloc(sizeof(Condition));
    if (condition == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }
    
    condition->description = safe_strdup(description);
    condition->next = NULL;
    
    /* Add to condition list */
    if (norm->condition == NULL) {
        norm->condition = condition;
    } else {
        /* Find the end of the condition list */
        Condition* current = norm->condition;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = condition;
    }
}

/**
 * Add a norm to a schema
 */
void add_norm_to_schema(Schema* schema, Norm* norm) {
    if (schema == NULL || norm == NULL) {
        return;
    }
    
    /* Add to end of norm list */
    if (schema->norms == NULL) {
        schema->norms = norm;
    } else {
        Norm* current = schema->norms;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = norm;
    }
}

/**
 * Create a new violation
 */
Violation* create_violation(int norm_number, char* role, DeonticOperator deontic, char* consequence) {
    /* Create violation */
    Violation* violation = (Violation*)malloc(sizeof(Violation));
    if (violation == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    
    /* Create violation reference */
    ViolationRef* vref = (ViolationRef*)malloc(sizeof(ViolationRef));
    if (vref == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        free(violation);
        return NULL;
    }
    
    vref->norm_number = norm_number;
    vref->next = NULL;
    
    violation->violated_norms = vref;
    violation->role = safe_strdup(role);
    violation->deontic = deontic;
    violation->consequence = safe_strdup(consequence);
    violation->next = NULL;
    
    return violation;
}

/**
 * Create a compound violation
 */
Violation* create_compound_violation(int norm1, int norm2, char* role, DeonticOperator deontic, char* consequence) {
    /* Create violation with first reference */
    Violation* violation = create_violation(norm1, role, deontic, consequence);
    if (violation == NULL) {
        return NULL;
    }
    
    /* Add second reference */
    ViolationRef* vref = (ViolationRef*)malloc(sizeof(ViolationRef));
    if (vref == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        /* Free the first violation */
        free(violation->violated_norms);
        free(violation->role);
        free(violation->consequence);
        free(violation);
        return NULL;
    }
    
    vref->norm_number = norm2;
    vref->next = NULL;
    
    violation->violated_norms->next = vref;
    
    return violation;
}

/**
 * Add a violation to a schema
 */
void add_violation_to_schema(Schema* schema, Violation* violation) {
    if (schema == NULL || violation == NULL) {
        return;
    }
    
    /* Add to end of violation list */
    if (schema->violations == NULL) {
        schema->violations = violation;
    } else {
        Violation* current = schema->violations;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = violation;
    }
}

/**
 * Create a new legal fact
 */
LegalFact* create_legal_fact(char* description, char* evidence) {
    LegalFact* fact = (LegalFact*)malloc(sizeof(LegalFact));
    if (fact == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    
    fact->description = safe_strdup(description);
    fact->evidence = safe_strdup(evidence);
    fact->next = NULL;
    
    return fact;
}

/**
 * Add a legal fact to a schema
 */
void add_fact_to_schema(Schema* schema, LegalFact* fact) {
    if (schema == NULL || fact == NULL) {
        return;
    }
    
    /* Add to end of fact list */
    if (schema->facts == NULL) {
        schema->facts = fact;
    } else {
        LegalFact* current = schema->facts;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = fact;
    }
}

/**
 * Create a new agenda
 */
Agenda* create_agenda(char* requesting_role, ComplianceType compliance, char* institution, char* beneficiary_role) {
    Agenda* agenda = (Agenda*)malloc(sizeof(Agenda));
    if (agenda == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    
    agenda->requesting_role = safe_strdup(requesting_role);
    agenda->compliance = compliance;
    agenda->institution = safe_strdup(institution);
    agenda->beneficiary_role = safe_strdup(beneficiary_role);
    agenda->is_essential = false;
    agenda->norm_remedies = NULL;
    agenda->next = NULL;
    
    return agenda;
}

/**
 * Set whether an agenda uses essential norms
 */
void set_agenda_essential(Agenda* agenda, bool essential) {
    if (agenda != NULL) {
        agenda->is_essential = essential;
    }
}

/**
 * Add a norm/remedy to an agenda
 */
void add_norm_remedy_to_agenda(Agenda* agenda, char* description) {
    if (agenda == NULL) {
        return;
    }
    
    NormRemedy* remedy = (NormRemedy*)malloc(sizeof(NormRemedy));
    if (remedy == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }
    
    remedy->description = safe_strdup(description);
    remedy->next = NULL;
    
    /* Add to end of remedy list */
    if (agenda->norm_remedies == NULL) {
        agenda->norm_remedies = remedy;
    } else {
        NormRemedy* current = agenda->norm_remedies;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = remedy;
    }
}

/**
 * Add an agenda to a schema
 */
void add_agenda_to_schema(Schema* schema, Agenda* agenda) {
    if (schema == NULL || agenda == NULL) {
        return;
    }
    
    /* Add to end of agenda list */
    if (schema->agendas == NULL) {
        schema->agendas = agenda;
    } else {
        Agenda* current = schema->agendas;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = agenda;
    }
}

/**
 * Set institution details in a schema
 */
void set_institution(Schema* schema, char* name, InstitutionType type, Multiplicity multiplicity, char* legal_domain) {
    if (schema == NULL) {
        return;
    }
    
    /* Free previous values if any */
    free(schema->institution.name);
    free(schema->institution.legal_domain);
    
    /* Set new values */
    schema->institution.name = safe_strdup(name);
    schema->institution.type = type;
    schema->institution.multiplicity = multiplicity;
    schema->institution.legal_domain = safe_strdup(legal_domain);
}

/**
 * Helper function to convert deontic operator to string
 */
static const char* deontic_to_string(DeonticOperator deontic) {
    switch (deontic) {
        case DEONTIC_OBLIGATION:
            return "OB";
        case DEONTIC_PROHIBITION:
            return "PR";
        case DEONTIC_PRIVILEGE:
            return "PVG";
        case DEONTIC_CLAIM_RIGHT:
            return "CR";
        default:
            return "??";
    }
}

/**
 * Helper function to convert institution type to string
 */
static const char* institution_type_to_string(InstitutionType type) {
    switch (type) {
        case INST_CONTRACT:
            return "CONTRACT";
        case INST_PROCEDURE:
            return "PROCEDURE";
        case INST_LEGAL_ACT:
            return "LEGAL_ACT";
        case INST_LEGAL_FACT:
            return "LEGAL_FACT";
        default:
            return "UNKNOWN";
    }
}

/**
 * Helper function to sanitize a string for the Kelsen parser
 * Removes problematic characters like $ signs
 */
/* Add this function to schema_types.c */

/**
 * Helper function to sanitize a string for the Kelsen parser
 * Removes problematic characters like $ signs and other non-alphanumeric chars
 */
static char* sanitize_for_kelsen(const char* input) {
    if (input == NULL) {
        return NULL;
    }
    
    char* output = strdup(input);
    if (output == NULL) {
        return NULL;
    }
    
    int i, j;
    for (i = 0, j = 0; input[i] != '\0'; i++) {
        /* Skip problematic characters: $, ", ', special punctuation */
        if (input[i] == '$' || 
            input[i] == '"' || 
            input[i] == '\'' ||
            input[i] == ',' ||
            input[i] == ';' ||
            input[i] == '{' ||
            input[i] == '%' ||
            input[i] == '}') {
            continue;
        }
        
        /* Replace some problematic characters with spaces */
        if (input[i] == '(' || 
            input[i] == ')' ||
            input[i] == '[' ||
            input[i] == ']') {
            output[j++] = ' ';
            continue;
        }
        
        output[j++] = input[i];
    }
    output[j] = '\0';
    
    return output;
}


/* Array to store string names */
#define MAX_NORMS 100
static char norm_string_names[MAX_NORMS][128];

/* Function to generate distinctive string names */
static void generate_distinctive_string_name(char* dest, const char* action, int norm_index) {
    if (!dest || !action) return;
    
    /* Copy action text, replacing spaces/special chars with underscores */
    int i, j;
    for (i = 0, j = 0; action[i] && j < 30; i++) {
        if (isalnum((unsigned char)action[i])) {
            dest[j++] = action[i];
        } else if (isspace((unsigned char)action[i]) || !isalnum((unsigned char)action[i])) {
            /* Don't add consecutive underscores */
            if (j > 0 && dest[j-1] != '_') {
                dest[j++] = '_';
            }
        }
    }
    
    /* Ensure the name ends with the norm number for uniqueness */
    sprintf(dest + j, "_%d", norm_index);
}

/* Function to initialize all string names */
static void initialize_string_names(Schema* schema) {
    /* Clear the array */
    memset(norm_string_names, 0, sizeof(norm_string_names));
    
    /* Generate all string names */
    Norm* norm = schema->norms;
    int norm_index = 1;
    
    while (norm != NULL && norm_index <= MAX_NORMS) {
        if (norm->action) {
            generate_distinctive_string_name(norm_string_names[norm_index-1], norm->action, norm_index);
        }
        norm_index++;
        norm = norm->next;
    }
}


/**
 * Updated generate_kelsen_code function for proper Kelsen syntax
 * 
 * This function maps our schema representation to the formal Kelsen
 * programming language format according to the comprehensive construction guide.
 * 
 * Fixes include:
 * 1. Proper encoding handling for string manipulation
 * 2. Improved conditional norm mapping using AND operators
 * 3. Correct asset type assignment based on scope
 */

char* generate_kelsen_code(Schema* schema) {
    if (schema == NULL) {
        return NULL;
    }
    
    initialize_string_names(schema);
    
    /* Allocate buffer for output */
    char* buffer = (char*)malloc(20000 * sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    
    int pos = 0;
    
    /* Step 1: Generate string definitions for action strings */
    pos += sprintf(buffer + pos, "// String definitions for actions\n");
    
    /* Generate the base contract string */
    const char* inst_name = schema->institution.name;
    char inst_name_lower[128] = {0};
    if (inst_name) {
        int i = 0;
        while (inst_name[i] && i < 127) {
            inst_name_lower[i] = tolower((unsigned char)inst_name[i]);
            i++;
        }
        inst_name_lower[i] = '\0';
    }
    
    pos += sprintf(buffer + pos, "string %s = \"acuerda %s\";\n", 
                  inst_name_lower, inst_name_lower);
    
    /* Generate action strings for each norm */
	Norm* norm = schema->norms;
	int norm_index = 1;  /* Add this line to initialize the counter */

	while (norm != NULL) {
		if (norm->action) {
		    char* sanitized_action = sanitize_for_kelsen(norm->action);
		    pos += sprintf(buffer + pos, "string %s = \"%s\";\n", 
		                  norm_string_names[norm_index-1], sanitized_action);
		    free(sanitized_action);
		}
		
		norm_index++;
		norm = norm->next;
	}
    
    pos += sprintf(buffer + pos, "\n");
    
    /* Step 2: Generate subject declarations */
    pos += sprintf(buffer + pos, "// Subject declarations\n");
    
    /* Find unique roles in the schema */
    char* roles[20]; /* Array to hold up to 20 unique roles */
    int role_count = 0;
    
    /* Look for roles in norms */
    norm = schema->norms;
    while (norm != NULL && role_count < 20) {
        if (norm->role) {
            bool found = false;
            for (int i = 0; i < role_count; i++) {
                if (roles[i] && strcmp(roles[i], norm->role) == 0) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                roles[role_count] = strdup(norm->role);
                role_count++;
            }
        }
        
        norm = norm->next;
    }
    
    /* Look for roles in violations */
    Violation* viol = schema->violations;
    while (viol != NULL && role_count < 20) {
        if (viol->role) {
            bool found = false;
            for (int i = 0; i < role_count; i++) {
                if (roles[i] && strcmp(roles[i], viol->role) == 0) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                roles[role_count] = strdup(viol->role);
                role_count++;
            }
        }
        
        viol = viol->next;
    }
    
    /* Look for roles in agendas */
    Agenda* agenda = schema->agendas;
    while (agenda != NULL && role_count < 20) {
        /* Check requesting role */
        if (agenda->requesting_role) {
            bool found = false;
            for (int i = 0; i < role_count; i++) {
                if (roles[i] && strcmp(roles[i], agenda->requesting_role) == 0) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                roles[role_count] = strdup(agenda->requesting_role);
                role_count++;
            }
        }
        
        /* Check beneficiary role */
        if (agenda->beneficiary_role) {
            bool found = false;
            for (int i = 0; i < role_count; i++) {
                if (roles[i] && strcmp(roles[i], agenda->beneficiary_role) == 0) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                roles[role_count] = strdup(agenda->beneficiary_role);
                role_count++;
            }
        }
        
        agenda = agenda->next;
    }
    
    /* Generate subject declarations for each role */
    for (int i = 0; i < role_count; i++) {
        if (roles[i]) {
            /* Convert role to uppercase for subject identifier */
            char role_upper[128] = {0};
            int j = 0;
            while (roles[i][j] && j < 127) {
                role_upper[j] = toupper((unsigned char)roles[i][j]);
                j++;
            }
            role_upper[j] = '\0';
            
            pos += sprintf(buffer + pos, "subject %s = \"Placeholder %s\", \"Placeholder address\", 12345678, \"placeholder%s@example.com\";\n", 
                          role_upper, roles[i], roles[i]);
        }
    }
    
    pos += sprintf(buffer + pos, "\n");
    
    /* Step 3: Generate the base contract asset */
    pos += sprintf(buffer + pos, "// Base contract asset\n");
    
    /* Define default subject roles for the contract */
    char first_role_upper[128] = "PLACEHOLDER";
    char second_role_upper[128] = "PLACEHOLDER";
    
    if (role_count >= 2 && roles[0] && roles[1]) {
        /* Safely convert first role to uppercase */
        int j = 0;
        while (roles[0][j] && j < 127) {
            first_role_upper[j] = toupper((unsigned char)roles[0][j]);
            j++;
        }
        first_role_upper[j] = '\0';
        
        /* Safely convert second role to uppercase */
        j = 0;
        while (roles[1][j] && j < 127) {
            second_role_upper[j] = toupper((unsigned char)roles[1][j]);
            j++;
        }
        second_role_upper[j] = '\0';
    }
    
    /* Create PascalCase asset name */
    char asset_name[128] = {0};
    if (schema->institution.name) {
        strncpy(asset_name, schema->institution.name, 127);
        asset_name[127] = '\0';
    }
    
    pos += sprintf(buffer + pos, "asset %s = Service, +, %s, %s, %s;\n\n", 
                  asset_name, first_role_upper, inst_name_lower, second_role_upper);
    
    /* Step 4: Generate assets for each norm */
    pos += sprintf(buffer + pos, "// Norm assets\n");
    
    norm = schema->norms;
    norm_index = 1;

	int norm_number = 1;
	while (norm != NULL) {

		/* Create asset name based on the action */
		char asset_name[128] = {0};
		
		if (norm->action) {
		    strncpy(asset_name, norm->action, 127);
		    asset_name[127] = '\0';
		    
		    /* Cut off at first space */
		    char* space_in_asset = strchr(asset_name, ' ');
		    if (space_in_asset) *space_in_asset = '\0';
		    
		    /* Capitalize first letter for PascalCase */
		    if (asset_name[0]) {
		        asset_name[0] = toupper((unsigned char)asset_name[0]);
		    }
		    
		    /* Convert role to uppercase for subject */
		    char role_upper[128] = "PLACEHOLDER";
		    
		    if (norm->role) {
		        int j = 0;
		        while (norm->role[j] && j < 127) {
		            role_upper[j] = toupper((unsigned char)norm->role[j]);
		            j++;
		        }
		        role_upper[j] = '\0';
		    }
		    
		    /* Determine service type and operator based on scope */
		    char* type = "Service";
		    char* operator = "+";
		    bool is_property = false;
		    
		    if (norm->scope) {
		        /* If scope contains "inmueble" or similar terms, it might be Property */
		        if (strstr(norm->scope->description, "inmueble") ||
		            strstr(norm->scope->description, "propiedad")) {
		            /* For Property, the format should NOT include the operator */
		            type = "Property";
		            if (strstr(norm->scope->description, "inmueble")) {
		                type = "Property, NM";  /* Non-movable property */
		            } else {
		                type = "Property, M";   /* Movable property */
		            }
		            is_property = true;
		        } else if (strstr(norm->scope->description, "pago")) {
		            /* Special case for payment - this is a service not property */
		            type = "Service";
		        }
		        
		        /* If action is an omission, use negative operator (for Service only) */
		        if (!is_property && (strstr(norm->action, "no ") ||
		            strstr(norm->action, "abstenerse") ||
		            strstr(norm->action, "evitar"))) {
		            operator = "-";
		        }
		    }
		    
		    /* Find the string name for this action */
        	const char* string_name = norm_string_names[norm_index-1];
		    
		    /* Find the target subject (default to the other main role) */
		    char target_role_upper[128] = "PLACEHOLDER";
		    
		    if (role_count >= 2 && roles[0] && roles[1]) {
		        const char* target_role = NULL;
		        
		        if (norm->role && strcmp(norm->role, roles[0]) == 0) {
		            target_role = roles[1];
		        } else {
		            target_role = roles[0];
		        }
		        
		        if (target_role) {
		            int j = 0;
		            while (target_role[j] && j < 127) {
		                target_role_upper[j] = toupper((unsigned char)target_role[j]);
		                j++;
		            }
		            target_role_upper[j] = '\0';
		        }
		    }
		    
		    /* Generate the asset declaration with the correct format */
		    if (is_property) {
		        /* Property format doesn't include operator */
		        pos += sprintf(buffer + pos, "asset %sAsset%d = %s, %s, %s, %s;\n", 
		                      asset_name, norm_index, type, role_upper, 
		                      string_name, target_role_upper);
		    } else {
		        /* Service format includes operator */
		        pos += sprintf(buffer + pos, "asset %sAsset%d = %s, %s, %s, %s, %s;\n", 
		                      asset_name, norm_index, type, operator, role_upper, 
		                      string_name, target_role_upper);
		    }
		    
		    /* Map deontic operator */
		    char* deontic;
		    switch (norm->deontic) {
		        case DEONTIC_OBLIGATION:
		            deontic = "OB";
		            break;
		        case DEONTIC_PROHIBITION:
		            deontic = "PR";
		            break;
		        case DEONTIC_PRIVILEGE:
		            deontic = "PVG";
		            break;
		        case DEONTIC_CLAIM_RIGHT:
		            deontic = "CR";
		            break;
		        default:
		            deontic = "OB";  /* Default */
		    }
		    
		    /* For conditional norms */

			/**
			 * Add this to generate_kelsen_code() in schema_types.c
			 * 
			 * This function is responsible for generating Kelsen code from the parsed schema
			 * 
			 * The changes below focus on handling norm references in conditional norms
			 */

			/* Find this section in generate_kelsen_code() where it processes conditional norms */
			/* For conditional norms */
			if (norm->condition) {
				/* Check if this is a reference to another norm */
				if (strncmp(norm->condition->description, "NORM_REFERENCE:", 15) == 0) {
					int referenced_norm = atoi(norm->condition->description + 15);
					
					/* Find the referenced norm's asset name */
					Norm* ref_norm = schema->norms;
					char ref_asset_name[128] = {0};
					
					for (int i = 1; i < referenced_norm && ref_norm != NULL; i++) {
						ref_norm = ref_norm->next;
					}
					
					if (ref_norm != NULL && ref_norm->action) {
						strncpy(ref_asset_name, ref_norm->action, 127);
						ref_asset_name[127] = '\0';
						
						/* Cut off at first space */
						char* space_in_ref = strchr(ref_asset_name, ' ');
						if (space_in_ref) *space_in_ref = '\0';
						
						/* Capitalize first letter */
						if (ref_asset_name[0]) {
						    ref_asset_name[0] = toupper((unsigned char)ref_asset_name[0]);
						}
						
						/* Create a clause that depends on the referenced norm's asset */
						pos += sprintf(buffer + pos, "clause norm%d = { %s AND %sAsset%d, %s(%sAsset%d) };\n", 
						              norm_index, schema->institution.name, ref_asset_name, 
						              referenced_norm, deontic, asset_name, norm_index);
					} else {
						/* Fallback if referenced norm not found */
						pos += sprintf(buffer + pos, "clause norm%d = { %s, %s(%sAsset%d) };\n", 
						              norm_index, schema->institution.name, deontic, 
						              asset_name, norm_index);
					}
				} else {
					/* Regular text condition - existing code */
					pos += sprintf(buffer + pos, "// Conditional norm\n");
					
					/* Create a string definition for the condition */
					pos += sprintf(buffer + pos, "string condition%d = \"%s\";\n", 
						          norm_index, norm->condition->description);
					
					/* Create a condition asset using the defined string and appropriate subjects */
					pos += sprintf(buffer + pos, "asset Condition%d = Service, +, %s, condition%d, %s;\n", 
						          norm_index, role_upper, norm_index, target_role_upper);
					
					/* Create the clause using AND to combine the base condition with the conditional asset */
					pos += sprintf(buffer + pos, "clause norm%d = { %s AND Condition%d, %s(%sAsset%d) };\n", 
						          norm_index, schema->institution.name, norm_index, deontic, 
						          asset_name, norm_index);
				}
			} else {
				/* Regular norms - existing code */
				pos += sprintf(buffer + pos, "clause norm%d = { %s, %s(%sAsset%d) };\n", 
						      norm_index, schema->institution.name, deontic, 
						      asset_name, norm_index);
			}
		}
		
		norm_index++;
		norm = norm->next;
	}
    
    pos += sprintf(buffer + pos, "\n");
    
    /* Step 6: Generate violation clauses */

	if (schema->violations != NULL) {
		pos += sprintf(buffer + pos, "// Violation clauses\n");
		
		viol = schema->violations;
		int viol_count = 1;
		
		while (viol != NULL) {
		    /* Get the violated norm asset(s) */
		    ViolationRef* vref = viol->violated_norms;
		    
		    /* Single violation */
		    if (vref->next == NULL) {
		        /* Find the corresponding norm to get its asset */
		        norm = schema->norms;
		        for (int i = 1; i < vref->norm_number && norm != NULL; i++) {
		            norm = norm->next;
		        }
		        
		        if (norm != NULL && norm->action) {
		            /* Create asset name for the violation consequence */
		            char cons_asset_name[128] = {0};
		            
		            if (viol->consequence) {
		                strncpy(cons_asset_name, viol->consequence, 127);
		                cons_asset_name[127] = '\0';
		                
		                /* Cut off at first space */
		                char* space_in_cons = strchr(cons_asset_name, ' ');
		                if (space_in_cons) *space_in_cons = '\0';
		                
		                /* Capitalize first letter */
		                if (cons_asset_name[0]) {
		                    cons_asset_name[0] = toupper((unsigned char)cons_asset_name[0]);
		                }
		            }
		            
		            /* Create the violated norm's asset name */
		            char norm_asset_name[128] = {0};
		            strncpy(norm_asset_name, norm->action, 127);
		            norm_asset_name[127] = '\0';
		            
		            /* Cut off at first space */
		            char* space_in_norm = strchr(norm_asset_name, ' ');
		            if (space_in_norm) *space_in_norm = '\0';
		            
		            /* Capitalize first letter */
		            if (norm_asset_name[0]) {
		                norm_asset_name[0] = toupper((unsigned char)norm_asset_name[0]);
		            }
		            
		            /* Create a unique string name for the violation */
		            char viol_string_name[128] = {0};
		            sprintf(viol_string_name, "violation_string_%d", viol_count);
		            
		            /* Create string for the consequence - use a unique name */
		            pos += sprintf(buffer + pos, "string %s = \"%s\";\n", 
		                          viol_string_name, viol->consequence);
		            
		            /* Convert violation role to uppercase */
		            char role_upper[128] = "PLACEHOLDER";
		            
		            if (viol->role) {
		                int j = 0;
		                while (viol->role[j] && j < 127) {
		                    role_upper[j] = toupper((unsigned char)viol->role[j]);
		                    j++;
		                }
		                role_upper[j] = '\0';
		            }
		            
		            /* Find the target subject */
		            char target_role_upper[128] = "PLACEHOLDER";
		            
		            if (role_count >= 2 && roles[0] && roles[1]) {
		                const char* target_role = NULL;
		                
		                if (viol->role && strcmp(viol->role, roles[0]) == 0) {
		                    target_role = roles[1];
		                } else {
		                    target_role = roles[0];
		                }
		                
		                if (target_role) {
		                    int j = 0;
		                    while (target_role[j] && j < 127) {
		                        target_role_upper[j] = toupper((unsigned char)target_role[j]);
		                        j++;
		                    }
		                    target_role_upper[j] = '\0';
		                }
		            }
		            
		            /* Create the consequence asset - use the string variable */
		            pos += sprintf(buffer + pos, "asset %sConsequence%d = Service, +, %s, %s, %s;\n", 
		                          cons_asset_name, viol_count, role_upper, viol_string_name, target_role_upper);
		            
		            /* Map deontic operator */
		            char* deontic;
		            switch (viol->deontic) {
		                case DEONTIC_OBLIGATION:
		                    deontic = "OB";
		                    break;
		                case DEONTIC_PROHIBITION:
		                    deontic = "PR";
		                    break;
		                case DEONTIC_PRIVILEGE:
		                    deontic = "PVG";
		                    break;
		                case DEONTIC_CLAIM_RIGHT:
		                    deontic = "CR";
		                    break;
		                default:
		                    deontic = "CR";  /* Default for consequences */
		            }
		            
		            /* Create the violation clause with a unique name */
		            char violation_clause_name[128] = {0};
		            sprintf(violation_clause_name, "viol_clause_%d", viol_count);
		            
		            pos += sprintf(buffer + pos, "clause %s = { not(%sAsset%d), %s(%sConsequence%d) };\n", 
		                          violation_clause_name, norm_asset_name, vref->norm_number, deontic, 
		                          cons_asset_name, viol_count);
		        }
		    } else {
		        /* Compound violation with multiple norm references */
		        pos += sprintf(buffer + pos, "// Compound violation for norms %d and %d\n", 
		                      vref->norm_number, vref->next->norm_number);
		        
		        /* Create asset for the consequence */
		        char cons_asset_name[128] = {0};
		        
		        if (viol->consequence) {
		            strncpy(cons_asset_name, viol->consequence, 127);
		            cons_asset_name[127] = '\0';
		            
		            /* Cut off at first space */
		            char* space_in_comp = strchr(cons_asset_name, ' ');
		            if (space_in_comp) *space_in_comp = '\0';
		            
		            /* Capitalize first letter */
		            if (cons_asset_name[0]) {
		                cons_asset_name[0] = toupper((unsigned char)cons_asset_name[0]);
		            }
		        }
		        
		        /* Create a unique string name for the compound violation */
		        char compound_string_name[128] = {0};
		        sprintf(compound_string_name, "compound_violation_string_%d", viol_count);
		        
		        /* Create string for the consequence with unique name */
		        pos += sprintf(buffer + pos, "string %s = \"%s\";\n", 
		                      compound_string_name, viol->consequence);
		        
		        /* Convert violation role to uppercase */
		        char role_upper[128] = "PLACEHOLDER";
		        
		        if (viol->role) {
		            int j = 0;
		            while (viol->role[j] && j < 127) {
		                role_upper[j] = toupper((unsigned char)viol->role[j]);
		                j++;
		            }
		            role_upper[j] = '\0';
		        }
		        
		        /* Find the target subject */
		        char target_role_upper[128] = "PLACEHOLDER";
		        
		        if (role_count >= 2 && roles[0] && roles[1]) {
		            const char* target_role = NULL;
		            
		            if (viol->role && strcmp(viol->role, roles[0]) == 0) {
		                target_role = roles[1];
		            } else {
		                target_role = roles[0];
		            }
		            
		            if (target_role) {
		                int j = 0;
		                while (target_role[j] && j < 127) {
		                    target_role_upper[j] = toupper((unsigned char)target_role[j]);
		                    j++;
		                }
		                target_role_upper[j] = '\0';
		            }
		        }
		        
		        /* Create the consequence asset with unique string reference */
		        pos += sprintf(buffer + pos, "asset %sCompoundConsequence%d = Service, +, %s, %s, %s;\n", 
		                      cons_asset_name, viol_count, role_upper, compound_string_name, target_role_upper);
		        
		        /* Map deontic operator */
		        char* deontic;
		        switch (viol->deontic) {
		            case DEONTIC_OBLIGATION:
		                deontic = "OB";
		                break;
		            case DEONTIC_PROHIBITION:
		                deontic = "PR";
		                break;
		            case DEONTIC_PRIVILEGE:
		                deontic = "PVG";
		                break;
		            case DEONTIC_CLAIM_RIGHT:
		                deontic = "CR";
		                break;
		            default:
		                deontic = "CR";  /* Default for consequences */
		        }
		        
		        /* Find the first violated norm */
		        norm = schema->norms;
		        for (int i = 1; i < vref->norm_number && norm != NULL; i++) {
		            norm = norm->next;
		        }
		        
		        char norm1_asset_name[128] = "Unknown";
		        if (norm != NULL && norm->action) {
		            strncpy(norm1_asset_name, norm->action, 127);
		            norm1_asset_name[127] = '\0';
		            
		            /* Cut off at first space */
		            char* space_in_norm1 = strchr(norm1_asset_name, ' ');
		            if (space_in_norm1) *space_in_norm1 = '\0';
		            
		            /* Capitalize first letter */
		            if (norm1_asset_name[0]) {
		                norm1_asset_name[0] = toupper((unsigned char)norm1_asset_name[0]);
		            }
		        }
		        
		        /* Find the second violated norm */
		        norm = schema->norms;
		        for (int i = 1; i < vref->next->norm_number && norm != NULL; i++) {
		            norm = norm->next;
		        }
		        
		        char norm2_asset_name[128] = "Unknown";
		        if (norm != NULL && norm->action) {
		            strncpy(norm2_asset_name, norm->action, 127);
		            norm2_asset_name[127] = '\0';
		            
		            /* Cut off at first space */
		            char* space_in_norm2 = strchr(norm2_asset_name, ' ');
		            if (space_in_norm2) *space_in_norm2 = '\0';
		            
		            /* Capitalize first letter */
		            if (norm2_asset_name[0]) {
		                norm2_asset_name[0] = toupper((unsigned char)norm2_asset_name[0]);
		            }
		        }
		        
		        /* Create the compound violation clause with a unique name */
		        char compound_clause_name[128] = {0};
		        sprintf(compound_clause_name, "compound_viol_clause_%d", viol_count);
		        
		        /* Create the compound violation clause */
		        pos += sprintf(buffer + pos, "clause %s = { not(%sAsset%d) AND not(%sAsset%d), %s(%sCompoundConsequence%d) };\n", 
		                      compound_clause_name, norm1_asset_name, vref->norm_number, 
		                      norm2_asset_name, vref->next->norm_number, 
		                      deontic, cons_asset_name, viol_count);
		    }
		    
		    viol_count++;
		    viol = viol->next;
		}
		
		pos += sprintf(buffer + pos, "\n");
	}
    
    /* Step 7: Generate facts */
    if (schema->facts != NULL) {
        pos += sprintf(buffer + pos, "// Facts\n");
        
        LegalFact* fact = schema->facts;
        int fact_count = 1;
        
        while (fact != NULL) {
            /* Create fact identifier in UPPER_SNAKE_CASE */
            char fact_id[128] = {0};
            
            if (fact->description) {
    			
    			char* sanitized_desc = sanitize_for_kelsen(fact->description);
                
                strncpy(fact_id, sanitized_desc, 127);
                fact_id[127] = '\0';
                
                free(sanitized_desc);
                
                /* Convert spaces to underscores and uppercase */
                for (int i = 0; fact_id[i]; i++) {
                    if (isspace((unsigned char)fact_id[i])) {
                        fact_id[i] = '_';
                    } else {
                        fact_id[i] = toupper((unsigned char)fact_id[i]);
                    }
                }
                
                /* Truncate to a reasonable length */
                if (strlen(fact_id) > 30) {
                    fact_id[30] = '\0';
                }
                
                /* Add index to ensure uniqueness */
                char suffix[16];
                sprintf(suffix, "_%d", fact_count);
                
                /* Make sure we don't overflow */
                if (strlen(fact_id) + strlen(suffix) < 128) {
                    strcat(fact_id, suffix);
                }
                
                /* Try to find a related asset based on description */
                char related_asset[128] = {0};
                strcpy(related_asset, schema->institution.name);  /* Default to base contract */
                
                for (int i = 0; i < norm_number - 1; i++) {
                    Norm* current_norm = schema->norms;
                    for (int j = 0; j < i && current_norm; j++) {
                        current_norm = current_norm->next;
                    }
                    
                    if (current_norm && current_norm->action && 
                        fact->description && strstr(fact->description, current_norm->action)) {
                        /* Found a related norm, use its asset */
                        strncpy(related_asset, current_norm->action, 127);
                        related_asset[127] = '\0';
                        
                        /* Cut off at first space */
                        char* space_related_asset = strchr(related_asset, ' ');
                        if (space_related_asset) *space_related_asset = '\0';
                        
                        /* Capitalize first letter */
                        if (related_asset[0]) {
                            related_asset[0] = toupper((unsigned char)related_asset[0]);
                        }
                        
                        sprintf(related_asset + strlen(related_asset), "Asset%d", i + 1);
                        break;
                    }
                }
                
                /* Generate the fact */
                pos += sprintf(buffer + pos, "fact %s = %s, \"%s\", \"%s\";\n", 
                              fact_id, related_asset, fact->description, fact->evidence);
            }
            
            fact_count++;
            fact = fact->next;
        }
        
        pos += sprintf(buffer + pos, "\n");
    }
    
    /* Step 8: Generate agendas */
    if (schema->agendas != NULL) {
        pos += sprintf(buffer + pos, "// Agendas\n");
        
        agenda = schema->agendas;
        int agenda_count = 1;
        
        while (agenda != NULL) {
            /* Create agenda identifier in PascalCase */
            char agenda_id[128] = {0};
            
            if (agenda->requesting_role) {
                /* Base on the requesting role and compliance type */
                strncpy(agenda_id, agenda->requesting_role, 127);
                agenda_id[127] = '\0';
                
                /* Capitalize first letter */
                if (agenda_id[0]) {
                    agenda_id[0] = toupper((unsigned char)agenda_id[0]);
                }
                
                /* Add compliance type */
                strcat(agenda_id, agenda->compliance == COMPLIANCE_FULFILLED ? "Fulfillment" : "Breach");
                
                /* Add index for uniqueness */
                char suffix[16];
                sprintf(suffix, "%d", agenda_count);
                
                /* Make sure we don't overflow */
                if (strlen(agenda_id) + strlen(suffix) < 128) {
                    strcat(agenda_id, suffix);
                }
                
                /* Determine agenda type */
                char* agenda_type = agenda->compliance == COMPLIANCE_FULFILLED ? "FULFILL" : "BREACH";
                
                /* Start agenda */
                pos += sprintf(buffer + pos, "agenda %s = %s {", agenda_id, agenda_type);
                
                /* Add base contract asset */
                pos += sprintf(buffer + pos, "%s", schema->institution.name);
                
                /* If essential, add all norm assets */
                if (agenda->is_essential) {
                    /* Add all norm assets as references */
                    norm = schema->norms;
                    int i = 1;
                    
                    while (norm != NULL) {
                        if (norm->action) {
                            /* Create asset name based on the action */
                            char asset_name[128] = {0};
                            strncpy(asset_name, norm->action, 127);
                            asset_name[127] = '\0';
                            
                            /* Cut off at first space */
                            char* space_asset_name = strchr(asset_name, ' ');
                            if (space_asset_name) *space_asset_name = '\0';
                            
                            /* Capitalize first letter */
                            if (asset_name[0]) {
                                asset_name[0] = toupper((unsigned char)asset_name[0]);
                            }
                            
                            pos += sprintf(buffer + pos, ", %sAsset%d", asset_name, i);
                        }
                        
                        i++;
                        norm = norm->next;
                    }
                } else if (agenda->norm_remedies) {
                    /* Add specific remedies */
                    NormRemedy* remedy = agenda->norm_remedies;
                    
                    while (remedy != NULL) {
                        if (remedy->description) {
                            pos += sprintf(buffer + pos, ",\n    // %s", remedy->description);
                        }
                        remedy = remedy->next;
                    }
                }
                
                /* End agenda */
                pos += sprintf(buffer + pos, "};\n");
            }
            
            agenda_count++;
            agenda = agenda->next;
        }
    }
    
    /* Free the role array */
    for (int i = 0; i < role_count; i++) {
        if (roles[i]) {
            free(roles[i]);
        }
    }
    
    return buffer;
}

/**
 * legal_context.c
 * 
 * Implementation of legal context enhancement functions
 */


 /* Third-party JSON parsing library */

/* Legal context data */
static cJSON* legal_context = NULL;





/**
 * Convert a string to a variable name
 */
static char* string_to_var_name(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    char* var_name = (char*)malloc(strlen(str) + 1);
    if (var_name == NULL) {
        return NULL;
    }
    
    /* Get first word only */
    int i;
    for (i = 0; str[i] != '\0' && str[i] != ' ' && i < 20; i++) {
        var_name[i] = tolower((unsigned char)str[i]);
    }
    var_name[i] = '\0';
    
    return var_name;
}

/**
 * Convert a role to uppercase for subject declaration
 */
static char* role_to_subject(const char* role) {
    if (role == NULL) {
        return NULL;
    }
    
    char* subject = (char*)malloc(strlen(role) + 1);
    if (subject == NULL) {
        return NULL;
    }
    
    int i;
    for (i = 0; role[i] != '\0'; i++) {
        subject[i] = toupper((unsigned char)role[i]);
    }
    subject[i] = '\0';
    
    return subject;
}

/**
 * Determine asset type based on object description
 */
static const char* determine_asset_type(const char* object) {
    if (object == NULL) {
        return "Service";
    }
    
    if (strstr(object, "inmueble") || 
        strstr(object, "propiedad") || 
        strstr(object, "bien")) {
        return "Property, NM";  /* Non-movable property */
    } else if (strstr(object, "documento") || 
               strstr(object, "precio") || 
               strstr(object, "pago")) {
        return "Property, M";   /* Movable property */
    }
    
    return "Service";
}

/**
 * Initialize the legal context module
 */
bool legal_context_init(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening legal context file: %s\n", filename);
        return false;
    }
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    /* Read file content */
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return false;
    }
    
    size_t read_size = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (read_size != file_size) {
        fprintf(stderr, "Error reading legal context file\n");
        free(buffer);
        return false;
    }
    
    buffer[file_size] = '\0';
    
    /* Parse JSON */
    legal_context = cJSON_Parse(buffer);
    free(buffer);
    
    if (legal_context == NULL) {
        fprintf(stderr, "Error parsing legal context file\n");
        return false;
    }
    
    return true;
}

/**
 * Clean up resources used by the legal context module
 */
void legal_context_cleanup(void) {
    if (legal_context != NULL) {
        cJSON_Delete(legal_context);
        legal_context = NULL;
    }
}

/**
 * Check if legal context is initialized
 */
bool context_is_initialized(void) {
    return legal_context != NULL;
}

/**
 * Check if an institution is in the conditions list
 */
static bool is_institution_in_conditions(cJSON* conditions, const char* institution) {
    if (conditions == NULL || !cJSON_IsArray(conditions) || institution == NULL) {
        return false;
    }
    
    int size = cJSON_GetArraySize(conditions);
    for (int i = 0; i < size; i++) {
        cJSON* condition = cJSON_GetArrayItem(conditions, i);
        if (cJSON_IsString(condition) && 
            strcasecmp(condition->valuestring, institution) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * Get roles for an institution
 */
/**
 * Get roles for an institution
 */
static void get_institution_roles(const char* institution, char** role1, char** role2) {
    if (institution == NULL || role1 == NULL || role2 == NULL) {
        return;
    }
    
    /* Default roles */
    *role1 = "parte1";
    *role2 = "parte2";
    
    /* Get roles from config */
    cJSON* roles = cJSON_GetObjectItem(legal_context, "roles");
    if (roles == NULL) {
        return;
    }
    
    cJSON* inst_roles = cJSON_GetObjectItem(roles, institution);
    if (inst_roles == NULL || !cJSON_IsArray(inst_roles)) {
        return;
    }
    
    int size = cJSON_GetArraySize(inst_roles);
    if (size >= 1) {
        cJSON* role = cJSON_GetArrayItem(inst_roles, 0);
        if (cJSON_IsString(role)) {
            *role1 = role->valuestring;
        }
    }
    
    if (size >= 2) {
        cJSON* role = cJSON_GetArrayItem(inst_roles, 1);
        if (cJSON_IsString(role)) {
            *role2 = role->valuestring;
        }
    }
}


/**
 * Map JSON role to institution role
 */
static const char* map_role(const char* json_role, const char* institution, 
                         const char* inst_role1, const char* inst_role2) {
    if (json_role == NULL) {
        return inst_role1;  /* Default to first role */
    }
    
    if (strcasecmp(json_role, inst_role1) == 0) {
        return inst_role1;
    } else if (strcasecmp(json_role, inst_role2) == 0) {
        return inst_role2;
    }
    
    /* If no match, use given role */
    return json_role;
}
char* generate_kelsen_code_with_context(Schema* schema) {
    if (schema == NULL) {
        return NULL;
    }
    
    /* Call the original code generation function */
    char* base_code = generate_kelsen_code(schema);
    if (base_code == NULL) {
        return NULL;
    }
    
    /* Check if context is available */
    if (!context_is_initialized()) {
        return base_code;
    }
    
    /* Get institution name */
    const char* institution = schema->institution.name;
    if (institution == NULL) {
        return base_code;
    }
    
    /* Create a buffer for enhanced code - make it larger to account for annotations */
    char* enhanced_code = (char*)malloc(strlen(base_code) + 20000);
    if (enhanced_code == NULL) {
        free(base_code);
        return NULL;
    }
    
    /* Copy the base code */
    strcpy(enhanced_code, base_code);
    int pos = strlen(base_code);
    
    /* Begin legal context section */
    pos += sprintf(enhanced_code + pos, "\n// =========================================================================\n");
    pos += sprintf(enhanced_code + pos, "// LEGAL CONTEXT EXTENSIONS\n");
    pos += sprintf(enhanced_code + pos, "// =========================================================================\n\n");
    
    /* Get institution roles */
    char *inst_role1, *inst_role2;
    get_institution_roles(institution, &inst_role1, &inst_role2);
    
    /* Track string definitions to avoid duplicates */
    char string_definitions[50][128];
    int string_count = 0;
    
    /* Buffer for string definitions */
    char string_section[5000] = "// String definitions for legal context actions\n";
    int string_pos = strlen(string_section);
    
    /* Buffer for assets */
    char asset_section[10000] = "";
    int asset_pos = 0;
    
    /* Buffer for clauses */
    char clause_section[10000] = "";
    int clause_pos = 0;
    
    /* Process sources */
    cJSON* sources = cJSON_GetObjectItem(legal_context, "sources");
    if (sources != NULL) {
        /* Iterate through source_names */
        cJSON* source_entry;
        cJSON_ArrayForEach(source_entry, sources) {
            /* Get source */
            cJSON* source = cJSON_GetObjectItem(sources, source_entry->string);
            if (source == NULL) continue;
            
            /* Source header */
            asset_pos += sprintf(asset_section + asset_pos, 
                               "// -------------------------------------------------------------------------\n");
            asset_pos += sprintf(asset_section + asset_pos, 
                               "// Assets from %s\n", source_entry->string);
            asset_pos += sprintf(asset_section + asset_pos, 
                               "// -------------------------------------------------------------------------\n\n");
            
            /* Get source name and type */
            cJSON* source_name = cJSON_GetObjectItem(source, "nombre");
            cJSON* source_type = cJSON_GetObjectItem(source, "tipo");
            
            /* Process norms */
            cJSON* normas = cJSON_GetObjectItem(source, "normas");
            if (normas == NULL) continue;
            
            /* Iterate through norm_names */
            cJSON* norm_entry;
            cJSON_ArrayForEach(norm_entry, normas) {
                /* Get norm */
                cJSON* norm = cJSON_GetObjectItem(normas, norm_entry->string);
                if (norm == NULL) continue;
                
                /* Get norm id */
                cJSON* norm_id = cJSON_GetObjectItem(norm, "id");
                if (norm_id == NULL || !cJSON_IsString(norm_id)) continue;
                
                /* Get norm structure */
                cJSON* estructura = cJSON_GetObjectItem(norm, "estructura");
                if (estructura == NULL) continue;
                
                /* Get conditions */
                cJSON* conditions = cJSON_GetObjectItem(estructura, "condiciones");
                
                /* Skip if not applicable to this institution */
                if (conditions == NULL || 
                    !is_institution_in_conditions(conditions, institution)) {
                    continue;
                }
                
                /* Get action */
                cJSON* action = cJSON_GetObjectItem(estructura, "accion");
                if (action == NULL || !cJSON_IsString(action)) continue;
                
                /* Add string definition if not already defined */
				/* Add string definition if not already defined */
				bool string_exists = false;
				char* var_name = string_to_var_name(action->valuestring);

				/* First check if this string exists in the base code */
				char *str_search = base_code;
				char search_pattern[256];
				sprintf(search_pattern, "string %s =", var_name);
				if (strstr(str_search, search_pattern) != NULL) {
					string_exists = true;
				}

				/* Then check our local tracking */
				if (!string_exists) {
					for (int i = 0; i < string_count; i++) {
						if (strcmp(string_definitions[i], var_name) == 0) {
							string_exists = true;
							break;
						}
					}
				}

				/* If string exists, use legal_ prefix */
				if (string_exists) {
					char *legal_var_name = malloc(strlen(var_name) + 7);
					sprintf(legal_var_name, "legal_%s", var_name);
					free(var_name);
					var_name = legal_var_name;
					string_exists = false; /* Reset for tracking */
				}

				if (!string_exists && string_count < 50) {
					/* Store string name */
					strncpy(string_definitions[string_count], var_name, 127);
					string_count++;
					
					/* Add string definition */
					char* sanitized_action = sanitize_for_kelsen(action->valuestring);
					string_pos += sprintf(string_section + string_pos, 
								        "string %s = \"%s\";\n", 
								        var_name, sanitized_action);
					free(sanitized_action);
				}
                
                /* Get roles */
                cJSON* activo = cJSON_GetObjectItem(estructura, "activo");
                cJSON* pasivo = cJSON_GetObjectItem(estructura, "pasivo");
                
                /* Get deontic operator */
                cJSON* deontico = cJSON_GetObjectItem(estructura, "deontico");
                const char* deontic_op = "OB";  /* Default to obligation */
                
                if (deontico != NULL && cJSON_IsString(deontico)) {
                    if (strcmp(deontico->valuestring, "prohibicion") == 0) {
                        deontic_op = "PR";
                    } else if (strcmp(deontico->valuestring, "privilegio") == 0) {
                        deontic_op = "PVG";
                    } else if (strcmp(deontico->valuestring, "derecho") == 0) {
                        deontic_op = "CR";
                    }
                }
                
                /* Get object */
                cJSON* objeto = cJSON_GetObjectItem(estructura, "objeto");
                const char* object_type = determine_asset_type(
                    objeto != NULL && cJSON_IsString(objeto) ? objeto->valuestring : NULL);
                
                /* Get derivation */
                cJSON* derivada = cJSON_GetObjectItem(norm, "derivadaDe");
                
                /* Get context */
                cJSON* contexto = cJSON_GetObjectItem(norm, "contexto");
                
                /* Add source comment */
                asset_pos += sprintf(asset_section + asset_pos, "// Source: %s - %s\n", 
                                   source_name != NULL && cJSON_IsString(source_name) ? 
                                   source_name->valuestring : source_entry->string,
                                   norm_id->valuestring);
                
                /* Add derivation if present */
                if (derivada != NULL && cJSON_IsString(derivada)) {
                    asset_pos += sprintf(asset_section + asset_pos, "// Derived from: %s\n", 
                                       derivada->valuestring);
                }
                
                /* Add context if present */
                if (contexto != NULL && cJSON_IsArray(contexto)) {
                    asset_pos += sprintf(asset_section + asset_pos, "// Context: ");
                    
                    int ctx_size = cJSON_GetArraySize(contexto);
                    for (int i = 0; i < ctx_size; i++) {
                        cJSON* ctx = cJSON_GetArrayItem(contexto, i);
                        if (cJSON_IsString(ctx)) {
                            asset_pos += sprintf(asset_section + asset_pos, "%s%s", 
                                               i > 0 ? ", " : "", ctx->valuestring);
                        }
                    }
                    asset_pos += sprintf(asset_section + asset_pos, "\n");
                }
                
                /* Add note about applicability */
                asset_pos += sprintf(asset_section + asset_pos, "// Note: Applies to %s (in conditions list)\n", 
                                   institution);
                
                /* Process the roles based on our convention */
                if (activo != NULL && cJSON_IsString(activo) && 
                    pasivo != NULL && cJSON_IsString(pasivo)) {
                    
                    /* Check if roles match institution roles */
                    const char* role1 = map_role(activo->valuestring, institution, inst_role1, inst_role2);
                    const char* role2 = map_role(pasivo->valuestring, institution, inst_role1, inst_role2);
                    
                    /* Create asset name */
                    char asset_name[128];
                    char *norm_name = norm_entry->string;
                    
                    /* Add asset for primary direction */
                    char* role1_subject = role_to_subject(role1);
                    char* role2_subject = role_to_subject(role2);
                    
                    /* Create asset - format depends on type */
                    if (strncmp(object_type, "Property", 8) == 0) {
                        /* Property format doesn't include operator */
                        asset_pos += sprintf(asset_section + asset_pos, 
                                           "asset %sAsset = %s, %s, %s, %s;\n", 
                                           norm_name, object_type, role1_subject, var_name, role2_subject);
                    } else {
                        /* Service format includes operator */
                        asset_pos += sprintf(asset_section + asset_pos, 
                                           "asset %sAsset = %s, +, %s, %s, %s;\n", 
                                           norm_name, object_type, role1_subject, var_name, role2_subject);
                    }
                    
                    /* Add clause */
                    clause_pos += sprintf(clause_section + clause_pos, 
                                       "clause %s_obligation = { %s, %s(%sAsset) };\n", 
                                       norm_name, institution, deontic_op, norm_name);
                    
                    /* If roles don't match expected institution roles, create reciprocal obligation */
                    bool roles_match = 
                        (strcasecmp(role1, inst_role1) == 0 && strcasecmp(role2, inst_role2) == 0) ||
                        (strcasecmp(role1, inst_role2) == 0 && strcasecmp(role2, inst_role1) == 0);
                    
                    if (!roles_match) {
                        /* Add reciprocal asset */
                        if (strncmp(object_type, "Property", 8) == 0) {
                            /* Property format doesn't include operator */
                            asset_pos += sprintf(asset_section + asset_pos, 
                                               "asset %sAsset_Reciprocal = %s, %s, %s, %s;\n", 
                                               norm_name, object_type, role2_subject, var_name, role1_subject);
                        } else {
                            /* Service format includes operator */
                            asset_pos += sprintf(asset_section + asset_pos, 
                                               "asset %sAsset_Reciprocal = %s, +, %s, %s, %s;\n", 
                                               norm_name, object_type, role2_subject, var_name, role1_subject);
                        }
                        
                        /* Add reciprocal clause */
                        clause_pos += sprintf(clause_section + clause_pos, 
                                           "clause %s_obligation_reciprocal = { %s, %s(%sAsset_Reciprocal) };\n", 
                                           norm_name, institution, deontic_op, norm_name);
                    }
                    
                    /* Add newline */
                    asset_pos += sprintf(asset_section + asset_pos, "\n");
                    
                    free(role1_subject);
                    free(role2_subject);
                }
                
                free(var_name);
            }
        }
    }
    
    /* Combine sections */
    if (string_count > 0) {
        pos += sprintf(enhanced_code + pos, "%s\n", string_section);
    }
    
    if (asset_pos > 0) {
        pos += sprintf(enhanced_code + pos, "%s", asset_section);
    }
    
    if (clause_pos > 0) {
        pos += sprintf(enhanced_code + pos, 
                      "// -------------------------------------------------------------------------\n");
        pos += sprintf(enhanced_code + pos, "// Obligation clauses from legal sources\n");
        pos += sprintf(enhanced_code + pos, 
                      "// -------------------------------------------------------------------------\n\n");
        pos += sprintf(enhanced_code + pos, "%s", clause_section);
    }
    
    /* Clean up */
    free(base_code);
    
    return enhanced_code;
}
