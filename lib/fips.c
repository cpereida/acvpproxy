/*
 * Copyright (C) 2018 - 2020, Stephan Mueller <smueller@chronox.de>
 *
 * License: see LICENSE file in root directory
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constructor.h"
#include "fips.h"
#include "hash/hmac.h"
#include "hash/hash.h"

#define FIPS_LOGGER_PREFIX	"FIPS POST: "

static int fips_post_hmac_sha256(void)
{
	const char key[] = "\x85";
	const char msg[] = "\xC9\x0E\x0F\x1E\x8C\xA1\xFD\x0E"
			   "\x0B\x17\xE4\xFA\xC4\xB6\xAA\x73";
	const char mac[] = "\xff\xd9\xd4\x56\xf0\xea\x5f\x9f"
			   "\x6e\x69\xf6\x05\xe4\x66\xc3\x8c"
			   "\x9f\x77\x4a\x37\x1c\xb0\xd4\xfb"
			   "\x78\x2d\xca\xbb\x1c\x25\x20\x4b";
	unsigned char *calculated = NULL;
	size_t calculated_len;
	int ret;

	if (!hmac(HASH_TYPE_SHA256,
		  key, sizeof(key) - 1,
		  msg, sizeof(msg) - 1,
		  &calculated, &calculated_len)) {
		fprintf(stderr, FIPS_LOGGER_PREFIX "HMAC calculation failed\n");
		ret = -EFAULT;
		goto out;
	}

	if (calculated_len != sizeof(mac) - 1) {
		fprintf(stderr, FIPS_LOGGER_PREFIX
			"Calculated MAC length has unexpected length\n");
		ret = -EINVAL;
		goto out;
	}

	if (memcmp(calculated, mac, calculated_len)) {
		fprintf(stderr, FIPS_LOGGER_PREFIX "Message mismatch\n");
		ret = -EBADMSG;
		goto out;
	}

	ret = 0;

out:
	if (calculated)
		free(calculated);
	return ret;
}

ACVP_DEFINE_CONSTRUCTOR(fips_post)
static void fips_post(void)
{
	int ret = fips_post_hmac_sha256();

	if (ret)
		exit(-ret);

	ret = fips_post_integrity();
	if (ret)
		exit(-ret);
}
