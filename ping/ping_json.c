/*
 * Copyright (c) 2024 Georg Pfuetzenreuter <mail+ip@georg-pfuetzenreuter.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ping.h"
#include "stdarg.h"

void construct_json(struct ping_rts *rts, int ptype, char *key, ...) {
	if (!rts->opt_json) {
		return;
	}

	char * val_str;
	int val_int;
	struct json_object *valjobj;

	va_list ap;
	va_start (ap, *key);

	if (!rts->pjobj) {
		rts->pjobj = json_object_new_object();
	}

	switch (ptype) {
		case PING_JSON_STR:
			val_str = va_arg (ap, char *);
			valjobj = json_object_new_string(val_str);
			break;

		case PING_JSON_INT:
			val_int = va_arg (ap, int);
			valjobj = json_object_new_int(val_int);
			break;
	}

	va_end (ap);

	json_object_object_add(rts->pjobj, key, valjobj);
}

void construct_json_statistics(struct ping_rts *rts, struct timespec tv, char *rttmin, char *rttavg, char *rttmax, char *rttmdev) {
	if (!rts->opt_json) {
		return;
	}

	struct json_object *pjrtt;
	pjrtt = json_object_new_object();

	if (!rts->pjstats) {
		rts->pjstats = json_object_new_object();
	}

	json_object_object_add(rts->pjstats, "host", json_object_new_string(rts->hostname));
	json_object_object_add(rts->pjstats, "transmitted", json_object_new_int(rts->ntransmitted));
	json_object_object_add(rts->pjstats, "received", json_object_new_int(rts->nreceived));
	json_object_object_add(rts->pjstats, "duplicates", json_object_new_int(rts->nrepeats));
	json_object_object_add(rts->pjstats, "corrupted", json_object_new_int(rts->nchecksum));
	json_object_object_add(rts->pjstats, "errors", json_object_new_int(rts->nerrors));
	// TODO: deduplicate calculations / maybe add calculated loss/time to rts?
	json_object_object_add(rts->pjstats, "loss", json_object_new_int((float)((((long long)(rts->ntransmitted - rts->nreceived)) * 100.0) / rts->ntransmitted)));
	json_object_object_add(rts->pjstats, "time", json_object_new_int((unsigned long long)(1000 * tv.tv_sec + (tv.tv_nsec + 500000) / 1000000)));

	json_object_object_add(rts->pjstats, "rtt", pjrtt);
	json_object_object_add(pjrtt, "min", json_object_new_string(rttmin));
	json_object_object_add(pjrtt, "avg", json_object_new_string(rttavg));
	json_object_object_add(pjrtt, "max", json_object_new_string(rttmax));
	json_object_object_add(pjrtt, "mdev", json_object_new_string(rttmdev));

	if (rts->pipesize > 1) {
		json_object_object_add(rts->pjstats, "pipe", json_object_new_int(rts->pipesize));
	}
}

void construct_json_statistics_flood(struct ping_rts *rts, char *ipg, char *ewma) {
	if (!rts->opt_json || !rts->pjstats) {
		return;
	}

	json_object_object_add(rts->pjstats, "ipg", json_object_new_string(ipg));
	json_object_object_add(rts->pjstats, "ewma", json_object_new_string(ewma));
}

void print_json(struct ping_rts *rts) {
	if (rts->opt_json && rts->pjobj) {
		printf("%s\n", json_object_to_json_string_ext(rts->pjobj, JSON_C_TO_STRING_SPACED));
		fflush(stdout);
	}
}

void print_json_statistics(struct ping_rts *rts) {
	if (rts->opt_json && rts->pjstats)
		printf("%s", json_object_to_json_string_ext(rts->pjstats, JSON_C_TO_STRING_SPACED));
}

void error_json(struct ping_rts *rts, int status, char *errtype, char *errmsg, int ptype, char *extrakey, ...) { 
	if (!rts->opt_json) {
		return;
	}

	char * extraval_str;
	int extraval_int;
	struct json_object *valjobj;
	struct json_object *errjobj;
	struct json_object *pjobj;

	errjobj = json_object_new_object();
	pjobj = json_object_new_object();

	va_list ap;
	va_start (ap, *extrakey);

	switch (ptype) {
		case PING_JSON_NUL:
			break;

		case PING_JSON_STR:
			extraval_str = va_arg (ap, char *);
			valjobj = json_object_new_string(extraval_str);
			break;

		case PING_JSON_INT:
			extraval_int = va_arg (ap, int);
			valjobj = json_object_new_int(extraval_int);
			break;
	}

	va_end (ap);

	json_object_object_add(errjobj, "type", json_object_new_string(errtype));
	json_object_object_add(errjobj, "error", json_object_new_string(errmsg));
	json_object_object_add(errjobj, "status", json_object_new_int(status));

	if (ptype != PING_JSON_NUL)
		json_object_object_add(errjobj, extrakey, valjobj);

	json_object_object_add(pjobj, "error", errjobj);

	printf("%s\n", json_object_to_json_string_ext(pjobj, JSON_C_TO_STRING_SPACED));
	fflush(stdout);

	if (status > 0)
		exit(status);
}
