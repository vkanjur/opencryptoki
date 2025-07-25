/*
 * COPYRIGHT (c) International Business Machines Corp. 2011-2017
 *
 * This program is provided under the terms of the Common Public License,
 * version 1.0 (CPL-1.0). Any use, reproduction or distribution for this
 * software constitutes recipient's acceptance of CPL-1.0 terms which can be
 * found in the file LICENSE file or at
 * https://opensource.org/licenses/cpl1.0.php
 */

/*
 * openCryptoki testcase for RSA
 *
 * August 18, 2011
 *
 * Fionnuala Gunter <fin@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "pkcs11types.h"
#include "common.c"
#include "regress.h"
#include "mech_to_str.h"

#include "rsa.h"

#define CHUNK 20

/* This function should test:
 * RSA Key Generation, usign CKM_RSA_PKCS_KEY_PAIR_GEN
 * RSA SignUpdate for generated test vectors, mechanism chosen by caller
 * RSA VerifyUpdate for generated test vectors, mechanism chosen by caller
 *
 * 1. Generate RSA Key Pair
 * 2. Generate message
 * 3. Sign message
 * 4. Verify signature
 *
 */
CK_RV do_SignVerifyUpdateRSA(struct GENERATED_TEST_SUITE_INFO *tsuite)
{
    unsigned int i;                      // test vector index
    unsigned int j;                      // message byte index
    int inc, count, len;
    CK_BYTE message[MAX_MESSAGE_SIZE];
    CK_ULONG message_len;
    CK_BYTE signature[MAX_SIGNATURE_SIZE];
    CK_ULONG signature_len;

    CK_MECHANISM mech;
    CK_OBJECT_HANDLE publ_key, priv_key;

    CK_SLOT_ID slot_id = SLOT_ID;
    CK_SESSION_HANDLE session;
    CK_FLAGS flags;
    CK_BYTE user_pin[PKCS11_MAX_PIN_LEN];
    CK_ULONG user_pin_len;
    CK_RV rc;

    char *s;

    // begin testsuite
    testsuite_begin("%s SignUpdate VerifyUpdate.", tsuite->name);
    testcase_rw_session();
    testcase_user_login();

    // skip tests if the slot doesn't support this mechanism
    if (!mech_supported(slot_id, tsuite->mech.mechanism)) {
        testsuite_skip(tsuite->tvcount,
                       "Slot %u doesn't support %s (0x%x)",
                       (unsigned int) slot_id,
                       mech_to_str(tsuite->mech.mechanism),
                       (unsigned int) tsuite->mech.mechanism);
        goto testcase_cleanup;
    }

    // iterate over test vectors
    for (i = 0; i < tsuite->tvcount; i++) {
        // get public exponent from test vector
        if (p11_ahex_dump(&s, tsuite->tv[i].publ_exp,
                          tsuite->tv[i].publ_exp_len) == NULL) {
            testcase_error("p11_ahex_dump() failed");
            rc = -1;
            goto testcase_cleanup;
        }

        // begin test
        testcase_begin("%s Sign and Verify with test vector %u, "
                       "\npubl_exp='%s', mod_bits='%lu', keylen='%lu'.",
                       tsuite->name, i, s,
                       tsuite->tv[i].modbits, tsuite->tv[i].keylen);

        if (tsuite->tv[i].modbits > 4096 && !rsa8k) {
            testcase_skip("Tests with  modbits='%lu' are not enabled",
                          tsuite->tv[i].modbits);
            free(s);
            continue;
        }
        if (!keysize_supported(slot_id, tsuite->mech.mechanism,
                               tsuite->tv[i].modbits)) {
            testcase_skip("Token in slot %lu cannot be used with modbits='%lu'",
                          SLOT_ID, tsuite->tv[i].modbits);
            free(s);
            continue;
        }

        if (is_ep11_token(slot_id)) {
            if (!is_valid_ep11_pubexp(tsuite->tv[i].publ_exp,
                                      tsuite->tv[i].publ_exp_len)) {
                testcase_skip("EP11 Token cannot be used with publ_exp.='%s'", s);
                free(s);
                continue;
            }
        }

        if (is_cca_token(slot_id)) {
            if (!is_valid_cca_pubexp(tsuite->tv[i].publ_exp,
                                     tsuite->tv[i].publ_exp_len)) {
                testcase_skip("CCA Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (is_soft_token(slot_id)) {
            if (!is_valid_soft_pubexp(tsuite->tv[i].publ_exp,
                                      tsuite->tv[i].publ_exp_len)) {
                testcase_skip("Soft Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }


        if (is_tpm_token(slot_id)) {
            if ((!is_valid_tpm_pubexp(tsuite->tv[i].publ_exp,
                                      tsuite->tv[i].publ_exp_len))
                || (!is_valid_tpm_modbits(tsuite->tv[i].modbits))) {
                testcase_skip("TPM Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (is_icsf_token(slot_id)) {
            if (!is_valid_icsf_pubexp(tsuite->tv[i].publ_exp,
                                      tsuite->tv[i].publ_exp_len) ||
                (tsuite->tv[i].modbits < 1024)) {
                testcase_skip("ICSF Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (tsuite->tv[i].modbits <= 512 &&
            (tsuite->mech.mechanism == CKM_SHA384_RSA_PKCS ||
             tsuite->mech.mechanism == CKM_SHA512_RSA_PKCS ||
             tsuite->mech.mechanism == CKM_SHA3_384_RSA_PKCS ||
             tsuite->mech.mechanism == CKM_SHA3_512_RSA_PKCS)) {
            testcase_skip("Mechanism %s can not be used with a key with mod_bits='%lu'.",
                          mech_to_str(tsuite->mech.mechanism),
                          tsuite->tv[i].modbits);
            free(s);
            continue;
        }

        // free memory
        free(s);

        rc = CKR_OK;            // set rc

        // clear buffers
        memset(message, 0, MAX_MESSAGE_SIZE);
        memset(signature, 0, MAX_SIGNATURE_SIZE);

        // get test vector parameters
        message_len = tsuite->tv[i].inputlen;

        // generate key pair
        rc = generate_RSA_PKCS_KeyPair_cached(session,
                                              tsuite->tv[i].modbits,
                                              tsuite->tv[i].publ_exp,
                                              tsuite->tv[i].publ_exp_len,
                                              &publ_key, &priv_key);
        if (rc != CKR_OK) {
            if (rc == CKR_POLICY_VIOLATION) {
                testcase_skip("RSA key generation is not allowed by policy");
                continue;
            }
            if (rc == CKR_KEY_SIZE_RANGE) {
                testcase_skip("RSA key size is not in supported range");
                continue;
            }

            testcase_error("generate_RSA_PKCS_KeyPair_cached(), rc=%s",
                           p11_get_ckr(rc));
            goto testcase_cleanup;
        }

        // generate message
        for (j = 0; j < message_len; j++) {
            message[j] = (j + 1) % 255;
        }

        // get  mech
        mech = tsuite->mech;

        // initialize Sign (length only)
        rc = funcs->C_SignInit(session, &mech, priv_key);
        if (rc != CKR_OK) {
            testcase_error("C_SignInit(), rc=%s", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        // set buffer size
        signature_len = 0;

        // do SignUpdate
        if (message_len > 0) {
            len = message_len;
            for (count = 0; len > 0; count += inc) {
                if (len < CHUNK)
                    inc = len;
                else
                    inc = CHUNK;

                rc = funcs->C_SignUpdate(session, message + count, inc);
                if (rc != CKR_OK) {
                    testcase_error("C_SignUpdate(), rc=%s.", p11_get_ckr(rc));
                    goto testcase_cleanup;
                }
                len -= inc;
            }
        } else {
            rc = funcs->C_SignUpdate(session, NULL, 0);
            if (rc != CKR_OK) {
                testcase_error("C_SignUpdate(), rc=%s.", p11_get_ckr(rc));
                goto testcase_cleanup;
            }
        }

        /* get the required length */
        testcase_new_assertion();
        rc = funcs->C_SignFinal(session, NULL, &signature_len);
        if (rc != CKR_OK) {
            testcase_error("C_SignFinal(),rc=%s.", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        if (signature_len == (tsuite->tv[i].modbits / 8)) {
            testcase_pass("C_SignFinal set output length.");
        } else {
            testcase_fail("C_SignFinal failed to set length: "
                          "expected %lu, got %lu.",
                          signature_len, tsuite->tv[i].modbits / 8);
            goto testcase_cleanup;
        }

        rc = funcs->C_SignFinal(session, signature, &signature_len);
        if (rc != CKR_OK) {
            testcase_error("C_SignFinal(),rc=%s.", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        // initialize Verify
        rc = funcs->C_VerifyInit(session, &mech, publ_key);
        if (rc != CKR_OK) {
            testcase_error("C_VerifyInit(), rc=%s", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        // do VerifyUpdate
        len = message_len;
        if (message_len > 0) {
            for (count = 0; len > 0; count += inc) {
                if (len < CHUNK)
                    inc = len;
                else
                    inc = CHUNK;

                rc = funcs->C_VerifyUpdate(session, message + count, inc);
                if (rc != CKR_OK) {
                    testcase_error("C_VerifyUpdate(), rc=%s.", p11_get_ckr(rc));
                    goto testcase_cleanup;
                }
                len -= inc;
            }
        } else {
            rc = funcs->C_VerifyUpdate(session, NULL, 0);
            if (rc != CKR_OK) {
                testcase_error("C_VerifyUpdate(), rc=%s.", p11_get_ckr(rc));
                goto testcase_cleanup;
            }
        }
        rc = funcs->C_VerifyFinal(session, signature, signature_len);

        // check results
        testcase_new_assertion();
        if (rc == CKR_OK) {
            testcase_pass("C_VerifyFinal.");
        } else {
            testcase_fail("C_VerifyFinal, rc=%s", p11_get_ckr(rc));
        }

    }

testcase_cleanup:
    free_rsa_key_cache(session);
    testcase_user_logout();
    rc = funcs->C_CloseAllSessions(slot_id);
    if (rc != CKR_OK) {
        testcase_error("C_CloesAllSessions, rc=%s", p11_get_ckr(rc));
    }

    return rc;
}


/* This function should test:
 * RSA Key Generation, usign CKM_RSA_PKCS_KEY_PAIR_GEN
 * RSA-PSS Sign, mechanism chosen by caller
 * RSA-PSS Verify, mechanism chosen by caller
 *
 * 1. Generate RSA Key Pair
 * 2. Generate message
 * 3. Sign message
 * 4. Verify signature
 *
 */
#define MAX_HASH_SIZE 64
CK_RV do_SignVerifyUpdate_RSAPSS(struct GENERATED_TEST_SUITE_INFO * tsuite)
{
    unsigned int i;                      // test vector index
    unsigned int j;                      // message byte index
    int len;
    CK_BYTE message[MAX_MESSAGE_SIZE];
    CK_BYTE signature[MAX_SIGNATURE_SIZE];
    CK_ULONG message_len, signature_len, data_done;

    CK_MECHANISM mech;
    CK_OBJECT_HANDLE publ_key, priv_key;

    CK_SLOT_ID slot_id = SLOT_ID;
    CK_SESSION_HANDLE session;
    CK_FLAGS flags;
    CK_BYTE user_pin[PKCS11_MAX_PIN_LEN];
    CK_ULONG user_pin_len;
    CK_RV rc;
    CK_RSA_PKCS_PSS_PARAMS pss_params;

    char *s;

    // begin testsuite
    testsuite_begin("%s SignUpdate VerifyUpdate.", tsuite->name);
    testcase_rw_session();
    testcase_user_login();

    // skip tests if the slot doesn't support this mechanism
    if (!mech_supported(slot_id, tsuite->mech.mechanism)) {
        testsuite_skip(tsuite->tvcount,
                       "Slot %u doesn't support %s (0x%x)",
                       (unsigned int) slot_id,
                       mech_to_str(tsuite->mech.mechanism),
                       (unsigned int) tsuite->mech.mechanism);
        goto testcase_cleanup;
    }
    // iterate over test vectors
    for (i = 0; i < tsuite->tvcount; i++) {

        // get public exponent from test vector
        if (p11_ahex_dump(&s, tsuite->tv[i].publ_exp,
                          tsuite->tv[i].publ_exp_len) == NULL) {
            testcase_error("p11_ahex_dump() failed");
            rc = -1;
            goto testcase_cleanup;
        }
        // begin test
        testcase_begin("%s Sign and Verify with test vector %u, "
                       "\npubl_exp='%s', mod_bits='%lu', keylen='%lu'.",
                       tsuite->name, i, s,
                       tsuite->tv[i].modbits, tsuite->tv[i].keylen);

        if (tsuite->tv[i].modbits > 4096 && !rsa8k) {
            testcase_skip("Tests with  modbits='%lu' are not enabled",
                          tsuite->tv[i].modbits);
            free(s);
            continue;
        }
        if (!keysize_supported(slot_id, tsuite->mech.mechanism,
                               tsuite->tv[i].modbits)) {
            testcase_skip("Token in slot %lu cannot be used with modbits='%lu'",
                          SLOT_ID, tsuite->tv[i].modbits);
            free(s);
            continue;
        }

        if (is_ep11_token(slot_id)) {
            if (!is_valid_ep11_pubexp(tsuite->tv[i].publ_exp,
                                      tsuite->tv[i].publ_exp_len)) {
                testcase_skip("EP11 Token cannot be used with publ_exp.='%s'", s);
                free(s);
                continue;
            }
        }

        if (is_cca_token(slot_id)) {
            if (!is_valid_cca_pubexp(tsuite->tv[i].publ_exp,
                                     tsuite->tv[i].publ_exp_len)) {
                testcase_skip("CCA Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (is_soft_token(slot_id)) {
            if (!is_valid_soft_pubexp(tsuite->tv[i].publ_exp,
                                      tsuite->tv[i].publ_exp_len)) {
                testcase_skip("Soft Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        // free memory
        free(s);

        rc = CKR_OK;            // set rc

        // clear buffers
        memset(message, 0, MAX_MESSAGE_SIZE);
        memset(signature, 0, MAX_SIGNATURE_SIZE);

        // get test vector parameters
        message_len = tsuite->tv[i].inputlen;

        // generate key pair
        rc = generate_RSA_PKCS_KeyPair_cached(session,
                                              tsuite->tv[i].modbits,
                                              tsuite->tv[i].publ_exp,
                                              tsuite->tv[i].publ_exp_len,
                                              &publ_key, &priv_key);
        if (rc != CKR_OK) {
            if (rc == CKR_POLICY_VIOLATION) {
                testcase_skip("RSA key generation is not allowed by policy");
                continue;
            }
            if (rc == CKR_KEY_SIZE_RANGE) {
                testcase_skip("RSA key size is not in supported range");
                continue;
            }

            testcase_error("generate_RSA_PKCS_KeyPair_cached(), rc=%s",
                           p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        // generate message
        for (j = 0; j < message_len; j++) {
            message[j] = (j + 1) % 255;
        }

        // set mechanism for signing the digest
        mech = tsuite->mech;
        pss_params = tsuite->tv[i].pss_params;
        mech.pParameter = &pss_params;
        mech.ulParameterLen = sizeof(CK_RSA_PKCS_PSS_PARAMS);

        // initialize Sign (length only)
        rc = funcs->C_SignInit(session, &mech, priv_key);
        if (rc != CKR_OK) {
            testcase_error("C_SignInit(), rc=%s", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        // set buffer size
        signature_len = 0;
        data_done = 0;

        // do SignUpdate
        if (tsuite->tv[i].num_chunks) {
            CK_BYTE *data_chunk = NULL;

            for (j = 0; j < (unsigned int)tsuite->tv[i].num_chunks; j++) {
                if (tsuite->tv[i].chunks[j] == -1) {
                    len = 0;
                    data_chunk = NULL;
                } else if (tsuite->tv[i].chunks[j] == 0) {
                    len = 0;
                    data_chunk = (CK_BYTE *) "";
                } else {
                    len = tsuite->tv[i].chunks[j];
                    data_chunk = message + data_done;
                }

                rc = funcs->C_SignUpdate(session, data_chunk, len);
                if (rc != CKR_OK) {
                    testcase_error("C_SignUpdate rc=%s", p11_get_ckr(rc));
                    goto testcase_cleanup;
                }
                data_done += len;
            }
        } else {
            rc = funcs->C_SignUpdate(session, message, message_len);
            if (rc != CKR_OK) {
                testcase_error("C_SignUpdate rc=%s", p11_get_ckr(rc));
                goto testcase_cleanup;
            }
        }


        /* get the required length */
        testcase_new_assertion();
        rc = funcs->C_SignFinal(session, NULL, &signature_len);
        if (rc != CKR_OK) {
            testcase_error("C_SignFinal(),rc=%s.", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        if (signature_len == (tsuite->tv[i].modbits / 8)) {
            testcase_pass("C_SignFinal set output length.");
        } else {
            testcase_fail("C_SignFinal failed to set length: "
                          "expected %lu, got %lu.",
                          signature_len, tsuite->tv[i].modbits / 8);
            goto testcase_cleanup;
        }

        rc = funcs->C_SignFinal(session, signature, &signature_len);
        if (rc != CKR_OK) {
            testcase_error("C_SignFinal(),rc=%s.", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        // initialize Verify
        rc = funcs->C_VerifyInit(session, &mech, publ_key);
        if (rc != CKR_OK) {
            testcase_error("C_VerifyInit(), rc=%s", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
        // do VerifyUpdate
        data_done = 0;
        if (tsuite->tv[i].num_chunks) {
            CK_BYTE *data_chunk = NULL;

            for (j = 0; j < (unsigned int)tsuite->tv[i].num_chunks; j++) {
                if (tsuite->tv[i].chunks[j] == -1) {
                    len = 0;
                    data_chunk = NULL;
                } else if (tsuite->tv[i].chunks[j] == 0) {
                    len = 0;
                    data_chunk = (CK_BYTE *) "";
                } else {
                    len = tsuite->tv[i].chunks[j];
                    data_chunk = message + data_done;
                }

                rc = funcs->C_VerifyUpdate(session, data_chunk, len);
                if (rc != CKR_OK) {
                    testcase_error("C_VerifyUpdate rc=%s", p11_get_ckr(rc));
                    goto testcase_cleanup;
                }
                data_done += len;
            }
        } else {
            rc = funcs->C_VerifyUpdate(session, message, message_len);
            if (rc != CKR_OK) {
                testcase_error("C_VerifyUpdate rc=%s", p11_get_ckr(rc));
                goto testcase_cleanup;
            }
        }
        rc = funcs->C_VerifyFinal(session, signature, signature_len);

        // check results
        testcase_new_assertion();
        if (rc == CKR_OK) {
            testcase_pass("C_VerifyFinal.");
        } else {
            testcase_fail("C_VerifyFinal(), rc=%s", p11_get_ckr(rc));
        }
    }

testcase_cleanup:
    free_rsa_key_cache(session);
    testcase_user_logout();
    rc = funcs->C_CloseAllSessions(slot_id);
    if (rc != CKR_OK) {
        testcase_error("C_CloesAllSessions, rc=%s", p11_get_ckr(rc));
    }

    return rc;
}

/* This function should test:
 * C_Verify, mechanism chosen by caller
 *
 * 1. Get message from test vector
 * 2. Get signature from test vector
 * 3. Verify signature
 *
 */
CK_RV do_VerifyUpdateRSA(struct PUBLISHED_TEST_SUITE_INFO * tsuite)
{
    unsigned int i, inc, len, j;
    CK_BYTE actual[MAX_SIGNATURE_SIZE];
    CK_BYTE message[MAX_MESSAGE_SIZE];
    CK_ULONG message_len;
    CK_BYTE signature[MAX_SIGNATURE_SIZE];
    CK_ULONG signature_len;

    CK_MECHANISM mech;
    CK_OBJECT_HANDLE publ_key;

    CK_SLOT_ID slot_id = SLOT_ID;
    CK_SESSION_HANDLE session;
    CK_FLAGS flags;
    CK_BYTE user_pin[PKCS11_MAX_PIN_LEN];
    CK_ULONG user_pin_len;
    CK_RV rc, loc_rc;

    char *s;

    // begin testsuite
    testsuite_begin("%s Verify.", tsuite->name);
    testcase_rw_session();
    testcase_user_login();

    // skip tests if the slot doesn't support this mechanism
    if (!mech_supported(slot_id, tsuite->mech.mechanism)) {
        testsuite_skip(tsuite->tvcount,
                       "Slot %u doesn't support %s (0x%x)",
                       (unsigned int) slot_id,
                       mech_to_str(tsuite->mech.mechanism),
                       (unsigned int) tsuite->mech.mechanism);
        goto testcase_cleanup;
    }
    // iterate over test vectors
    for (i = 0; i < tsuite->tvcount; i++) {

        if (p11_ahex_dump(&s, tsuite->tv[i].pub_exp,
                          tsuite->tv[i].pubexp_len) == NULL) {
            testcase_error("p11_ahex_dump() failed");
            rc = -1;
            goto testcase_cleanup;
        }

        testcase_begin("%s Verify with test vector %u.", tsuite->name, i);

        if (tsuite->tv[i].mod_len * 8 > 4096 && !rsa8k) {
            testcase_skip("Tests with  modbits='%lu' are not enabled",
                          tsuite->tv[i].mod_len * 8);
            free(s);
            continue;
        }
        if (!keysize_supported(slot_id, tsuite->mech.mechanism,
                               tsuite->tv[i].mod_len * 8)) {
            testcase_skip("Token in slot %lu cannot be used with modbits='%lu'",
                          SLOT_ID, tsuite->tv[i].mod_len * 8);
            free(s);
            continue;
        }

        // special case for EP11
        // modulus length must be multiple of 128 byte
        // skip test if modulus length has unsuported size
        if (is_ep11_token(slot_id)) {
            if ((tsuite->tv[i].mod_len % 128) != 0) {
                testcase_skip("EP11 Token cannot be used with "
                              "this key size (no 128bit granularity).");
                free(s);
                continue;
            }
        }

        if (is_ep11_token(slot_id)) {
            if (!is_valid_ep11_pubexp(tsuite->tv[i].pub_exp,
                                      tsuite->tv[i].pubexp_len)) {
                testcase_skip("EP11 Token cannot be used with pub_exp.='%s'", s);
                free(s);
                continue;
            }
        }

        if (is_tpm_token(slot_id)) {
            if ((!is_valid_tpm_pubexp(tsuite->tv[i].pub_exp,
                                      tsuite->tv[i].pubexp_len)) ||
                (!is_valid_tpm_modbits(tsuite->tv[i].mod_len))) {
                testcase_skip("TPM Token cannot be used with pub_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (is_cca_token(slot_id)) {
            if (!is_valid_cca_pubexp(tsuite->tv[i].pub_exp,
                                     tsuite->tv[i].pubexp_len)) {
                testcase_skip("CCA Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (is_soft_token(slot_id)) {
            if (!is_valid_soft_pubexp(tsuite->tv[i].pub_exp,
                                      tsuite->tv[i].pubexp_len)) {
                testcase_skip("Soft Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        // free memory
        free(s);

        rc = CKR_OK;            // set return value

        // clear buffers
        memset(message, 0, MAX_MESSAGE_SIZE);
        memset(signature, 0, MAX_SIGNATURE_SIZE);
        memset(actual, 0, MAX_SIGNATURE_SIZE);

        // get message
        message_len = tsuite->tv[i].msg_len;
        memcpy(message, tsuite->tv[i].msg, message_len);

        // get signature
        signature_len = tsuite->tv[i].sig_len;
        memcpy(signature, tsuite->tv[i].sig, signature_len);

        // create (public) key handle
        rc = create_RSAPublicKey(session,
                                 tsuite->tv[i].mod,
                                 tsuite->tv[i].pub_exp,
                                 tsuite->tv[i].mod_len,
                                 tsuite->tv[i].pubexp_len, &publ_key);

        if (rc != CKR_OK) {
            if (rc == CKR_POLICY_VIOLATION) {
                testcase_skip("RSA key import is not allowed by policy");
                continue;
            }
            if (rc == CKR_KEY_SIZE_RANGE) {
                testcase_skip("RSA key size is not in supported range");
                continue;
            }

            testcase_error("create_RSAPublicKey(), rc=%s", p11_get_ckr(rc));
            goto error;
        }
        // set mechanism
        mech = tsuite->mech;

        // initialize verify
        rc = funcs->C_VerifyInit(session, &mech, publ_key);
        if (rc != CKR_OK) {
            testcase_error("C_VerifyInit(), rc=%s", p11_get_ckr(rc));
            goto error;
        }
        // do verify
        len = message_len;
        for (j = 0; len > 0; j += inc) {
            if (len < CHUNK)
                inc = len;
            else
                inc = CHUNK;

            rc = funcs->C_VerifyUpdate(session, message + j, inc);
            if (rc != CKR_OK) {
                testcase_error("C_VerifyUpdate(), rc=%s.", p11_get_ckr(rc));
                goto error;
            }
            len -= inc;
        }

        // check result
        testcase_new_assertion();

        rc = funcs->C_VerifyFinal(session, signature, signature_len);
        if (rc == CKR_OK) {
            testcase_pass("C_Verify.");
        } else {
            testcase_fail("%s Sign Verify with test vector %u "
                          "failed.", tsuite->name, i);
        }

        // clean up
        rc = funcs->C_DestroyObject(session, publ_key);
        if (rc != CKR_OK) {
            testcase_error("C_DestroyObject(), rc=%s.", p11_get_ckr(rc));
            goto testcase_cleanup;
        }

    }
    goto testcase_cleanup;
error:
    loc_rc = funcs->C_DestroyObject(session, publ_key);
    if (loc_rc != CKR_OK) {
        testcase_error("C_DestroyObject(), rc=%s.", p11_get_ckr(loc_rc));
    }

testcase_cleanup:
    testcase_user_logout();
    rc = funcs->C_CloseAllSessions(slot_id);
    if (rc != CKR_OK) {
        testcase_error("C_CloseAllSessions rc=%s", p11_get_ckr(rc));
    }

    return rc;
}

/* This function should test:
 * C_Sign_Update and C_SignFinal, mechanism chosen by caller
 *
 * 1. Get message from test vector
 * 2. Get expected signature from test vector
 * 3. Sign message
 * 4. Compare expected signature with actual signature
 *
 */
CK_RV do_SignUpdateRSA(struct PUBLISHED_TEST_SUITE_INFO * tsuite)
{
    unsigned int i, len, j;
    CK_BYTE message[MAX_MESSAGE_SIZE];
    CK_BYTE actual[MAX_SIGNATURE_SIZE];
    CK_BYTE expected[MAX_SIGNATURE_SIZE];
    CK_ULONG message_len, actual_len, expected_len;
    CK_ULONG data_done;

    CK_MECHANISM mech;
    CK_OBJECT_HANDLE priv_key;

    CK_SLOT_ID slot_id = SLOT_ID;
    CK_SESSION_HANDLE session;
    CK_FLAGS flags;
    CK_BYTE user_pin[PKCS11_MAX_PIN_LEN];
    CK_ULONG user_pin_len;
    CK_RV rc, loc_rc;
    char *s;

    // begin testsuite
    testsuite_begin("%s Sign. ", tsuite->name);
    testcase_rw_session();
    testcase_user_login();

    // skip tests if the slot doesn't support this mechanism **/
    if (!mech_supported(slot_id, tsuite->mech.mechanism)) {
        testsuite_skip(tsuite->tvcount,
                       "Slot %u doesn't support %s (0x%x)",
                       (unsigned int) slot_id,
                       mech_to_str(tsuite->mech.mechanism),
                       (unsigned int) tsuite->mech.mechanism);
        goto testcase_cleanup;
    }
    // iterate over test vectors
    for (i = 0; i < tsuite->tvcount; i++) {
        if (p11_ahex_dump(&s, tsuite->tv[i].pub_exp,
                          tsuite->tv[i].pubexp_len) == NULL) {
            testcase_error("p11_ahex_dump() failed");
            rc = -1;
            goto testcase_cleanup;
        }
        testcase_begin("%s Sign with test vector %u.", tsuite->name, i);

        if (tsuite->tv[i].mod_len * 8 > 4096 && !rsa8k) {
            testcase_skip("Tests with  modbits='%lu' are not enabled",
                          tsuite->tv[i].mod_len * 8);
            free(s);
            continue;
        }
        if (!keysize_supported(slot_id, tsuite->mech.mechanism,
                               tsuite->tv[i].mod_len * 8)) {
            testcase_skip("Token in slot %lu cannot be used with modbits='%lu'",
                          SLOT_ID, tsuite->tv[i].mod_len * 8);
            free(s);
            continue;
        }

        // special case for ica
        // prime1, prime2, exp1, exp2, coef
        // must be size mod_len/2 or smaller
        // skip test if prime1, or prime2, or exp1,
        // or exp2 or coef are too long
        if (is_ica_token(slot_id)) {
            // check sizes
            if ((tsuite->tv[i].prime1_len >
                 (tsuite->tv[i].mod_len / 2)) ||
                (tsuite->tv[i].prime2_len >
                 (tsuite->tv[i].mod_len / 2)) ||
                (tsuite->tv[i].exp1_len >
                 (tsuite->tv[i].mod_len / 2)) ||
                (tsuite->tv[i].exp2_len >
                 (tsuite->tv[i].mod_len / 2)) ||
                (tsuite->tv[i].coef_len > (tsuite->tv[i].mod_len / 2))) {
                testcase_skip("ICA Token cannot be used with this test vector.");
                free(s);
                continue;
            }

        }
        // special case for EP11
        // modulus length must be multiple of 128 byte
        // skip test if modulus length has unsuported size
        if (is_ep11_token(slot_id)) {
            if ((tsuite->tv[i].mod_len % 128) != 0) {
                testcase_skip("EP11 Token cannot be used with "
                              "this key size (no 128bit granularity).");
                free(s);
                continue;
            }
        }

        if (is_ep11_token(slot_id)) {
            if (!is_valid_ep11_pubexp(tsuite->tv[i].pub_exp,
                                      tsuite->tv[i].pubexp_len)) {
                testcase_skip("EP11 Token cannot be used with publ_exp.='%s'", s);
                free(s);
                continue;
            }
        }

        if (is_tpm_token(slot_id)) {
            if ((!is_valid_tpm_pubexp(tsuite->tv[i].pub_exp,
                                      tsuite->tv[i].pubexp_len)) ||
                (!is_valid_tpm_modbits(tsuite->tv[i].mod_len))) {
                testcase_skip("TPM Token cannot be used with pub_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (is_cca_token(slot_id)) {
            if (!is_valid_cca_pubexp(tsuite->tv[i].pub_exp,
                                     tsuite->tv[i].pubexp_len)) {
                testcase_skip("CCA Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        if (is_soft_token(slot_id)) {
            if (!is_valid_soft_pubexp(tsuite->tv[i].pub_exp,
                                      tsuite->tv[i].pubexp_len)) {
                testcase_skip("Soft Token cannot be used with publ_exp='%s'.", s);
                free(s);
                continue;
            }
        }

        free(s);

        rc = CKR_OK;            // set return value

        // clear buffers
        memset(message, 0, MAX_MESSAGE_SIZE);
        memset(actual, 0, MAX_SIGNATURE_SIZE);
        memset(expected, 0, MAX_SIGNATURE_SIZE);

        actual_len = 0;         // get this from opencryptoki

        data_done = 0;

        // get message
        message_len = tsuite->tv[i].msg_len;
        memcpy(message, tsuite->tv[i].msg, message_len);

        // get (expected) signature
        expected_len = tsuite->tv[i].sig_len;
        memcpy(expected, tsuite->tv[i].sig, expected_len);

        // create (private) key handle
        rc = create_RSAPrivateKey(session,
                                  tsuite->tv[i].mod,
                                  tsuite->tv[i].pub_exp,
                                  tsuite->tv[i].priv_exp,
                                  tsuite->tv[i].prime1,
                                  tsuite->tv[i].prime2,
                                  tsuite->tv[i].exp1,
                                  tsuite->tv[i].exp2,
                                  tsuite->tv[i].coef,
                                  tsuite->tv[i].mod_len,
                                  tsuite->tv[i].pubexp_len,
                                  tsuite->tv[i].privexp_len,
                                  tsuite->tv[i].prime1_len,
                                  tsuite->tv[i].prime2_len,
                                  tsuite->tv[i].exp1_len,
                                  tsuite->tv[i].exp2_len,
                                  tsuite->tv[i].coef_len, &priv_key);
        if (rc != CKR_OK) {
            if (rc == CKR_POLICY_VIOLATION) {
                testcase_skip("RSA key import is not allowed by policy");
                continue;
            }
            if (rc == CKR_KEY_SIZE_RANGE) {
                testcase_skip("RSA key size is not in supported range");
                continue;
            }

            testcase_error("create_RSAPrivateKey(), rc=%s", p11_get_ckr(rc));
            goto error;
        }
        // set mechanism
        mech = tsuite->mech;

        // initialize signing
        rc = funcs->C_SignInit(session, &mech, priv_key);
        if (rc != CKR_OK) {
            testcase_error("C_SignInit(), rc=%s.", p11_get_ckr(rc));
            goto error;
        }
        // do signing
        if (tsuite->tv[i].num_chunks) {
            CK_BYTE *data_chunk = NULL;

            for (j = 0; j < (unsigned int)tsuite->tv[i].num_chunks; j++) {
                if (tsuite->tv[i].chunks[j] == -1) {
                    len = 0;
                    data_chunk = NULL;
                } else if (tsuite->tv[i].chunks[j] == 0) {
                    len = 0;
                    data_chunk = (CK_BYTE *) "";
                } else {
                    len = tsuite->tv[i].chunks[j];
                    data_chunk = message + data_done;
                }

                rc = funcs->C_SignUpdate(session, data_chunk, len);
                if (rc != CKR_OK) {
                    testcase_error("C_SignUpdate rc=%s", p11_get_ckr(rc));
                    goto testcase_cleanup;
                }

                data_done += len;
            }
        } else {
            rc = funcs->C_SignUpdate(session, message, message_len);
            if (rc != CKR_OK) {
                testcase_error("C_SignUpdate rc=%s", p11_get_ckr(rc));
                goto testcase_cleanup;
            }
        }

        /* get the required length */
        testcase_new_assertion();
        rc = funcs->C_SignFinal(session, NULL, &actual_len);
        if (rc != CKR_OK) {
            testcase_error("C_SignFinal(),rc=%s.", p11_get_ckr(rc));
            goto error;
        }
        if (actual_len == tsuite->tv[i].mod_len) {
            testcase_pass("C_SignFinal set output length.");
        } else {
            testcase_fail("C_SignFinal failed to set length: "
                          "expected %lu, got %lu.",
                          actual_len, tsuite->tv[i].mod_len);
            goto error;
        }

        rc = funcs->C_SignFinal(session, actual, &actual_len);
        if (rc != CKR_OK) {
            testcase_error("C_SignFinal(),rc=%s.", p11_get_ckr(rc));
            goto error;
        }
        // check results
        testcase_new_assertion();

        if (actual_len != expected_len) {
            testcase_fail("%s Sign with test vector %u failed. "
                          "Expected len=%lu, found len=%lu.",
                          tsuite->name, i, expected_len, actual_len);
        } else if (memcmp(actual, expected, expected_len)) {
            testcase_fail("%s Sign with test vector %u failed. "
                          "Signature data does not match test vector "
                          "signature.", tsuite->name, i);

        } else {
            testcase_pass("C_Sign.");
        }

        // clean up
        rc = funcs->C_DestroyObject(session, priv_key);
        if (rc != CKR_OK) {
            testcase_error("C_DestroyObject(), rc=%s.", p11_get_ckr(rc));
            goto testcase_cleanup;
        }
    }
    goto testcase_cleanup;
error:
    loc_rc = funcs->C_DestroyObject(session, priv_key);
    if (loc_rc != CKR_OK) {
        testcase_error("C_DestroyObject, rc=%s.", p11_get_ckr(loc_rc));
    }
testcase_cleanup:
    testcase_user_logout();
    loc_rc = funcs->C_CloseAllSessions(slot_id);
    if (loc_rc != CKR_OK) {
        testcase_error("C_CloseAllSessions, rc=%s.", p11_get_ckr(loc_rc));
    }

    return rc;
}

CK_RV rsa_funcs(void)
{
    unsigned int i;
    CK_RV rv = CKR_OK;

    // published (known answer) tests
    for (i = 0; i < NUM_OF_PUBLISHED_TESTSUITES; i++) {
        rv = do_SignUpdateRSA(&published_test_suites[i]);
        if (rv != CKR_OK && (!no_stop))
            break;
    }

    for (i = 0; i < NUM_OF_PUBLISHED_TESTSUITES; i++) {
        rv = do_VerifyUpdateRSA(&published_test_suites[i]);
        if (rv != CKR_OK && (!no_stop))
            break;
    }

    // generated sign verify tests
    for (i = 0; i < NUM_OF_GENERATED_SIGVER_UPDATE_TESTSUITES; i++) {
        rv = do_SignVerifyUpdateRSA(&generated_sigver_update_test_suites[i]);
        if (rv != CKR_OK && (!no_stop))
            break;
    }

    for (i = 0; i < NUM_OF_GENERATED_PSS_UPDATE_TESTSUITES; i++) {
        rv = do_SignVerifyUpdate_RSAPSS(&generated_pss_update_test_suites[i]);
        if (rv != CKR_OK && (!no_stop))
            break;
    }

    return rv;
}

int main(int argc, char **argv)
{
    int rc;
    CK_C_INITIALIZE_ARGS cinit_args;
    CK_RV rv;

    rc = do_ParseArgs(argc, argv);
    if (rc != 1) {
        return rc;
    }

    printf("Using slot #%lu...\n\n", SLOT_ID);
    printf("With option: no_stop: %d\n", no_stop);

    rc = do_GetFunctionList();
    if (!rc) {
        PRINT_ERR("ERROR do_GetFunctionList() Failed, rx = 0x%0x\n", rc);
        return rc;
    }

    memset(&cinit_args, 0x0, sizeof(cinit_args));
    cinit_args.flags = CKF_OS_LOCKING_OK;

    funcs->C_Initialize(&cinit_args);
    {
        CK_SESSION_HANDLE hsess = 0;
        rc = funcs->C_GetFunctionStatus(hsess);
        if (rc != CKR_FUNCTION_NOT_PARALLEL) {
            return rc;
        }

        rc = funcs->C_CancelFunction(hsess);
        if (rc != CKR_FUNCTION_NOT_PARALLEL) {
            return rc;
        }
    }

    testcase_setup();
    rv = rsa_funcs();
    testcase_print_result();

    funcs->C_Finalize(NULL);

    return testcase_return(rv);
}
