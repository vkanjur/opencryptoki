/*
 * COPYRIGHT (c) International Business Machines Corp. 2019
 *
 * This program is provided under the terms of the Common Public License,
 * version 1.0 (CPL-1.0). Any use, reproduction or distribution for this
 * software constitutes recipient's acceptance of CPL-1.0 terms which can be
 * found in the file LICENSE file or at
 * https://opensource.org/licenses/cpl1.0.php
 */


#ifndef _EC_CURVES_H_
#define _EC_CURVES_H_

/*
 * OIDs and their DER encoding for the EC curves supported by OpenCryptoki:
 */

/* brainpoolP160r1: 1.3.36.3.3.2.8.1.1.1 */
#define OCK_BRAINPOOL_P160R1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x01 }

/* brainpoolP160t1: 1.3.36.3.3.2.8.1.1.2 */
#define OCK_BRAINPOOL_P160T1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x02 }

/* brainpoolP192r1: 1.3.36.3.3.2.8.1.1.3 */
#define OCK_BRAINPOOL_P192R1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x03 }

/* brainpoolP192t1: 1.3.36.3.3.2.8.1.1.4 */
#define OCK_BRAINPOOL_P192T1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x04 }

/* brainpoolP224r1: 1.3.36.3.3.2.8.1.1.5 */
#define OCK_BRAINPOOL_P224R1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x05 }

/* brainpoolP224t1: 1.3.36.3.3.2.8.1.1.6 */
#define OCK_BRAINPOOL_P224T1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x06 }

/* brainpoolP256r1: 1.3.36.3.3.2.8.1.1.7 */
#define OCK_BRAINPOOL_P256R1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x07 }

/* brainpoolP256t1: 1.3.36.3.3.2.8.1.1.8 */
#define OCK_BRAINPOOL_P256T1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x08 }

/* brainpoolP320r1: 1.3.36.3.3.2.8.1.1.9 */
#define OCK_BRAINPOOL_P320R1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x09 }

/* brainpoolP320t1: 1.3.36.3.3.2.8.1.1.10 */
#define OCK_BRAINPOOL_P320T1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x0A }

/* brainpoolP384r1: 1.3.36.3.3.2.8.1.1.11 */
#define OCK_BRAINPOOL_P384R1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x0B }

/* brainpoolP384t1: 1.3.36.3.3.2.8.1.1.12 */
#define OCK_BRAINPOOL_P384T1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x0C }

/* brainpoolP512r1: 1.3.36.3.3.2.8.1.1.13 */
#define OCK_BRAINPOOL_P512R1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x0D }

/* brainpoolP512t1: 1.3.36.3.3.2.8.1.1.14 */
#define OCK_BRAINPOOL_P512T1    { 0x06, 0x09, 0x2B, 0x24, 0x03, 0x03, \
                                  0x02, 0x08, 0x01, 0x01, 0x0E }

/* prime192: 1.2.840.10045.3.1.1 */
#define OCK_PRIME192V1          { 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, \
                                  0x3D, 0x03, 0x01, 0x01 }

 /* secp224: 1.3.132.0.33 */
#define OCK_SECP224R1           { 0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x21 }

 /* prime256: 1.2.840.10045.3.1.7 */
#define OCK_PRIME256V1          { 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, \
                                  0x3D, 0x03, 0x01, 0x07 }

 /* secp384: 1.3.132.0.34 */
#define OCK_SECP384R1           { 0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x22 }

/* secp521: 1.3.132.0.35 */
#define OCK_SECP521R1           { 0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x23 }

/* secp256k1:  1.3.132.0.10 */
#define OCK_SECP256K1           { 0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x0A }


#endif                          // _EC_CURVES_H_