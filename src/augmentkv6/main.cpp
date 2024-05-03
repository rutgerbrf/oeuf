// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <chrono>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <arrow/acero/exec_plan.h>
#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/dataset/api.h>
#include <arrow/filesystem/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include <tmi8/kv1_index.hpp>
#include <tmi8/kv1_lexer.hpp>
#include <tmi8/kv1_parser.hpp>
#include <tmi8/kv1_types.hpp>
#include <tmi8/kv6_parquet.hpp>

using namespace std::string_view_literals;

namespace ac = arrow::acero;
namespace ds = arrow::dataset;
namespace cp = arrow::compute;
using namespace arrow;

using TimingClock = std::conditional_t<
  std::chrono::high_resolution_clock::is_steady,
  std::chrono::high_resolution_clock,
  std::chrono::steady_clock>;

std::string readKv1() {
  fputs("Reading KV1 from standard input\n", stderr);

  char buf[4096];
  std::string data;
  while (!feof(stdin) && !ferror(stdin)) {
    size_t read = fread(buf, sizeof(char), 4096, stdin);
    data.append(buf, read);
  }
  if (ferror(stdin)) {
    fputs("Error when reading from stdin\n", stderr);
    exit(1);
  }
  fprintf(stderr, "Read %lu bytes\n", data.size());

  return data;
}

std::vector<Kv1Token> lex() {
  std::string data = readKv1();

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

bool parse(Kv1Records &into) {
  std::vector<Kv1Token> tokens = lex();

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

struct BasicJourneyKey {
  std::string data_owner_code;
  std::string line_planning_number;
  int         journey_number;

  auto operator<=>(const BasicJourneyKey &) const = default;
};

size_t hash_value(const BasicJourneyKey &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.line_planning_number);
  boost::hash_combine(seed, k.journey_number);

  return seed;
}

using BasicJourneyKeySet = std::unordered_set<BasicJourneyKey, boost::hash<BasicJourneyKey>>;

arrow::Result<BasicJourneyKeySet> basicJourneys(std::shared_ptr<arrow::Table> table) {
  ac::TableSourceNodeOptions table_source_node_options(table);
  ac::Declaration table_source("table_source", std::move(table_source_node_options));
  auto aggregate_options = ac::AggregateNodeOptions{
    /* .aggregates = */ {},
    /* .keys = */ { "data_owner_code", "line_planning_number", "journey_number" },
  };
  ac::Declaration aggregate("aggregate", { std::move(table_source) }, std::move(aggregate_options));
  
  std::shared_ptr<arrow::Table> result;
  ARROW_ASSIGN_OR_RAISE(result, ac::DeclarationToTable(std::move(aggregate)));

  std::shared_ptr<arrow::ChunkedArray> data_owner_codes = result->GetColumnByName("data_owner_code");
  std::shared_ptr<arrow::ChunkedArray> line_planning_numbers = result->GetColumnByName("line_planning_number");
  std::shared_ptr<arrow::ChunkedArray> journey_numbers = result->GetColumnByName("journey_number");

  int i_data_owner_codes_chunk = 0;
  int i_journey_numbers_chunk = 0;
  int i_line_planning_numbers_chunk = 0;
  int i_in_data_owner_codes_chunk = 0;
  int i_in_journey_numbers_chunk = 0;
  int i_in_line_planning_numbers_chunk = 0;

  BasicJourneyKeySet journeys;

  for (int64_t i = 0; i < result->num_rows(); i++) {
    auto data_owner_codes_chunk = std::static_pointer_cast<arrow::StringArray>(data_owner_codes->chunk(i_data_owner_codes_chunk));
    auto line_planning_numbers_chunk = std::static_pointer_cast<arrow::StringArray>(line_planning_numbers->chunk(i_line_planning_numbers_chunk));
    auto journey_numbers_chunk = std::static_pointer_cast<arrow::UInt32Array>(journey_numbers->chunk(i_journey_numbers_chunk));

    std::string_view data_owner_code = data_owner_codes_chunk->Value(i_in_data_owner_codes_chunk);
    std::string_view line_planning_number = line_planning_numbers_chunk->Value(i_in_line_planning_numbers_chunk);
    uint32_t journey_number = journey_numbers_chunk->Value(i_in_journey_numbers_chunk);

    journeys.emplace(
      std::string(data_owner_code),
      std::string(line_planning_number),
      journey_number
    );

    i_in_data_owner_codes_chunk++;
    i_in_line_planning_numbers_chunk++;
    i_in_journey_numbers_chunk++;
    if (i_in_data_owner_codes_chunk >= data_owner_codes_chunk->length()) {
      i_data_owner_codes_chunk++;
      i_in_data_owner_codes_chunk = 0;
    }
    if (i_in_line_planning_numbers_chunk >= line_planning_numbers_chunk->length()) {
      i_line_planning_numbers_chunk++;
      i_in_line_planning_numbers_chunk = 0;
    }
    if (i_in_journey_numbers_chunk >= journey_numbers_chunk->length()) {
      i_journey_numbers_chunk++;
      i_in_journey_numbers_chunk = 0;
    }
  }

  return journeys;
}

struct DistanceKey {
  BasicJourneyKey journey;
  std::string last_passed_user_stop_code;

  auto operator<=>(const DistanceKey &) const = default;
};

size_t hash_value(const DistanceKey &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.journey);
  boost::hash_combine(seed, k.last_passed_user_stop_code);

  return seed;
}

struct DistanceTimingLink {
  const Kv1JourneyPatternTimingLink *jopatili;
  double distance_since_start_of_journey = 0; // at the start of the link
};

using DistanceMap = std::unordered_map<DistanceKey, double, boost::hash<DistanceKey>>;

// Returns a map, where
//   DataOwnerCode + LinePlanningNumber + JourneyNumber + UserStopCode ->
//     Distance of Last User Stop
DistanceMap makeDistanceMap(Kv1Records &records, Kv1Index &index, BasicJourneyKeySet &journeys) {
  std::unordered_map<
    Kv1JourneyPattern::Key,
    std::vector<DistanceTimingLink>, 
    boost::hash<Kv1JourneyPattern::Key>> jopatili_index;
  std::unordered_map<
    BasicJourneyKey,
    const Kv1PublicJourney *,
    boost::hash<BasicJourneyKey>> journey_index;
  for (size_t i = 0; i < records.public_journeys.size(); i++) {
    const Kv1PublicJourney *pujo = &records.public_journeys[i];
    
    BasicJourneyKey journey_key(
      pujo->key.data_owner_code,
      pujo->key.line_planning_number,
      pujo->key.journey_number);

    if (journeys.contains(journey_key)) {
      journey_index[journey_key] = pujo;

      Kv1JourneyPattern::Key jopa_key(
        pujo->key.data_owner_code,
        pujo->key.line_planning_number,
        pujo->journey_pattern_code);
      jopatili_index[jopa_key] = {};
    }
  }

  for (size_t i = 0; i < records.journey_pattern_timing_links.size(); i++) {
    const Kv1JourneyPatternTimingLink *jopatili = &records.journey_pattern_timing_links[i];
    Kv1JourneyPattern::Key jopa_key(
      jopatili->key.data_owner_code,
      jopatili->key.line_planning_number,
      jopatili->key.journey_pattern_code);
    if (jopatili_index.contains(jopa_key)) {
      jopatili_index[jopa_key].push_back(DistanceTimingLink(jopatili, 0));
    }
  }

  for (auto &[jopa_key, timing_links] : jopatili_index) {
    std::sort(timing_links.begin(), timing_links.end(), [](auto a, auto b) {
      return a.jopatili->key.timing_link_order < b.jopatili->key.timing_link_order;
    });

    const std::string transport_type = index.journey_patterns[jopa_key]->p_line->transport_type;

    for (size_t i = 1; i < timing_links.size(); i++) {
      DistanceTimingLink *timing_link      = &timing_links[i];
      DistanceTimingLink *prev_timing_link = &timing_links[i - 1];

      const Kv1Link::Key link_key(
        prev_timing_link->jopatili->key.data_owner_code,
        prev_timing_link->jopatili->user_stop_code_begin,
        prev_timing_link->jopatili->user_stop_code_end,
        transport_type);
      double link_distance = index.links[link_key]->distance;
      timing_link->distance_since_start_of_journey =
        prev_timing_link->distance_since_start_of_journey + link_distance;
    }
  }

  // DataOwnerCode + LinePlanningNumber + JourneyNumber + UserStopCode ->
  //   Distance of Last User Stop
  DistanceMap distance_map;

  for (const auto &journey : journeys) {
    const Kv1PublicJourney *pujo = journey_index[journey];
    if (pujo == nullptr) {
      std::cerr << "Warning: No PUJO found for [" << journey.data_owner_code << "] "
                << journey.line_planning_number << "/" << journey.journey_number << std::endl;
      continue;
    }
    Kv1JourneyPattern::Key jopa_key(
      pujo->key.data_owner_code,
      pujo->key.line_planning_number,
      pujo->journey_pattern_code);
    for (const auto &timing_link : jopatili_index[jopa_key]) {
      DistanceKey key(journey, timing_link.jopatili->user_stop_code_begin);
      distance_map[key] = timing_link.distance_since_start_of_journey;
    }
  }

  return distance_map;
}

arrow::Result<std::shared_ptr<arrow::Table>> augment(
  std::shared_ptr<arrow::Table> table,
  const DistanceMap &distance_map
) {
  for (int i = 0; i < table->num_columns(); i++) {
    if (table->column(i)->num_chunks() > 1) {
      std::stringstream ss;
      ss << "Error: Expected column " << i
         << " (" << table->ColumnNames()[i] << ") to have 1 chunk, got "
         << table->column(i)->num_chunks();
      return arrow::Status::Invalid(ss.str());
    }
  }

  auto data_owner_codes = std::static_pointer_cast<arrow::StringArray>(table->GetColumnByName("data_owner_code")->chunk(0));
  auto line_planning_numbers = std::static_pointer_cast<arrow::StringArray>(table->GetColumnByName("line_planning_number")->chunk(0));
  auto journey_numbers = std::static_pointer_cast<arrow::UInt32Array>(table->GetColumnByName("journey_number")->chunk(0));
  auto user_stop_codes = std::static_pointer_cast<arrow::StringArray>(table->GetColumnByName("user_stop_code")->chunk(0));
  auto distance_since_last_user_stops = std::static_pointer_cast<arrow::UInt32Array>(table->GetColumnByName("distance_since_last_user_stop")->chunk(0));
  auto timestamps = std::static_pointer_cast<arrow::TimestampArray>(table->GetColumnByName("timestamp")->chunk(0));

  auto timestamps_type = table->schema()->GetFieldByName("timestamp")->type();
  if (timestamps_type->id() != arrow::Type::TIMESTAMP)
    return arrow::Status::Invalid("Field 'timestamp' does not have expected type TIMESTAMP");
  if (std::static_pointer_cast<arrow::TimestampType>(timestamps_type)->unit() != arrow::TimeUnit::MILLI)
    return arrow::Status::Invalid("Field 'timestamp' does not have unit MILLI");
  if (!std::static_pointer_cast<arrow::TimestampType>(timestamps_type)->timezone().empty())
    return arrow::Status::Invalid("Field 'timestamp' should have empty time zone name");

  std::shared_ptr<arrow::Field> field_distance_since_start_of_journey =
    arrow::field("distance_since_start_of_journey", arrow::uint32());
  std::shared_ptr<arrow::Field> field_day_of_week =
    arrow::field("timestamp_iso_day_of_week", arrow::int64());
  std::shared_ptr<arrow::Field> field_date =
    arrow::field("timestamp_date", arrow::date32());
  std::shared_ptr<arrow::Field> field_local_time =
    arrow::field("timestamp_local_time", arrow::time32(arrow::TimeUnit::SECOND));
  arrow::UInt32Builder distance_since_start_of_journey_builder;
  arrow::Int64Builder  day_of_week_builder;
  arrow::Date32Builder date_builder;
  arrow::Time32Builder local_time_builder(arrow::time32(arrow::TimeUnit::SECOND), arrow::default_memory_pool());

  const std::chrono::time_zone *amsterdam = std::chrono::locate_zone("Europe/Amsterdam");

  for (int64_t i = 0; i < table->num_rows(); i++) {
    DistanceKey key(
      BasicJourneyKey(
        std::string(data_owner_codes->Value(i)),
        std::string(line_planning_numbers->Value(i)),
        journey_numbers->Value(i)),
      std::string(user_stop_codes->Value(i)));

    uint32_t distance_since_last_user_stop = distance_since_last_user_stops->Value(i);
    if (distance_map.contains(key)) {
      uint32_t total_distance = distance_since_last_user_stop + static_cast<uint32_t>(distance_map.at(key));
      ARROW_RETURN_NOT_OK(distance_since_start_of_journey_builder.Append(total_distance));
    } else {
      ARROW_RETURN_NOT_OK(distance_since_start_of_journey_builder.AppendNull());
    }

    // Welp, this has gotten a bit complicated!
    std::chrono::sys_seconds timestamp(std::chrono::floor<std::chrono::seconds>(std::chrono::milliseconds(timestamps->Value(i))));
    std::chrono::zoned_seconds zoned_timestamp(amsterdam, timestamp);
    std::chrono::local_seconds local_timestamp(zoned_timestamp);
    std::chrono::local_days local_date = std::chrono::floor<std::chrono::days>(local_timestamp);
    std::chrono::year_month_day date(local_date);
    std::chrono::weekday day_of_week(local_date);
    std::chrono::hh_mm_ss<std::chrono::seconds> time(local_timestamp - local_date);
    std::chrono::sys_days unix_date(date);

    int64_t iso_day_of_week = day_of_week.iso_encoding();
    int32_t unix_days = static_cast<int32_t>(unix_date.time_since_epoch().count());
    int32_t secs_since_midnight = static_cast<int32_t>(std::chrono::seconds(time).count());

    ARROW_RETURN_NOT_OK(day_of_week_builder.Append(iso_day_of_week));
    ARROW_RETURN_NOT_OK(date_builder.Append(unix_days));
    ARROW_RETURN_NOT_OK(local_time_builder.Append(secs_since_midnight));
  }

  ARROW_ASSIGN_OR_RAISE(auto distance_since_start_of_journey_col_chunk, distance_since_start_of_journey_builder.Finish());
  ARROW_ASSIGN_OR_RAISE(auto day_of_week_col_chunk, day_of_week_builder.Finish());
  ARROW_ASSIGN_OR_RAISE(auto date_col_chunk, date_builder.Finish());
  ARROW_ASSIGN_OR_RAISE(auto local_time_col_chunk, local_time_builder.Finish());
  auto distance_since_start_of_journey_col =
    std::make_shared<arrow::ChunkedArray>(distance_since_start_of_journey_col_chunk);
  auto day_of_week_col = std::make_shared<arrow::ChunkedArray>(day_of_week_col_chunk);
  auto date_col        = std::make_shared<arrow::ChunkedArray>(date_col_chunk);
  auto local_time_col  = std::make_shared<arrow::ChunkedArray>(local_time_col_chunk);
  
  ARROW_ASSIGN_OR_RAISE(table, table->AddColumn(
    table->num_columns(),
    field_distance_since_start_of_journey,
    distance_since_start_of_journey_col));
  ARROW_ASSIGN_OR_RAISE(table, table->AddColumn(table->num_columns(), field_day_of_week, day_of_week_col));
  ARROW_ASSIGN_OR_RAISE(table, table->AddColumn(table->num_columns(), field_date, date_col));
  ARROW_ASSIGN_OR_RAISE(table, table->AddColumn(table->num_columns(), field_local_time, local_time_col));

  return table;
}

arrow::Status processTables(Kv1Records &records, Kv1Index &index) {
  std::shared_ptr<arrow::io::RandomAccessFile> input;
  ARROW_ASSIGN_OR_RAISE(input, arrow::io::ReadableFile::Open("oeuf-input.parquet"));

  std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
  ARROW_RETURN_NOT_OK(parquet::arrow::OpenFile(input, arrow::default_memory_pool(), &arrow_reader));

  std::shared_ptr<arrow::Table> table;
  ARROW_RETURN_NOT_OK(arrow_reader->ReadTable(&table));

  std::cerr << "Input KV6 file has " << table->num_rows() << " rows" << std::endl;
  ARROW_ASSIGN_OR_RAISE(BasicJourneyKeySet journeys, basicJourneys(table));
  std::cerr << "Found " << journeys.size() << " distinct journeys" << std::endl;
  DistanceMap distance_map = makeDistanceMap(records, index, journeys);
  std::cerr << "Distance map has " << distance_map.size() << " keys" << std::endl;

  std::cerr << "Creating augmented table" << std::endl;
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Table> augmented, augment(table, distance_map));

  std::cerr << "Writing augmented table" << std::endl;
  return writeArrowTableAsParquetFile(*augmented, "oeuf-augmented.parquet");
}

int main(int argc, char *argv[]) {
  Kv1Records records;
  if (!parse(records)) {
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

  arrow::Status st = processTables(records, index);
  if (!st.ok()) {
    std::cerr << "Failed to process tables: " << st << std::endl;
    return EXIT_FAILURE;
  }
}
