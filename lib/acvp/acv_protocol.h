/*
 * Copyright (C) 2020 - 2021, Stephan Mueller <smueller@chronox.de>
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

#ifndef ACV_PROTOCOL_H
#define ACV_PROTOCOL_H

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

static const struct acvp_net_proto acv_proto_def = {
	.url_base = "acvp/v1",
	.proto_version = "1.0",
	.proto_version_keyword = "acvVersion",
	.proto = acv_protocol,
	.proto_name = "ACVP",
	.basedir = ACVP_DS_DATADIR,
	.basedir_production = ACVP_DS_DATADIR_PRODUCTION,
	.secure_basedir = ACVP_DS_CREDENTIALDIR,
	.secure_basedir_production = ACVP_DS_CREDENTIALDIR_PRODUCTION,
};

#ifdef __cplusplus
}
#endif

#endif /* ACV_PROTOCOL_H */