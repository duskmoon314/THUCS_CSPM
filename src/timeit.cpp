#include <cstdio>

#include <cxxopts.hpp>

#include "timeit.hpp"

auto main(int argc, char **argv) -> int {
  cxxopts::Options options("timeit", "Time a command");

  u8 mode;
  std::string cmd;

  // clang-format off
  options.add_options()
    ("h,help", "Print help")
    ("m,mode", "Mode to use (0: syscall, 1: rdtsc)", cxxopts::value<u8>(mode)->default_value("0"))
    ("cmd", "Command to run", cxxopts::value(cmd))
  ;
  // clang-format on

  options.parse_positional({"cmd"});

  auto result = options.parse(argc, argv);

  if (result["help"].as<bool>() || !result.count("cmd")) {
    printf("%s\n", options.help().c_str());
    return 0;
  }

  switch (mode) {
  case 0: {
    u64 us = timeit_syscall([&]() { system(cmd.c_str()); });
    printf("timeit_syscall: %lu us\n", us);
    return 0;
  }
  case 1: {
    u64 cycles = timeit_rdtsc([&]() { system(cmd.c_str()); });
    printf("timeit_rdtsc: %lu cycles\n", cycles);
    return 0;
  }
  }
}