// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <tmi8/kv1_parser.hpp>

using rune = uint32_t;

static size_t decodeUtf8Cp(std::string_view s, rune *dest = nullptr) {
  rune res = 0xFFFD;
  size_t length = 1;

  if (s.size() == 0)
    return 0;
  const uint8_t *b = reinterpret_cast<const uint8_t *>(s.data());
  if (!(b[0] & 0x80))
    res = static_cast<rune>(b[0]);
  else if ((b[0] & 0xE0) == 0xC0) {
    length = 2;
    if (s.size() >= 2 && (b[1] & 0xC0) == 0x80) {
      res  = static_cast<rune>(b[0] & ~0xC0) << 6;
      res |= static_cast<rune>(b[1] & ~0x80);
    }
  } else if ((b[0] & 0xF0) == 0xE0) {
    length = 3;
    if (s.size() >= 3 && (b[1] & 0xC0) == 0x80 && (b[2] & 0xC0) == 0x80) {
      res  = static_cast<rune>(b[0] & ~0xE0) << 12;
      res |= static_cast<rune>(b[1] & ~0x80) << 6;
      res |= static_cast<rune>(b[2] & ~0x80);
    }
  } else if (b[0] == 0xF0) {
    length = 4;
    if (s.size() >= 4 && (b[1] & 0xC0) == 0x80 && (b[2] & 0xC0) == 0x80 && (b[3] & 0xC0) == 0x80) {
      res  = static_cast<rune>(b[0] & ~0xF0) << 18;
      res |= static_cast<rune>(b[1] & ~0x80) << 12;
      res |= static_cast<rune>(b[2] & ~0x80) << 6;
      res |= static_cast<rune>(b[3] & ~0x80);
    }
  }

  if (dest)
    *dest = res;
  return length;
}

// Counts the number of codepoints in a valid UTF-8 string. Returns SIZE_MAX if
// the string contains invalid UTF-8 codepoints.
static size_t stringViewLengthUtf8(std::string_view sv) {
  size_t codepoints = 0;
  while (sv.size() > 0) {
    size_t codepoint_size = decodeUtf8Cp(sv);
    if (codepoint_size == 0) return SIZE_MAX;
    codepoints++;
    sv = sv.substr(codepoint_size);
  }
  return codepoints;
}

Kv1Parser::Kv1Parser(std::vector<Kv1Token> tokens, Kv1Records &parse_into)
  : tokens(std::move(tokens)),
    records(parse_into)
{}

bool Kv1Parser::atEnd() const {
  return pos >= tokens.size();
}

void Kv1Parser::eatRowEnds() {
  while (!atEnd() && tokens[pos].type == KV1_TOKEN_ROW_END) pos++;
}

const Kv1Token *Kv1Parser::cur() const {
  if (atEnd()) return nullptr;
  return &tokens[pos];
}

const std::string *Kv1Parser::eatCell(std::string_view parsing_what) {
  const Kv1Token *tok = cur();
  if (!tok) {
    record_errors.push_back(std::format("Expected cell but got end of file when parsing {}", parsing_what));
    return nullptr;
  }
  if (tok->type == KV1_TOKEN_ROW_END) {
    record_errors.push_back(std::format("Expected cell but got end of row when parsing {}", parsing_what));
    return nullptr;
  }
  pos++;
  return &tok->data;
}

void Kv1Parser::requireString(std::string_view field, bool mandatory, size_t max_length, std::string_view value) {
  if (value.empty() && mandatory) {
    record_errors.push_back(std::format("{} has length zero but is required", field));
    return;
  }
  size_t codepoints = stringViewLengthUtf8(value);
  if (codepoints == SIZE_MAX) {
    global_errors.push_back(std::format("{} contains invalid UTF-8 code points", field));
    return;
  }
  if (codepoints > max_length) {
    record_errors.push_back(std::format("{} has length ({}) that is greater than maximum length ({})",
                                  field, value.size(), max_length));
  }
}

static inline std::optional<bool> parseBoolean(std::string_view src) {
  if (src == "1") return true;
  if (src == "0") return false;
  if (src == "true") return true;
  if (src == "false") return false;
  return std::nullopt;
}

std::optional<bool> Kv1Parser::requireBoolean(std::string_view field, bool mandatory, std::string_view value) {
  if (value.empty()) {
    if (mandatory)
      record_errors.push_back(std::format("{} is required, but has no value", field));
    return std::nullopt;
  }
  auto parsed = parseBoolean(value);
  if (!parsed.has_value())
    record_errors.push_back(std::format("{} should have value \"1\", \"0\", \"true\" or \"false\"", field));
  return parsed;
}

static inline size_t countDigits(long x) {
  size_t digits = 0;
  while (x != 0) { digits++; x /= 10; }
  return digits;
}

std::optional<double> Kv1Parser::requireNumber(std::string_view field, bool mandatory, size_t max_digits, std::string_view value) {
  if (value.empty()) {
    if (mandatory)
      record_errors.push_back(std::format("{} has no value but is required", field));
    return std::nullopt;
  }

  double parsed;
  auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed, std::chars_format::fixed);
  if (ec != std::errc()) {
    record_errors.push_back(std::format("{} has a bad value that cannot be parsed as a number", field));
    return std::nullopt;
  }
  if (ptr != value.data() + value.size()) {
    record_errors.push_back(std::format("{} contains characters that were not parsed as a number", field));
    return std::nullopt;
  }
 
  size_t digits = countDigits(static_cast<long>(parsed));
  if (digits > max_digits) {
    record_errors.push_back(std::format("{} contains more digits (in the integral part) ({}) than allowed ({})",
                                  field, digits, max_digits));
    return std::nullopt;
  }

  return parsed;
}

static inline bool isHexDigit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static inline uint8_t fromHex(char c) {
  if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
  else if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
  return 0;
}

static std::optional<RgbColor> parseRgbColor(std::string_view src) {
  bool valid = src.size() == 6
    && isHexDigit(src[0]) && isHexDigit(src[1])
    && isHexDigit(src[2]) && isHexDigit(src[3])
    && isHexDigit(src[4]) && isHexDigit(src[5]);
  if (!valid) return std::nullopt;
  uint8_t r = static_cast<uint8_t>(fromHex(src[0]) << 4) + fromHex(src[1]);
  uint8_t g = static_cast<uint8_t>(fromHex(src[2]) << 4) + fromHex(src[3]);
  uint8_t b = static_cast<uint8_t>(fromHex(src[4]) << 4) + fromHex(src[5]);
  return RgbColor{ r, g, b };
}

std::optional<RgbColor> Kv1Parser::requireRgbColor(std::string_view field, bool mandatory, std::string_view value) {
  if (value.empty()) {
    if (mandatory)
      record_errors.push_back(std::format("{} is required, but has no value", field));
    return std::nullopt;
  }
  auto parsed = parseRgbColor(value);
  if (!parsed.has_value())
    record_errors.push_back(std::format("{} should be an RGB color, i.e. a sequence of six hexadecimally represented nibbles", field));
  return parsed;
}

std::optional<double> Kv1Parser::requireRdCoord(std::string_view field, bool mandatory, size_t min_digits, std::string_view value) {
  if (value.empty()) {
    if (mandatory)
      record_errors.push_back(std::format("{} is required, but has no value", field));
    return std::nullopt;
  }
  if (value.size() > 15) {
    record_errors.push_back(std::format("{} may not have more than 15 characters", field));
    return std::nullopt;
  }

  double parsed;
  auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed, std::chars_format::fixed);
  if (ec != std::errc()) {
    record_errors.push_back(std::format("{} has a bad value that cannot be parsed as a number", field));
    return std::nullopt;
  }
  if (ptr != value.data() + value.size()) {
    record_errors.push_back(std::format("{} contains characters that were not parsed as a number", field));
    return std::nullopt;
  }
 
  size_t digits = countDigits(static_cast<long>(parsed));
  if (digits < min_digits) {
    record_errors.push_back(std::format("{} contains less digits (in the integral part) ({}) than required ({}) [value: {}]",
                                  field, digits, min_digits, value));
    return std::nullopt;
  }

  return parsed;
}

std::string Kv1Parser::eatString(std::string_view field, bool mandatory, size_t max_length) {
  auto value = eatCell(field);
  if (!record_errors.empty()) return {};
  requireString(field, mandatory, max_length, *value);
  return std::move(*value);
}

std::optional<bool> Kv1Parser::eatBoolean(std::string_view field, bool mandatory) {
  auto value = eatCell(field);
  if (!record_errors.empty()) return {};
  return requireBoolean(field, mandatory, *value);
}

std::optional<double> Kv1Parser::eatNumber(std::string_view field, bool mandatory, size_t max_digits) {
  auto value = eatCell(field);
  if (!record_errors.empty()) return {};
  return requireNumber(field, mandatory, max_digits, *value);
}

std::optional<RgbColor> Kv1Parser::eatRgbColor(std::string_view field, bool mandatory) {
  auto value = eatCell(field);
  if (!record_errors.empty()) return {};
  return requireRgbColor(field, mandatory, *value);
}

std::optional<double> Kv1Parser::eatRdCoord(std::string_view field, bool mandatory, size_t min_digits) {
  auto value = eatCell(field);
  if (!record_errors.empty()) return {};
  return requireRdCoord(field, mandatory, min_digits, *value);
}

std::string Kv1Parser::parseHeader() {
  auto record_type       = eatString("<header>.Recordtype",        true, 10);
  auto version_number    = eatString("<header>.VersionNumber",     true,  2);
  auto implicit_explicit = eatString("<header>.Implicit/Explicit", true,  1);
  if (!record_errors.empty()) return {};

  if (version_number != "1") {
    record_errors.push_back("<header>.VersionNumber should be 1");
    return "";
  }
  if (implicit_explicit != "I") {
    record_errors.push_back("<header>.Implicit/Explicit should be 'I'");
    return "";
  }

  return record_type;
}

void Kv1Parser::eatRestOfRow() {
  while (!atEnd() && cur()->type != KV1_TOKEN_ROW_END) pos++;
}

void Kv1Parser::parse() {
  while (!atEnd()) {
    eatRowEnds();
    if (atEnd()) return;

    std::string record_type = parseHeader();
    if (!record_errors.empty()) break;
    if (!type_parsers.contains(record_type)) {
      warns.push_back(std::format("Recordtype ({}) is bad or names a record type that this program cannot process",
                                  record_type));
      eatRestOfRow();
      continue;
    }

    ParseFunc parseType = Kv1Parser::type_parsers.at(record_type);
    (this->*parseType)();
    if (cur() && cur()->type != KV1_TOKEN_ROW_END) {
      record_errors.push_back(std::format("Parser function for Recordtype ({}) did not eat all record fields",
                                    record_type));
      eatRestOfRow();
    }
    if (!record_errors.empty()) {
      global_errors.insert(global_errors.end(), record_errors.begin(), record_errors.end());
      record_errors.clear();
    }
  }
}

void Kv1Parser::parseOrganizationalUnit() {
  auto data_owner_code          = eatString("ORUN.DataOwnerCode",          true,   10);
  auto organizational_unit_code = eatString("ORUN.OrganizationalUnitCode", true,   10);
  auto name                     = eatString("ORUN.Name",                   true,   50);
  auto organizational_unit_type = eatString("ORUN.OrganizationalUnitType", true,   10);
  auto description              = eatString("ORUN.Description",            false, 255);
  if (!record_errors.empty()) return;

  records.organizational_units.emplace_back(
    Kv1OrganizationalUnit::Key(
      data_owner_code,
      organizational_unit_code),
    name,
    organizational_unit_type,
    description);
}

static inline bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

// Parse a string of the format YYYY-MM-DD.
static std::optional<std::chrono::year_month_day> parseYyyymmdd(std::string_view src) {
  bool valid = src.size() == 10
    && isDigit(src[0]) && isDigit(src[1])
    && isDigit(src[2]) && isDigit(src[3]) && src[4] == '-'
    && isDigit(src[5]) && isDigit(src[6]) && src[7] == '-'
    && isDigit(src[8]) && isDigit(src[9]);
  if (!valid) return std::nullopt;
  int year  = (src[0] - '0') * 1000 + (src[1] - '0') * 100 + (src[2] - '0') * 10 + src[3] - '0';
  int month = (src[5] - '0') * 10 + src[6] - '0';
  int day   = (src[8] - '0') * 10 + src[9] - '0';
  return std::chrono::year(year) / std::chrono::month(month) / std::chrono::day(day);
}

// Parse a string of the format HH:MM:SS.
static std::optional<std::chrono::hh_mm_ss<std::chrono::seconds>> parseHhmmss(std::string_view src) {
  bool valid = src.size() == 8
    && isDigit(src[0]) && isDigit(src[1]) && src[2] == ':'
    && isDigit(src[3]) && isDigit(src[4]) && src[5] == ':'
    && isDigit(src[6]) && isDigit(src[7]);
  if (!valid) return std::nullopt;
  int hh = (src[0] - '0') * 10 + src[1] - '0';
  int mm = (src[3] - '0') * 10 + src[4] - '0';
  int ss = (src[6] - '0') * 10 + src[7] - '0';
  // The check for the hour not being greater than 32 comes from the fact the
  // specification explicitly allows hours greater than 23, noting that the
  // period 24:00-32:00 is equivalent to 00:00-08:00 in the next day, for
  // exploitation of two days.
  if (hh > 32 || mm > 59 || ss > 59) return std::nullopt;
  return std::chrono::hh_mm_ss(std::chrono::hours(hh) + std::chrono::minutes(mm) + std::chrono::seconds(ss));
}

static std::optional<std::chrono::sys_seconds> parseDateTime(std::string_view src, const std::chrono::time_zone *amsterdam, std::string_view *error = nullptr) {
#define ERROR(err) do { if (error) *error = err; return std::nullopt; } while (0)
  if (src.size() > 23) ERROR("timestamp string is too big");
  if (src.size() < 17) ERROR("timestamp string is too small");

  bool valid_year = isDigit(src[0]) && isDigit(src[1]) && isDigit(src[2]) && isDigit(src[3]);
  if (!valid_year) ERROR("year has bad format");

  size_t month_off = src[4] == '-' ? 5 : 4;
  size_t day_off   = src[month_off + 2] == '-' ? month_off + 3 : month_off + 2;
  size_t time_off  = day_off + 2;
  if (src[time_off] != 'T' && src[time_off] != ' ')
    ERROR("missing date/time separator");
  size_t tzd_off = time_off + 9;
  // For clarity, TZD stands for Time Zone Designator. It often takes the form
  // of Z (Zulu, UTC+00:00) or as an offset from UTC in hours and minutes,
  // formatted as ±HH:MM (e.g. +01:00, -12:00).

  if (time_off + 8 >= src.size()) ERROR("bad format, not enough space for hh:mm:ss");

  int year   = (src[0] - '0') * 1000 + (src[1] - '0') * 100 + (src[2] - '0') * 10 + src[3] - '0';
  int month  = (src[month_off] - '0') * 10 + src[month_off + 1] - '0';
  int day    = (src[day_off] - '0') * 10 + src[day_off + 1] - '0';
  int hour   = (src[time_off + 1] - '0') * 10 + src[time_off + 2] - '0';
  int minute = (src[time_off + 4] - '0') * 10 + src[time_off + 5] - '0';
  int second = (src[time_off + 7] - '0') * 10 + src[time_off + 8] - '0';

  auto date = std::chrono::year(year)  / std::chrono::month(month)    / std::chrono::day(day);
  auto time = std::chrono::hours(hour) + std::chrono::minutes(minute) + std::chrono::seconds(second);

  std::chrono::sys_seconds unix_start_of_day;
  if (tzd_off < src.size()) {
    unix_start_of_day = std::chrono::sys_days(date);
  } else {
    auto local_days = std::chrono::local_days(date);
    std::chrono::zoned_seconds zoned_start_of_day = std::chrono::zoned_time(amsterdam, local_days);
    unix_start_of_day = std::chrono::sys_seconds(zoned_start_of_day);
  }

  std::chrono::minutes offset(0);
  if (tzd_off + 1 == src.size() && src[tzd_off] != 'Z') {
    ERROR("bad TZD (missing Zulu indicator)");
  } else if (tzd_off + 6 == src.size()) {
    bool valid_tzd = (src[tzd_off] == '+' || src[tzd_off] == '-')
      && isDigit(src[tzd_off + 1]) && isDigit(src[tzd_off + 2]) && src[tzd_off + 3] == ':'
      && isDigit(src[tzd_off + 4]) && isDigit(src[tzd_off + 5]);
    if (!valid_tzd) ERROR("bad offset TZD format (expected +|-hh:mm)");
    int sign   =  src[tzd_off] == '-' ? -1 : 1;
    int tzd_hh = (src[tzd_off + 1] - '0') * 10 + src[tzd_off + 2] - '0';
    int tzd_mm = (src[tzd_off + 3] - '0') * 10 + src[tzd_off + 4] - '0';
    offset = sign * std::chrono::minutes(tzd_hh * 60 + tzd_mm);
  } else if (tzd_off < src.size()) {
    // There is a TZD but we literally have no clue how to parse it :/
    ERROR("cannot parse TZD of unexpected length");
  }

  return unix_start_of_day + time - offset;
#undef ERROR
}

void Kv1Parser::parseHigherOrganizationalUnit() {
  auto data_owner_code                 = eatString("ORUNORUN.DataOwnerCode",                true, 10);
  auto organizational_unit_code_parent = eatString("ORUNORUN.OrganizationalUnitCodeParent", true, 10);
  auto organizational_unit_code_child  = eatString("ORUNORUN.OrganizationalUnitCodeChild",  true, 10);
  auto valid_from_raw                  = eatString("ORUNORUN.ValidFrom",                    true, 10);
  if (!record_errors.empty()) return;

  auto valid_from = parseYyyymmdd(valid_from_raw);
  if (!valid_from) {
    record_errors.push_back("ORUNORUN.ValidFrom has invalid format, should be YYYY-MM-DD");
    return;
  }

  records.higher_organizational_units.emplace_back(
    Kv1HigherOrganizationalUnit::Key(
      data_owner_code,
      organizational_unit_code_parent,
      organizational_unit_code_child,
      *valid_from));
}

void Kv1Parser::parseUserStopPoint() {
  auto data_owner_code     = eatString ("USRSTOP.DataOwnerCode",        true,   10);
  auto user_stop_code      = eatString ("USRSTOP.UserStopCode",         true,   10);
  auto timing_point_code   = eatString ("USRSTOP.TimingPointCode",      false,  10);
  auto get_in              = eatBoolean("USRSTOP.GetIn",                true      );
  auto get_out             = eatBoolean("USRSTOP.GetOut",               true      );
                             eatCell   ("USRSTOP.<deprecated field #1>"           );
  auto name                = eatString ("USRSTOP.Name",                 true,   50);
  auto town                = eatString ("USRSTOP.Town",                 true,   50);
  auto user_stop_area_code = eatString ("USRSTOP.UserStopAreaCode",     false,  10);
  auto stop_side_code      = eatString ("USRSTOP.StopSideCode",         true,   10);
                             eatCell   ("USRSTOP.<deprecated field #2>"           );
                             eatCell   ("USRSTOP.<deprecated field #3>"           );
  auto minimal_stop_time   = eatNumber ("USRSTOP.MinimalStopTime",      true,    5);
  auto stop_side_length    = eatNumber ("USRSTOP.StopSideLength",       false,   3);
  auto description         = eatString ("USRSTOP.Description",          false, 255);
  auto user_stop_type      = eatString ("USRSTOP.UserStopType",         true,   10);
  auto quay_code           = eatString ("USRSTOP.QuayCode",             false,  30);
  if (!record_errors.empty()) return;

  records.user_stop_points.emplace_back(
    Kv1UserStopPoint::Key(
      data_owner_code,
      user_stop_code),
    timing_point_code,
    *get_in,
    *get_out,
    name,
    town,
    user_stop_area_code,
    stop_side_code,
    *minimal_stop_time,
    stop_side_length,
    description,
    user_stop_type,
    quay_code);
}

void Kv1Parser::parseUserStopArea() {
  auto data_owner_code     = eatString("USRSTAR.DataOwnerCode",        true,   10);
  auto user_stop_area_code = eatString("USRSTAR.UserStopAreaCode",     true,   10);
  auto name                = eatString("USRSTAR.Name",                 true,   50);
  auto town                = eatString("USRSTAR.Town",                 true,   50);
                             eatCell  ("USRSTAR.<deprecated field #1>"           );
                             eatCell  ("USRSTAR.<deprecated field #2>"           );
  auto description         = eatString("USRSTAR.Description",          false, 255);
  if (!record_errors.empty()) return;

  records.user_stop_areas.emplace_back(
    Kv1UserStopArea::Key(
      data_owner_code,
      user_stop_area_code),
    name,
    town,
    description);
}

void Kv1Parser::parseTimingLink() {
  auto data_owner_code      = eatString("TILI.DataOwnerCode",     true,   10);
  auto user_stop_code_begin = eatString("TILI.UserStopCodeBegin", true,   10);
  auto user_stop_code_end   = eatString("TILI.UserStopCodeEnd",   true,   10);
  auto minimal_drive_time   = eatNumber("TILI.MinimalDriveTime",  false,   5);
  auto description          = eatString("TILI.Description",       false, 255);
  if (!record_errors.empty()) return;

  records.timing_links.emplace_back(
    Kv1TimingLink::Key(
      data_owner_code, 
      user_stop_code_begin,
      user_stop_code_end),
    minimal_drive_time,
    description);
}

void Kv1Parser::parseLink() {
  auto data_owner_code      = eatString("LINK.DataOwnerCode",        true,   10);
  auto user_stop_code_begin = eatString("LINK.UserStopCodeBegin",    true,   10);
  auto user_stop_code_end   = eatString("LINK.UserStopCodeEnd",      true,   10);
                                eatCell("LINK.<deprecated field #1>"           );
  auto distance             = eatNumber("LINK.Distance",             true,    6);
  auto description          = eatString("LINK.Description",          false, 255);
  auto transport_type       = eatString("LINK.TransportType",        true,    5);
  if (!record_errors.empty()) return;

  records.links.emplace_back(
    Kv1Link::Key(
      data_owner_code,
      user_stop_code_begin,
      user_stop_code_end,
      transport_type),
    *distance,
    description);
}

void Kv1Parser::parseLine() {
  auto data_owner_code      = eatString  ("LINE.DataOwnerCode",      true,   10);
  auto line_planning_number = eatString  ("LINE.LinePlanningNumber", true,   10);
  auto line_public_number   = eatString  ("LINE.LinePublicNumber",   true,    4);
  auto line_name            = eatString  ("LINE.LineName",           true,   50);
  auto line_ve_tag_number   = eatNumber  ("LINE.LineVeTagNumber",    true,    3);
  auto description          = eatString  ("LINE.Description",        false, 255);
  auto transport_type       = eatString  ("LINE.TransportType",      true,    5);
  auto line_icon            = eatNumber  ("LINE.LineIcon",           false,   4);
  auto line_color           = eatRgbColor("LINE.LineColor",          false     );
  auto line_text_color      = eatRgbColor("LINE.LineTextColor",      false     );
  if (!record_errors.empty()) return;

  // NOTE: This check, although it should be performed to comply with the
  // specification, is not actually honored by transit operators (such as
  // Connexxion) :/ That's enough reason to keep it disabled here for now.
  // if (*line_ve_tag_number < 0 || *line_ve_tag_number > 399) {
  //   record_errors.push_back(std::format("LINE.LineVeTagNumber is out of range [0-399] with value {}", *line_ve_tag_number));
  //   return;
  // }
  if (*line_ve_tag_number != static_cast<short>(*line_ve_tag_number))
    record_errors.push_back("LINE.LineVeTagNumber should be an integer");
  if (line_icon && *line_icon != static_cast<short>(*line_icon))
    record_errors.push_back("LINE.LineIcon should be an integer");
  if (!record_errors.empty()) return;

  records.lines.emplace_back(
    Kv1Line::Key(
      data_owner_code,
      line_planning_number),
    line_public_number,
    line_name,
    static_cast<short>(*line_ve_tag_number),
    description,
    transport_type,
    static_cast<std::optional<short>>(line_icon),
    line_color,
    line_text_color);
}

void Kv1Parser::parseDestination() {
  auto data_owner_code           = eatString  ("DEST.DataOwnerCode",           true, 10);
  auto dest_code                 = eatString  ("DEST.DestCode",                true, 10);
  auto dest_name_full            = eatString  ("DEST.DestNameFull",            true, 50);
  auto dest_name_main            = eatString  ("DEST.DestNameMain",            true, 24);
  auto dest_name_detail          = eatString  ("DEST.DestNameDetail",         false, 24);
  auto relevant_dest_name_detail = eatBoolean ("DEST.RelevantDestNameDetail",  true    );
  auto dest_name_main_21         = eatString  ("DEST.DestNameMain21",          true, 21);
  auto dest_name_detail_21       = eatString  ("DEST.DestNameDetail21",       false, 21);
  auto dest_name_main_19         = eatString  ("DEST.DestNameMain19",          true, 19);
  auto dest_name_detail_19       = eatString  ("DEST.DestNameDetail19",       false, 19);
  auto dest_name_main_16         = eatString  ("DEST.DestNameMain16",          true, 16);
  auto dest_name_detail_16       = eatString  ("DEST.DestNameDetail16",       false, 16);
  auto dest_icon                 = eatNumber  ("DEST.DestIcon",               false,  4);
  auto dest_color                = eatRgbColor("DEST.DestColor",              false    );
  // NOTE: Deviating from the offical KV1 specification here. It specifies that
  // the maximum length for this field should be 30, but then proceeds to
  // specify that it should contain a RGB value comprising of three
  // hexadecimally encoded octets, i.e. six characters. We assume that the
  // latter is correct and the intended interpretation.
  auto dest_text_color           = eatRgbColor("DEST.DestTextColor",          false    );
  if (!record_errors.empty()) return;

  if (dest_icon && *dest_icon != static_cast<short>(*dest_icon)) {
    record_errors.push_back("DEST.DestIcon should be an integer");
    return;
  }

  records.destinations.emplace_back(
    Kv1Destination::Key(
      data_owner_code,
      dest_code),
    dest_name_full,
    dest_name_main,
    dest_name_detail,
    *relevant_dest_name_detail,
    dest_name_main_21,
    dest_name_detail_21,
    dest_name_main_19,
    dest_name_detail_19,
    dest_name_main_16,
    dest_name_detail_16,
    dest_icon,
    dest_color,
    dest_text_color);
}

void Kv1Parser::parseJourneyPattern() {
  auto data_owner_code      = eatString("JOPA.DataOwnerCode",       true,  10);
  auto line_planning_number = eatString("JOPA.LinePlanningNumber",  true,  10);
  auto journey_pattern_code = eatString("JOPA.JourneyPatternCode",  true,  10);
  auto journey_pattern_type = eatString("JOPA.JourneyPatternType",  true,  10);
  auto direction            = eatString("JOPA.Direction",           true,   1);
  auto description          = eatString("JOPA.Description",        false, 255);
  if (!record_errors.empty()) return;

  if (direction != "1" && direction != "2" && direction != "A" && direction != "B") {
    record_errors.push_back("JOPA.Direction should be in [1, 2, A, B]");
    return;
  }

  records.journey_patterns.emplace_back(
    Kv1JourneyPattern::Key(
      data_owner_code,
      line_planning_number,
      journey_pattern_code),
    journey_pattern_type,
    direction[0],
    description);
}

void Kv1Parser::parseConcessionFinancerRelation() {
  auto data_owner_code      = eatString("CONFINREL.DataOwnerCode",       true, 10);
  auto con_fin_rel_code     = eatString("CONFINREL.ConFinRelCode",       true, 10);
  auto concession_area_code = eatString("CONFINREL.ConcessionAreaCode",  true, 10);
  auto financer_code        = eatString("CONFINREL.FinancerCode",       false, 10);
  if (!record_errors.empty()) return;

  records.concession_financer_relations.emplace_back(
    Kv1ConcessionFinancerRelation::Key(
      data_owner_code,
      con_fin_rel_code),
    concession_area_code,
    financer_code);
}

void Kv1Parser::parseConcessionArea() {
  auto data_owner_code      = eatString("CONAREA.DataOwnerCode",      true,  10);
  auto concession_area_code = eatString("CONAREA.ConcessionAreaCode", true,  10);
  auto description          = eatString("CONAREA.Description",        true, 255);
  if (!record_errors.empty()) return;

  records.concession_areas.emplace_back(
    Kv1ConcessionArea::Key(
      data_owner_code,
      concession_area_code),
    description);
}

void Kv1Parser::parseFinancer() {
  auto data_owner_code = eatString("FINANCER.DataOwnerCode", true,  10);
  auto financer_code   = eatString("FINANCER.FinancerCode",  true,  10);
  auto description     = eatString("FINANCER.Description",   true, 255);
  if (!record_errors.empty()) return;

  records.financers.emplace_back(
    Kv1Financer::Key(
      data_owner_code,
      financer_code),
    description);
}

void Kv1Parser::parseJourneyPatternTimingLink() {
  auto data_owner_code      = eatString  ("JOPATILI.DataOwnerCode",        true, 10);
  auto line_planning_number = eatString  ("JOPATILI.LinePlanningNumber",   true, 10);
  auto journey_pattern_code = eatString  ("JOPATILI.JourneyPatternCode",   true, 10);
  auto timing_link_order    = eatNumber  ("JOPATILI.TimingLinkOrder",      true,  3);
  auto user_stop_code_begin = eatString  ("JOPATILI.UserStopCodeBegin",    true, 10);
  auto user_stop_code_end   = eatString  ("JOPATILI.UserStopCodeEnd",      true, 10);
  auto con_fin_rel_code     = eatString  ("JOPATILI.ConFinRelCode",        true, 10);
  auto dest_code            = eatString  ("JOPATILI.DestCode",             true, 10);
                              eatCell    ("JOPATILI.<deprecated field #1>"         );
  auto is_timing_stop       = eatBoolean ("JOPATILI.IsTimingStop",         true    );
  auto display_public_line  = eatString  ("JOPATILI.DisplayPublicLine",    false, 4);
  auto product_formula_type = eatNumber  ("JOPATILI.ProductFormulaType",   false, 4);
  auto get_in               = eatBoolean ("JOPATILI.GetIn",                true    );
  auto get_out              = eatBoolean ("JOPATILI.GetOut",               true    );
  auto show_flexible_trip   = eatString  ("JOPATILI.ShowFlexibleTrip",     false, 8);
  auto line_dest_icon       = eatNumber  ("JOPATILI.LineDestIcon",         false, 4);
  auto line_dest_color      = eatRgbColor("JOPATILI.LineDestColor",        false   );
  auto line_dest_text_color = eatRgbColor("JOPATILI.LineDestTextColor",    false   );
  if (!record_errors.empty()) return;

  if (line_dest_icon && *line_dest_icon != static_cast<short>(*line_dest_icon))
    record_errors.push_back("JOPATILI.LineDestIcon should be an integer");
  if (!show_flexible_trip.empty() && show_flexible_trip != "TRUE" &&
       show_flexible_trip != "FALSE" && show_flexible_trip != "REALTIME")
    record_errors.push_back("JOPATILI.ShowFlexibleTrip should be in BISON E21 values [TRUE, FALSE, REALTIME]");
  if (!record_errors.empty()) return;

  records.journey_pattern_timing_links.emplace_back(
    Kv1JourneyPatternTimingLink::Key(
      data_owner_code,
      line_planning_number,
      journey_pattern_code,
      static_cast<short>(*timing_link_order)),
    user_stop_code_begin,
    user_stop_code_end,
    con_fin_rel_code,
    dest_code,
    *is_timing_stop,
    display_public_line,
    product_formula_type,
    *get_in,
    *get_out,
    show_flexible_trip,
    line_dest_icon,
    line_dest_color,
    line_dest_text_color);
}

void Kv1Parser::parsePoint() {
  auto data_owner_code        = eatString("POINT.DataOwnerCode",        true,   10);
  auto point_code             = eatString("POINT.PointCode",            true,   10);
                                eatCell  ("POINT.<deprecated field #1>"           );
  auto point_type             = eatString("POINT.PointType",            true,   10);
  auto coordinate_system_type = eatString("POINT.CoordinateSystemType", true,   10);
  // NOTE: We deviate from the specification here once again. The specification
  // notes that LocationX_EW should contain 'at least 6 positions'. Assuming
  // that this is referring to the amount of digits, we have to lower this to
  // 4. Otherwise, some positions in the Netherlands and Belgium are
  // unrepresentable.
  auto location_x_ew          = eatRdCoord("POINT.LocationX_EW",        true,    4);
  auto location_y_ew          = eatRdCoord("POINT.LocationX_EW",        true,    6);
  auto location_z             = eatRdCoord("POINT.LocationZ",           false,   0);
  auto description            = eatString ("POINT.Description",         false, 255);
  if (!record_errors.empty()) return;

  records.points.emplace_back(
    Kv1Point::Key(
      std::move(data_owner_code),
      std::move(point_code)),
    std::move(point_type),
    std::move(coordinate_system_type),
    *location_x_ew,
    *location_y_ew,
    location_z,
    std::move(description));
}

void Kv1Parser::parsePointOnLink() {
  auto data_owner_code              = eatString("POOL.DataOwnerCode",             true,  10);
  auto user_stop_code_begin         = eatString("POOL.UserStopCodeBegin",         true,  10);
  auto user_stop_code_end           = eatString("POOL.UserStopCodeEnd",           true,  10);
                                      eatCell  ("POOL.<deprecated field #1>"               );
  auto point_data_owner_code        = eatString("POOL.PointDataOwnerCode",        true,  10);
  auto point_code                   = eatString("POOL.PointCode",                 true,  10);
  auto distance_since_start_of_link = eatNumber("POOL.DistanceSinceStartOfLink",  true,   5);
  auto segment_speed                = eatNumber("POOL.SegmentSpeed",             false,   4);
  auto local_point_speed            = eatNumber("POOL.LocalPointSpeed",          false,   4);
  auto description                  = eatString("POOL.Description",              false, 255);
  auto transport_type               = eatString("POOL.TransportType",             true,   5);
  if (!record_errors.empty()) return;

  records.point_on_links.emplace_back(
    Kv1PointOnLink::Key(
      data_owner_code,
      user_stop_code_begin,
      user_stop_code_end,
      point_data_owner_code,
      point_code,
      transport_type),
    *distance_since_start_of_link,
    segment_speed,
    local_point_speed,
    std::move(description));
}

void Kv1Parser::parseIcon() {
  auto data_owner_code = eatString("ICON.DataOwnerCode", true,   10);
  auto icon_number     = eatNumber("ICON.IconNumber",    true,    4);
  auto icon_uri        = eatString("ICON.IconURI",       true, 1024);
  if (!record_errors.empty()) return;

  if (*icon_number != static_cast<short>(*icon_number)) {
    record_errors.push_back("ICON.IconNumber should be an integer");
    return;
  }

  records.icons.emplace_back(
    Kv1Icon::Key(
      data_owner_code,
      static_cast<short>(*icon_number)),
    icon_uri);
}

void Kv1Parser::parseNotice() {
  auto data_owner_code = eatString("NOTICE.DataOwnerCode", true,   10);
  auto notice_code     = eatString("NOTICE.NoticeCode",    true,   20);
  auto notice_content  = eatString("NOTICE.NoticeContent", true, 1024);
  if (!record_errors.empty()) return;

  records.notices.emplace_back(
    Kv1Notice::Key(
      data_owner_code,
      notice_code),
    notice_content);
}

void Kv1Parser::parseNoticeAssignment() {
  auto data_owner_code          = eatString("NTCASSGNM.DataOwnerCode",           true, 10);
  auto notice_code              = eatString("NTCASSGNM.NoticeCode",              true, 20);
  auto assigned_object          = eatString("NTCASSGNM.AssignedObject",          true,  8);
  auto timetable_version_code   = eatString("NTCASSGNM.TimetableVersionCode",   false, 10);
  auto organizational_unit_code = eatString("NTCASSGNM.OrganizationalUnitCode", false, 10);
  auto schedule_code            = eatString("NTCASSGNM.ScheduleCode",           false, 10);
  auto schedule_type_code       = eatString("NTCASSGNM.ScheduleTypeCode",       false, 10);
  auto period_group_code        = eatString("NTCASSGNM.PeriodGroupCode",        false, 10);
  auto specific_day_code        = eatString("NTCASSGNM.SpecificDayCode",        false, 10);
  auto day_type                 = eatString("NTCASSGNM.DayType",                false,  7);
  auto line_planning_number     = eatString("NTCASSGNM.LinePlanningNumber",      true, 10);
  auto journey_number           = eatNumber("NTCASSGNM.JourneyNumber",          false,  6);
  auto stop_order               = eatNumber("NTCASSGNM.StopOrder",              false,  4);
  auto journey_pattern_code     = eatString("NTCASSGNM.JourneyPatternCode",     false, 10);
  auto timing_link_order        = eatNumber("NTCASSGNM.TimingLinkOrder",        false,  3);
  auto user_stop_code           = eatString("NTCASSGNM.UserStopCode",           false, 10);
  if (!record_errors.empty()) return;

  if (journey_number && *journey_number != static_cast<short>(*journey_number))
    record_errors.push_back("NTCASSGNM.JourneyNumber should be an integer");
  if (journey_number && (*journey_number < 0 || *journey_number > 999'999))
    record_errors.push_back("NTCASSGNM.JourneyNumber should be within the range [0-999999]");
  if (stop_order && *stop_order != static_cast<short>(*stop_order))
    record_errors.push_back("NTCASSGNM.StopOrder should be an integer");
  if (!journey_number && (assigned_object == "PUJO" || assigned_object == "PUJOPASS"))
    record_errors.push_back("NTCASSGNM.JourneyNumber is required for AssignedObject PUJO/PUJOPASS");
  if (journey_pattern_code.empty() && assigned_object == "JOPATILI")
    record_errors.push_back("NTCASSGNM.JourneyPatternCode is required for AssignedObject JOPATILI");
  if (!record_errors.empty()) return;

  records.notice_assignments.emplace_back(
    data_owner_code,
    notice_code,
    assigned_object,
    timetable_version_code,
    organizational_unit_code,
    schedule_code,
    schedule_type_code,
    period_group_code,
    specific_day_code,
    day_type,
    line_planning_number,
    static_cast<std::optional<int>>(journey_number),
    static_cast<std::optional<short>>(stop_order),
    journey_pattern_code,
    timing_link_order,
    user_stop_code);
}

void Kv1Parser::parseTimeDemandGroup() {
  auto data_owner_code        = eatString("TIMDEMGRP.DataOwnerCode",       true, 10);
  auto line_planning_number   = eatString("TIMDEMGRP.LinePlanningNumber",  true, 10);
  auto journey_pattern_code   = eatString("TIMDEMGRP.JourneyPatternCode",  true, 10);
  auto time_demand_group_code = eatString("TIMDEMGRP.TimeDemandGroupCode", true, 10);
  if (!record_errors.empty()) return;

  records.time_demand_groups.emplace_back(
    Kv1TimeDemandGroup::Key(
      data_owner_code,
      line_planning_number,
      journey_pattern_code,
      time_demand_group_code));
}

void Kv1Parser::parseTimeDemandGroupRunTime() {
  auto data_owner_code        = eatString("TIMDEMRNT.DataOwnerCode",       true,  10);
  auto line_planning_number   = eatString("TIMDEMRNT.LinePlanningNumber",  true,  10);
  auto journey_pattern_code   = eatString("TIMDEMRNT.JourneyPatternCode",  true,  10);
  auto time_demand_group_code = eatString("TIMDEMRNT.TimeDemandGroupCode", true,  10);
  auto timing_link_order      = eatNumber("TIMDEMRNT.TimingLinkOrder",     true,   3);
  auto user_stop_code_begin   = eatString("TIMDEMRNT.UserStopCodeBegin",   true,  10);
  auto user_stop_code_end     = eatString("TIMDEMRNT.UserStopCodeEnd",     true,  10);
  auto total_drive_time       = eatNumber("TIMDEMRNT.TotalDriveTime",      true,   5);
  auto drive_time             = eatNumber("TIMDEMRNT.DriveTime",           true,   5);
  auto expected_delay         = eatNumber("TIMDEMRNT.ExpectedDelay",       false,  5);
  auto layover_time           = eatNumber("TIMDEMRNT.LayOverTime",         false,  5);
  auto stop_wait_time         = eatNumber("TIMDEMRNT.StopWaitTime",        true,   5);
  auto minimum_stop_time      = eatNumber("TIMDEMRNT.MinimumStopTime",     false,  5);
  if (!record_errors.empty()) return;

  if (timing_link_order && *timing_link_order != static_cast<short>(*timing_link_order)) {
    record_errors.push_back("TIMDEMRNT.TimingLinkOrder should be an integer");
    return;
  }

  records.time_demand_group_run_times.emplace_back(
    Kv1TimeDemandGroupRunTime::Key(
      data_owner_code,
      line_planning_number,
      journey_pattern_code,
      time_demand_group_code,
      static_cast<short>(*timing_link_order)),
    user_stop_code_begin,
    user_stop_code_end,
    *total_drive_time,
    *drive_time,
    expected_delay,
    layover_time,
    *stop_wait_time,
    minimum_stop_time);
}

void Kv1Parser::parsePeriodGroup() {
  auto data_owner_code   = eatString("PEGR.DataOwnerCode",    true,  10);
  auto period_group_code = eatString("PEGR.PeriodGroupCode",  true,  10);
  auto description       = eatString("PEGR.Description",     false, 255);
  if (!record_errors.empty()) return;

  records.period_groups.emplace_back(
    Kv1PeriodGroup::Key(
      data_owner_code,
      period_group_code),
    description);
}

void Kv1Parser::parseSpecificDay() {
  auto data_owner_code   = eatString("SPECDAY.DataOwnerCode",    true,  10);
  auto specific_day_code = eatString("SPECDAY.SpecificDayCode",  true,  10);
  auto name              = eatString("SPECDAY.Name",             true,  50);
  auto description       = eatString("SPECDAY.Description",     false, 255);
  if (!record_errors.empty()) return;

  records.specific_days.emplace_back(
    Kv1SpecificDay::Key(
      data_owner_code,
      specific_day_code),
    name,
    description);
}

void Kv1Parser::parseTimetableVersion() {
  auto data_owner_code          = eatString("TIVE.DataOwnerCode",           true,  10);
  auto organizational_unit_code = eatString("TIVE.OrganizationalUnitCode",  true,  10);
  auto timetable_version_code   = eatString("TIVE.TimetableVersionCode",    true,  10);
  auto period_group_code        = eatString("TIVE.PeriodGroupCode",         true,  10);
  auto specific_day_code        = eatString("TIVE.SpecificDayCode",         true,  10);
  auto valid_from_raw           = eatString("TIVE.ValidFrom",               true,  10);
  auto timetable_version_type   = eatString("TIVE.TimetableVersionType",    true,  10);
  auto valid_thru_raw           = eatString("TIVE.ValidThru",              false,  10);
  auto description              = eatString("TIVE.Description",            false, 255);
  if (!record_errors.empty()) return;

  auto valid_from = parseYyyymmdd(valid_from_raw);
  if (!valid_from)
    record_errors.push_back("TIVE.ValidFrom has invalid format, should be YYYY-MM-DD");
  std::optional<std::chrono::year_month_day> valid_thru;
  if (!valid_thru_raw.empty()) {
    valid_thru = parseYyyymmdd(valid_thru_raw);
    if (!valid_thru) {
      record_errors.push_back("TIVE.ValidFrom has invalid format, should be YYYY-MM-DD");
    }
  }
  if (!description.empty())
    record_errors.push_back("TIVE.Description should be empty");
  if (!record_errors.empty()) return;

  records.timetable_versions.emplace_back(
    Kv1TimetableVersion::Key(
      data_owner_code,
      organizational_unit_code,
      timetable_version_code,
      period_group_code,
      specific_day_code),
    *valid_from,
    timetable_version_type,
    valid_thru,
    description);
}

void Kv1Parser::parsePublicJourney() {
  auto data_owner_code          = eatString ("PUJO.DataOwnerCode",           true, 10);
  auto timetable_version_code   = eatString ("PUJO.TimetableVersionCode",    true, 10);
  auto organizational_unit_code = eatString ("PUJO.OrganizationalUnitCode",  true, 10);
  auto period_group_code        = eatString ("PUJO.PeriodGroupCode",         true, 10);
  auto specific_day_code        = eatString ("PUJO.SpecificDayCode",         true, 10);
  auto day_type                 = eatString ("PUJO.DayType",                 true,  7);
  auto line_planning_number     = eatString ("PUJO.LinePlanningNumber",      true, 10);
  auto journey_number           = eatNumber ("PUJO.JourneyNumber",           true,  6);
  auto time_demand_group_code   = eatString ("PUJO.TimeDemandGroupCode",     true, 10);
  auto journey_pattern_code     = eatString ("PUJO.JourneyPatternCode",      true, 10);
  auto departure_time_raw       = eatString ("PUJO.DepartureTime",           true,  8);
  auto wheelchair_accessible    = eatString ("PUJO.WheelChairAccessible",    true, 13);
  auto data_owner_is_operator   = eatBoolean("PUJO.DataOwnerIsOperator",     true    );
  auto planned_monitored        = eatBoolean("PUJO.PlannedMonitored",        true    );
  auto product_formula_type     = eatNumber ("PUJO.ProductFormulaType",     false,  4);
  auto show_flexible_trip       = eatString ("PUJO.ShowFlexibleTrip",       false,  8);
  if (!record_errors.empty()) return;

  auto departure_time = parseHhmmss(departure_time_raw);
  if (!departure_time)
    record_errors.push_back("PUJO.DepartureTime has a bad format");
  if (*journey_number < 0 || *journey_number > 999'999)
    record_errors.push_back("PUJO.JourneyNumber should be within the range [0-999999]");
  if (*journey_number != static_cast<int>(*journey_number))
    record_errors.push_back("PUJO.JourneyNumber should be an integer");
  if (product_formula_type && *product_formula_type != static_cast<short>(*product_formula_type))
    record_errors.push_back("PUJO.ProductFormulaType should be an integer");
  if (wheelchair_accessible != "ACCESSIBLE" && wheelchair_accessible != "NOTACCESSIBLE" && wheelchair_accessible != "UNKNOWN")
    record_errors.push_back("PUJO.WheelChairAccessible should be in BISON E3 values [ACCESSIBLE, NOTACCESSIBLE, UNKNOWN]");
  if (!show_flexible_trip.empty() && show_flexible_trip != "TRUE" &&
       show_flexible_trip != "FALSE" && show_flexible_trip != "REALTIME")
    record_errors.push_back("PUJO.ShowFlexibleTrip should be in BISON E21 values [TRUE, FALSE, REALTIME]");
  if (!record_errors.empty()) return;

  records.public_journeys.emplace_back(
    Kv1PublicJourney::Key(
      data_owner_code,
      timetable_version_code,
      organizational_unit_code,
      period_group_code,
      specific_day_code,
      day_type,
      line_planning_number,
      static_cast<int>(*journey_number)),
    time_demand_group_code,
    journey_pattern_code,
    *departure_time,
    wheelchair_accessible,
    *data_owner_is_operator,
    *planned_monitored,
    product_formula_type,
    show_flexible_trip);
}

void Kv1Parser::parsePeriodGroupValidity() {
  auto data_owner_code          = eatString("PEGRVAL.DataOwnerCode",          true, 10);
  auto organizational_unit_code = eatString("PEGRVAL.OrganizationalUnitCode", true, 10);
  auto period_group_code        = eatString("PEGRVAL.PeriodGroupCode",        true, 10);
  auto valid_from_raw           = eatString("PEGRVAL.ValidFrom",              true, 10);
  auto valid_thru_raw           = eatString("PEGRVAL.ValidThru",              true, 10);
  if (!record_errors.empty()) return;

  auto valid_from = parseYyyymmdd(valid_from_raw);
  auto valid_thru = parseYyyymmdd(valid_thru_raw);
  if (!valid_from)
    record_errors.push_back("PEGRVAL.ValidFrom has invalid format, should be YYYY-MM-DD");
  if (!valid_thru)
    record_errors.push_back("PEGRVAL.ValidThru has invalid format, should be YYYY-MM-DD");
  if (!record_errors.empty()) return;

  records.period_group_validities.emplace_back(
    Kv1PeriodGroupValidity::Key(
      data_owner_code,
      organizational_unit_code,
      period_group_code,
      *valid_from),
    *valid_thru);
}

void Kv1Parser::parseExceptionalOperatingDay() {
  auto data_owner_code          = eatString("EXCOPDAY.DataOwnerCode",           true,  10);
  auto organizational_unit_code = eatString("EXCOPDAY.OrganizationalUnitCode",  true,  10);
  auto valid_date_raw           = eatString("EXCOPDAY.ValidDate",               true,  23);
  auto day_type_as_on           = eatString("EXCOPDAY.DayTypeAsOn",             true,   7);
  auto specific_day_code        = eatString("EXCOPDAY.SpecificDayCode",         true,  10);
  auto period_group_code        = eatString("EXCOPDAY.PeriodGroupCode",        false,  10);
  auto description              = eatString("EXCOPDAY.Description",            false, 255);
  if (!record_errors.empty()) return;

  std::string_view error;
  auto valid_date = parseDateTime(valid_date_raw, amsterdam, &error);
  if (!valid_date) {
    record_errors.push_back(std::format("EXCOPDAY.ValidDate has an bad format (value: {}): {}", valid_date_raw, error));
    return;
  }

  records.exceptional_operating_days.emplace_back(
    Kv1ExceptionalOperatingDay::Key(
      data_owner_code,
      organizational_unit_code,
      *valid_date),
    day_type_as_on,
    specific_day_code,
    period_group_code,
    description);
}

void Kv1Parser::parseScheduleVersion() {
  auto data_owner_code          = eatString("SCHEDVERS.DataOwnerCode",           true,  10);
  auto organizational_unit_code = eatString("SCHEDVERS.OrganizationalUnitCode",  true,  10);
  auto schedule_code            = eatString("SCHEDVERS.ScheduleCode",            true,  10);
  auto schedule_type_code       = eatString("SCHEDVERS.ScheduleTypeCode",        true,  10);
  auto valid_from_raw           = eatString("SCHEDVERS.ValidFrom",               true,  10);
  auto valid_thru_raw           = eatString("SCHEDVERS.ValidThru",              false,  10);
  auto description              = eatString("SCHEDVERS.Description",            false, 255);
  if (!record_errors.empty()) return;

  auto valid_from = parseYyyymmdd(valid_from_raw);
  if (!valid_from)
    record_errors.push_back("SCHEDVERS.ValidFrom has invalid format, should be YYYY-MM-DD");
  std::optional<std::chrono::year_month_day> valid_thru;
  if (!valid_thru_raw.empty()) {
    valid_thru = parseYyyymmdd(valid_thru_raw);
    if (!valid_thru) {
      record_errors.push_back("SCHEDVERS.ValidFrom has invalid format, should be YYYY-MM-DD");
    }
  }
  if (!description.empty())
    record_errors.push_back("SCHEDVERS.Description should be empty");
  if (!record_errors.empty()) return;
 
  records.schedule_versions.emplace_back(
    Kv1ScheduleVersion::Key(
      data_owner_code,
      organizational_unit_code,
      schedule_code,
      schedule_type_code),
    *valid_from,
    valid_thru,
    description);
}

void Kv1Parser::parsePublicJourneyPassingTimes() {
  auto data_owner_code           = eatString ("PUJOPASS.DataOwnerCode",           true, 10);
  auto organizational_unit_code  = eatString ("PUJOPASS.OrganizationalUnitCode",  true, 10);
  auto schedule_code             = eatString ("PUJOPASS.ScheduleCode",            true, 10);
  auto schedule_type_code        = eatString ("PUJOPASS.ScheduleTypeCode",        true, 10);
  auto line_planning_number      = eatString ("PUJOPASS.LinePlanningNumber",      true, 10);
  auto journey_number            = eatNumber ("PUJOPASS.JourneyNumber",           true,  6);
  auto stop_order                = eatNumber ("PUJOPASS.StopOrder",               true,  4);
  auto journey_pattern_code      = eatString ("PUJOPASS.JourneyPatternCode",      true, 10);
  auto user_stop_code            = eatString ("PUJOPASS.UserStopCode",            true, 10);
  auto target_arrival_time_raw   = eatString ("PUJOPASS.TargetArrivalTime",      false,  8);
  auto target_departure_time_raw = eatString ("PUJOPASS.TargetDepartureTime",    false,  8);
  auto wheelchair_accessible     = eatString ("PUJOPASS.WheelChairAccessible",    true, 13);
  auto data_owner_is_operator    = eatBoolean("PUJOPASS.DataOwnerIsOperator",     true    );
  auto planned_monitored         = eatBoolean("PUJOPASS.PlannedMonitored",        true    );
  auto product_formula_type      = eatNumber ("PUJOPASS.ProductFormulaType",     false,  4);
  auto show_flexible_trip        = eatString ("PUJOPASS.ShowFlexibleTrip",       false,  8);
  if (!record_errors.empty()) return;

  if (*journey_number < 0 || *journey_number > 999'999)
    record_errors.push_back("PUJOPASS.JourneyNumber should be within the range [0-999999]");
  if (*journey_number != static_cast<int>(*journey_number))
    record_errors.push_back("PUJOPASS.JourneyNumber should be an integer");
  if (*stop_order != static_cast<short>(*stop_order))
    record_errors.push_back("PUJOPASS.StopOrder should be an integer");
  if (product_formula_type && *product_formula_type != static_cast<short>(*product_formula_type))
    record_errors.push_back("PUJOPASS.ProductFormulaType should be an integer");
  if (wheelchair_accessible != "ACCESSIBLE" && wheelchair_accessible != "NOTACCESSIBLE" && wheelchair_accessible != "UNKNOWN")
    record_errors.push_back("PUJOPASS.WheelChairAccessible should be in BISON E3 values [ACCESSIBLE, NOTACCESSIBLE, UNKNOWN]");
  if (!show_flexible_trip.empty() && show_flexible_trip != "TRUE" &&
       show_flexible_trip != "FALSE" && show_flexible_trip != "REALTIME")
    record_errors.push_back("PUJOPASS.ShowFlexibleTrip should be in BISON E21 values [TRUE, FALSE, REALTIME]");
  std::optional<std::chrono::hh_mm_ss<std::chrono::seconds>> target_arrival_time;
  if (!target_arrival_time_raw.empty()) {
    target_arrival_time = parseHhmmss(target_arrival_time_raw);
    if (!target_arrival_time) {
      record_errors.push_back("PUJOPASS.TargetArrivalTime has invalid format, should be HH:MM:SS");
    }
  }
  std::optional<std::chrono::hh_mm_ss<std::chrono::seconds>> target_departure_time;
  if (!target_departure_time_raw.empty()) {
    target_departure_time = parseHhmmss(target_departure_time_raw);
    if (!target_departure_time) {
      record_errors.push_back("PUJOPASS.TargetDepartureTime has invalid format, should be HH:MM:SS");
    }
  }
  if (!record_errors.empty()) return;

  records.public_journey_passing_times.emplace_back(
    Kv1PublicJourneyPassingTimes::Key(
      data_owner_code,
      organizational_unit_code,
      schedule_code,
      schedule_type_code,
      line_planning_number,
      static_cast<int>(*journey_number),
      static_cast<short>(*stop_order)),
    journey_pattern_code,
    user_stop_code,
    target_arrival_time,
    target_departure_time,
    wheelchair_accessible,
    *data_owner_is_operator,
    *planned_monitored,
    product_formula_type,
    show_flexible_trip);
}

void Kv1Parser::parseOperatingDay() {
  auto data_owner_code          = eatString("OPERDAY.DataOwnerCode",           true,  10);
  auto organizational_unit_code = eatString("OPERDAY.OrganizationalUnitCode",  true,  10);
  auto schedule_code            = eatString("OPERDAY.ScheduleCode",            true,  10);
  auto schedule_type_code       = eatString("OPERDAY.ScheduleTypeCode",        true,  10);
  auto valid_date_raw           = eatString("OPERDAY.ValidDate",               true,  10);
  auto description              = eatString("OPERDAY.Description",            false, 255);
  if (!record_errors.empty()) return;

  auto valid_date = parseYyyymmdd(valid_date_raw);
  if (!valid_date)
    record_errors.push_back("OPERDAY.ValidDate has invalid format, should be YYYY-MM-DD");
  if (!record_errors.empty()) return;
 
  records.operating_days.emplace_back(
    Kv1OperatingDay::Key(
      data_owner_code,
      organizational_unit_code,
      schedule_code,
      schedule_type_code,
      *valid_date),
    description);
}

const std::unordered_map<std::string_view, Kv1Parser::ParseFunc> Kv1Parser::type_parsers{
  { "ORUN",      &Kv1Parser::parseOrganizationalUnit         },
  { "ORUNORUN",  &Kv1Parser::parseHigherOrganizationalUnit   },
  { "USRSTOP",   &Kv1Parser::parseUserStopPoint              },
  { "USRSTAR",   &Kv1Parser::parseUserStopArea               },
  { "TILI",      &Kv1Parser::parseTimingLink                 },
  { "LINK",      &Kv1Parser::parseLink                       },
  { "LINE",      &Kv1Parser::parseLine                       },
  { "DEST",      &Kv1Parser::parseDestination                },
  { "JOPA",      &Kv1Parser::parseJourneyPattern             },
  { "CONFINREL", &Kv1Parser::parseConcessionFinancerRelation },
  { "CONAREA",   &Kv1Parser::parseConcessionArea             },
  { "FINANCER",  &Kv1Parser::parseFinancer                   },
  { "JOPATILI",  &Kv1Parser::parseJourneyPatternTimingLink   },
  { "POINT",     &Kv1Parser::parsePoint                      },
  { "POOL",      &Kv1Parser::parsePointOnLink                },
  { "ICON",      &Kv1Parser::parseIcon                       },
  { "NOTICE",    &Kv1Parser::parseNotice                     },
  { "NTCASSGNM", &Kv1Parser::parseNoticeAssignment           },
  { "TIMDEMGRP", &Kv1Parser::parseTimeDemandGroup            },
  { "TIMDEMRNT", &Kv1Parser::parseTimeDemandGroupRunTime     },
  { "PEGR",      &Kv1Parser::parsePeriodGroup                },
  { "SPECDAY",   &Kv1Parser::parseSpecificDay                },
  { "TIVE",      &Kv1Parser::parseTimetableVersion           },
  { "PUJO",      &Kv1Parser::parsePublicJourney              },
  { "PEGRVAL",   &Kv1Parser::parsePeriodGroupValidity        },
  { "EXCOPDAY",  &Kv1Parser::parseExceptionalOperatingDay    },
  { "SCHEDVERS", &Kv1Parser::parseScheduleVersion            },
  { "PUJOPASS",  &Kv1Parser::parsePublicJourneyPassingTimes  },
  { "OPERDAY",   &Kv1Parser::parseOperatingDay               },
};
