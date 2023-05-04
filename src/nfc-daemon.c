/*-
 * nfc-daemon is a modification of nfc-poll from the libnfc-project.
 * The modifications were made by:
 * Copyright (C) 2020	  Wolfgang Hotwagner
 *
 * The following license and copyrights are from the original nfc-poll:
 *
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2020      Adam Laurie
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
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
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

#include "utils.h"
#include "logger.h"
#include "runner.h"

#define MAX_DEVICE_COUNT 16
#define MAX_TARGET_COUNT 16

static nfc_device *pnd = NULL;
static nfc_context *context;

/* define the loglevel */
enum log_levels loglevel = error;


static void stop_polling(int sig)
{
  (void) sig;
  if (pnd != NULL)
    nfc_abort_command(pnd);
  else {
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
}

static void print_usage(const char *progname)
{
  printf("usage: %s [-v]\n", progname);
  printf("  -v\t\t verbose display\n");
  printf("  -h\t\t print help screen\n");
  printf("  -t\t\t dry-run. no script-execution\n");
  printf("  -l [0-4]\t use loglevel\n");
  printf("  -x [cmd]\t command to run, UID will be passed as first argument\n");
}

int main(int argc, char *argv[])
{
  int option;
  bool verbose = false;
  char *endptr;
  bool dry_run = false;

  signal(SIGINT, stop_polling);

  /* Display libnfc version */
  const char *acLibnfcVersion = nfc_version();

  char * runscript = malloc(255);
  strcpy(runscript, "");

  while((option = getopt(argc, argv, "vhl:tx:")) != -1)
  {
	switch(option)
	{
		case 'v':
			verbose = true;
			break;
		case 'h':
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		case 'x':
            strcpy(runscript, optarg);
            break;
		case 'l':
			errno = 0;
			int val = strtol(optarg,&endptr,10);
			if ((errno == ERANGE)
				|| (errno != 0 && val == 0)) {
				fprintf(stderr,"invalid loglevel: %s\n",strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(set_loglevel(val) < 0)
			{
				fprintf(stderr,"loglevel must be between 0-4\n");
				exit(EXIT_FAILURE);
			}
			break;
                case 't':
			dry_run = true;
			break;
		default:
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
	}
  }

  if (strlen(runscript) == 0) {
      log_debug("no runscript specified, doing dry run");
      dry_run = true;
  } else {
      log_debug("runscript is %s", runscript);
  }
  log_info("%s uses libnfc %s", argv[0], acLibnfcVersion);

  int res = 0;

  nfc_init(&context);
  if (context == NULL) {
    log_error("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  nfc_target ant[MAX_TARGET_COUNT];
  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    log_error("%s", "Unable to open NFC device.");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  log_info("NFC reader: %s opened", nfc_device_get_name(pnd));
  nfc_modulation nm;
  char *uid = malloc(100);
  while(1)
  {
      nm.nmt = NMT_ISO14443A;
      nm.nbr = NBR_106;
      // List ISO14443A targets
      if ((res = nfc_initiator_list_passive_targets(pnd, nm, ant, MAX_TARGET_COUNT)) >= 0) {
        int n;
        if (verbose) {
          printf("%d ISO14443A passive target(s) found%s\n", res, (res == 0) ? ".\n" : ":");
        }
        for (n = 0; n < res; n++) {
          if (verbose) print_nfc_target(&ant[n], verbose);
          snprint_UID(uid,100, &ant[n]);
          printf("UID=%s\n",uid);
          fflush(stdout);
          if(!dry_run) run_script(runscript,uid);
        }
      } else {
          printf("Error %d",res);
          exit(EXIT_FAILURE);
      }
   }
   nfc_free(uid);

  exit(EXIT_SUCCESS);
}
