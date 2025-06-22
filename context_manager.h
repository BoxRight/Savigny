/**
 * context_manager.h
 * 
 * Functions for loading and querying the legal context database
 * for the Kelsen schema transpiler
 */

#ifndef CONTEXT_MANAGER_H
#define CONTEXT_MANAGER_H

#include <stdbool.h>
#include "schema_types.h"
#include <cjson/cJSON.h>

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
 * @brief Applies automated norms to a schema based on its domain and rules.
 *
 * @param schema The schema to enrich with automated norms.
 */
void schema_apply_automated_norms(Schema* schema);

#endif /* CONTEXT_MANAGER_H */
