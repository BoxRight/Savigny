/**
 * context_manager.h
 * 
 * Functions for loading and querying the legal context database
 * for the Kelsen schema transpiler
 */

#ifndef CONTEXT_MANAGER_H
#define CONTEXT_MANAGER_H
#ifndef LEGAL_CONTEXT_H
#define LEGAL_CONTEXT_H

#include <stdbool.h>
#include <cJSON.h>
#include "schema_types.h"





/**
 * Initialize the legal context module
 * 
 * @param filename Path to the JSON file containing legal sources
 * @return true if initialization succeeded, false otherwise
 */
bool legal_context_init(const char* filename);

/**
 * Clean up resources used by the legal context module
 */
void legal_context_cleanup(void);

/**
 * Generate Kelsen code enhanced with legal context
 * 
 * @param schema The schema to generate code for
 * @return The generated Kelsen code, or NULL on error
 */
char* generate_kelsen_code_with_context(Schema* schema);

#endif /* LEGAL_CONTEXT_H */
/**
 * Initialize the context manager with a JSON file
 * 
 * @param context_file Path to the JSON context file
 * @return true if initialization succeeded, false otherwise
 */
bool context_init(const char* context_file);

/**
 * Check if the context manager is initialized
 * 
 * @return true if the context manager is initialized, false otherwise
 */
bool context_is_initialized(void);

/**
 * Clean up resources used by the context manager
 */
void context_cleanup(void);

/**
 * Get a legal norm definition from the context
 * 
 * @param source The source identifier (e.g., "CODIGO_CIVIL")
 * @param norm_id The norm identifier (e.g., "Art1545")
 * @return A cJSON object representing the norm, or NULL if not found
 */
cJSON* context_get_norm(const char* source, const char* norm_id);

/**
 * Check if a norm has a direct relationship with another norm
 * 
 * @param source1 Source of the first norm
 * @param norm_id1 ID of the first norm
 * @param source2 Source of the second norm
 * @param norm_id2 ID of the second norm
 * @return true if a relationship exists, false otherwise
 */
bool context_has_relationship(const char* source1, const char* norm_id1, 
                              const char* source2, const char* norm_id2);

/**
 * Get the mapped role for a specific contract type
 * 
 * @param contract_type The type of contract (e.g., "compraventa")
 * @param generic_role The generic role to map (e.g., "deudor")
 * @return The mapped specific role or NULL if not found
 */
const char* context_map_role(const char* contract_type, const char* generic_role);

/**
 * Enrich a norm with contextual information
 * 
 * @param norm The norm to enrich
 * @param context The context database
 * @return true if the norm was enriched, false otherwise
 */
bool context_enrich_norm(Norm* norm, cJSON* context);

/**
 * Validate that a norm is legally consistent with the context
 * 
 * @param norm The norm to validate
 * @return true if the norm is valid, false otherwise
 */
bool context_validate_norm(Norm* norm);

/**
 * Get context information for a specific legal domain
 * 
 * @param domain The legal domain (e.g., "derecho-patrimonial-privado")
 * @return A cJSON object with domain-specific context, or NULL if not found
 */
cJSON* context_get_domain_info(const char* domain);

/**
 * Get additional annotations for a Kelsen clause based on context
 * 
 * @param norm The norm being converted to a Kelsen clause
 * @return A string with additional annotations or NULL if none
 */
char* context_get_kelsen_annotations(Norm* norm);


cJSON* context_infer_role_mappings(const char* contract_type);
#endif /* CONTEXT_MANAGER_H */
