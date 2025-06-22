/**
 * context_manager.c
 * 
 * Implementation of the legal context manager and automated norm generation.
 */

#include "context_manager.h"
#include "config_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

/* The loaded context database */
static cJSON* legal_context = NULL;

/**
 * Initialize the legal context module
 */
bool legal_context_init(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening context file: %s\n", filename);
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return false;
    }
    
    fread(buffer, 1, file_size, file);
    fclose(file);
    buffer[file_size] = '\0';
    
    legal_context = cJSON_Parse(buffer);
    free(buffer);
    
    if (legal_context == NULL) {
        fprintf(stderr, "Error parsing context file: %s\n", cJSON_GetErrorPtr());
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
 * Get a legal norm definition from the context
 */
cJSON* context_get_norm(const char* source, const char* norm_id) {
    if (legal_context == NULL || source == NULL || norm_id == NULL) {
        return NULL;
    }
    
    /* Navigate to the source */
    cJSON* sources = cJSON_GetObjectItemCaseSensitive(legal_context, "sources");
    if (!cJSON_IsObject(sources)) {
        return NULL;
    }
    
    cJSON* source_obj = cJSON_GetObjectItemCaseSensitive(sources, source);
    if (!cJSON_IsObject(source_obj)) {
        return NULL;
    }
    
    /* Navigate to norms and find the specified norm */
    cJSON* norms = cJSON_GetObjectItemCaseSensitive(source_obj, "normas");
    if (!cJSON_IsObject(norms)) {
        return NULL;
    }
    
    return cJSON_GetObjectItemCaseSensitive(norms, norm_id);
}

/**
 * Check if a norm has a direct relationship with another norm
 */
bool context_has_relationship(const char* source1, const char* norm_id1, 
                              const char* source2, const char* norm_id2) {
    cJSON* norm1 = context_get_norm(source1, norm_id1);
    if (norm1 == NULL) {
        return false;
    }
    
    /* Check if norm1 is derived from norm2 */
    cJSON* derived_from = cJSON_GetObjectItemCaseSensitive(norm1, "derivadaDe");
    if (cJSON_IsString(derived_from)) {
        char full_id[256];
        snprintf(full_id, sizeof(full_id), "%s.%s", source2, norm_id2);
        
        if (strcmp(derived_from->valuestring, full_id) == 0) {
            return true;
        }
    }
    
    /* TODO: Also check for other types of relationships if needed */
    
    return false;
}


/**
 * Updated function to map roles using either explicit mappings or inference
 */
const char* context_map_role(const char* contract_type, const char* generic_role) {
    if (legal_context == NULL || contract_type == NULL || generic_role == NULL) {
        return NULL;
    }
    
    /* First try using explicit mappings if available */
    cJSON* role_mappings = cJSON_GetObjectItemCaseSensitive(legal_context, "roleMappings");
    if (cJSON_IsObject(role_mappings)) {
        cJSON* contract_mappings = cJSON_GetObjectItemCaseSensitive(role_mappings, contract_type);
        if (cJSON_IsObject(contract_mappings)) {
            /* Check for direct mapping */
            cJSON* specific_role = cJSON_GetObjectItemCaseSensitive(contract_mappings, generic_role);
            if (cJSON_IsString(specific_role)) {
                return specific_role->valuestring;
            } 
            else if (cJSON_IsArray(specific_role)) {
                /* Return the first role in the array */
                cJSON* first_role = cJSON_GetArrayItem(specific_role, 0);
                if (cJSON_IsString(first_role)) {
                    return first_role->valuestring;
                }
            }
        }
    }
    
    /* If explicit mapping not found, try inferring the mapping */
    cJSON* inferred_mappings = context_infer_role_mappings(contract_type);
    if (inferred_mappings != NULL) {
        cJSON* specific_role = cJSON_GetObjectItemCaseSensitive(inferred_mappings, generic_role);
        if (cJSON_IsString(specific_role)) {
            /* Save the result for future reference */
            const char* result = strdup(specific_role->valuestring);
            cJSON_Delete(inferred_mappings);
            return result;
        } 
        else if (cJSON_IsArray(specific_role)) {
            /* Return the first role in the array */
            cJSON* first_role = cJSON_GetArrayItem(specific_role, 0);
            if (cJSON_IsString(first_role)) {
                /* Save the result for future reference */
                const char* result = strdup(first_role->valuestring);
                cJSON_Delete(inferred_mappings);
                return result;
            }
        }
        
        cJSON_Delete(inferred_mappings);
    }
    
    /* If all else fails, return the generic role as-is */
    return generic_role;
}

/**
 * Validate that a norm is legally consistent with the context
 */
bool context_validate_norm(Norm* norm) {
    if (norm == NULL || legal_context == NULL) {
        return false;
    }
    
    /* This is a simplistic validation example.
       In a real implementation, you would check if the norm's structure
       is consistent with the legal framework. */
    
    /* Validate against role mappings */
    if (norm->role != NULL) {
        const char* institution = "Arrendamiento"; /* Example - you would get this from the schema */
        
        /* Check if the role is valid for this institution */
        cJSON* role_mappings = cJSON_GetObjectItemCaseSensitive(legal_context, "roleMappings");
        if (cJSON_IsObject(role_mappings)) {
            cJSON* inst_mappings = cJSON_GetObjectItemCaseSensitive(role_mappings, institution);
            
            if (cJSON_IsObject(inst_mappings)) {
                /* Check each mapping */
                cJSON* mapping;
                cJSON_ArrayForEach(mapping, inst_mappings) {
                    if (cJSON_IsString(mapping) && strcmp(mapping->valuestring, norm->role) == 0) {
                        /* Role is valid */
                        return true;
                    }
                    else if (cJSON_IsArray(mapping)) {
                        /* Check array of roles */
                        cJSON* role_item;
                        cJSON_ArrayForEach(role_item, mapping) {
                            if (cJSON_IsString(role_item) && strcmp(role_item->valuestring, norm->role) == 0) {
                                /* Role is valid */
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    /* If we reach here, the validation failed */
    return false;
}

/**
 * Get context information for a specific legal domain
 */
cJSON* context_get_domain_info(const char* domain) {
    if (legal_context == NULL || domain == NULL) {
        return NULL;
    }
    
    /* This function would extract domain-specific information
       from the context database */
    
    /* Check each source for relevant domain information */
    cJSON* sources = cJSON_GetObjectItemCaseSensitive(legal_context, "sources");
    if (!cJSON_IsObject(sources)) {
        return NULL;
    }
    
    /* Create a new object to hold domain-specific information */
    cJSON* domain_info = cJSON_CreateObject();
    if (domain_info == NULL) {
        return NULL;
    }
    
    /* For each source, check if it's relevant to the domain */
    cJSON* source;
    cJSON_ArrayForEach(source, sources) {
        if (!cJSON_IsObject(source)) continue;
        
        cJSON* norms = cJSON_GetObjectItemCaseSensitive(source, "normas");
        if (!cJSON_IsObject(norms)) continue;
        
        cJSON* norm_obj;
        cJSON_ArrayForEach(norm_obj, norms) {
            if (!cJSON_IsObject(norm_obj)) continue;
            
            /* Check if this norm is relevant to the domain */
            cJSON* contexto = cJSON_GetObjectItemCaseSensitive(norm_obj, "contexto");
            if (!cJSON_IsArray(contexto)) continue;
            
            cJSON* ctx_item;
            cJSON_ArrayForEach(ctx_item, contexto) {
                if (!cJSON_IsString(ctx_item)) continue;
                
                /* If domain matches a context item, add this norm to domain_info */
                if (strcmp(ctx_item->valuestring, domain) == 0 || 
                    strstr(domain, ctx_item->valuestring) != NULL) {
                    
                    /* Add this norm to our domain info */
                    cJSON* norm_id = cJSON_GetObjectItemCaseSensitive(norm_obj, "id");
                    if (cJSON_IsString(norm_id)) {
                        /* Add a reference to this norm */
                        cJSON_AddItemReferenceToObject(domain_info, norm_id->valuestring, norm_obj);
                    }
                    
                    break; /* Move to next norm */
                }
            }
        }
    }
    
    /* Return the accumulated domain info */
    return domain_info;
}

/**
 * Improved implementation for context_get_kelsen_annotations
 * 
 * This version uses more sophisticated matching to find relevant legal articles
 */
char* context_get_kelsen_annotations(Norm* norm) {
    if (norm == NULL || legal_context == NULL) {
        return NULL;
    }
    
    /* Buffer for the annotations */
    char* annotations = (char*)malloc(1024);
    if (annotations == NULL) {
        return NULL;
    }
    
    annotations[0] = '\0'; /* Initialize as empty string */
    
    /* Get the sources from the context */
    cJSON* sources = cJSON_GetObjectItemCaseSensitive(legal_context, "sources");
    if (!cJSON_IsObject(sources)) {
        free(annotations);
        return NULL;
    }
    
    /* Look for matches to our norm */
    cJSON* source;
    cJSON_ArrayForEach(source, sources) {
        if (!cJSON_IsObject(source)) continue;
        
        /* Get source name and type */
        cJSON* source_name = cJSON_GetObjectItemCaseSensitive(source, "nombre");
        cJSON* source_type = cJSON_GetObjectItemCaseSensitive(source, "tipo");
        
        if (!cJSON_IsString(source_name) || !cJSON_IsString(source_type)) continue;
        
        /* Process norms in this source */
        cJSON* norms = cJSON_GetObjectItemCaseSensitive(source, "normas");
        if (!cJSON_IsObject(norms)) continue;
        
        cJSON* norm_obj;
        cJSON_ArrayForEach(norm_obj, norms) {
            if (!cJSON_IsObject(norm_obj)) continue;
            
            /* Get the norm ID */
            cJSON* norm_id = cJSON_GetObjectItemCaseSensitive(norm_obj, "id");
            if (!cJSON_IsString(norm_id)) continue;
            
            /* Get the structure */
            cJSON* estructura = cJSON_GetObjectItemCaseSensitive(norm_obj, "estructura");
            if (!cJSON_IsObject(estructura)) continue;
            
            /* Get action and passive */
            cJSON* accion = cJSON_GetObjectItemCaseSensitive(estructura, "accion");
            cJSON* pasivo = cJSON_GetObjectItemCaseSensitive(estructura, "pasivo");
            cJSON* objeto = cJSON_GetObjectItemCaseSensitive(estructura, "objeto");
            
            if (!cJSON_IsString(accion)) continue;
            
            /* Check if this norm is related to our norm */
            bool matched = false;
            
            /* Check action similarity - look for key words */
            if (norm->action) {
                /* Look for key verbs that might indicate similarity */
                const char* action_verbs[] = {"entregar", "pagar", "reparar", "garantizar", "transferir"};
                for (int i = 0; i < 5; i++) {
                    if (strstr(norm->action, action_verbs[i]) && strstr(accion->valuestring, action_verbs[i])) {
                        matched = true;
                        break;
                    }
                }
                
                /* Also match based on objects if available */
                if (cJSON_IsString(objeto) && norm->scope) {
                    const char* objects[] = {"bien", "producto", "precio", "pago", "servicio", "inmueble"};
                    for (int i = 0; i < 6; i++) {
                        if (strstr(norm->scope->description, objects[i]) && strstr(objeto->valuestring, objects[i])) {
                            matched = true;
                            break;
                        }
                    }
                }
                
                /* Look for role matching if we have passive */
                if (cJSON_IsString(pasivo) && norm->role) {
                    /* Direct role match */
                    if (strcasecmp(pasivo->valuestring, norm->role) == 0) {
                        matched = true;
                    }
                    
                    /* Or match via role mapping */
                    const char* mapped_role = context_map_role("compraventa", pasivo->valuestring);
                    if (mapped_role && strcasecmp(mapped_role, norm->role) == 0) {
                        matched = true;
                    }
                }
            }
            
            /* If we found a match, add an annotation */
            if (matched) {
                char annotation[512];
                sprintf(annotation, "// Related to %s: %s - ", 
                        source_name->valuestring, 
                        norm_id->valuestring);
                
                /* Add a description of the legal principle */
                strcat(annotation, accion->valuestring);
                strcat(annotation, "\n");
                
                /* Add to our annotations */
                strcat(annotations, annotation);
            }
        }
    }
    
    /* Check if we found any annotations */
    if (strlen(annotations) == 0) {
        free(annotations);
        return NULL;
    }
    
    return annotations;
}

/**
 * Function to dynamically infer role mappings from legal norms
 * 
 * This eliminates the need for explicit roleMappings in the JSON database
 */
cJSON* context_infer_role_mappings(const char* contract_type) {
    if (legal_context == NULL || contract_type == NULL) {
        return NULL;
    }
    
    /* Create a new object to hold inferred role mappings */
    cJSON* inferred_mappings = cJSON_CreateObject();
    if (inferred_mappings == NULL) {
        return NULL;
    }
    
    /* Navigate to sources */
    cJSON* sources = cJSON_GetObjectItemCaseSensitive(legal_context, "sources");
    if (!cJSON_IsObject(sources)) {
        cJSON_Delete(inferred_mappings);
        return NULL;
    }
    
    /* For each source, look at its norms */
    cJSON* source;
    cJSON_ArrayForEach(source, sources) {
        if (!cJSON_IsObject(source)) continue;
        
        cJSON* norms = cJSON_GetObjectItemCaseSensitive(source, "normas");
        if (!cJSON_IsObject(norms)) continue;
        
        /* Examine each norm */
        cJSON* norm_obj;
        cJSON_ArrayForEach(norm_obj, norms) {
            if (!cJSON_IsObject(norm_obj)) continue;
            
            /* Check if this norm relates to our contract type */
            cJSON* contexto = cJSON_GetObjectItemCaseSensitive(norm_obj, "contexto");
            if (!cJSON_IsArray(contexto)) continue;
            
            bool relevant_context = false;
            cJSON* context_item;
            cJSON_ArrayForEach(context_item, contexto) {
                if (!cJSON_IsString(context_item)) continue;
                
                if (strcasecmp(context_item->valuestring, contract_type) == 0) {
                    relevant_context = true;
                    break;
                }
            }
            
            if (!relevant_context) continue;
            
            /* Get the estructura of the norm */
            cJSON* estructura = cJSON_GetObjectItemCaseSensitive(norm_obj, "estructura");
            if (!cJSON_IsObject(estructura)) continue;
            
            /* Extract passive subject */
            cJSON* pasivo = cJSON_GetObjectItemCaseSensitive(estructura, "pasivo");
            if (!cJSON_IsString(pasivo)) continue;
            
            /* Extract action text to find additional roles */
            cJSON* accion = cJSON_GetObjectItemCaseSensitive(estructura, "accion");
            if (!cJSON_IsString(accion)) continue;
            
            /* Infer generic role type based on context and action */
            const char* pasivo_role = pasivo->valuestring;
            const char* generic_role = NULL;
            
            /* Simple heuristics to map passive subject to generic role */
            /* For example, if action contains "pagar" and passive is "comprador" */
            if (strstr(accion->valuestring, "pagar") != NULL) {
                generic_role = "deudor";
            } 
            else if (strstr(accion->valuestring, "entregar") != NULL) {
                generic_role = "obligado_entrega";
            }
            else if (strstr(accion->valuestring, "mantener") != NULL || 
                     strstr(accion->valuestring, "reparar") != NULL) {
                generic_role = "obligado_mantenimiento";
            }
            /* Default case */
            else {
                generic_role = "contratante";
            }
            
            /* Add or update role in inferred mappings */
            if (generic_role != NULL) {
                cJSON* existing_role = cJSON_GetObjectItemCaseSensitive(inferred_mappings, generic_role);
                
                if (existing_role == NULL) {
                    /* Add new role mapping */
                    cJSON_AddStringToObject(inferred_mappings, generic_role, pasivo_role);
                } 
                else if (cJSON_IsString(existing_role)) {
                    /* Convert to array if different from existing */
                    if (strcmp(existing_role->valuestring, pasivo_role) != 0) {
                        /* Replace with array containing both values */
                        cJSON* array = cJSON_CreateArray();
                        if (array != NULL) {
                            cJSON_AddItemToArray(array, cJSON_CreateString(existing_role->valuestring));
                            cJSON_AddItemToArray(array, cJSON_CreateString(pasivo_role));
                            
                            /* Replace existing item with array */
                            cJSON_ReplaceItemInObject(inferred_mappings, generic_role, array);
                        }
                    }
                }
                else if (cJSON_IsArray(existing_role)) {
                    /* Check if value already in array */
                    bool found = false;
                    cJSON* item;
                    cJSON_ArrayForEach(item, existing_role) {
                        if (cJSON_IsString(item) && strcmp(item->valuestring, pasivo_role) == 0) {
                            found = true;
                            break;
                        }
                    }
                    
                    /* Add to array if not found */
                    if (!found) {
                        cJSON_AddItemToArray(existing_role, cJSON_CreateString(pasivo_role));
                    }
                }
            }
            
            /* Extract other roles from action text */
            /* This is a simple approach - in reality you would use NLP techniques */
            if (strstr(accion->valuestring, "comprador") != NULL && 
                strcmp(pasivo_role, "comprador") != 0) {
                cJSON_AddStringToObject(inferred_mappings, "receptor", "comprador");
            }
            
            if (strstr(accion->valuestring, "vendedor") != NULL && 
                strcmp(pasivo_role, "vendedor") != 0) {
                cJSON_AddStringToObject(inferred_mappings, "proveedor", "vendedor");
            }
            
            if (strstr(accion->valuestring, "arrendador") != NULL && 
                strcmp(pasivo_role, "arrendador") != 0) {
                cJSON_AddStringToObject(inferred_mappings, "propietario", "arrendador");
            }
            
            if (strstr(accion->valuestring, "arrendatario") != NULL && 
                strcmp(pasivo_role, "arrendatario") != 0) {
                cJSON_AddStringToObject(inferred_mappings, "usuario", "arrendatario");
            }
        }
    }
    
    /* Check if we found any mappings */
    if (cJSON_GetArraySize(inferred_mappings) == 0) {
        cJSON_Delete(inferred_mappings);
        return NULL;
    }
    
    /* Add inverse mappings for complete bidirectional mapping */
    /* For example, if we have deudor -> comprador, add comprador -> deudor */
    cJSON* copy = cJSON_Duplicate(inferred_mappings, true);
    if (copy != NULL) {
        cJSON* item;
        cJSON_ArrayForEach(item, copy) {
            const char* key = item->string;
            
            if (cJSON_IsString(item)) {
                const char* value = item->valuestring;
                
                /* Add inverse mapping if not already present */
                if (cJSON_GetObjectItemCaseSensitive(inferred_mappings, value) == NULL) {
                    cJSON_AddStringToObject(inferred_mappings, value, key);
                }
            }
            else if (cJSON_IsArray(item)) {
                /* For arrays, add each value->key mapping */
                cJSON* array_item;
                cJSON_ArrayForEach(array_item, item) {
                    if (cJSON_IsString(array_item)) {
                        const char* value = array_item->valuestring;
                        
                        /* Add inverse mapping if not already present */
                        if (cJSON_GetObjectItemCaseSensitive(inferred_mappings, value) == NULL) {
                            cJSON_AddStringToObject(inferred_mappings, value, key);
                        }
                    }
                }
            }
        }
        
        cJSON_Delete(copy);
    }
    
    return inferred_mappings;
}

char* generate_kelsen_code_with_context(const Schema* schema) {
    // ... implementation ...
    return buffer;
}

static void apply_domain_defaults(Schema* schema, cJSON* domain_defaults, int* next_norm_id) {
    const char* domain = schema->institution->domain;
    cJSON* defaults = cJSON_GetObjectItem(domain_defaults, domain);
    if (!cJSON_IsArray(defaults)) {
        return;
    }

    // Find the highest existing norm ID to avoid conflicts.
    int max_id = 0;
    for (Norm* n = schema->norms; n != NULL; n = n->next) {
        if (n->number > max_id) {
            max_id = n->number;
        }
    }
    // Start automated norm IDs from a higher number.
    int next_norm_id = (max_id / 100 + 1) * 100;

    cJSON* default_norm_json;
    cJSON_ArrayForEach(default_norm_json, defaults) {
        char* role = cJSON_GetObjectItem(default_norm_json, "role")->valuestring;
        char* deontic_str = cJSON_GetObjectItem(default_norm_json, "deontic")->valuestring;
        char* action = cJSON_GetObjectItem(default_norm_json, "action")->valuestring;
        
        if (role && deontic_str && action) {
            char final_action[1024];
            strncpy(final_action, action, sizeof(final_action) - 1);

            cJSON* reference_json = cJSON_GetObjectItem(default_norm_json, "reference");
            if (reference_json && cJSON_IsString(reference_json)) {
                snprintf(final_action + strlen(final_action), sizeof(final_action) - strlen(final_action),
                         " [Ref: %s]", reference_json->valuestring);
            }
            
            DeonticOperator deontic = config_get_deontic_operator(deontic_str);
            Norm* new_norm = create_norm((*next_norm_id)++, strdup(role), deontic, strdup(final_action));
            
            cJSON* scope_json = cJSON_GetObjectItem(default_norm_json, "scope");
            if (scope_json && cJSON_IsString(scope_json)) {
                add_scope_to_norm(new_norm, strdup(scope_json->valuestring));
            }
            
            add_norm_to_schema(schema, new_norm);
            printf("Added default norm for domain %s: %s\n", domain, final_action);
        }
    }
}

static void apply_universal_templates(Schema* schema, cJSON* universal_templates, int* next_norm_id) {
    const char* domain = schema->institution->domain;
    cJSON* templates = cJSON_GetObjectItem(universal_templates, domain);
    if (!cJSON_IsArray(templates)) {
        return;
    }

    // Create a snapshot of original norms to iterate over
    Norm* original_norms[100]; // Assume max 100 original norms
    int count = 0;
    for (Norm* n = schema->norms; n != NULL && count < 100; n = n->next) {
        if (n->number < 1000) { // Simple check for original vs generated
             original_norms[count++] = n;
        }
    }

    for (int i = 0; i < count; i++) {
        Norm* user_norm = original_norms[i];
        cJSON* template_json;
        cJSON_ArrayForEach(template_json, templates) {
            char* role = cJSON_GetObjectItem(template_json, "role")->valuestring;
            char* deontic_str = cJSON_GetObjectItem(template_json, "deontic")->valuestring;
            char* action_template = cJSON_GetObjectItem(template_json, "action")->valuestring;

            if (role && deontic_str && action_template) {
                char final_action[1024];
                // Replace placeholders
                char temp_action[1024];
                strncpy(temp_action, action_template, sizeof(temp_action));
                // A basic replace for %{rule_id} and %{rule_action}
                char* placeholder_id = strstr(temp_action, "%{rule_id}");
                if (placeholder_id) {
                    sprintf(final_action, "%.*s%d%s", (int)(placeholder_id - temp_action), temp_action, user_norm->number, placeholder_id + strlen("%{rule_id}"));
                    strncpy(temp_action, final_action, sizeof(temp_action));
                }
                char* placeholder_action = strstr(temp_action, "%{rule_action}");
                 if (placeholder_action) {
                    sprintf(final_action, "%.*s%s%s", (int)(placeholder_action - temp_action), temp_action, user_norm->action, placeholder_action + strlen("%{rule_action}"));
                } else {
                    strncpy(final_action, temp_action, sizeof(final_action));
                }

                DeonticOperator deontic = config_get_deontic_operator(deontic_str);
                Norm* new_norm = create_norm((*next_norm_id)++, strdup(role), deontic, strdup(final_action));
                add_norm_to_schema(schema, new_norm);
                printf("Added template-based norm for rule %d: %s\n", user_norm->number, final_action);
            }
        }
    }
}

static void apply_conditional_on_id(Schema* schema, cJSON* conditional_on_id, int* next_norm_id) {
    Norm* user_norm = schema->norms;
    while(user_norm != NULL) {
        if (user_norm->number >= 1000) { // Skip generated norms
            user_norm = user_norm->next;
            continue;
        }

        char rule_id_str[10];
        snprintf(rule_id_str, sizeof(rule_id_str), "%d", user_norm->number);
        
        cJSON* conditional_norms = cJSON_GetObjectItem(conditional_on_id, rule_id_str);
        if (cJSON_IsArray(conditional_norms)) {
            cJSON* norm_json;
            cJSON_ArrayForEach(norm_json, conditional_norms) {
                // ... (logic to create and add norm, similar to domain defaults)
                char* role = cJSON_GetObjectItem(norm_json, "role")->valuestring;
                char* deontic_str = cJSON_GetObjectItem(norm_json, "deontic")->valuestring;
                char* action = cJSON_GetObjectItem(norm_json, "action")->valuestring;

                if (role && deontic_str && action) {
                    char final_action[1024];
                    strncpy(final_action, action, sizeof(final_action) - 1);

                    cJSON* reference_json = cJSON_GetObjectItem(norm_json, "reference");
                    if (reference_json && cJSON_IsString(reference_json)) {
                         snprintf(final_action + strlen(final_action), sizeof(final_action) - strlen(final_action),
                                 " [Ref: %s]", reference_json->valuestring);
                    }
                    
                    DeonticOperator deontic = config_get_deontic_operator(deontic_str);
                    Norm* new_norm = create_norm((*next_norm_id)++, strdup(role), deontic, strdup(final_action));
                    add_norm_to_schema(schema, new_norm);
                    printf("Added conditional norm triggered by rule %d: %s\n", user_norm->number, final_action);
                }
            }
        }
        user_norm = user_norm->next;
    }
}

/**
 * @brief Applies automated norms to a schema based on its domain and rules.
 *
 * This function orchestrates the three-tiered enrichment process:
 * 1. Applies domain-specific default norms.
 * 2. Applies universal template norms to all user-defined rules.
 * 3. Applies conditional norms based on specific user-defined rule IDs.
 *
 * @param schema The schema to enrich with automated norms.
 */
void schema_apply_automated_norms(Schema* schema) {
    if (schema == NULL || schema->institution == NULL || schema->institution->domain == NULL) {
        return;
    }

    cJSON* automated_norms_config = config_get_automated_norms();
    if (automated_norms_config == NULL) {
        return; // No automated norms configured
    }

    int last_user_norm_id = 0;
    for (Norm* n = schema->norms; n != NULL; n = n->next) {
        if (n->number > last_user_norm_id) {
            last_user_norm_id = n->number;
        }
    }
    int next_automated_norm_id = (last_user_norm_id / 100 + 1) * 100;

    // Tier 1: Apply Domain-Specific Default Norms
    cJSON* domain_defaults = cJSON_GetObjectItem(automated_norms_config, "domain_defaults");
    if (domain_defaults) {
        apply_domain_defaults(schema, domain_defaults, &next_automated_norm_id);
    }

    // Tier 2: Apply Universal Template Norms
    cJSON* universal_templates = cJSON_GetObjectItem(automated_norms_config, "universal_templates");
    if (universal_templates) {
        apply_universal_templates(schema, universal_templates, &next_automated_norm_id);
    }

    // Tier 3: Apply Rule-ID-Specific Conditional Norms
    cJSON* conditional_on_id = cJSON_GetObjectItem(automated_norms_config, "conditional_on_id");
    if (conditional_on_id) {
        apply_conditional_on_id(schema, conditional_on_id, &next_automated_norm_id);
    }
}

