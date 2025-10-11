#include "parser.h"
#include "match.h"
#include "strpool.h"

#include <unistd.h>

struct options {
  const char *match;
  const char *delimiter;
  bool print_raw;
  bool stream;
  bool null_sep;
  bool flush_stdout;
};

static void parse_options(int argc, char *const *argv,
                          struct options *options) {
  int opt;
  while ((opt = getopt(argc, argv, "+s0rfd:")) != -1) {
    switch (opt) {
      case 'f': {
        options->flush_stdout = true;
        break;
      }
      case 's': {
        options->stream = true;
        break;
      }
      case 'r': {
        options->print_raw = true;
        break;
      }
      case 'd': {
        options->delimiter = optarg;
        break;
      }
      case '0': {
        options->null_sep = true;
        break;
      }
      case '?': {
        exit(1);
      }
    }
  }

  if (optind < argc) {
    options->match = argv[optind];
  } else {
    fprintf(stderr, "You must specify a match\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  struct options options = {
    .match = NULL,
    .delimiter = NULL,
    .print_raw = false,
    .stream = false,
    .null_sep = false,
    .flush_stdout = false,
  };

  parse_options(argc, argv, &options);

  struct match *match = match_parse(options.match);

  struct strpool strpool;
  strpool_init(&strpool);

  struct parser parser = {
    .file = stdin,
    .strpool = &strpool,
    .print_option = PRINT_NONE,
    .delimiter = options.delimiter ? options.delimiter : "\n",
  };

  if (options.print_raw)
    parser.print_option |= PRINT_RAW;

  if (options.null_sep)
    parser.print_option |= PRINT_NULL_SEP;

  if (options.flush_stdout)
    parser.print_option |= PRINT_FLUSH_STDOUT;

  if (options.stream) {
    start_stream_matching(&parser, match);
  } else {
    start_matching(&parser, match);
  }

  strpool_destroy(&strpool);
  match_delete(match);

  return 0;
}
