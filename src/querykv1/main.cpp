// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <chrono>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <tmi8/kv1_types.hpp>
#include <tmi8/kv1_index.hpp>
#include <tmi8/kv1_lexer.hpp>
#include <tmi8/kv1_parser.hpp>

#include "cliopts.hpp"
#include "joparoute.hpp"
#include "journeyinfo.hpp"
#include "journeyroute.hpp"
#include "journeys.hpp"
#include "schedule.hpp"

using namespace std::string_view_literals;

using TimingClock = std::conditional_t<
  std::chrono::high_resolution_clock::is_steady,
  std::chrono::high_resolution_clock,
  std::chrono::steady_clock>;

std::string readKv1(const char *path) {
  FILE *in = stdin;
  if (path != "-"sv) in = fopen(path, "rb");
  else fputs("Reading KV1 from standard input\n", stderr);
  if (!in) {
    fprintf(stderr, "Open %s: %s\n", path, strerrordesc_np(errno));
    exit(1);
  }

  char buf[4096];
  std::string data;
  while (!feof(in) && !ferror(in)) {
    size_t read = fread(buf, sizeof(char), 4096, in);
    data.append(buf, read);
  }
  if (ferror(in)) {
    if (path == "-"sv)
      fputs("Error when reading from stdin\n", stderr);
    else
      fprintf(stderr, "Error reading from file \"%s\"\n", path);
    exit(1);
  }
  fprintf(stderr, "Read %lu bytes\n", data.size());

  if (path != "-"sv)
    fclose(in);

  return data;
}

std::vector<Kv1Token> lex(const char *path) {
  std::string data = readKv1(path);

  auto start = TimingClock::now();
  Kv1Lexer lexer(data);
  lexer.lex();
  auto end = TimingClock::now();

  std::chrono::duration<double> elapsed{end - start};
  double bytes = static_cast<double>(data.size()) / 1'000'000;
  double speed = bytes / elapsed.count();

  if (!lexer.errors.empty()) {
    fputs("Lexer reported errors:\n", stderr);
    for (const auto &error : lexer.errors)
      fprintf(stderr, "- %s\n", error.c_str());
    exit(1);
  }

  fprintf(stderr, "Got %lu tokens\n", lexer.tokens.size());
  fprintf(stderr, "Duration: %f s\n", elapsed.count());
  fprintf(stderr, "Speed: %f MB/s\n", speed);

  return std::move(lexer.tokens);
}

bool parse(const char *path, Kv1Records &into) {
  std::vector<Kv1Token> tokens = lex(path);

  Kv1Parser parser(tokens, into);
  parser.parse();

  bool ok = true;
  if (!parser.gerrors.empty()) {
    ok = false;
    fputs("Parser reported errors:\n", stderr);
    for (const auto &error : parser.gerrors)
      fprintf(stderr, "- %s\n", error.c_str());
  }
  if (!parser.warns.empty()) {
    fputs("Parser reported warnings:\n", stderr);
    for (const auto &warn : parser.warns)
      fprintf(stderr, "- %s\n", warn.c_str());
  }

  fprintf(stderr, "Parsed %lu records\n", into.size());

  return ok;
}

void printParsedRecords(const Kv1Records &records) {
  fputs("Parsed records:\n", stderr);
  fprintf(stderr, "  organizational_units: %lu\n", records.organizational_units.size());
  fprintf(stderr, "  higher_organizational_units: %lu\n", records.higher_organizational_units.size());
  fprintf(stderr, "  user_stop_points: %lu\n", records.user_stop_points.size());
  fprintf(stderr, "  user_stop_areas: %lu\n", records.user_stop_areas.size());
  fprintf(stderr, "  timing_links: %lu\n", records.timing_links.size());
  fprintf(stderr, "  links: %lu\n", records.links.size());
  fprintf(stderr, "  lines: %lu\n", records.lines.size());
  fprintf(stderr, "  destinations: %lu\n", records.destinations.size());
  fprintf(stderr, "  journey_patterns: %lu\n", records.journey_patterns.size());
  fprintf(stderr, "  concession_financer_relations: %lu\n", records.concession_financer_relations.size());
  fprintf(stderr, "  concession_areas: %lu\n", records.concession_areas.size());
  fprintf(stderr, "  financers: %lu\n", records.financers.size());
  fprintf(stderr, "  journey_pattern_timing_links: %lu\n", records.journey_pattern_timing_links.size());
  fprintf(stderr, "  points: %lu\n", records.points.size());
  fprintf(stderr, "  point_on_links: %lu\n", records.point_on_links.size());
  fprintf(stderr, "  icons: %lu\n", records.icons.size());
  fprintf(stderr, "  notices: %lu\n", records.notices.size());
  fprintf(stderr, "  notice_assignments: %lu\n", records.notice_assignments.size());
  fprintf(stderr, "  time_demand_groups: %lu\n", records.time_demand_groups.size());
  fprintf(stderr, "  time_demand_group_run_times: %lu\n", records.time_demand_group_run_times.size());
  fprintf(stderr, "  period_groups: %lu\n", records.period_groups.size());
  fprintf(stderr, "  specific_days: %lu\n", records.specific_days.size());
  fprintf(stderr, "  timetable_versions: %lu\n", records.timetable_versions.size());
  fprintf(stderr, "  public_journeys: %lu\n", records.public_journeys.size());
  fprintf(stderr, "  period_group_validities: %lu\n", records.period_group_validities.size());
  fprintf(stderr, "  exceptional_operating_days: %lu\n", records.exceptional_operating_days.size());
  fprintf(stderr, "  schedule_versions: %lu\n", records.schedule_versions.size());
  fprintf(stderr, "  public_journey_passing_times: %lu\n", records.public_journey_passing_times.size());
  fprintf(stderr, "  operating_days: %lu\n", records.operating_days.size());
}

void printIndexSize(const Kv1Index &index) {
  fputs("Index size:\n", stderr);
  fprintf(stderr, "  organizational_units: %lu\n", index.organizational_units.size());
  fprintf(stderr, "  user_stop_points: %lu\n", index.user_stop_points.size());
  fprintf(stderr, "  user_stop_areas: %lu\n", index.user_stop_areas.size());
  fprintf(stderr, "  timing_links: %lu\n", index.timing_links.size());
  fprintf(stderr, "  links: %lu\n", index.links.size());
  fprintf(stderr, "  lines: %lu\n", index.lines.size());
  fprintf(stderr, "  destinations: %lu\n", index.destinations.size());
  fprintf(stderr, "  journey_patterns: %lu\n", index.journey_patterns.size());
  fprintf(stderr, "  concession_financer_relations: %lu\n", index.concession_financer_relations.size());
  fprintf(stderr, "  concession_areas: %lu\n", index.concession_areas.size());
  fprintf(stderr, "  financers: %lu\n", index.financers.size());
  fprintf(stderr, "  journey_pattern_timing_links: %lu\n", index.journey_pattern_timing_links.size());
  fprintf(stderr, "  points: %lu\n", index.points.size());
  fprintf(stderr, "  point_on_links: %lu\n", index.point_on_links.size());
  fprintf(stderr, "  icons: %lu\n", index.icons.size());
  fprintf(stderr, "  notices: %lu\n", index.notices.size());
  fprintf(stderr, "  time_demand_groups: %lu\n", index.time_demand_groups.size());
  fprintf(stderr, "  time_demand_group_run_times: %lu\n", index.time_demand_group_run_times.size());
  fprintf(stderr, "  period_groups: %lu\n", index.period_groups.size());
  fprintf(stderr, "  specific_days: %lu\n", index.specific_days.size());
  fprintf(stderr, "  timetable_versions: %lu\n", index.timetable_versions.size());
  fprintf(stderr, "  public_journeys: %lu\n", index.public_journeys.size());
  fprintf(stderr, "  period_group_validities: %lu\n", index.period_group_validities.size());
  fprintf(stderr, "  exceptional_operating_days: %lu\n", index.exceptional_operating_days.size());
  fprintf(stderr, "  schedule_versions: %lu\n", index.schedule_versions.size());
  fprintf(stderr, "  public_journey_passing_times: %lu\n", index.public_journey_passing_times.size());
  fprintf(stderr, "  operating_days: %lu\n", index.operating_days.size());
}

int main(int argc, char *argv[]) {
  Options options = parseOptions(argc, argv);

  Kv1Records records;
  if (!parse(options.kv1_file_path, records)) {
    fputs("Error parsing records, exiting\n", stderr);
    return EXIT_FAILURE;
  }
  printParsedRecords(records);
  fputs("Indexing...\n", stderr);
  Kv1Index index(&records);
  fprintf(stderr, "Indexed %lu records\n", index.size());
  // Only notice assignments are not indexed. If this equality is not valid,
  // then this means that we had duplicate keys or that something else went
  // wrong. That would really not be great.
  assert(index.size() == records.size() - records.notice_assignments.size());
  printIndexSize(index);
  fputs("Linking records...\n", stderr);
  kv1LinkRecords(index);
  fputs("Done linking\n", stderr);

  if (options.subcommand == "joparoute"sv) jopaRoute(options, records, index);
  if (options.subcommand == "journeyroute"sv) journeyRoute(options, records, index);
  if (options.subcommand == "journeys"sv) journeys(options, records, index);
  if (options.subcommand == "journeyinfo"sv) journeyInfo(options, records, index);
  if (options.subcommand == "schedule"sv) schedule(options, records, index);
}
