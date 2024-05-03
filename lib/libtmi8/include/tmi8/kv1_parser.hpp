// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#ifndef OEUF_LIBTMI8_KV1_PARSER_HPP
#define OEUF_LIBTMI8_KV1_PARSER_HPP

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <tmi8/kv1_lexer.hpp>
#include <tmi8/kv1_types.hpp>

struct Kv1Parser {
  explicit Kv1Parser(std::vector<Kv1Token> tokens, Kv1Records &parse_into);
  
  void parse();

 private:
  // Method pointer to a method of Kv1Parser (i.e. a function that takes
  // 'this'; is not static) that takes no arguments and also does not return
  // anything.
  using ParseFunc = void (Kv1Parser::*)();
  static const std::unordered_map<std::string_view, ParseFunc> type_parsers;

  bool atEnd() const;
  void eatRowEnds();
  const Kv1Token *cur() const;
  const std::string *eatCell(std::string_view parsing_what);
  std::string parseHeader();
  void eatRestOfRow();

  void requireString(std::string_view field, bool mandatory, size_t max_length, std::string_view value);
  std::optional<bool> requireBoolean(std::string_view field, bool mandatory, std::string_view value);
  std::optional<double> requireNumber(std::string_view field, bool mandatory, size_t max_digits, std::string_view value);
  std::optional<RgbColor> requireRgbColor(std::string_view field, bool mandatory, std::string_view value);
  std::optional<double> requireRdCoord(std::string_view field, bool mandatory, size_t min_digits, std::string_view value);

  std::string eatString(std::string_view field, bool mandatory, size_t max_length);
  std::optional<bool> eatBoolean(std::string_view field, bool mandatory);
  std::optional<double> eatNumber(std::string_view field, bool mandatory, size_t max_digits);
  std::optional<RgbColor> eatRgbColor(std::string_view field, bool mandatory);
  std::optional<double> eatRdCoord(std::string_view field, bool mandatory, size_t min_digits);

  void parseOrganizationalUnit();
  void parseHigherOrganizationalUnit();
  void parseUserStopPoint();
  void parseUserStopArea();
  void parseTimingLink();
  void parseLink();
  void parseLine();
  void parseDestination();
  void parseJourneyPattern();
  void parseConcessionFinancerRelation();
  void parseConcessionArea();
  void parseFinancer();
  void parseJourneyPatternTimingLink();
  void parsePoint();
  void parsePointOnLink();
  void parseIcon();
  void parseNotice();
  void parseNoticeAssignment();
  void parseTimeDemandGroup();
  void parseTimeDemandGroupRunTime();
  void parsePeriodGroup();
  void parseSpecificDay();
  void parseTimetableVersion();
  void parsePublicJourney();
  void parsePeriodGroupValidity();
  void parseExceptionalOperatingDay();
  void parseScheduleVersion();
  void parsePublicJourneyPassingTimes();
  void parseOperatingDay();

  size_t pos = 0;
  std::vector<Kv1Token> tokens;
  const std::chrono::time_zone *amsterdam = std::chrono::locate_zone("Europe/Amsterdam");

 public:
  std::vector<std::string> warns;
  std::vector<std::string> global_errors;
  std::vector<std::string> record_errors;
  Kv1Records &records;
};

#endif // OEUF_LIBTMI8_KV1_PARSER_HPP
