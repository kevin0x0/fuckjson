#include "parser.h"
#include "match.h"
#include "strpool.h"
#include "utils.h"
#include <string.h>

struct options {
  const char *match;
  bool print_raw;
  bool stream;
  bool joinline;
};

static void parse_long_options(const char *option, struct options *options) {
  if (strcmp(option, "--stream") == 0) {
    options->stream = true;
  } else {
    fprintf(stderr, "Unknown option: %s\n", option);
    exit(1);
  }
}

static void parse_short_options(int argc, const char **argv, int current,
                                struct options *options) {
  (void)argc;

  const char *arg = argv[current] + 1;
  for (;*arg; ++arg) {
    switch (*arg) {
      case 's': {
        options->stream = true;
        break;
      }
      case 'r': {
        options->print_raw = true;
        break;
      }
      case 'j': {
        options->joinline = true;
        break;
      }
      default: {
        fprintf(stderr, "Unknown option: -%c\n", *arg);
        exit(1);
      }
    }
  }
}

static void parse_options(int argc, const char **argv,
                          struct options *options) {
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (arg[0] != '-') {
      options->match = arg;
    } else if (arg[1] == '-') {
      parse_long_options(arg, options);
    } else {
      parse_short_options(argc, argv, i, options);
    }
  }

  if (unlikely(!options->match)) {
    fprintf(stderr, "You must specify match command\n");
    exit(1);
  }
}

int main(int argc, const char **argv) {
  struct options options = {
    .match = NULL,
    .print_raw = false,
    .stream = false,
    .joinline = false,
  };

  parse_options(argc, argv, &options);

  struct match *match = match_parse(options.match);

  struct strpool strpool;
  strpool_init(&strpool);

  struct parser parser = {
    .file = stdin,
    .strpool = &strpool,
    .print_option = PRINT_NONE,
  };

  if (options.print_raw)
    parser.print_option |= PRINT_RAW;

  if (options.joinline)
    parser.print_option |= PRINT_JOINLINE;

  if (options.stream) {
    start_stream_matching(&parser, match);
  } else {
    start_matching(&parser, match);
  }

  strpool_destroy(&strpool);
  match_delete(match);

  return 0;
}
