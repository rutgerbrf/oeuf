// vim:set sw=2 ts=2 sts et:

#ifndef OEUF_QUERYKV1_CLIOPTS_HPP
#define OEUF_QUERYKV1_CLIOPTS_HPP

#include <vector>

#define LONG_OPTIONS \
/*  name                  req/opt/no arg     long        short */
  X(kv1_file_path,        required_argument, "kv1",      0 ) \
  X(line_planning_number, required_argument, "line",     0 ) \
  X(journey_number,       required_argument, "journey",  0 ) \
  X(journey_pattern_code, required_argument, "jopa",     0 ) \
  X(begin_stop_code,      required_argument, "begin",    0 ) \
  X(end_stop_code,        required_argument, "end",      0 ) \
  X(help,                 no_argument,       "help",    'h')

#define SHORT_OPTIONS \
  X(output_file_path, required_argument, nullptr, 'o')

struct Options {
  const char *subcommand = nullptr;
  std::vector<const char *> positional;
#define X(name, argument, long_, short_) const char *name = nullptr;
  LONG_OPTIONS
  SHORT_OPTIONS
#undef X
};

extern const char *opt_set;
extern const char *opt_unset;

Options parseOptions(int argc, char *argv[]);

#endif // OEUF_QUERYKV1_CLIOPTS_HPP
