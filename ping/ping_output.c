/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2024-2025 Georg Pfuetzenreuter <mail+ip@georg-pfuetzenreuter.net>
 */

#include "ping.h"
#include "stdarg.h"

void ping_print_int(struct ping_rts *rts, char *msg, char *json_key, int json_val) {
	if (rts->opt_json) {
		construct_json(rts, PING_JSON_INT, json_key, json_val);
	} else {
		printf(msg, json_val);
	}
}

void ping_print_uint(struct ping_rts *rts, char *msg, char *json_key, unsigned int json_val) {
	if (rts->opt_json) {
		construct_json(rts, PING_JSON_UINT, json_key, json_val);
	} else {
		printf(msg, json_val);
	}
}

void ping_print_str(struct ping_rts *rts, char *msg, char *json_key, char *json_val) {
	if (rts->opt_json) {
		construct_json(rts, PING_JSON_STR, json_key, json_val);
	} else {
		printf(msg, json_val);
	}
}

inline void ping_print_version(struct ping_rts *rts) {
	if (rts->opt_json) {
		construct_json(rts, PING_JSON_STR, "version", PACKAGE_VERSION);
		print_json_packet(rts);
	} else {
		printf(PACKAGE_VERSION);
		print_config();
	}
}

inline void ping_print_finish(struct ping_rts *rts) {
	if(!rts->opt_json) {
		putchar('\n');
		fflush(stdout);
	}
}

void ping_error(struct ping_rts *rts, int status, int errnum, char *format, ...) { 
	char msg[50];

	va_list ap;
	va_start (ap, *format);

	vsnprintf(msg, sizeof(msg), format, ap);

	if (rts->opt_json) {
		construct_json_error(rts, errnum, msg);
		if (status > 0)
			exit(status);
	} else {
		error(status, errnum, "%s", msg); \
	}
}
