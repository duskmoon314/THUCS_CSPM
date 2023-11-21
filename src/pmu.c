#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "papi.h"

int event_set = PAPI_NULL;
char mode = 'c';

void __attribute__((constructor)) load_cspm_pmu() {
  printf("====================\n");
  printf("CSPM: [INFO] Loading CSPM PMU...\n");

  // Loading config from env "CAPM_PMU" via getopt
  // -m <mode> (c, 1, 2, t, b)
  char *env = getenv("CSPM_PMU");
  env = env ? env : "";
  int argc = 1;
  char *argv[10];
  argv[0] = "cspm_pmu"; // pseudo argv[0]

  char *token = strtok(env, " ");
  while (token) {
    argv[argc++] = token;
    token = strtok(NULL, " ");
  }

  optind = 0;
  int opt;
  while ((opt = getopt(argc, argv, "m:")) != -1) {
    switch (opt) {
    case 'm':
      mode = optarg[0];
      break;
    default:
      printf("CSPM: [ERROR] Unknown option: %c\n", opt);
      exit(1);
    }
  }
  optind = 0;

  int ret;
  ret = PAPI_library_init(PAPI_VER_CURRENT);
  if (ret != PAPI_VER_CURRENT) {
    printf("CSPM: [ERROR] PAPI library initializetion error!\n");
    exit(1);
  }

  ret = PAPI_create_eventset(&event_set);
  if (ret != PAPI_OK) {
    printf("CSPM: [ERROR] PAPI event set creation error!\n");
    exit(1);
  }

  int events[10] = {// CPI
                    PAPI_TOT_CYC, PAPI_TOT_INS,
                    // L1 miss
                    PAPI_L1_DCM, PAPI_LST_INS,
                    // L2 miss
                    PAPI_L2_DCM, PAPI_L2_DCA,
                    // TLB miss
                    PAPI_TLB_DM, PAPI_LST_INS,
                    // Branch miss
                    PAPI_BR_MSP, PAPI_BR_PRC};

  switch (mode) {
  case 'c':
    // CPI mode
    // Add TOT_CYC and TOT_INS
    ret = PAPI_add_events(event_set, events, 2);
    if (ret != PAPI_OK) {
      printf("CSPM: [ERROR] PAPI event add error! %d: %s\n", ret,
             PAPI_strerror(ret));
      exit(1);
    }
    break;

  case '1':
    // L1 miss mode
    // Add L1_DCM, LST_INS
    ret = PAPI_add_events(event_set, events + 2, 2);
    if (ret != PAPI_OK) {
      printf("CSPM: [ERROR] PAPI event add error! %d: %s\n", ret,
             PAPI_strerror(ret));
      exit(1);
    }
    break;

  case '2':
    // L2 miss mode
    // Add L2_DCM, L2_DCA
    ret = PAPI_add_events(event_set, events + 4, 2);
    if (ret != PAPI_OK) {
      printf("CSPM: [ERROR] PAPI event add error! %d: %s\n", ret,
             PAPI_strerror(ret));
      exit(1);
    }
    break;

  case 't':
    // TLB miss mode
    // Add TLB_DM, LST_INS
    ret = PAPI_add_events(event_set, events + 6, 2);
    if (ret != PAPI_OK) {
      printf("CSPM: [ERROR] PAPI event add error! %d: %s\n", ret,
             PAPI_strerror(ret));
      exit(1);
    }
    break;

  case 'b':
    // Branch miss mode
    // Add BR_MSP, BR_PRC
    ret = PAPI_add_events(event_set, events + 8, 2);
    if (ret != PAPI_OK) {
      printf("CSPM: [ERROR] PAPI event add error! %d: %s\n", ret,
             PAPI_strerror(ret));
      exit(1);
    }
    break;

  default:
    printf("CSPM: [ERROR] Unknown mode: %c\n", mode);
    exit(1);
    break;
  }

  printf("CSPM: [INFO] CSPM PMU loaded!\n");
  printf("====================\n");

  PAPI_start(event_set);
}

void __attribute__((destructor)) unload_cspm_pmu() {
  long long values[2];
  PAPI_stop(event_set, values);

  printf("====================\n");
  printf("CSPM: [INFO] Unloading CSPM PMU...\n");

  switch (mode) {
  case 'c':
    // CPI mode
    printf("CPI: %f\n", (double)values[0] / (double)values[1]);
    printf("  TOT_CYC: %lld\n", values[0]);
    printf("  TOT_INS: %lld\n", values[1]);
    break;

  case '1':
    // L1 miss mode
    printf("L1 miss: %f\n", (double)values[0] / (double)values[1]);
    printf("  L1_DCM: %lld\n", values[0]);
    printf("  LST_INS: %lld\n", values[1]);
    break;

  case '2':
    // L2 miss mode
    printf("L2 miss: %f\n", (double)values[0] / (double)values[1]);
    printf("  L2_DCM: %lld\n", values[0]);
    printf("  L2_DCA: %lld\n", values[1]);
    break;

  case 't':
    // TLB miss mode
    printf("TLB miss: %f\n", (double)values[0] / (double)values[1]);
    printf("  TLB_DM: %lld\n", values[0]);
    printf("  LST_INS: %lld\n", values[1]);
    break;

  case 'b':
    // Branch miss mode
    printf("Branch miss: %f\n",
           (double)values[0] / ((double)values[1] + (double)values[0]));
    printf("  BR_MSP: %lld\n", values[0]);
    printf("  BR_PRC: %lld\n", values[1]);
    break;
  }

  printf("CSPM: [INFO] CSPM PMU unloaded!\n");
  printf("====================\n");
}
