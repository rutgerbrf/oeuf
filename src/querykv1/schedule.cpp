// vim:set sw=2 ts=2 sts et:

#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "daterange.hpp"
#include "schedule.hpp"

using namespace std::string_view_literals;

void schedule(const Options &options, Kv1Records &records, Kv1Index &index) {
  FILE *out = stdout;
  if (options.output_file_path != "-"sv)
    out = fopen(options.output_file_path, "wb");
  if (!out) {
    fprintf(stderr, "Open %s: %s\n", options.output_file_path, strerrordesc_np(errno));
    exit(EXIT_FAILURE);
  }

  std::cerr << "Generating schedule for " << options.line_planning_number << std::endl;

  std::unordered_multimap<std::string, Kv1PeriodGroupValidity> period_group_validities;
  for (const auto &pegr : records.period_group_validities)
    period_group_validities.insert({ pegr.key.period_group_code, pegr });
  std::unordered_multimap<std::string, Kv1PublicJourney> public_journeys;
  for (const auto &pujo : records.public_journeys)
    public_journeys.insert({ pujo.key.timetable_version_code, pujo });

  std::cout << "line_planning_number,journey_number,date,departure_time" << std::endl;
  for (const auto &tive : records.timetable_versions) {
    std::vector<DateRange> tive_pegrval_ranges;

    auto pegrval_range = period_group_validities.equal_range(tive.key.period_group_code);
    for (auto it = pegrval_range.first; it != pegrval_range.second; it++) {
      const auto &[_, pegrval] = *it;
      tive_pegrval_ranges.emplace_back(pegrval.key.valid_from, pegrval.valid_thru);
    }

    DateRangeSeq seq(tive_pegrval_ranges.begin(), tive_pegrval_ranges.end());
    seq = seq.clampFrom(tive.valid_from);
    if (tive.valid_thru)
      seq = seq.clampThru(*tive.valid_thru);

    for (const auto &range : seq) for (auto date : range) {
      auto weekday = std::chrono::year_month_weekday(std::chrono::sys_days(date)).weekday();

      auto pujo_range = public_journeys.equal_range(tive.key.timetable_version_code);
      for (auto itt = pujo_range.first; itt != pujo_range.second; itt++) {
        const auto &[_, pujo] = *itt;

        if (pujo.key.line_planning_number == options.line_planning_number && pujo.key.day_type.size() == 7
         && pujo.key.day_type[weekday.iso_encoding() - 1] == static_cast<char>('0' + weekday.iso_encoding())) {
          std::cout << pujo.key.line_planning_number << "," << pujo.key.journey_number << ","
                    << date << "," << pujo.departure_time << std::endl;
        }
      }
    }
  }

  if (options.output_file_path != "-"sv) fclose(out);
}
