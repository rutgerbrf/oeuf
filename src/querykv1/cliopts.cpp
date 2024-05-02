// vim:set sw=2 ts=2 sts et:

#include <cstdlib>
#include <cstdio>
#include <string>
#include <string_view>

#include <getopt.h>

#include "cliopts.hpp"

using namespace std::string_view_literals;

const char *opt_set = "";
const char *opt_unset = nullptr;

const char help[] = R"(Usage: %1$s [OPTIONS] <COMMAND>

Global Options:
      --kv1 <PATH>  Path to file containing all KV1 data, '-' for stdin
  -h, --help        Print this help

Commands:
  joparoute     Generate CSV for journey pattern route
  journeyinfo   Print some information on a journey
  journeyroute  Generate CSV for journey route
  journeys      List journeys of a specific line going from stop A to B
  schedule      Generate schedule
)";

const char joparoute_help[] = R"(Usage: %1$s joparoute --line <NUMBER> --jopa <CODE> [OPTIONS]

Options:
      --line <NUMBER>  Line planning number as in schedule
      --jopa <CODE>    Journey pattern code as in KV1 data
  -o <PATH>            Path of file to write to, '-' for stdout

Global Options:
      --kv1 <PATH>  Path to file containing all KV1 data, '-' for stdin
  -h, --help        Print this help
)";

const char journeyroute_help[] = R"(Usage: %1$s journeyroute --line <NUMBER> [OPTIONS]

Options:
      --line <NUMBER>     Line planning number as in KV1 data
      --journey <NUMBER>  Journey number as in KV1 data
  -o <PATH>               Path of file to write to, '-' for stdout

Global Options:
      --kv1 <PATH>  Path to file containing all KV1 data, '-' for stdin
  -h, --help        Print this help
)";

const char journeys_help[] = R"(Usage: %1$s journeys --line <NUMBER> --begin <STOP> --end <STOP> [OPTIONS]

For the --begin and --end arguments, use the following format:
  --begin/--end stop:<USRSTOP CODE>
  --begin/--end star:<USRSTAR CODE>

Options:
      --begin <STOP>   User stop code/area of stop the journey should begin at
      --end <STOP>     User stop code/area of stop the journey should end at
      --line <NUMBER>  Line planning number to filter on
  -o <PATH>            Path of file to write to, '-' for stdout

Global Options:
      --kv1 <PATH>  Path to file containing all KV1 data, '-' for stdin
  -h, --help        Print this help
)";

const char journeyinfo_help[] = R"(Usage: %1$s journeyinfo --line <NUMBER> --journey <NUMBER> [OPTIONS]

Options:
      --line <NUMBER>     Line planning number to filter on
      --journey <NUMBER>  Journey number as in schedule

Global Options:
      --kv1 <PATH>  Path to file containing all KV1 data, '-' for stdin
  -h, --help        Print this help
)";

const char schedule_help[] = R"(Usage: %1$s schedule --line <NUMBER> [OPTIONS]

Options:
      --line <NUMBER>  Line planning number to generate schedule for
  -o <PATH>            Path of file to write to, '-' for stdout

Global Options:
      --kv1 <PATH>  Path to file containing all KV1 data, '-' for stdin
  -h, --help        Print this help
)";

void journeyRouteValidateOptions(const char *progname, Options *options) {
#define X(name, argument, long_, short_) \
  if (#name != "kv1_file_path"sv  && #name != "line_planning_number"sv \
   && #name != "journey_number"sv && #name != "help"sv && #name != "output_file_path"sv) \
    if (options->name) { \
      if (long_) { \
        if (short_) fprintf(stderr, "%s: unexpected flag --%s (-%c) for journeyroute subcommand\n\n", progname, static_cast<const char *>(long_), short_); \
        else fprintf(stderr, "%s: unexpected flag --%s for journeyroute subcommand\n\n", progname, static_cast<const char *>(long_)); \
      } else if (short_) fprintf(stderr, "%s: unexpected flag -%c for journeyroute subcommand\n\n", progname, short_); \
      fprintf(stderr, journeyroute_help, progname); \
      exit(1); \
    }
  LONG_OPTIONS
  SHORT_OPTIONS
#undef X

  if (options->positional.size() > 0) {
    fprintf(stderr, "%s: unexpected positional argument(s) for journeyroute subcommand\n\n", progname);
    for (auto pos : options->positional) fprintf(stderr, "opt: %s\n", pos);
    fprintf(stderr, journeyroute_help, progname);
    exit(1);
  }

  if (!options->kv1_file_path)
    options->kv1_file_path = "-";
  if (!options->output_file_path)
    options->output_file_path = "-";
  if (options->kv1_file_path == ""sv) {
    fprintf(stderr, "%s: KV1 file path cannot be empty\n\n", progname);
    fprintf(stderr, journeyroute_help, progname);
    exit(1);
  }
  if (options->output_file_path == ""sv) {
    fprintf(stderr, "%s: output file path cannot be empty\n\n", progname);
    fprintf(stderr, journeyroute_help, progname);
    exit(1);
  }
  if (!options->journey_number || options->journey_number == ""sv) {
    fprintf(stderr, "%s: journey number must be provided\n\n", progname);
    fprintf(stderr, journeyroute_help, progname);
    exit(1);
  }
  if (!options->line_planning_number || options->line_planning_number == ""sv) {
    fprintf(stderr, "%s: line planning number must be provided\n\n", progname);
    fprintf(stderr, journeyroute_help, progname);
    exit(1);
  }
}

void scheduleValidateOptions(const char *progname, Options *options) {
#define X(name, argument, long_, short_) \
  if (#name != "kv1_file_path"sv && #name != "help"sv \
   && #name != "line_planning_number"sv && #name != "output_file_path"sv) \
    if (options->name) { \
      if (long_) { \
        if (short_) fprintf(stderr, "%s: unexpected flag --%s (-%c) for schedule subcommand\n\n", progname, static_cast<const char *>(long_), short_); \
        else fprintf(stderr, "%s: unexpected flag --%s for schedule subcommand\n\n", progname, static_cast<const char *>(long_)); \
      } else if (short_) fprintf(stderr, "%s: unexpected flag -%c for schedule subcommand\n\n", progname, short_); \
      fprintf(stderr, schedule_help, progname); \
      exit(1); \
    }
  LONG_OPTIONS
  SHORT_OPTIONS
#undef X

  if (options->positional.size() > 0) {
    fprintf(stderr, "%s: unexpected positional argument(s) for schedule subcommand\n\n", progname);
    for (auto pos : options->positional) fprintf(stderr, "opt: %s\n", pos);
    fprintf(stderr, schedule_help, progname);
    exit(1);
  }

  if (!options->kv1_file_path)
    options->kv1_file_path = "-";
  if (!options->output_file_path)
    options->output_file_path = "-";
  if (options->kv1_file_path == ""sv) {
    fprintf(stderr, "%s: KV1 file path cannot be empty\n\n", progname);
    fprintf(stderr, schedule_help, progname);
    exit(1);
  }
  if (options->output_file_path == ""sv) {
    fprintf(stderr, "%s: output file path cannot be empty\n\n", progname);
    fprintf(stderr, schedule_help, progname);
    exit(1);
  }
  if (!options->line_planning_number || options->line_planning_number == ""sv) {
    fprintf(stderr, "%s: line planning number must be provided\n\n", progname);
    fprintf(stderr, schedule_help, progname);
    exit(1);
  }
}

void journeysValidateOptions(const char *progname, Options *options) {
#define X(name, argument, long_, short_) \
  if (#name != "kv1_file_path"sv && #name != "help"sv \
   && #name != "line_planning_number"sv && #name != "output_file_path"sv \
   && #name != "begin_stop_code"sv && #name != "end_stop_code"sv) \
    if (options->name) { \
      if (long_) { \
        if (short_) fprintf(stderr, "%s: unexpected flag --%s (-%c) for journeys subcommand\n\n", progname, static_cast<const char *>(long_), short_); \
        else fprintf(stderr, "%s: unexpected flag --%s for journeys subcommand\n\n", progname, static_cast<const char *>(long_)); \
      } else if (short_) fprintf(stderr, "%s: unexpected flag -%c for journeys subcommand\n\n", progname, short_); \
      fprintf(stderr, journeys_help, progname); \
      exit(1); \
    }
  LONG_OPTIONS
  SHORT_OPTIONS
#undef X

  if (options->positional.size() > 0) {
    fprintf(stderr, "%s: unexpected positional argument(s) for journeys subcommand\n\n", progname);
    for (auto pos : options->positional) fprintf(stderr, "opt: %s\n", pos);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }

  if (!options->kv1_file_path)
    options->kv1_file_path = "-";
  if (!options->output_file_path)
    options->output_file_path = "-";
  if (options->kv1_file_path == ""sv) {
    fprintf(stderr, "%s: KV1 file path cannot be empty\n\n", progname);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }
  if (options->output_file_path == ""sv) {
    fprintf(stderr, "%s: output file path cannot be empty\n\n", progname);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }
  if (!options->line_planning_number || options->line_planning_number == ""sv) {
    fprintf(stderr, "%s: line planning number must be provided\n\n", progname);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }
  if (!options->begin_stop_code || options->begin_stop_code == ""sv) {
    fprintf(stderr, "%s: start user stop code must be provided\n\n", progname);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }
  if (!options->end_stop_code || options->end_stop_code == ""sv) {
    fprintf(stderr, "%s: end user stop code must be provided\n\n", progname);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }
  if (!std::string_view(options->begin_stop_code).starts_with("star:")
   && !std::string_view(options->begin_stop_code).starts_with("stop:")) {
    fprintf(stderr, "%s: begin user stop code must be prefixed with star:/stop:\n\n", progname);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }
  if (!std::string_view(options->end_stop_code).starts_with("star:")
   && !std::string_view(options->end_stop_code).starts_with("stop:")) {
    fprintf(stderr, "%s: end user stop code must be prefixed with star:/stop:\n\n", progname);
    fprintf(stderr, journeys_help, progname);
    exit(1);
  }
}

void journeyInfoValidateOptions(const char *progname, Options *options) {
#define X(name, argument, long_, short_) \
  if (#name != "kv1_file_path"sv  && #name != "line_planning_number"sv \
   && #name != "journey_number"sv && #name != "help"sv) \
    if (options->name) { \
      if (long_) { \
        if (short_) fprintf(stderr, "%s: unexpected flag --%s (-%c) for journeyinfo subcommand\n\n", progname, static_cast<const char *>(long_), short_); \
        else fprintf(stderr, "%s: unexpected flag --%s for journeyinfo subcommand\n\n", progname, static_cast<const char *>(long_)); \
      } else if (short_) fprintf(stderr, "%s: unexpected flag -%c for journeyinfo subcommand\n\n", progname, short_); \
      fprintf(stderr, journeyinfo_help, progname); \
      exit(1); \
    }
  LONG_OPTIONS
  SHORT_OPTIONS
#undef X

  if (options->positional.size() > 0) {
    fprintf(stderr, "%s: unexpected positional argument(s) for journeyinfo subcommand\n\n", progname);
    for (auto pos : options->positional) fprintf(stderr, "opt: %s\n", pos);
    fprintf(stderr, journeyinfo_help, progname);
    exit(1);
  }

  if (!options->kv1_file_path)
    options->kv1_file_path = "-";
  if (options->kv1_file_path == ""sv) {
    fprintf(stderr, "%s: KV1 file path cannot be empty\n\n", progname);
    fprintf(stderr, journeyinfo_help, progname);
    exit(1);
  }
  if (!options->journey_number || options->journey_number == ""sv) {
    fprintf(stderr, "%s: journey number must be provided\n\n", progname);
    fprintf(stderr, journeyinfo_help, progname);
    exit(1);
  }
  if (!options->line_planning_number || options->line_planning_number == ""sv) {
    fprintf(stderr, "%s: line planning number must be provided\n\n", progname);
    fprintf(stderr, journeyinfo_help, progname);
    exit(1);
  }
}

void jopaRouteValidateOptions(const char *progname, Options *options) {
#define X(name, argument, long_, short_) \
  if (#name != "kv1_file_path"sv  && #name != "line_planning_number"sv \
   && #name != "journey_pattern_code"sv && #name != "help"sv && #name != "output_file_path"sv) \
    if (options->name) { \
      if (long_) { \
        if (short_) fprintf(stderr, "%s: unexpected flag --%s (-%c) for joparoute subcommand\n\n", progname, static_cast<const char *>(long_), short_); \
        else fprintf(stderr, "%s: unexpected flag --%s for joparoute subcommand\n\n", progname, static_cast<const char *>(long_)); \
      } else if (short_) fprintf(stderr, "%s: unexpected flag -%c for joparoute subcommand\n\n", progname, short_); \
      fprintf(stderr, joparoute_help, progname); \
      exit(1); \
    }
  LONG_OPTIONS
  SHORT_OPTIONS
#undef X

  if (options->positional.size() > 0) {
    fprintf(stderr, "%s: unexpected positional argument(s) for joparoute subcommand\n\n", progname);
    for (auto pos : options->positional) fprintf(stderr, "opt: %s\n", pos);
    fprintf(stderr, joparoute_help, progname);
    exit(1);
  }

  if (!options->kv1_file_path)
    options->kv1_file_path = "-";
  if (!options->output_file_path)
    options->output_file_path = "-";
  if (options->kv1_file_path == ""sv) {
    fprintf(stderr, "%s: KV1 file path cannot be empty\n\n", progname);
    fprintf(stderr, joparoute_help, progname);
    exit(1);
  }
  if (options->output_file_path == ""sv) {
    fprintf(stderr, "%s: output file path cannot be empty\n\n", progname);
    fprintf(stderr, joparoute_help, progname);
    exit(1);
  }
  if (!options->journey_pattern_code || options->journey_pattern_code == ""sv) {
    fprintf(stderr, "%s: journey pattern code must be provided\n\n", progname);
    fprintf(stderr, joparoute_help, progname);
    exit(1);
  }
  if (!options->line_planning_number || options->line_planning_number == ""sv) {
    fprintf(stderr, "%s: line planning number must be provided\n\n", progname);
    fprintf(stderr, joparoute_help, progname);
    exit(1);
  }
}

struct ShortFlag {
  int has_arg;
  int c;
};

template<ShortFlag ...flags>
const std::string mkargarr =
  (std::string()
   + ...
   + (flags.c == 0
      ? ""
      : std::string((const char[]){ flags.c, '\0' })
      + (flags.has_arg == required_argument
         ? ":"
         : flags.has_arg == optional_argument
           ? "::"
           : "")));

#define X(name, has_arg, long_, short_) ShortFlag(has_arg, short_),
const std::string argarr = mkargarr<SHORT_OPTIONS LONG_OPTIONS ShortFlag(no_argument, 0)>;
#undef X

Options parseOptions(int argc, char *argv[]) {
  const char *progname = argv[0];

  // Struct with options for augmentkv6.
  Options options;

  static option long_options[] = {
#define X(name, argument, long_, short_) { long_, argument, nullptr, short_ },
    LONG_OPTIONS
#undef X
    { 0 },
  };

  int c;
  int option_index = 0;
  bool error = false;
  while ((c = getopt_long(argc, argv, argarr.c_str(), long_options, &option_index)) != -1) {
    // If a long option was used, c corresponds with val. We have val = 0 for
    // options which have no short alternative, so checking for c = 0 gives us
    // whether a long option with no short alternative was used.
    // Below, we check for c = 'h', which corresponds with the long option
    // '--help', for which val = 'h'.
    if (c == 0) {
      const char *name = long_options[option_index].name;
#define X(opt_name, opt_has_arg, opt_long, opt_short) \
      if (name == opt_long ## sv) { options.opt_name = optarg; continue; }
      LONG_OPTIONS
#undef X
      error = true;
    }
#define X(opt_name, opt_has_arg, opt_long, opt_short) \
    if (c == opt_short) { options.opt_name = optarg ? optarg : opt_set; continue; }
    LONG_OPTIONS
    SHORT_OPTIONS
#undef X
    error = true;
  }

  if (optind < argc)
    options.subcommand = argv[optind++];
  while (optind < argc)
    options.positional.push_back(argv[optind++]);

  if (options.subcommand
   && options.subcommand != "schedule"sv
   && options.subcommand != "joparoute"sv
   && options.subcommand != "journeyinfo"sv
   && options.subcommand != "journeyroute"sv
   && options.subcommand != "journeys"sv) {
    fprintf(stderr, "%s: unknown subcommand '%s'\n\n", progname, options.subcommand);
    fprintf(stderr, help, progname);
    exit(1);
  }
  if (options.subcommand && error) {
    fputc('\n', stderr);
    if (options.subcommand == "joparoute"sv) fprintf(stderr, joparoute_help, progname);
    if (options.subcommand == "journeyinfo"sv) fprintf(stderr, journeyinfo_help, progname);
    if (options.subcommand == "journeyroute"sv) fprintf(stderr, journeyroute_help, progname);
    if (options.subcommand == "journeys"sv) fprintf(stderr, journeys_help, progname);
    if (options.subcommand == "schedule"sv) fprintf(stderr, schedule_help, progname);
    exit(1);
  }
  if (error || !options.subcommand) {
    if (!options.subcommand) fprintf(stderr, "%s: no subcommand provided\n", progname);
    fputc('\n', stderr);
    fprintf(stderr, help, progname);
    exit(1);
  }
  if (options.help) {
    if (options.subcommand == "joparoute"sv) fprintf(stderr, joparoute_help, progname);
    if (options.subcommand == "journeyinfo"sv) fprintf(stderr, journeyinfo_help, progname);
    if (options.subcommand == "journeyroute"sv) fprintf(stderr, journeyroute_help, progname);
    if (options.subcommand == "journeys"sv) fprintf(stderr, journeys_help, progname);
    if (options.subcommand == "schedule"sv) fprintf(stderr, schedule_help, progname);
    exit(0);
  }

  if (options.subcommand == "joparoute"sv)
    jopaRouteValidateOptions(progname, &options);
  if (options.subcommand == "journeyinfo"sv)
    journeyInfoValidateOptions(progname, &options);
  if (options.subcommand == "journeyroute"sv)
    journeyRouteValidateOptions(progname, &options);
  if (options.subcommand == "journeys"sv)
    journeysValidateOptions(progname, &options);
  if (options.subcommand == "schedule"sv)
    scheduleValidateOptions(progname, &options);

  return options;
}
