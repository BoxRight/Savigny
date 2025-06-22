/**
 * config_validator.c
 * 
 * Implementation of configuration validation functions
 */

#include "config_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cJSON.h> /* Third-party JSON parsing library */

/* Configuration data */
static cJSON* config = NULL;
static const char* current_institution = NULL;

/* Helper function to calculate string similarity (Levenshtein distance) */
static int levenshtein_distance(const char* s1, const char* s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    
    /* Create a matrix for dynamic programming */
    int matrix[len1 + 1][len2 + 1];
    
    /* Initialize first row and column */
    for (int i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }
    
    for (int j = 0; j <= len2; j++) {
        matrix[0][j] = j;
    }
    
    /* Fill the matrix */
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (tolower(s1[i-1]) != tolower(s2[j-1]));
            
            /* Deletion */
            int deletion = matrix[i-1][j] + 1;
            /* Insertion */
            int insertion = matrix[i][j-1] + 1;
            /* Substitution */
            int substitution = matrix[i-1][j-1] + cost;
            
            /* Find minimum */
            matrix[i][j] = deletion;
            if (insertion < matrix[i][j]) {
                matrix[i][j] = insertion;
            }
            if (substitution < matrix[i][j]) {
                matrix[i][j] = substitution;
            }
        }
    }
    
    /* Return distance */
    return matrix[len1][len2];
}

/**
 * Initialize the configuration validator
 */
bool config_init(const char* config_file) {
    FILE* file = fopen(config_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening configuration file: %s\n", config_file);
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
        fprintf(stderr, "Error reading configuration file\n");
        free(buffer);
        return false;
    }
    
    buffer[file_size] = '\0';
    
    /* Parse JSON */
    config = cJSON_Parse(buffer);
    free(buffer);
    
    if (config == NULL) {
        fprintf(stderr, "Error parsing configuration file\n");
        return false;
    }
    
    return true;
}

/**
 * Clean up resources used by the configuration validator
 */
void config_cleanup(void) {
    if (config != NULL) {
        cJSON_Delete(config);
        config = NULL;
    }
    current_institution = NULL;
}

/**
 * Set current institution context
 */
void config_set_current_institution(const char* institution) {
    current_institution = institution;
}

/**
 * Get current institution context
 */
const char* config_get_current_institution(void) {
    return current_institution;
}

/**
 * Get the entire automated_norms configuration object
 */
cJSON* config_get_automated_norms(void) {
    if (config == NULL) {
        return NULL;
    }
    return cJSON_GetObjectItem(config, "automated_norms");
}

/**
 * Validate an institution name
 */
bool config_is_valid_institution(const char* institution) {
    if (config == NULL || institution == NULL) {
        return false;
    }
    
    cJSON* instituciones = cJSON_GetObjectItem(config, "instituciones");
    if (instituciones == NULL) {
        return false;
    }
    
    int size = cJSON_GetArraySize(instituciones);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(instituciones, i);
        if (cJSON_IsString(item) && strcasecmp(item->valuestring, institution) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * Validate an institution type
 */
bool config_is_valid_type(const char* type) {
    if (config == NULL || type == NULL) {
        return false;
    }
    
    cJSON* tipos = cJSON_GetObjectItem(config, "tipos");
    if (tipos == NULL) {
        return false;
    }
    
    int size = cJSON_GetArraySize(tipos);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(tipos, i);
        if (cJSON_IsString(item) && strcasecmp(item->valuestring, type) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * Validate a legal domain
 */
bool config_is_valid_domain(const char* domain) {
    if (config == NULL || domain == NULL) {
        return false;
    }
    
    cJSON* dominios = cJSON_GetObjectItem(config, "dominios");
    if (dominios == NULL) {
        return false;
    }
    
    int size = cJSON_GetArraySize(dominios);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(dominios, i);
        if (cJSON_IsString(item) && strcasecmp(item->valuestring, domain) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * Validate a role for the current institution
 */
bool config_is_valid_role(const char* role) {
    if (current_institution == NULL) {
        return false;
    }
    
    return config_is_valid_role_for_institution(current_institution, role);
}

/**
 * Validate a role for a specific institution
 */
bool config_is_valid_role_for_institution(const char* institution, const char* role) {
    if (config == NULL || institution == NULL || role == NULL) {
        return false;
    }
    
    cJSON* roles = cJSON_GetObjectItem(config, "roles");
    if (roles == NULL) {
        return false;
    }
    
    cJSON* inst_roles = cJSON_GetObjectItem(roles, institution);
    if (inst_roles == NULL) {
        return false;
    }
    
    int size = cJSON_GetArraySize(inst_roles);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(inst_roles, i);
        if (cJSON_IsString(item) && strcasecmp(item->valuestring, role) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * Get a deontic operator from a string representation
 */
DeonticOperator config_get_deontic_operator(const char* text) {
    if (text == NULL) {
        return DEONTIC_OBLIGATION; /* Default */
    }
    
    if (strcasecmp(text, "debe") == 0) {
        return DEONTIC_OBLIGATION;
    } else if (strcasecmp(text, "no-debe") == 0) {
        return DEONTIC_PROHIBITION;
    } else if (strcasecmp(text, "puede") == 0) {
        return DEONTIC_PRIVILEGE;
    } else if (strcasecmp(text, "tiene-derecho-a") == 0) {
        return DEONTIC_CLAIM_RIGHT;
    } else {
        return DEONTIC_OBLIGATION; /* Default */
    }
}

/**
 * Get an institution type from a string representation
 */
InstitutionType config_get_institution_type(const char* text) {
    if (text == NULL) {
        return INST_CONTRACT; /* Default */
    }
    
    if (strcasecmp(text, "contrato") == 0) {
        return INST_CONTRACT;
    } else if (strcasecmp(text, "procedimiento") == 0) {
        return INST_PROCEDURE;
    } else if (strcasecmp(text, "acto jurídico") == 0 || 
               strcasecmp(text, "acto-juridico") == 0) {
        return INST_LEGAL_ACT;
    } else if (strcasecmp(text, "hecho jurídico") == 0 ||
               strcasecmp(text, "hecho-juridico") == 0) {
        return INST_LEGAL_FACT;
    } else {
        return INST_CONTRACT; /* Default */
    }
}

/**
 * Get a multiplicity from a string representation
 */
Multiplicity config_get_multiplicity(const char* text) {
    if (text == NULL) {
        return MULT_MULTIPLE; /* Default */
    }
    
    if (strcasecmp(text, "múltiples") == 0 || 
        strcasecmp(text, "multiples") == 0 ||
        strcasecmp(text, "multiple") == 0) {
        return MULT_MULTIPLE;
    } else if (strcasecmp(text, "una") == 0 ||
               strcasecmp(text, "un") == 0 ||
               strcasecmp(text, "single") == 0) {
        return MULT_SINGLE;
    } else {
        return MULT_MULTIPLE; /* Default */
    }
}

/**
 * Get a compliance type from a string representation
 */
ComplianceType config_get_compliance_type(const char* text) {
    if (text == NULL) {
        return COMPLIANCE_FULFILLED; /* Default */
    }
    
    if (strcasecmp(text, "cumplimiento") == 0) {
        return COMPLIANCE_FULFILLED;
    } else if (strcasecmp(text, "incumplimiento") == 0) {
        return COMPLIANCE_BREACHED;
    } else {
        return COMPLIANCE_FULFILLED; /* Default */
    }
}

/**
 * Suggest a correction for a possibly misspelled institution
 */
const char* config_suggest_institution(const char* institution) {
    if (config == NULL || institution == NULL) {
        return NULL;
    }
    
    cJSON* instituciones = cJSON_GetObjectItem(config, "instituciones");
    if (instituciones == NULL) {
        return NULL;
    }
    
    int min_distance = 1000;
    const char* suggestion = NULL;
    
    int size = cJSON_GetArraySize(instituciones);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(instituciones, i);
        if (cJSON_IsString(item)) {
            int distance = levenshtein_distance(institution, item->valuestring);
            if (distance < min_distance && distance <= 3) { /* Max 3 edits */
                min_distance = distance;
                suggestion = item->valuestring;
            }
        }
    }
    
    return suggestion;
}

/**
 * Suggest a correction for a possibly misspelled role
 */
const char* config_suggest_role(const char* role) {
    if (config == NULL || current_institution == NULL || role == NULL) {
        return NULL;
    }
    
    cJSON* roles = cJSON_GetObjectItem(config, "roles");
    if (roles == NULL) {
        return NULL;
    }
    
    cJSON* inst_roles = cJSON_GetObjectItem(roles, current_institution);
    if (inst_roles == NULL) {
        return NULL;
    }
    
    int min_distance = 1000;
    const char* suggestion = NULL;
    
    int size = cJSON_GetArraySize(inst_roles);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(inst_roles, i);
        if (cJSON_IsString(item)) {
            int distance = levenshtein_distance(role, item->valuestring);
            if (distance < min_distance && distance <= 3) { /* Max 3 edits */
                min_distance = distance;
                suggestion = item->valuestring;
            }
        }
    }
    
    return suggestion;
}
