/*
 * Licensed materials, Property of IBM Corp.
 *
 * OpenCryptoki ICSF token - LDAP functions
 *
 * (C) COPYRIGHT International Business Machines Corp. 2012
 *
 * Author: Marcelo Cerri (mhcerri@br.ibm.com)
 *
 */

#ifndef ICSF_H
#define ICSF_H

#include <ldap.h>
#include "pkcs11types.h"

/* OIDs used for PKCS extension */
#define ICSF_REQ_OID "1.3.18.0.2.12.83"
#define ICSF_RES_OID "1.3.18.0.2.12.84"

/* Tag numbers for each ICSF call */
#define ICSF_TAG_CSFPTRC 14

/* Return codes */
#define ICSF_RC_SUCCESS 0
#define ICSF_RC_PARTIAL_SUCCESS 4
#define ICSF_RC_IS_ERROR(rc) \
	(rc > ICSF_RC_PARTIAL_SUCCESS)

/* Default lengths */
#define ICSF_HANDLE_LEN 44
#define ICSF_TOKEN_NAME_LEN 32
#define ICSF_MANUFACTURER_LEN 32
#define ICSF_MODEL_LEN 16
#define ICSF_SERIAL_LEN 16
#define ICSF_RULE_ITEM_LEN 8

int
icsf_login(LDAP **ld, const char *uri, const char *dn,
	   const char *password);

int
icsf_sasl_login(LDAP **ld, const char *uri, const char *cert,
	        const char *key, const char *ca, const char *ca_dir);

int
icsf_logout(LDAP *ld);

int
icsf_check_pkcs_extension(LDAP *ld);

int
icsf_create_token(LDAP *ld, const char *token_name,
	          const char *manufacturer_id, const char *model,
	          const char *serial_number);

#endif
