#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct io_config {
  int output_fd;
  double size_fractor;
  double time_fractor;
};

struct io_config config;

ssize_t (*system_read)(int fd, void *buf, size_t count) = NULL;
ssize_t (*system_write)(int fd, const void *buf, size_t count) = NULL;

size_t read_count = 0;
size_t write_count = 0;

size_t read_size = 0;
size_t write_size = 0;

double read_time = 0;
double write_time = 0;

void __attribute__((constructor)) load_cspm_io() {
  printf("====================\n");
  printf("CSPM: [INFO] Loading CSPM IO\n");

  // Loading config from env "CSPM_IO" via getopt
  // -o <output file>
  // -s <size unit> (B, KB, MB, GB)
  // -t <time unit> (s, ms, us, ns)
  char *env = getenv("CSPM_IO");
  env = env ? env : "";
  printf("CSPM: [INFO] env CSPM_IO: %s\n", env);
  int argc = 1;
  char *argv[10];
  argv[0] = "cspm_io"; // pseudo argv[0]

  char *token = strtok(env, " ");
  while (token) {
    argv[argc++] = token;
    token = strtok(NULL, " ");
  }

  char *output_file_name = "io.log";
  char size_unit = 'B';
  char time_unit = 's';

  optind = 0;
  int opt;
  while ((opt = getopt(argc, argv, "s:t:o:")) != -1) {
    switch (opt) {
    case 'o':
      if (!optarg) {
        printf("CSPM: [ERROR] Usage: -o <output file>\n");
        exit(1);
      }
      output_file_name = optarg;
      break;
    case 's':
      size_unit = optarg[0];
      break;
    case 't':
      time_unit = optarg[0];
      break;
    default:
      printf("CSPM: [ERROR] Invalid option: %c\n", opt);
      exit(1);
      break;
    }
  }
  optind = 0;

  config.output_fd = open(output_file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (config.output_fd == -1) {
    printf("CSPM: [ERROR] Cannot open output file: %s\n", output_file_name);
    exit(1);
  }

  switch (size_unit) {
  case 'B':
    config.size_fractor = 1;
    break;
  case 'K':
    config.size_fractor = 1e-3;
    break;
  case 'M':
    config.size_fractor = 1e-6;
    break;
  case 'G':
    config.size_fractor = 1e-9;
    break;

  default:
    printf("CSPM: [ERROR] Invalid size unit: %c. Using default (Byte).\n",
           size_unit);
    config.size_fractor = 1;
    break;
  }

  switch (time_unit) {
  case 's':
    config.time_fractor = 1;
    break;
  case 'm':
    config.time_fractor = 1e3;
    break;
  case 'u':
    config.time_fractor = 1e6;
    break;
  case 'n':
    config.time_fractor = 1e9;
    break;

  default:
    printf("CSPM: [ERROR] Invalid time unit: %c. Using default (Second).\n",
           time_unit);
    config.time_fractor = 1;
    break;
  }

  system_read = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
  system_write =
      (ssize_t(*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");

  // Print config
  printf("CSPM: [INFO] Config:\n");
  printf("CSPM: [INFO]   Output file: %s\n", output_file_name);
  printf("CSPM: [INFO]   Size unit: %c\n", size_unit);
  printf("CSPM: [INFO]   Time unit: %c\n", time_unit);
  printf("====================\n\n");

  // Dump basic info
  dprintf(config.output_fd, "=========== CSPM IO ===========\n\n");
  dprintf(config.output_fd, "# CONFIG\n\n");
  dprintf(config.output_fd, "Output file: %s\n", output_file_name);
  dprintf(config.output_fd, "Size unit: %c\n", size_unit);
  dprintf(config.output_fd, "Time unit: %c\n\n", time_unit);
  dprintf(config.output_fd, "# TRACE\n\n");
}

void __attribute__((destructor)) unload_cspm_io() {
  printf("====================\n");
  printf("CSPM: [INFO] Unloading CSPM IO\n");

  // Calculate average size, time
  double avg_read_size = (double)read_size / (double)read_count;
  double avg_write_size = (double)write_size / (double)write_count;

  double avg_read_time = read_time / (double)read_count;
  double avg_write_time = write_time / (double)write_count;

  dprintf(config.output_fd, "\n# SUMMARY\n\n");
  dprintf(config.output_fd, "Count (R/W): %lu / %lu\n", read_count,
          write_count);

  dprintf(config.output_fd, "Size (RA/RT/WA/WT): %f / %f / %f / %f\n",
          avg_read_size * config.size_fractor, read_size * config.size_fractor,
          avg_write_size * config.size_fractor,
          write_size * config.size_fractor);

  dprintf(config.output_fd, "Time (RA/RT/WA/WT): %f / %f / %f / %f\n",
          avg_read_time * config.time_fractor, read_time * config.time_fractor,
          avg_write_time * config.time_fractor,
          write_time * config.time_fractor);

  close(config.output_fd);
  printf("====================\n\n");
}

ssize_t read(int fd, void *buf, size_t count) {
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  ssize_t ret = system_read(fd, buf, count);
  clock_gettime(CLOCK_MONOTONIC, &end);

  double start_time = (double)start.tv_sec + (double)start.tv_nsec / 1e9;
  double end_time = (double)end.tv_sec + (double)end.tv_nsec / 1e9;

  read_count += 1;
  read_size += count;

  read_time += end_time - start_time;

  dprintf(config.output_fd, "- read (%d, %p, %lu) = %zd [%.4f](%f-%f)\n", fd,
          buf, count, ret, (end_time - start_time) * config.time_fractor,
          start_time, end_time);

  return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  ssize_t ret = system_write(fd, buf, count);
  clock_gettime(CLOCK_MONOTONIC, &end);

  double start_time = (double)start.tv_sec + (double)start.tv_nsec / 1e9;
  double end_time = (double)end.tv_sec + (double)end.tv_nsec / 1e9;

  write_count += 1;
  write_size += count;

  write_time += end_time - start_time;

  dprintf(config.output_fd, "- write (%d, %p, %lu) = %zd [%.4f](%f-%f)\n", fd,
          buf, count, ret, (end_time - start_time) * config.time_fractor,
          start_time, end_time);

  return ret;
}
