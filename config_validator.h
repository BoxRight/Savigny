/**
 * config_validator.h
 * 
 * Functions for loading and validating tokens against a JSON configuration file
 */

#ifndef CONFIG_VALIDATOR_H
#define CONFIG_VALIDATOR_H

#include <stdbool.h>
#include "schema_types.h"

/**
 * Initialize the configuration validator
 * 
 * @param config_file Path to the JSON configuration file
 * @return true if initialization succeeded, false otherwise
 */
bool config_init(const char* config_file);

/**
 * Clean up resources used by the configuration validator
 */
void config_cleanup(void);

/**
 * Set the current institution context
 * 
 * @param institution Name of the institution to set as current
 */
void config_set_current_institution(const char* institution);

/**
 * Get the current institution context
 * 
 * @return The current institution name or NULL if not set
 */
const char* config_get_current_institution(void);

/**
 * Validate an institution name
 * 
 * @param institution Name to validate
 * @return true if valid, false otherwise
 */
bool config_is_valid_institution(const char* institution);

/**
 * Validate an institution type
 * 
 * @param type Type to validate
 * @return true if valid, false otherwise
 */
bool config_is_valid_type(const char* type);

/**
 * Validate a legal domain
 * 
 * @param domain Domain to validate
 * @return true if valid, false otherwise
 */
bool config_is_valid_domain(const char* domain);

/**
 * Validate a role for the current institution
 * 
 * @param role Role to validate
 * @return true if valid, false otherwise
 */
bool config_is_valid_role(const char* role);

/**
 * Validate a role for a specific institution
 * 
 * @param institution Institution context
 * @param role Role to validate
 * @return true if valid, false otherwise
 */
bool config_is_valid_role_for_institution(const char* institution, const char* role);

/**
 * Get a deontic operator from a string representation
 * 
 * @param text String representation
 * @return The corresponding DeonticOperator value
 */
DeonticOperator config_get_deontic_operator(const char* text);

/**
 * Get an institution type from a string representation
 * 
 * @param text String representation
 * @return The corresponding InstitutionType value
 */
InstitutionType config_get_institution_type(const char* text);

/**
 * Get a multiplicity from a string representation
 * 
 * @param text String representation
 * @return The corresponding Multiplicity value
 */
Multiplicity config_get_multiplicity(const char* text);

/**
 * Get a compliance type from a string representation
 * 
 * @param text String representation
 * @return The corresponding ComplianceType value
 */
ComplianceType config_get_compliance_type(const char* text);

/**
 * Suggest a correction for a possibly misspelled institution
 * 
 * @param institution Potentially misspelled institution
 * @return A suggested correction or NULL if none found
 */
const char* config_suggest_institution(const char* institution);

/**
 * Suggest a correction for a possibly misspelled role
 * 
 * @param role Potentially misspelled role
 * @return A suggested correction or NULL if none found
 */
const char* config_suggest_role(const char* role);

#endif /* CONFIG_VALIDATOR_H */
