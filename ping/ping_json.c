/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2024 Georg Pfuetzenreuter <mail+ip@georg-pfuetzenreuter.net>
 */

#include "ping.h"
#include "stdarg.h"

static inline void json_emergency(char *msg) {
	error(1, 0, "%s", msg);
}

static inline void test_buflen(int have) {
	if (have < 0 || have >= PING_JSON_MAX)
		json_emergency("Overflow during JSON construction.");
}

static void json_end_array(char *part) {
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "]"));
}

static void json_start_object(char *part) {
	test_buflen(snprintf(part, PING_JSON_MAX, "{"));
}

static void json_end_object(char *part) {
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "}"));
}

static void json_continue(char *part) {
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, ", "));
}

static void json_kv_str(char *part, char *key, char *value) {
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "\"%s\": \"%s\"", key, value));
}

static void json_kv_str_continue(char *part, char *key, char *value) {
	json_kv_str(part, key, value);
	json_continue(part);
}

static void json_kv_int(char *part, char *key, int value) {
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "\"%s\": %d", key, value));
}

static void json_kv_uint(char *part, char *key, unsigned int value) {
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "\"%s\": %u", key, value));
}

static void json_kv_int_continue(char *part, char *key, int value) {
	json_kv_int(part, key, value);
	json_continue(part);
}

/* so far only supports arrays of strings */
static void json_kv_array(char *part, char *key, va_list ap) {
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "\"%s\": [", key));

	char * value;
	int count = 0;

	while (1) {
		value = va_arg(ap, char *);

		if (!value)
			break;

		if (count > 0)
			json_continue(part);

		curlen = strlen(part);
		test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "\"%s\"", value));

		count = count + 1;
	}

	json_end_array(part);
}

static void json_kv_object(char *part, char *key, char *value) {
	json_end_object(value);
	size_t curlen = strlen(part);
	test_buflen(snprintf(part + curlen, PING_JSON_MAX - curlen, "\"%s\": %s", key, value));
}

void construct_json(struct ping_rts *rts, int ptype, char *key, ...) {
	if (!rts->opt_json) {
		return;
	}

	char * val_str;
	int val_int;
	unsigned int val_uint;
	
	va_list ap;
	va_start (ap, *key);

	if (*rts->json_packet) {
		json_continue(rts->json_packet);
	} else {
		json_start_object(rts->json_packet);
	}

	switch (ptype) {
		case PING_JSON_ARR:
			json_kv_array(rts->json_packet, key, ap);
			break;

		case PING_JSON_STR:
			val_str = va_arg (ap, char *);
			json_kv_str(rts->json_packet, key, val_str);
			break;

		case PING_JSON_INT:
			val_int = va_arg (ap, int);
			json_kv_int(rts->json_packet, key, val_int);
			break;

		case PING_JSON_UINT:
			val_uint = va_arg (ap, unsigned int);
			json_kv_uint(rts->json_packet, key, val_uint);
			break;
	}


	va_end (ap);
}

void construct_json_statistics(struct ping_rts *rts, struct timespec tv, char *rttmin, char *rttavg, char *rttmax, char *rttmdev) {
	if (!rts->opt_json) {
		return;
	}

	if (*rts->json_stats) {
		json_continue(rts->json_stats);
	} else {
		json_start_object(rts->json_stats);
	}

	json_kv_str_continue(rts->json_stats, "host", rts->hostname);
	json_kv_int_continue(rts->json_stats, "transmitted", rts->ntransmitted);
	json_kv_int_continue(rts->json_stats, "received", rts->nreceived);
	json_kv_int_continue(rts->json_stats, "duplicates", rts->nrepeats);
	json_kv_int_continue(rts->json_stats, "corrupted", rts->nchecksum);
	json_kv_int_continue(rts->json_stats, "errors", rts->nerrors);
	json_kv_int_continue(rts->json_stats, "loss", (float)((((long long)(rts->ntransmitted - rts->nreceived)) * 100.0) / rts->ntransmitted));
	json_kv_int_continue(rts->json_stats, "time", (unsigned long long)(1000 * tv.tv_sec + (tv.tv_nsec + 500000) / 1000000));

	char json_rtt[PING_JSON_MAX];
	json_start_object(json_rtt);
	json_kv_str_continue(json_rtt, "min", rttmin);
	json_kv_str_continue(json_rtt, "avg", rttavg);
	json_kv_str_continue(json_rtt, "max", rttmax);
	json_kv_str(json_rtt, "mdev", rttmdev);

	json_kv_object(rts->json_stats, "rtt", json_rtt);

	if (rts->pipesize > 1) {
		json_continue(rts->json_stats);
		json_kv_int_continue(rts->json_stats, "pipe", rts->pipesize);
	}
}

void construct_json_statistics_flood(struct ping_rts *rts, char *ipg, char *ewma) {
	if (!rts->opt_json || !*rts->json_stats) {
		return;
	}

	// align with conditionals in finish()
	if (rts->nreceived && rts->timing && rts->pipesize < 2)
		json_continue(rts->json_stats);

	json_kv_str_continue(rts->json_stats, "ipg", ipg);
	json_kv_str(rts->json_stats, "ewma", ewma);
}

void print_json_and_reset(char *part) {
	printf("%s}\n", part);
	fflush(stdout);
	*part = '\0';
}

void print_json_packet(struct ping_rts *rts) {
	if (rts->opt_json && *rts->json_packet)
		print_json_and_reset(rts->json_packet);
}

void print_json_statistics(struct ping_rts *rts) {
	if (rts->opt_json && *rts->json_stats) {
		printf("%s}\n", rts->json_stats);
		fflush(stdout);
	}
}

void construct_json_error(struct ping_rts *rts, int errnum, char *errmsg) { 
	if (errnum < 0)
		json_emergency("Unable to process errnum for JSON construction.");

	if (errmsg && errnum == 0)
		construct_json(rts, PING_JSON_STR, "error", errmsg);
	else if (!errmsg)
		construct_json(rts, PING_JSON_STR, "error", strerror(errnum));
	else
		construct_json(rts, PING_JSON_ARR, "error", errmsg, strerror(errnum));

	print_json_packet(rts);
}
