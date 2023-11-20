#include <map>
#include <stack>
#include <string>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <cxxopts.hpp>
#include <libunwind-ptrace.h>
#include <libunwind-x86_64.h>
#include <libunwind.h>

#include <utils.hpp>

// A Node of one symbol, used to build the symbol tree
struct SymbolNode {
  std::string name;
  int count;
  std::map<std::string, SymbolNode *> children;
};

auto print_symbol_tree(SymbolNode *node, u8 depth) -> void {
  // Print the symbol tree in a DFS way
  printf("%*d:%*c%s\n", 6, node->count, depth * 2, ' ', node->name.c_str());
  for (auto &pair : node->children) {
    print_symbol_tree(pair.second, depth + 1);
  }
}

auto sampling(pid_t target, SymbolNode *tree) -> void {
  unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);
  if (!as) {
    printf("CSPM: [ERROR] unw_create_addr_space failed\n");
    exit(1);
  }

  unw_context_t uc;
  unw_getcontext(&uc);

  // Attach to the target
  int res = ptrace(PTRACE_ATTACH, target, 0, 0);
  if (res == -1) {
    perror("CSPM: [ERROR] ptrace failed ");
    exit(1);
  }

  // Wait for the target to stop
  int status;
  waitpid(target, &status, 0);
  if (!WIFSTOPPED(status)) {
    printf("CSPM: [ERROR] target is not stopped\n");
    if (WIFEXITED(status)) {
      // printf("CSPM: child exit: %d\n", WEXITSTATUS(status));
      return;
    }
    printf("CSPM: [ERROR] target is not exited either\n");
  }

  struct UPT_info *ui = (struct UPT_info *)_UPT_create(target);
  if (!ui) {
    printf("CSPM: [ERROR] _UPT_create failed\n");
    exit(1);
  }

  // Init cursor
  unw_cursor_t cursor;
  res = unw_init_remote(&cursor, as, ui);
  if (res != 0) {
    printf("CSPM: [ERROR] unw_init_remote failed ");
    switch (res) {
    case UNW_EINVAL:
      printf("(UNW_EINVAL)\n");
      break;
    case UNW_EUNSPEC:
      printf("(UNW_EUNSPEC)\n");
      break;
    case UNW_EBADREG:
      printf("(UNW_EBADREG)\n");
      break;
    default:
      printf("(UNKNOWN %d)\n", res);
      break;
    }
    // exit(1);
    return;
  }

  // Get full backtrace, save symbol to the tree
  std::stack<std::string> symbol_stack;

  do {
    unw_word_t offset;
    char name[256] = {0};

    res = unw_get_proc_name(&cursor, name, sizeof(name), &offset);
    if (res == 0) {
      // Get proc name success
      symbol_stack.push(std::string(name));
    }
    // TODO: handle error
  } while (unw_step(&cursor) > 0);

  SymbolNode *current = tree;
  current->count++;

  while (!symbol_stack.empty()) {
    std::string symbol = symbol_stack.top();
    symbol_stack.pop();

    if (current->children.count(symbol) == 0) {
      // Create new node
      SymbolNode *new_node = new SymbolNode();
      new_node->name = symbol;
      new_node->count = 0;
      current->children[symbol] = new_node;
    }
    current = current->children[symbol];
    current->count++;
  }

  _UPT_destroy(ui);

  ptrace(PTRACE_DETACH, target, 0, 0);

  unw_destroy_addr_space(as);
}

auto main(int argc, char **argv) -> int {
  cxxopts::Options options("profile", "Profiling a command");

  u32 interval;
  std::vector<std::string> cmds;

  // clang-format off
  options.add_options()
    ("h,help", "Print help")
    ("i,interval", "Interval to update profile (us) (default: 1000)",
    cxxopts::value(interval)->default_value("1000"))
    ("cmds", "command to run",
    cxxopts::value<std::vector<std::string>>(cmds))
  ;
  // clang-format on

  options.parse_positional({"cmds"});

  auto result = options.parse(argc, argv);

  if (result["help"].as<bool>() || !result.count("cmds")) {
    printf("%s\n", options.help().c_str());
    return 0;
  }

  // std::map<std::string, int> symbol_count;
  SymbolNode *symbol_tree = new SymbolNode();
  symbol_tree->name = "CSPM Symbol Tree";
  symbol_tree->count = 0;

  struct timeval start, end;

  pid_t target;
  target = fork();

  if (!target) {
    // child process
    std::vector<char *> args;
    for (auto &arg : cmds) {
      args.push_back(&arg[0]);
    }
    args.push_back(nullptr);
    execvp(args[0], args.data());

    return 0;
  } else {
    // parent process

    gettimeofday(&start, NULL);

    // sleep for a while to wait for child to start
    usleep(100000);

    while (true) {
      // Test if child exited
      int status;
      int wait_res = waitpid(target, &status, WNOHANG);
      if (wait_res == -1) {
        perror("CSPM: [ERROR] waitpid failed ");
        exit(1);
      }
      if (WIFEXITED(status)) {
        // child process exited

        gettimeofday(&end, NULL);
        printf("===== CSPM Profile =====\n");
        printf("CSPM: child exit: %d\n", WEXITSTATUS(status));
        printf("CSPM: time elapsed: %ld us\n",
               (end.tv_sec - start.tv_sec) * 1000000 +
                   (end.tv_usec - start.tv_usec));
        if (symbol_tree->count == 0) {
          printf("CSPM: No symbol is profiled\n");
        } else {
          print_symbol_tree(symbol_tree, 0);
        }
        exit(0);
      }

      sampling(target, symbol_tree);

      usleep(interval);
    }
  }
}