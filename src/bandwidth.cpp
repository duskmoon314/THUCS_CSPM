#include <cstdio>
#include <cstring>

#include <cxxopts.hpp>

#include "timeit.hpp"
#include "utils.hpp"

// Allocate 1GB of memory
u8 u8data[1 << 29];
u8 u8data2[1 << 29];

auto main(int argc, char **argv) -> int {
  cxxopts::Options options("bandwidth",
                           "Test bandwidth between CPU and Memory/Cache");

  std::string output_file;

  // clang-format off
    options.add_options()
      ("h,help", "Print help")
      ("o,output", "Output file", cxxopts::value(output_file)->default_value("bandwidth.csv"))
    ;
  // clang-format on

  auto result = options.parse(argc, argv);

  if (result["help"].as<bool>()) {
    printf("%s\n", options.help().c_str());
    return 0;
  }

  u64 *data = (u64 *)u8data;
  u64 *temp = (u64 *)u8data2;

  // Open file
  FILE *fp = fopen(output_file.c_str(), "w");
  fprintf(fp, "size,read,write\n");

  for (u64 k = (1 << 6); k < (1 << 26);
       k += k < (1 << 16) ? (k >> 3) : (k >> 2)) {
    printf("===== %lu =====\n", k << 3);

    u64 read_us = 0;
    u64 write_us = 0;

    u64 us = timeit_rdtsc([&]() {
      for (u16 i = 0; i < (k < (1 << 16) ? 20000 : 100); ++i) {
        memcpy(temp, data, (k >> 1) * sizeof(u64));
      }
    });
    read_us = us / (k < (1 << 16) ? 20000 : 100);

    us = timeit_rdtsc([&]() {
      for (u16 i = 0; i < (k < (1 << 16) ? 20000 : 100); ++i) {
        memset(data, 1, k * sizeof(u64));
      }
    });
    write_us = us / (k < (1 << 16) ? 20000 : 100);

    printf("\n");
    printf("Read: %lu c\n", read_us);
    printf("Write: %lu c\n", write_us);

    // Write to file
    fprintf(fp, "%lu,%lu,%lu\n", k << 3, read_us, write_us);
  }

  return 0;
}