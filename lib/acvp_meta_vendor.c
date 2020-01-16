/* ACVP proxy protocol handler for managing the vendor information
 *
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

#include "errno.h"
#include "string.h"

#include "binhexbin.h"
#include "internal.h"
#include "json_wrapper.h"
#include "logger.h"
#include "request_helper.h"

static int acvp_vendor_build(const struct def_vendor *def_vendor,
			     struct json_object **json_vendor)
{
	struct json_object *array = NULL, *entry = NULL, *vendor = NULL,
			   *address = NULL;
	int ret = -EINVAL;

	/*
	 * {
	 * "name": "Acme, LLC",
	 * "website": "www.acme.acme",
	 * "emails" : [ "inquiry@acme.acme" ],
	 * "phoneNumbers" : [
	 *	{
	 *		"number": "555-555-0001",
	 *		"type" : "fax"
	 *	}, {
	 *		"number": "555-555-0002",
	 *		"type" : "voice"
	 *	}
	 * ],
	 * "addresses" [
	 * 	{
	 *		"street1" : "123 Main Street",
	 *		"locality" : "Any Town",
	 *		"region" : "AnyState",
	 *		"country" : "USA",
	 *		"postalCode" : "123456"
	 *	}
	 * ]
	 * }
	 */

	vendor = json_object_new_object();
	CKNULL(vendor, -ENOMEM);

	/* Name, website */
	CKINT(json_object_object_add(vendor, "name",
			json_object_new_string(def_vendor->vendor_name)));
	CKINT(json_object_object_add(vendor, "website",
			json_object_new_string(def_vendor->vendor_url)));

	/* Emails not defined */

	/* Phone numbers not defined */

	/* Addresses */
	address = json_object_new_object();
	CKNULL(address, -ENOMEM);
	CKINT(json_object_object_add(address, "street1",
			json_object_new_string(def_vendor->addr_street)));
	CKINT(json_object_object_add(address, "locality",
			json_object_new_string(def_vendor->addr_locality)));
	CKINT(json_object_object_add(address, "region",
			json_object_new_string(def_vendor->addr_region)));
	CKINT(json_object_object_add(address, "country",
			json_object_new_string(def_vendor->addr_country)));
	CKINT(json_object_object_add(address, "postalCode",
			json_object_new_string(def_vendor->addr_zipcode)));
	array = json_object_new_array();
	CKNULL(array, -ENOMEM);
	CKINT(json_object_array_add(array, address));
	address = NULL;
	CKINT(json_object_object_add(vendor, "addresses", array));
	array = NULL;

	json_logger(LOGGER_DEBUG2, LOGGER_C_ANY, vendor, "Vendor JSON object");

	*json_vendor = vendor;

	return 0;

out:
	ACVP_JSON_PUT_NULL(array);
	ACVP_JSON_PUT_NULL(entry);
	ACVP_JSON_PUT_NULL(vendor);
	ACVP_JSON_PUT_NULL(address);
	return ret;
}

static int acvp_vendor_match(struct def_vendor *def_vendor,
			     struct json_object *json_vendor)
{
	struct json_object *tmp;
	uint32_t vendor_id;
	unsigned int i;
	int ret;
	const char *str, *vendorurl;
	bool found = false;

	CKINT(json_get_string(json_vendor, "url", &vendorurl));
	CKINT(acvp_get_trailing_number(vendorurl, &vendor_id));

	CKINT(json_get_string(json_vendor, "name", &str));
	CKINT(acvp_str_match(def_vendor->vendor_name, str,
			     def_vendor->acvp_vendor_id));

	CKINT(json_find_key(json_vendor, "addresses", &tmp, json_type_array));
	for (i = 0; i < json_object_array_length(tmp); i++) {
		struct json_object *contact =
				json_object_array_get_idx(tmp, i);
		uint32_t id;
		const char *postalcode, *addr_street, *addr_locality;

		CKINT(json_get_string(contact, "postalCode", &postalcode));
		CKINT(json_get_string(contact, "street1", &addr_street));
		CKINT(json_get_string(contact, "locality", &addr_locality));
		CKINT(json_get_string(contact, "url", &str));
		/* Get the oe ID which is the last pathname component */
		CKINT(acvp_get_trailing_number(str, &id));

		if (!strncmp(def_vendor->addr_street, addr_street,
			     strlen(def_vendor->addr_street)) &&
		    !strncmp(def_vendor->addr_locality, addr_locality,
			     strlen(def_vendor->addr_locality)) &&
		    !strncmp(def_vendor->addr_zipcode, postalcode,
			     strlen(def_vendor->addr_zipcode))) {
			def_vendor->acvp_addr_id = id;
			found = true;
			break;
		}
	}

	if (!found) {
		logger(LOGGER_VERBOSE, LOGGER_C_ANY,
		       "Vendor address not found for vendor ID %u\n",
		       vendor_id);
		ret = -ENOENT;
		goto out;
	}

	def_vendor->acvp_vendor_id = vendor_id;

out:
	return ret;
}

/* GET /vendors/<vendorId> */
static int acvp_vendor_get_match(const struct acvp_testid_ctx *testid_ctx,
				 struct def_vendor *def_vendor)
{
	struct json_object *resp = NULL, *data = NULL;
	ACVP_BUFFER_INIT(buf);
	int ret, ret2;
	char url[ACVP_NET_URL_MAXLEN];

	CKINT(acvp_create_url(NIST_VAL_OP_VENDOR, url, sizeof(url)));
	CKINT(acvp_extend_string(url, sizeof(url), "/%u",
				 def_vendor->acvp_vendor_id));

	ret2 = acvp_process_retry_testid(testid_ctx, &buf, url);

	CKINT(acvp_store_vendor_debug(testid_ctx, &buf, ret2));

	if (ret2) {
		ret = ret2;
		goto out;
	}

	CKINT(acvp_req_strip_version(buf.buf, &resp, &data));
	CKINT(acvp_vendor_match(def_vendor, data));

out:
	ACVP_JSON_PUT_NULL(resp);
	acvp_free_buf(&buf);
	return ret;
}

/* POST / PUT / DELETE /vendors */
static int acvp_vendor_register(const struct acvp_testid_ctx *testid_ctx,
				struct def_vendor *def_vendor,
				char *url, unsigned int urllen,
				enum acvp_http_type type)
{
	const struct acvp_ctx *ctx = testid_ctx->ctx;
	const struct acvp_req_ctx *req_details = &ctx->req_details;
	struct json_object *json_vendor = NULL;
	int ret;

	/* Build JSON object with the vendor specification */
	if (type != acvp_http_delete) {
		CKINT(acvp_vendor_build(def_vendor, &json_vendor));
	}

	CKINT(acvp_meta_register(testid_ctx, json_vendor, url, urllen,
				 &def_vendor->acvp_vendor_id, type));
	if (req_details->dump_register) {
		goto out;
	}

	/* Fetch address ID */
	CKINT(acvp_vendor_get_match(testid_ctx, def_vendor));

out:
	ACVP_JSON_PUT_NULL(json_vendor);
	return ret;
}

static int acvp_vendor_validate_one(const struct acvp_testid_ctx *testid_ctx,
				    struct def_vendor *def_vendor)
{
	const struct acvp_ctx *ctx = testid_ctx->ctx;
	const struct acvp_opts_ctx *ctx_opts = &ctx->options;
	int ret;
	enum acvp_http_type http_type;
	char url[ACVP_NET_URL_MAXLEN];

	logger_status(LOGGER_C_ANY, "Validating vendor reference %u\n",
		      def_vendor->acvp_vendor_id);

	ret = acvp_vendor_get_match(testid_ctx, def_vendor);

	CKINT_LOG(acvp_search_to_http_type(ret, ACVP_OPTS_DELUP_VENDOR,
					   ctx_opts, def_vendor->acvp_vendor_id,
					   &http_type),
		  "Conversion from search type to HTTP request type failed for vendor\n");

	if (http_type == acvp_http_none)
		goto out;

	CKINT(acvp_create_url(NIST_VAL_OP_VENDOR, url, sizeof(url)));
	CKINT(acvp_vendor_register(testid_ctx, def_vendor, url, sizeof(url),
				   http_type));

out:
	return ret;
}

static int acvp_vendor_match_cb(void *private, struct json_object *json_vendor)
{
	struct def_vendor *def_vendor = private;
	int ret;

	ret = acvp_vendor_match(def_vendor, json_vendor);

	/* We found a match */
	if (!ret)
		return EINTR;
	/* We found no match, yet there was no error */
	if (ret == -ENOENT)
		return 0;

	/* We received an error */
	return ret;
}

/* GET /vendors */
static int acvp_vendor_validate_all(const struct acvp_testid_ctx *testid_ctx,
				    struct def_vendor *def_vendor)
{
	const struct acvp_ctx *ctx = testid_ctx->ctx;
	const struct acvp_opts_ctx *ctx_opts = &ctx->options;
	int ret;
	char url[ACVP_NET_URL_MAXLEN], queryoptions[256], vendorstr[128];

	logger_status(LOGGER_C_ANY,
		      "Searching for vendor reference - this may take time\n");

	CKINT(acvp_create_url(NIST_VAL_OP_VENDOR, url, sizeof(url)));

	/* Set a query option consisting of vendor_name */
	CKINT(bin2hex_html(def_vendor->vendor_name,
			   (uint32_t)strlen(def_vendor->vendor_name),
			   vendorstr, sizeof(vendorstr)));
	snprintf(queryoptions, sizeof(queryoptions), "name[0]=contains:%s",
		 vendorstr);
	CKINT(acvp_append_urloptions(queryoptions, url, sizeof(url)));

	CKINT(acvp_paging_get(testid_ctx, url, def_vendor,
			      &acvp_vendor_match_cb));

	/* We found an entry and do not need to do anything */
	if (ret > 0) {
		ret = 0;
		goto out;
	}

	/* Our vendor data does not match any vendor on ACVP server */
	if (ctx_opts->register_new_vendor) {
		CKINT(acvp_create_url(NIST_VAL_OP_VENDOR, url, sizeof(url)));
		CKINT(acvp_vendor_register(testid_ctx, def_vendor,
					   url, sizeof(url), acvp_http_post));
	} else {
		logger(LOGGER_ERR, LOGGER_C_ANY,
		       "No vendor definition found - request registering this module\n");
		ret = -ENOENT;
		goto out;
	}

out:
	return ret;
}

int acvp_vendor_handle(const struct acvp_testid_ctx *testid_ctx)
{
	const struct acvp_ctx *ctx = testid_ctx->ctx;
	const struct acvp_req_ctx *req_details;
	const struct definition *def;
	struct def_vendor *def_vendor;
	struct json_object *json_vendor = NULL;
	int ret = 0, ret2;

	CKNULL_LOG(testid_ctx, -EINVAL,
		   "Vendor handling: testid_ctx missing\n");
	def = testid_ctx->def;
	CKNULL_LOG(def, -EINVAL,
		   "Vendor handling: cipher definitions missing\n");
	def_vendor = def->vendor;
	CKNULL_LOG(def_vendor, -EINVAL,
		   "Vendor handling: vendor definitions missing\n");
	CKNULL_LOG(ctx, -EINVAL, "Vendor validation: ACVP context missing\n");
	req_details = &ctx->req_details;

	/* Lock def_vendor */
	CKINT(acvp_def_get_vendor_id(def_vendor));

	if (req_details->dump_register) {
		char url[ACVP_NET_URL_MAXLEN];

		CKINT_ULCK(acvp_create_url(NIST_VAL_OP_VENDOR, url,
					   sizeof(url)));
		acvp_vendor_register(testid_ctx, def_vendor, url, sizeof(url),
				     acvp_http_post);
		goto unlock;
	}

	/* Check if we have an outstanding request */
	ret2 = acvp_meta_obtain_request_result(testid_ctx,
					       &def_vendor->acvp_vendor_id);
	/* Fetch address ID */
	ret2 |= acvp_meta_obtain_request_result(testid_ctx,
					        &def_vendor->acvp_addr_id);
	if (ret2) {
		ret = ret2;
		goto unlock;
	}

	if (def_vendor->acvp_vendor_id) {
		CKINT_ULCK(acvp_vendor_validate_one(testid_ctx, def_vendor));
	} else {
		CKINT_ULCK(acvp_vendor_validate_all(testid_ctx, def_vendor));
	}

unlock:
	ret |= acvp_def_put_vendor_id(def_vendor);
out:
	ACVP_JSON_PUT_NULL(json_vendor);
	return ret;
}
