#define _GNU_SOURCE
#include <security/pam_ext.h>
#include <security/pam_modules.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static void close_file(FILE** filep) {
	if (filep == NULL || *filep == NULL)
		return;
	fclose(*filep);
}

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	(void) flags;

	if (argc != 2) {
		pam_syslog(pamh, LOG_ERR, "Invalid usage, exactly two parameters required: pam_oom.so [absolute|relative] <adjust>");
		return PAM_SESSION_ERR;
	}

	bool is_absolute = strcmp(argv[0], "absolute") == 0;
	if (!is_absolute && strcmp(argv[0], "relative") != 0) {
		pam_syslog(pamh, LOG_ERR, "Invalid usage, first parameter must be either \"absolute\" or \"relative\"");
		return PAM_SESSION_ERR;
	}

	long adjustment;
	{
		char* endptr;
		adjustment = strtol(argv[1], &endptr, 10);
		if (*endptr) {
			pam_syslog(pamh, LOG_ERR, "Could not parse score adjustment");
			return PAM_SESSION_ERR;
		}
	}
	if (adjustment > 1000 || adjustment < -1000) {
		pam_syslog(pamh, LOG_ERR, "Specified score %ld is out of range [-1000, 1000]", adjustment);
		return PAM_SESSION_ERR;
	}
	

	__attribute__((cleanup(close_file))) FILE *oom_adj_file = fopen("/proc/self/oom_score_adj", "r+");
	if (oom_adj_file == NULL) {
		pam_syslog(pamh, LOG_ERR, "Could not open /proc/self/oom_score_adj: %m");
		return PAM_SESSION_ERR;
	}

	if (!is_absolute) {
		char buffer[128];
		ssize_t n = fread(buffer, 1, sizeof(buffer)-1, oom_adj_file);
		if (n < 0) {
			pam_syslog(pamh, LOG_ERR, "Could not read from /proc/self/oom_score_adj: %m");
			return PAM_SESSION_ERR;
		}
		buffer[n] = '\0';

		long previous_adjustment;
		{
			char* endptr;
			previous_adjustment = strtol(buffer, &endptr, 10);
			if (*endptr != '\n') {
				pam_syslog(pamh, LOG_ERR, "Could not parse score from /proc/self/oom_score_adj");
				return PAM_SESSION_ERR;
			}
		}
		long sum = adjustment + previous_adjustment;
		
		// clamp score to value between -999 and 1000, except if one parameter is -1000,
		// in which case -1000 is fine, too.
		if (sum <= -1000 && (previous_adjustment == -1000  || adjustment == -1000))
			adjustment = -1000; // -1000 => Never kill.
		else if (sum > 1000)
			adjustment = 1000;
		else if (sum < -999)
			adjustment = -999;
		else
			adjustment = sum;

		rewind(oom_adj_file);
	}

	char buffer[128];
	int len = snprintf(buffer, sizeof(buffer), "%ld\n", adjustment);
	ssize_t r = fwrite(buffer, 1, len + 1, oom_adj_file);
	if (r < 0) {
		pam_syslog(pamh, LOG_ERR, "Could not write new score %ld into /proc/self/oom_score_adj: %m", adjustment);
		return PAM_SESSION_ERR;
	}

	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char** argv) {
	(void) pamh, (void) flags, (void) argc, (void) argv;
	return PAM_SUCCESS;
}
