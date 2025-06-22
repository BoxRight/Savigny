/**
 * context_manager.c
 * 
 * Implementation of the legal context manager for the Kelsen schema transpiler
 */

#include "context_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>

/* The loaded context database */
static cJSON* legal_context = NULL;

/**
 * Initialize the context manager with a JSON file
 */
bool context_init(const char* context_file) {
    FILE* file = fopen(context_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening context file: %s\n", context_file);
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
        fprintf(stderr, "Error reading context file\n");
        free(buffer);
        return false;
    }
    
    buffer[file_size] = '\0';
    
    /* Parse JSON */
    legal_context = cJSON_Parse(buffer);
    free(buffer);
    
    if (legal_context == NULL) {
        fprintf(stderr, "Error parsing context file: %s\n", cJSON_GetErrorPtr());
        return false;
    }
    
    return true;
}


/**
 * Clean up resources used by the context manager
 */
void context_cleanup(void) {
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

