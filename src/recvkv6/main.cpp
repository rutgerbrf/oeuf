// vim:set sw=2 ts=2 sts et:

#include <array>
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <stack>
#include <string>
#include <sstream>
#include <vector>

#include <zlib.h>
#include <zmq.h>

#include <nlohmann/json.hpp>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

#include <rapidxml/rapidxml.hpp>

#include <tmi8/kv6_parquet.hpp>

#define CHUNK 16384

struct RawMessage {
  public:
    // Takes ownership of envelope and body
    RawMessage(zmq_msg_t envelope, zmq_msg_t body)
      : envelope(envelope), body(body)
    {}

    // Prevent copying
    RawMessage(const RawMessage &) = delete;
    RawMessage &operator=(RawMessage const &) = delete;

    std::string_view getEnvelope() {
      return static_cast<const char *>(zmq_msg_data(&envelope));
    }

    char *getBody() {
      return static_cast<char *>(zmq_msg_data(&body));
    }

    size_t getBodySize() {
      return zmq_msg_size(&body);
    }

    ~RawMessage() {
      zmq_msg_close(&envelope);
      zmq_msg_close(&body);
    }

  private:
    zmq_msg_t envelope;
    zmq_msg_t body;
};

std::optional<RawMessage> recvMsg(void *socket) {
  while (true) {
    zmq_msg_t envelope, body;
    int rc = zmq_msg_init(&envelope);
    assert(rc == 0);
    rc = zmq_msg_init(&body);
    assert(rc == 0);

    rc = zmq_msg_recv(&envelope, socket, 0);
    if (rc == -1) return std::nullopt;

    int more;
    size_t more_size = sizeof(more);
    rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
    if (!more) {
      zmq_msg_close(&envelope);
      zmq_msg_close(&body);
      continue;
    }
    
    rc = zmq_msg_recv(&body, socket, 0);
    if (rc == -1) return std::nullopt;

    rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
    assert(!more);

    return std::make_optional<RawMessage>(envelope, body);
  }
}

// Ensures that <return value>[output_size] == 0
char *decompress(char *raw, unsigned int input_size, unsigned int &output_size) {
  assert(input_size <= UINT32_MAX);

  z_stream strm;
  strm.next_in  = reinterpret_cast<unsigned char *>(raw);
  strm.avail_in = input_size;
  strm.zalloc   = Z_NULL;
  strm.zfree    = Z_NULL;
  strm.opaque   = Z_NULL;
  int rc = inflateInit2(&strm, 32);
  assert(rc == Z_OK);

  unsigned int buf_cap = CHUNK;
  unsigned int buf_len = 0;
  char *buf = static_cast<char *>(malloc(CHUNK));
  do {
    if (buf_len + CHUNK > buf_cap) {
      assert(buf_cap <= UINT32_MAX);
      buf_cap *= 2;
      buf = static_cast<char *>(realloc(buf, buf_cap));
    }
    strm.avail_out = buf_cap - buf_len;
    strm.next_out  = reinterpret_cast<unsigned char *>(buf + buf_len);

    unsigned long old_total = strm.total_out;
    rc = inflate(&strm, Z_FINISH);
    unsigned progress = static_cast<unsigned int>(strm.total_out - old_total);
    buf_len += progress;
    assert(progress != 0 || rc == Z_STREAM_END);
  } while (strm.total_in < input_size);

  if (buf_len == buf_cap) {
    buf = static_cast<char *>(realloc(buf, buf_len + 1));
  }
  buf[buf_len] = 0;
  output_size = buf_len;

  rc = inflateEnd(&strm);
  assert(rc == Z_OK);

  return buf;
}

struct Date {
  int16_t year  = 0;
  uint8_t month = 0;
  uint8_t day   = 0;

  static bool parse(Date &dest, std::string_view src) {
    dest.year = 0, dest.month = 0, dest.day = 0;

    int16_t y_mul_fac = 1;
    bool extended = false;

    size_t plus = src.find('+');
    if (plus != std::string_view::npos) {
      extended = true;
      src = src.substr(1);  // remove plus sign from the start
    }
    if (!extended) {
      size_t min_or_dash = src.find('-');
      if (min_or_dash == std::string_view::npos) return false;
      if (min_or_dash == 0) {
        y_mul_fac = -1;  // it's a minus sign
        src = src.substr(1);  // remove minus sign at the start
      }
    }

    int y_chars = 0;
    while (src.size() > 0 && src[0] >= '0' && src[0] <= '9') {
      dest.year = static_cast<int16_t>(dest.year * 10 + src[0] - '0');
      src = src.substr(1);
      y_chars++;
    }
    if (src.size() == 0) { dest.year = 0; return false; }
    if (src[0] != '-') { dest.year = 0; return false; }
    src = src.substr(1);  // remove dash
    if (y_chars < 4 || (y_chars > 4 && !extended)) { dest.year = 0; return false; }
    dest.year *= y_mul_fac;

    bool rest_correct = src.size() == 5
      && src[0] >= '0' && src[0] <= '9'
      && src[1] >= '0' && src[1] <= '9'
      && src[3] >= '0' && src[3] <= '9'
      && src[4] >= '0' && src[4] <= '9';
    if (!rest_correct) { dest.year = 0; return false; }
    dest.month = static_cast<uint8_t>((src[0] - '0') * 10 + src[1] - '0');
    dest.day   = static_cast<uint8_t>((src[3] - '0') * 10 + src[4] - '0');
    if (dest.month > 12 || dest.day > 31) {
      dest.year = 0, dest.month = 0, dest.day = 0;
      return false;
    }
    return true;
  }

  std::string toString() const {
    if (year < 0 || year > 9999 || month < 0 || month > 12 || day < 0 || day > 31)
      throw std::invalid_argument("one or more date components (year, month, day) out of range");
    char data[11] = "XXXX-XX-XX";
    sprintf(data, "%04u-%02u-%02u", year, month, day);
    return data;
  }

  std::chrono::days toUnixDays() const {
    std::chrono::year_month_day ymd{std::chrono::year(year), std::chrono::month(month), std::chrono::day(day)};
    // This is valid since C++20: as of C++20, the system clock is defined to measure the
    // Unix Time, the amount of seconds since Thursday 1 January 1970, without leap seconds.
    std::chrono::days since_epoch = std::chrono::sys_days(ymd).time_since_epoch();
    return since_epoch;
  }
};

struct Time {
  uint8_t hour   = 0;
  uint8_t minute = 0;
  uint8_t second = 0;

  static bool parse(Time &dest, std::string_view src) {
    bool okay = src.size() == 8
      && src[0] >= '0' && src[0] <= '9'
      && src[1] >= '0' && src[1] <= '9'
      && src[2] == ':'
      && src[3] >= '0' && src[3] <= '9'
      && src[4] >= '0' && src[4] <= '9'
      && src[5] == ':'
      && src[6] >= '0' && src[6] <= '9'
      && src[7] >= '0' && src[7] <= '9';
    if (!okay) return false;
    dest.hour   = static_cast<uint8_t>((src[0] - '0') * 10 + src[1] - '0');
    dest.minute = static_cast<uint8_t>((src[3] - '0') * 10 + src[4] - '0');
    dest.second = static_cast<uint8_t>((src[6] - '0') * 10 + src[7] - '0');
    if (dest.hour > 23 || dest.minute > 59 || dest.second > 59) {
      dest.hour = 0, dest.minute = 0, dest.second = 0;
      return false;
    }
    return true;
  }

  std::string toString() const {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
      throw std::invalid_argument("one or more time components (hour, minute, second) out of range");
    char data[9] = "XX:XX:XX";
    sprintf(data, "%02u:%02u:%02u", hour, minute, second);
    return data;
  }
};

// Time zone designator
struct Tzd {
  int16_t minutes = 0;

  static bool parse(Tzd &dest, std::string_view src) {
    dest.minutes = 0;

    if (src.size() == 0) return false;
    if (src == "Z") return true;

    int16_t multiplier = 1;
    if (src[0] == '-') multiplier = -1;
    else if (src[0] != '+') return false;
    src = src.substr(1);

    bool okay = src.size() == 5
      && src[0] >= '0' && src[0] <= '9'
      && src[1] >= '0' && src[1] <= '9'
      && src[2] == ':'
      && src[3] >= '0' && src[3] <= '9'
      && src[4] >= '0' && src[4] <= '9';
    if (!okay) return false;
    int16_t hours   = static_cast<int16_t>((src[0] - '0') * 10 + src[1] - '0');
    int16_t minutes = static_cast<int16_t>((src[3] - '0') * 10 + src[4] - '0');
    if (hours > 23 || minutes > 59) return false;
    dest.minutes = static_cast<int16_t>(multiplier * (60 * hours + minutes));
    return true;
  }

  std::string toString() const {
    if (minutes == 0)
      return "Z";
   
    bool negative = minutes < 0;
    int hours_off = abs(minutes / 60);
    int mins_off  = abs(minutes) - hours_off*60;
    if (hours_off > 23 || mins_off > 59)
      throw std::invalid_argument("offset out of range");
    char data[7] = "+XX:XX";
    sprintf(data, "%c%02u:%02u", negative ? '-' : '+', hours_off, mins_off);
    return data;
  }
};

struct Timestamp {
  Date date;
  Tzd  off;
  Time time;

  static bool parse(Timestamp &dest, std::string_view src) {
    size_t t = src.find('T');
    if (t == std::string_view::npos || t + 1 >= src.size()) return false;

    std::string_view date = src.substr(0, t);
    std::string_view time_and_tzd = src.substr(t + 1);
    if (time_and_tzd.size() < 9) return false;
    if (!Date::parse(dest.date, date)) return false;

    std::string_view time = time_and_tzd.substr(0, 8);
    std::string_view tzd  = time_and_tzd.substr(8);
    if (!Time::parse(dest.time, time)) return false;
    return Tzd::parse(dest.off, tzd);
  }

  std::string toString() const {
    return date.toString() + "T" + time.toString() + off.toString();
  }

  std::chrono::seconds toUnixSeconds() const {
    std::chrono::year_month_day ymd(std::chrono::year(date.year),
                                    std::chrono::month(date.month),
                                    std::chrono::day(date.day));
    std::chrono::sys_days sys_days(ymd);
    std::chrono::time_point<std::chrono::utc_clock, std::chrono::days> utc_days(sys_days.time_since_epoch());
    std::chrono::utc_seconds utc_seconds = std::chrono::time_point_cast<std::chrono::seconds>(utc_days);
    utc_seconds += std::chrono::hours(time.hour) + std::chrono::minutes(time.minute) +
                   std::chrono::seconds(time.second) - std::chrono::minutes(off.minutes);
    std::chrono::sys_seconds sys_seconds = std::chrono::utc_clock::to_sys(utc_seconds);
    std::chrono::seconds unix = sys_seconds.time_since_epoch();
    return unix;
  }
};

static const std::string_view TMI8_XML_NS = "http://bison.connekt.nl/tmi8/kv6/msg";

enum Kv6RecordType {
  KV6T_UNKNOWN   = 0,
  KV6T_DELAY     = 1,
  KV6T_INIT      = 2,
  KV6T_ARRIVAL   = 3,
  KV6T_ON_STOP   = 4,
  KV6T_DEPARTURE = 5,
  KV6T_ON_ROUTE  = 6,
  KV6T_ON_PATH   = 7,
  KV6T_OFF_ROUTE = 8,
  KV6T_END       = 9,
  // Always keep this updated to correspond to the 
  // first and last elements of the enumeration!
  _KV6T_FIRST_TYPE = KV6T_UNKNOWN,
  _KV6T_LAST_TYPE  = KV6T_END,
};

enum Kv6Field {
  KV6F_NONE                          =     0,
  KV6F_DATA_OWNER_CODE               =     1,
  KV6F_LINE_PLANNING_NUMBER          =     2,
  KV6F_OPERATING_DAY                 =     4,
  KV6F_JOURNEY_NUMBER                =     8,
  KV6F_REINFORCEMENT_NUMBER          =    16,
  KV6F_TIMESTAMP                     =    32,
  KV6F_SOURCE                        =    64,
  KV6F_PUNCTUALITY                   =   128,
  KV6F_USER_STOP_CODE                =   256,
  KV6F_PASSAGE_SEQUENCE_NUMBER       =   512,
  KV6F_VEHICLE_NUMBER                =  1024,
  KV6F_BLOCK_CODE                    =  2048,
  KV6F_WHEELCHAIR_ACCESSIBLE         =  4096,
  KV6F_NUMBER_OF_COACHES             =  8192,
  KV6F_RD_Y                          = 16384,
  KV6F_RD_X                          = 32768,
  KV6F_DISTANCE_SINCE_LAST_USER_STOP = 65536,
};

static constexpr Kv6Field KV6T_REQUIRED_FIELDS[_KV6T_LAST_TYPE + 1] = {
  // KV6T_UNKNOWN
    KV6F_NONE,
  // KV6T_DELAY
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_PUNCTUALITY),
  // KV6T_INIT
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_USER_STOP_CODE
  | KV6F_PASSAGE_SEQUENCE_NUMBER
  | KV6F_VEHICLE_NUMBER
  | KV6F_BLOCK_CODE
  | KV6F_WHEELCHAIR_ACCESSIBLE
  | KV6F_NUMBER_OF_COACHES),
  // KV6T_ARRIVAL
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_USER_STOP_CODE
  | KV6F_PASSAGE_SEQUENCE_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_VEHICLE_NUMBER
  | KV6F_PUNCTUALITY),
  // KV6T_ON_STOP
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_USER_STOP_CODE
  | KV6F_PASSAGE_SEQUENCE_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_VEHICLE_NUMBER
  | KV6F_PUNCTUALITY),
  // KV6T_DEPARTURE
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_USER_STOP_CODE
  | KV6F_PASSAGE_SEQUENCE_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_VEHICLE_NUMBER
  | KV6F_PUNCTUALITY),
  // KV6T_ON_ROUTE
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_USER_STOP_CODE
  | KV6F_PASSAGE_SEQUENCE_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_VEHICLE_NUMBER
  | KV6F_PUNCTUALITY
  | KV6F_RD_X
  | KV6F_RD_Y),
  // KV6T_ON_PATH
    KV6F_NONE,
  // KV6T_OFF_ROUTE
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_USER_STOP_CODE
  | KV6F_PASSAGE_SEQUENCE_NUMBER
  | KV6F_VEHICLE_NUMBER
  | KV6F_RD_X
  | KV6F_RD_Y),
  // KV6T_END
  static_cast<Kv6Field>(
    KV6F_DATA_OWNER_CODE
  | KV6F_LINE_PLANNING_NUMBER
  | KV6F_OPERATING_DAY
  | KV6F_JOURNEY_NUMBER
  | KV6F_REINFORCEMENT_NUMBER
  | KV6F_TIMESTAMP
  | KV6F_SOURCE
  | KV6F_USER_STOP_CODE
  | KV6F_PASSAGE_SEQUENCE_NUMBER
  | KV6F_VEHICLE_NUMBER),
};

static constexpr Kv6Field KV6T_OPTIONAL_FIELDS[_KV6T_LAST_TYPE + 1] = {
  // KV6T_UNKNOWN
  KV6F_NONE,
  // KV6T_DELAY
  KV6F_NONE,
  // KV6T_INIT
  KV6F_NONE,
  // KV6T_ARRIVAL
  static_cast<Kv6Field>(KV6F_RD_X | KV6F_RD_Y),
  // KV6T_ON_STOP
  static_cast<Kv6Field>(KV6F_RD_X | KV6F_RD_Y),
  // KV6T_DEPARTURE
  static_cast<Kv6Field>(KV6F_RD_X | KV6F_RD_Y),
  // KV6T_ON_ROUTE
  KV6F_DISTANCE_SINCE_LAST_USER_STOP,
  // KV6T_ON_PATH
  KV6F_NONE,
  // KV6T_OFF_ROUTE
  KV6F_NONE,
  // KV6T_END
  KV6F_NONE,
};

struct Kv6Record {
  Kv6RecordType type     = KV6T_UNKNOWN;
  Kv6Field      presence = KV6F_NONE;
  Kv6Field      next     = KV6F_NONE;
  std::string   data_owner_code;
  std::string   line_planning_number;
  std::string   source;
  std::string   user_stop_code;
  std::string   wheelchair_accessible;
  Date          operating_day;
  Timestamp     timestamp;
  uint32_t      block_code = 0;
  uint32_t      journey_number = 0;
  uint32_t      vehicle_number = 0;
  int32_t       rd_x = 0;
  int32_t       rd_y = 0;
  // The TMI8 specification is unclear: this field
  // might actually be called distancesincelaststop
  uint32_t      distance_since_last_user_stop = 0;
  uint16_t      passage_sequence_number = 0;
  int16_t       punctuality = 0;
  uint8_t       number_of_coaches = 0;
  uint8_t       reinforcement_number = 0;

  void markPresent(Kv6Field field) {
    presence = static_cast<Kv6Field>(presence | field);
  }

  void removeUnsupportedFields() {
    Kv6Field  required_fields = KV6T_REQUIRED_FIELDS[type];
    Kv6Field  optional_fields = KV6T_OPTIONAL_FIELDS[type];
    Kv6Field supported_fields = static_cast<Kv6Field>(required_fields | optional_fields);
    presence = static_cast<Kv6Field>(presence & supported_fields);
  }

  bool valid() {
    Kv6Field  required_fields = KV6T_REQUIRED_FIELDS[type];
    Kv6Field  optional_fields = KV6T_OPTIONAL_FIELDS[type];
    Kv6Field supported_fields = static_cast<Kv6Field>(required_fields | optional_fields);

    Kv6Field    required_field_presence = static_cast<Kv6Field>(presence &   required_fields);
    Kv6Field unsupported_field_presence = static_cast<Kv6Field>(presence & ~supported_fields);

    return required_field_presence == required_fields && !unsupported_field_presence;
  }
};

enum Tmi8VvTmPushInfoField {
  TMI8F_NONE          = 0,
  TMI8F_SUBSCRIBER_ID = 1,
  TMI8F_VERSION       = 2,
  TMI8F_DOSSIER_NAME  = 4,
  TMI8F_TIMESTAMP     = 8,
};

struct Tmi8VvTmPushInfo {
  Tmi8VvTmPushInfoField next     = TMI8F_NONE;
  Tmi8VvTmPushInfoField presence = TMI8F_NONE;
  std::string subscriber_id;
  std::string version;
  std::string dossier_name;
  Timestamp timestamp;
  std::vector<Kv6Record> messages;

  void markPresent(Tmi8VvTmPushInfoField field) {
    presence = static_cast<Tmi8VvTmPushInfoField>(presence | field);
  }

  bool valid() {
    const Tmi8VvTmPushInfoField REQUIRED_FIELDS =
      static_cast<Tmi8VvTmPushInfoField>(
          TMI8F_SUBSCRIBER_ID
        | TMI8F_VERSION
        | TMI8F_DOSSIER_NAME
        | TMI8F_TIMESTAMP);
    return (presence & REQUIRED_FIELDS) == REQUIRED_FIELDS;
  }
};

static const std::array<std::string_view, _KV6T_LAST_TYPE + 1> KV6_POS_INFO_RECORD_TYPES = {
  "UNKNOWN", "DELAY", "INIT", "ARRIVAL", "ONSTOP", "DEPARTURE", "ONROUTE", "ONPATH", "OFFROUTE", "END",
};

std::optional<std::string_view> findKv6PosInfoRecordTypeName(Kv6RecordType type) {
  if (type > _KV6T_LAST_TYPE)
    return std::nullopt;
  return KV6_POS_INFO_RECORD_TYPES[type];
}

const std::array<std::tuple<std::string_view, Kv6Field>, 17> KV6_POS_INFO_RECORD_FIELDS = {{
  { "dataownercode",             KV6F_DATA_OWNER_CODE               },
  { "lineplanningnumber",        KV6F_LINE_PLANNING_NUMBER          },
  { "operatingday",              KV6F_OPERATING_DAY                 },
  { "journeynumber",             KV6F_JOURNEY_NUMBER                },
  { "reinforcementnumber",       KV6F_REINFORCEMENT_NUMBER          },
  { "timestamp",                 KV6F_TIMESTAMP                     },
  { "source",                    KV6F_SOURCE                        },
  { "punctuality",               KV6F_PUNCTUALITY                   },
  { "userstopcode",              KV6F_USER_STOP_CODE                },
  { "passagesequencenumber",     KV6F_PASSAGE_SEQUENCE_NUMBER       },
  { "vehiclenumber",             KV6F_VEHICLE_NUMBER                },
  { "blockcode",                 KV6F_BLOCK_CODE                    },
  { "wheelchairaccessible",      KV6F_WHEELCHAIR_ACCESSIBLE         },
  { "numberofcoaches",           KV6F_NUMBER_OF_COACHES             },
  { "rd-y",                      KV6F_RD_Y                          },
  { "rd-x",                      KV6F_RD_X                          },
  { "distancesincelastuserstop", KV6F_DISTANCE_SINCE_LAST_USER_STOP },
}};

// Returns the maximum amount of digits such that it is guaranteed that
// a corresponding amount of repeated 9's can be represented by the type.
template<std::integral T>
constexpr size_t maxDigits() {
  size_t digits = 0;
  for (T x = std::numeric_limits<T>::max(); x != 0; x /= 10) digits++;
  return digits - 1;
}

template<size_t MaxDigits, std::unsigned_integral T>
constexpr bool parseUnsigned(T &out, std::string_view src) {
  static_assert(MaxDigits <= maxDigits<T>());
  if (src.size() > MaxDigits) return false;
  T res = 0;
  while (src.size() > 0) {
    if (src[0] < '0' || src[0] > '9') return false;
    res = static_cast<T>(res * 10 + src[0] - '0');
    src = src.substr(1);
  }
  out = res;
  return true;
}

template<size_t MaxDigits, std::signed_integral T>
constexpr bool parseSigned(T &out, std::string_view src) {
  static_assert(MaxDigits <= maxDigits<T>());
  if (src.size() == 0) return false;
  bool negative = src[0] == '-';
  if (negative) src = src.substr(1);
  if (src.size() > MaxDigits) return false;
  T res = 0;
  while (src.size() > 0) {
    if (src[0] < '0' || src[0] > '9') return false;
    res = static_cast<T>(res * 10 + src[0] - '0');
    src = src.substr(1);
  }
  out = negative ? -res : res;
  return true;
}

struct Xmlns {
  const Xmlns *next;
  std::string_view prefix;
  std::string_view url;
};

std::optional<std::string_view> resolve(std::string_view prefix, const Xmlns *nss) {
  while (nss)
    if (nss->prefix == prefix)
      return nss->url;
    else
      nss = nss->next;
  return std::nullopt;
}

template<typename T>
void withXmlnss(const rapidxml::xml_attribute<> *attr, const Xmlns *nss, const T &fn) {
  while (attr) {
    std::string_view name(attr->name(), attr->name_size());
    if (name.starts_with("xmlns")) {
      if (name.size() == 5) { // just xmlns
        Xmlns ns0 = {
          .next = nss,
          .url = std::string_view(attr->value(), attr->value_size()),
        };
        withXmlnss(attr->next_attribute(), &ns0, fn);
        return;
      } else if (name.size() > 6 && name[5] == ':') { // xmlns:<something>
        Xmlns ns0 = {
          .next = nss,
          .prefix = name.substr(6),
          .url = std::string_view(attr->value(), attr->value_size()),
        };
        withXmlnss(attr->next_attribute(), &ns0, fn);
        return;
      }
    }
    attr = attr->next_attribute();
  }
  fn(nss);
}

template<typename T>
void ifResolvable(const rapidxml::xml_node<> &node, const Xmlns *nss, const T &fn) {
  std::string_view name(node.name(), node.name_size());
  std::string_view ns;
  size_t colon = name.find(':');

  if (colon != std::string_view::npos) {
    if (colon >= name.size() - 1)  // last character
      return;
    ns = name.substr(0, colon);
    name = name.substr(colon + 1);
  }

  withXmlnss(node.first_attribute(), nss, [&](const Xmlns *nss) {
    std::optional<std::string_view> ns_url = resolve(ns, nss);
    if (!ns_url && !ns.empty()) return;
    if (!ns_url) fn(std::string_view(), name, nss);
    else fn(*ns_url, name, nss);
  });
}

template<typename T>
void ifTmi8Element(const rapidxml::xml_node<> &node, const Xmlns *nss, const T &fn) {
  ifResolvable(node, nss, [&](std::string_view ns_url, std::string_view name, const Xmlns *nss) {
    if (node.type() == rapidxml::node_element && (ns_url.empty() || ns_url == TMI8_XML_NS)) fn(name, nss);
  });
}

bool onlyTextElement(const rapidxml::xml_node<> &node) {
  return node.type() == rapidxml::node_element
      && node.first_node()
      && node.first_node() == node.last_node()
      && node.first_node()->type() == rapidxml::node_data;
}

std::string_view getValue(const rapidxml::xml_node<> &node) {
  return std::string_view(node.value(), node.value_size());
}

bool parseStringValue(std::string &into, size_t max_len, std::string_view val) {
  if (val.size() > max_len)
    return false;
  into = val;
  return true;
}

struct Kv6Parser {
  std::stringstream &errs;
  std::stringstream &warns;

  void error(std::string_view msg) {
    errs << msg << '\n';
  }

  void warn(std::string_view msg) {
    warns << msg << '\n';
  }

#define PERRASSERT(msg, ...)  do { if (!(__VA_ARGS__)) { error(msg); return; } } while (false)
#define PWARNASSERT(msg, ...) do { if (!(__VA_ARGS__)) { warn(msg);  return; } } while (false)

  std::optional<Kv6Record> parseKv6PosInfoRecord(Kv6RecordType type, const rapidxml::xml_node<> &node, const Xmlns *nss) {
    Kv6Record fields = { .type = type };
    for (const rapidxml::xml_node<> *child = node.first_node(); child; child = child->next_sibling()) {
      ifTmi8Element(*child, nss, [&](std::string_view name, const Xmlns *nss) {
        for (const auto &[fname, field] : KV6_POS_INFO_RECORD_FIELDS) {
          if (field == KV6F_NONE)
            continue;
          if (fname == name) {
            PWARNASSERT("Expected KV6 record field element to only contain data",
                        onlyTextElement(*child));
            std::string_view childval = getValue(*child);
            switch (field) {
             case KV6F_DATA_OWNER_CODE:
              PWARNASSERT("Invalid value for dataownercode",
                          parseStringValue(fields.data_owner_code, 10, childval));
              break;
             case KV6F_LINE_PLANNING_NUMBER:
              PWARNASSERT("Invalid value for lineplanningnumber",
                          parseStringValue(fields.line_planning_number, 10, childval));
              break;
             case KV6F_OPERATING_DAY:
              PWARNASSERT("Invalid value for operatatingday: not a valid date",
                          Date::parse(fields.operating_day, childval));
              break;
             case KV6F_JOURNEY_NUMBER:
              PWARNASSERT("Invalid value for journeynumber:"
                          " not a valid unsigned number with at most six digits",
                          parseUnsigned<6>(fields.journey_number, childval));
              break;
             case KV6F_REINFORCEMENT_NUMBER:
              PWARNASSERT("Invalid value for reinforcementnumber:"
                          " not a valid unsigned number with at most two digits",
                          parseUnsigned<2>(fields.reinforcement_number, childval));
              break;
             case KV6F_TIMESTAMP:
              PWARNASSERT("Invalid value for timestamp: not a valid timestamp",
                          Timestamp::parse(fields.timestamp, childval));
              break;
             case KV6F_SOURCE:
              PWARNASSERT("Invalid value for source:"
                          " not a valid string of at most 10 bytes",
                          parseStringValue(fields.source, 10, childval));
              break;
             case KV6F_PUNCTUALITY:
              PWARNASSERT("Invalid value for punctuality:"
                          " not a valid signed number with at most four digits",
                          parseSigned<4>(fields.punctuality, childval));
              break;
             case KV6F_USER_STOP_CODE:
              PWARNASSERT("Invalid value for userstopcode:"
                          " not a valid string of at most 10 bytes",
                          parseStringValue(fields.user_stop_code, 10, childval));
              break;
             case KV6F_PASSAGE_SEQUENCE_NUMBER:
              PWARNASSERT("Invalid value for passagesequencenumber:"
                          " not a valid unsigned number with at most four digits",
                          parseUnsigned<4>(fields.passage_sequence_number, childval));
              break;
             case KV6F_VEHICLE_NUMBER:
              PWARNASSERT("Invalid value for vehiclenumber:"
                          " not a valid unsigned number with at most six digits",
                          parseUnsigned<6>(fields.vehicle_number, childval));
              break;
             case KV6F_BLOCK_CODE:
              PWARNASSERT("Invalid value for blockcode:"
                          " not a valid unsigned number with at most eight digits",
                          parseUnsigned<8>(fields.block_code, childval));
              break;
             case KV6F_WHEELCHAIR_ACCESSIBLE:
              PWARNASSERT("Invalid value for wheelchairaccessible:"
                          " not a valid value for wheelchair accessibility",
                          childval == "ACCESSIBLE"
                       || childval == "NOTACCESSIBLE"
                       || childval == "UNKNOWN");
              fields.wheelchair_accessible = childval;
              break;
             case KV6F_NUMBER_OF_COACHES:
              PWARNASSERT("Invalid for numberofcoaches:"
                          " not a valid unsigned number with at most two digits",
                          parseUnsigned<2>(fields.number_of_coaches, childval));
              break;
             case KV6F_RD_X:
              PWARNASSERT("Invalid value for rd-x:"
                          " not a valid signed number with at most six digits",
                          parseSigned<6>(fields.rd_x, childval));
              break;
             case KV6F_RD_Y:
              PWARNASSERT("Invalid value for rd-y:"
                          " not a valid signed number with at most six digits",
                          parseSigned<6>(fields.rd_y, childval));
              break;
             case KV6F_DISTANCE_SINCE_LAST_USER_STOP:
              PWARNASSERT("Invalid value for distancesincelastuserstop:"
                          " not a valid unsigned number with at most five digits",
                          parseUnsigned<5>(fields.distance_since_last_user_stop, childval));
              break;
             case KV6F_NONE:
              error("NONE field type case should be unreachable in parseKv6PosInfoRecord");
              return;
            }
            fields.markPresent(field);
            break;
          }
        }
      });
    }

    fields.removeUnsupportedFields();

    if (!fields.valid())
      return std::nullopt;
    return fields;
  }

  std::vector<Kv6Record> parseKv6PosInfo(const rapidxml::xml_node<> &node, const Xmlns *nss) {
    std::vector<Kv6Record> records;
    for (const rapidxml::xml_node<> *child = node.first_node(); child; child = child->next_sibling()) {
      ifTmi8Element(*child, nss, [&](std::string_view name, const Xmlns *nss) {
        for (auto type = _KV6T_FIRST_TYPE;
             type != _KV6T_LAST_TYPE;
             type = static_cast<Kv6RecordType>(type + 1)) {
          if (type == KV6T_UNKNOWN)
            continue;
          if (KV6_POS_INFO_RECORD_TYPES[type] == name) {
            auto record = parseKv6PosInfoRecord(type, *child, nss);
            if (record) {
              records.push_back(*record);
            }
          }
        }
      });
    }
    return records;
  }

  std::optional<Tmi8VvTmPushInfo> parseVvTmPush(const rapidxml::xml_node<> &node, const Xmlns *nss) {
    Tmi8VvTmPushInfo info;
    for (const rapidxml::xml_node<> *child = node.first_node(); child; child = child->next_sibling()) {
      ifTmi8Element(*child, nss, [&](std::string_view name, const Xmlns *nss) {
        if (name == "Timestamp") {
          PERRASSERT("Invalid value for Timestamp: Bad format", onlyTextElement(*child));
          PERRASSERT("Invalid value for Timestamp: Invalid timestamp", Timestamp::parse(info.timestamp, getValue(*child)));
          info.markPresent(TMI8F_TIMESTAMP);
        } else if (name == "SubscriberID") {
          PERRASSERT("Invalid value for SubscriberID: Bad format", onlyTextElement(*child));
          info.subscriber_id = getValue(*child);
          info.markPresent(TMI8F_SUBSCRIBER_ID);
        } else if (name == "Version") {
          PERRASSERT("Invalid value for Version: Bad format", onlyTextElement(*child));
          info.version = getValue(*child);
          info.markPresent(TMI8F_VERSION);
        } else if (name == "DossierName") {
          PERRASSERT("Invalid value for DossierName: Bad format", onlyTextElement(*child));
          info.dossier_name = getValue(*child);
          info.markPresent(TMI8F_DOSSIER_NAME);
        } else if (name == "KV6posinfo") {
          info.messages = parseKv6PosInfo(*child, nss);
        }
      });
    }
    
    if (!info.valid())
      return std::nullopt;
    return info;
  }

  std::optional<Tmi8VvTmPushInfo> parse(const rapidxml::xml_document<> &doc) {
    std::optional<Tmi8VvTmPushInfo> msg;
    for (const rapidxml::xml_node<> *node = doc.first_node(); node; node = node->next_sibling()) {
      ifTmi8Element(*node, nullptr /* nss */, [&](std::string_view name, const Xmlns *nss) {
        if (name == "VV_TM_PUSH") {
          if (msg) {
            error("Duplicated VV_TM_PUSH");
            return;
          }
          msg = parseVvTmPush(*node, nss);
          if (!msg) {
            error("Invalid VV_TM_PUSH");
          }
        }
      });
    }
    if (!msg)
      error("Expected to find VV_TM_PUSH");
    return msg;
  }
};

std::optional<Tmi8VvTmPushInfo> parseXml(const rapidxml::xml_document<> &doc, std::stringstream &errs, std::stringstream &warns) {
  Kv6Parser parser = { errs, warns };
  return parser.parse(doc);
}

struct Metrics {
  prometheus::Counter   &messages_counter_ok;
  prometheus::Counter   &messages_counter_error;
  prometheus::Counter   &messages_counter_warning;
  prometheus::Counter   &rows_written_counter;
  prometheus::Histogram &records_hist;
  prometheus::Histogram &message_parse_hist;
  prometheus::Histogram &payload_size_hist;

  using BucketBoundaries = prometheus::Histogram::BucketBoundaries;

  enum class ParseStatus {
    OK,
    WARNING,
    ERROR,
  };

  Metrics(std::shared_ptr<prometheus::Registry> registry) :
    Metrics(registry, prometheus::BuildCounter()
      .Name("kv6_vv_tm_push_messages_total")
      .Help("Number of KV6 VV_TM_PUSH messages received")
      .Register(*registry))
  {}

  void addMeasurement(std::chrono::duration<double> took_secs, size_t payload_size, size_t records, ParseStatus parsed) {
    double millis = took_secs.count() * 1000.0;

    if (parsed == ParseStatus::OK)           messages_counter_ok.Increment();
    else if (parsed == ParseStatus::WARNING) messages_counter_warning.Increment();
    else if (parsed == ParseStatus::ERROR)   messages_counter_error.Increment();
    records_hist.Observe(static_cast<double>(records));
    message_parse_hist.Observe(millis);
    payload_size_hist.Observe(static_cast<double>(payload_size));
  }

  void rowsWritten(int64_t rows) {
    rows_written_counter.Increment(static_cast<double>(rows));
  }

 private:
  Metrics(std::shared_ptr<prometheus::Registry> registry,
          prometheus::Family<prometheus::Counter> &messages_counter) :
    messages_counter_ok(messages_counter
      .Add({{ "status", "ok" }})),
    messages_counter_error(messages_counter
      .Add({{ "status", "error" }})),
    messages_counter_warning(messages_counter
      .Add({{ "status", "warning" }})),
    rows_written_counter(prometheus::BuildCounter()
      .Name("kv6_vv_tm_push_records_written")
      .Help("Numer of VV_TM_PUSH records written to disk")
      .Register(*registry)
      .Add({})),
    records_hist(prometheus::BuildHistogram()
      .Name("kv6_vv_tm_push_records_amount")
      .Help("Number of KV6 VV_TM_PUSH records")
      .Register(*registry)
      .Add({}, BucketBoundaries{ 5.0, 10.0, 20.0, 50.0, 100.0, 250.0, 500.0 })),
    message_parse_hist(prometheus::BuildHistogram()
      .Name("kv6_vv_tm_push_message_parse_millis")
      .Help("Milliseconds taken to parse KV6 VV_TM_PUSH messages")
      .Register(*registry)
      .Add({}, BucketBoundaries{ 0.25, 0.5, 1.0, 2.5, 5.0, 10.0, 100.0, 1000.0, 2000.0 })),
    payload_size_hist(prometheus::BuildHistogram()
      .Name("kv6_payload_size")
      .Help("Sizes of KV6 ZeroMQ message payloads")
      .Register(*registry)
      .Add({}, BucketBoundaries{ 500.0, 1000.0, 2500.0, 5000.0, 10000.0, 25000.0, 50000.0 }))
  {}
};

// Note: it *must* hold that decompressed[size] == 0
std::optional<Tmi8VvTmPushInfo> parseMsg(char *decompressed, size_t size, Metrics &metrics, std::stringstream &errs, std::stringstream &warns) {
  auto start = std::chrono::steady_clock::now();

  std::optional<Tmi8VvTmPushInfo> info;

  if (decompressed[size] != 0) {
    errs << "Not parsing: missing null terminator" << '\n';
  } else {
    rapidxml::xml_document<> doc;
    constexpr int PARSE_FLAGS = rapidxml::parse_trim_whitespace
                              | rapidxml::parse_no_string_terminators
                              | rapidxml::parse_validate_closing_tags;

    try {
      doc.parse<PARSE_FLAGS>(decompressed);
      info = parseXml(doc, errs, warns);
    } catch (const rapidxml::parse_error &err) {
      errs << "XML parsing failed" << '\n';
    }
  }

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> took = end - start;

  if (info)
    if (warns.view().empty())
      metrics.addMeasurement(took, size, info->messages.size(), Metrics::ParseStatus::OK);
    else
      metrics.addMeasurement(took, size, info->messages.size(), Metrics::ParseStatus::WARNING);
  else
    metrics.addMeasurement(took, size, 0, Metrics::ParseStatus::ERROR);

  return info;
}

bool terminate = false;

void onSigIntOrTerm(int /* signum */) {
  terminate = true;
}

arrow::Result<std::shared_ptr<arrow::Table>> getTable(const std::vector<Kv6Record> &messages, size_t &rows_written) {
  ParquetBuilder builder;

  for (const auto &msg : messages) {
    Kv6Field present = msg.presence;
    Kv6Field required = KV6T_REQUIRED_FIELDS[msg.type];
    Kv6Field optional = KV6T_OPTIONAL_FIELDS[msg.type];
    if ((~msg.presence & required) != 0) {
      std::cout << "Invalid message: not all required fields present; skipping" << std::endl;
      continue;
    }
    Kv6Field used = static_cast<Kv6Field>(present & (required | optional));
    rows_written++;

    // RD-X and RD-Y fix: some datatypes have these fields marked as required, but still give option
    // of not providing these fields by setting them to -1. We want this normalized, where these
    // fields are instead simply marked as not present.
    if ((used & KV6F_RD_X) && msg.rd_x == -1)
      used = static_cast<Kv6Field>(used & ~KV6F_RD_X);
    if ((used & KV6F_RD_Y) && msg.rd_y == -1)
      used = static_cast<Kv6Field>(used & ~KV6F_RD_Y);

    ARROW_RETURN_NOT_OK(builder.types.Append(*findKv6PosInfoRecordTypeName(msg.type)));
    ARROW_RETURN_NOT_OK(used & KV6F_DATA_OWNER_CODE
                        ? builder.data_owner_codes.Append(msg.data_owner_code)
                        : builder.data_owner_codes.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_LINE_PLANNING_NUMBER
                        ? builder.line_planning_numbers.Append(msg.line_planning_number)
                        : builder.line_planning_numbers.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_OPERATING_DAY
                        ? builder.operating_days.Append(static_cast<int32_t>(msg.operating_day.toUnixDays().count()))
                        : builder.operating_days.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_JOURNEY_NUMBER
                        ? builder.journey_numbers.Append(msg.journey_number)
                        : builder.journey_numbers.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_REINFORCEMENT_NUMBER
                        ? builder.reinforcement_numbers.Append(msg.reinforcement_number)
                        : builder.reinforcement_numbers.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_TIMESTAMP
                        ? builder.timestamps.Append(msg.timestamp.toUnixSeconds().count())
                        : builder.timestamps.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_SOURCE
                        ? builder.sources.Append(msg.source)
                        : builder.sources.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_PUNCTUALITY
                        ? builder.punctualities.Append(msg.punctuality)
                        : builder.punctualities.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_USER_STOP_CODE
                        ? builder.user_stop_codes.Append(msg.user_stop_code)
                        : builder.user_stop_codes.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_PASSAGE_SEQUENCE_NUMBER
                        ? builder.passage_sequence_numbers.Append(msg.passage_sequence_number)
                        : builder.passage_sequence_numbers.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_VEHICLE_NUMBER
                        ? builder.vehicle_numbers.Append(msg.vehicle_number)
                        : builder.vehicle_numbers.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_BLOCK_CODE
                        ? builder.block_codes.Append(msg.block_code)
                        : builder.block_codes.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_WHEELCHAIR_ACCESSIBLE
                        ? builder.wheelchair_accessibles.Append(msg.wheelchair_accessible)
                        : builder.wheelchair_accessibles.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_NUMBER_OF_COACHES
                        ? builder.number_of_coaches.Append(msg.number_of_coaches)
                        : builder.number_of_coaches.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_RD_Y
                        ? builder.rd_ys.Append(msg.rd_y)
                        : builder.rd_ys.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_RD_X
                        ? builder.rd_xs.Append(msg.rd_x)
                        : builder.rd_xs.AppendNull());
    ARROW_RETURN_NOT_OK(used & KV6F_DISTANCE_SINCE_LAST_USER_STOP
                        ? builder.distance_since_last_user_stops.Append(msg.distance_since_last_user_stop)
                        : builder.distance_since_last_user_stops.AppendNull());
  }

  return builder.getTable();
}

std::tuple<int64_t, int64_t> getMinMaxTimestamp(const std::vector<Kv6Record> &messages) {
  if (messages.size() == 0)
    return { 0, 0 };
  int64_t min = std::numeric_limits<int64_t>::max();
  int64_t max = 0;
  for (const auto &message : messages) {
    if (~message.presence & KV6F_TIMESTAMP)
      continue;
    int64_t seconds = message.timestamp.toUnixSeconds().count();
    if (seconds < min)
      min = seconds;
    if (seconds > max)
      max = seconds;
  }
  if (min == std::numeric_limits<decltype(min)>::max())
    return { 0, 0 };  // this is stupid
  return { min, max };
}

arrow::Status writeParquet(const std::vector<Kv6Record> &messages, Metrics &metrics) {
  size_t rows_written = 0;
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Table> table, getTable(messages, rows_written));

  auto timestamp = std::chrono::round<std::chrono::seconds>(std::chrono::utc_clock::now());
  std::string filename = std::format("oeuf-{:%FT%T%Ez}.parquet", timestamp);
  ARROW_RETURN_NOT_OK(writeArrowTableAsParquetFile(*table, filename));
  std::cout << "Wrote Parquet file " << filename << std::endl;

  auto [min_timestamp, max_timestamp] = getMinMaxTimestamp(messages);
  std::ofstream metaf(filename + ".meta.json.part", std::ios::binary);
  nlohmann::json meta{
    { "min_timestamp", min_timestamp },
    { "max_timestamp", max_timestamp },
    { "rows_written",  rows_written  },
  };
  metaf << meta;
  metaf.close();
  std::filesystem::rename(filename + ".meta.json.part", filename + ".meta.json");

  metrics.rowsWritten(rows_written);

  return arrow::Status::OK();
}

using SteadyTime = std::chrono::steady_clock::time_point;

std::string dumpFailedMsg(std::string_view txt, std::string_view errs, std::string_view warns) {
  auto timestamp = std::chrono::round<std::chrono::seconds>(std::chrono::utc_clock::now());
  std::string filename = std::format("oeuf-error-{:%FT%T%Ez}.txt", timestamp);
  std::ofstream dumpf(filename, std::ios::binary);
  dumpf << "======= ERROR MESSAGES ========" << std::endl;
  dumpf << errs;
  dumpf << "======= WARNING MESSAGES ======" << std::endl;
  dumpf << warns;
  dumpf << "======= RECEIVED MESSAGE ======" << std::endl;
  dumpf << txt << std::endl;
  dumpf.close();
  return filename;
}

void handleMsg(RawMessage &msg, Metrics &metrics, SteadyTime &last_output, std::vector<Kv6Record> &msg_buf) {
  unsigned int decompressed_size = 0;
  if (msg.getBodySize() > std::numeric_limits<unsigned int>::max())
    std::cout << "parseMsg failed due to too large message" << std::endl;
  char *decompressed = decompress(msg.getBody(), static_cast<unsigned int>(msg.getBodySize()), decompressed_size);

  std::stringstream errs;
  std::stringstream warns;
  // We know that decompressed[decompressed_size] == 0 because decompress() ensures this.
  auto parsed_msg = parseMsg(decompressed, decompressed_size, metrics, errs, warns);
  if (parsed_msg) {
    const Tmi8VvTmPushInfo &info = *parsed_msg;
    auto new_msgs_it = info.messages.begin();
    while (new_msgs_it != info.messages.end()) {
      size_t remaining_space = MAX_PARQUET_CHUNK - msg_buf.size();
      size_t new_msgs_left   = info.messages.end() - new_msgs_it;
      auto   new_msgs_start  = new_msgs_it;
      auto   new_msgs_end    = new_msgs_start + std::min(remaining_space, new_msgs_left);
      new_msgs_it = new_msgs_end;
      msg_buf.insert(msg_buf.end(), new_msgs_start, new_msgs_end);

      bool time_expired = std::chrono::steady_clock::now() - last_output > std::chrono::minutes(5);
      if (msg_buf.size() >= MAX_PARQUET_CHUNK || (new_msgs_it == info.messages.end() && time_expired)) {
        arrow::Status status = writeParquet(msg_buf, metrics);
        if (!status.ok())
          std::cout << "Writing Parquet file failed: " << status << std::endl;
        msg_buf.clear();
        last_output = std::chrono::steady_clock::now();
      }
    }
    if (!errs.view().empty() || !warns.view().empty()) {
      std::filesystem::path dump_file = dumpFailedMsg(std::string_view(decompressed, decompressed_size), errs.str(), warns.str());
      std::cout << "parseMsg finished with warnings: details dumped to " << dump_file << std::endl;
    }
  } else {
    std::filesystem::path dump_file = dumpFailedMsg(std::string_view(decompressed, decompressed_size), errs.str(), warns.str());
    std::cout << "parseMsg failed: error details dumped to " << dump_file << std::endl;
  }
  free(decompressed);
}

int main(int argc, char *argv[]) {
  std::cout << "Working directory: " << std::filesystem::current_path() << std::endl;

  const char *metrics_addr = getenv("METRICS_ADDR");
  if (!metrics_addr || strlen(metrics_addr) == 0) {
    std::cout << "Error: no METRICS_ADDR set!" << std::endl;
    exit(EXIT_FAILURE);
  }
  prometheus::Exposer exposer{metrics_addr};

  bool prod = false;
  const char *prod_env = getenv("NDOV_PRODUCTION");
  if (prod_env && strcmp(prod_env, "true") == 0) prod = true;

  void *zmq_context = zmq_ctx_new();
  void *zmq_subscriber = zmq_socket(zmq_context, ZMQ_SUB);
  int rc = zmq_connect(zmq_subscriber, prod ? "tcp://pubsub.ndovloket.nl:7658" : "tcp://pubsub.besteffort.ndovloket.nl:7658");
  assert(rc == 0);

  const char *topic = "/CXX/KV6posinfo";
  rc = zmq_setsockopt(zmq_subscriber, ZMQ_SUBSCRIBE, topic, strlen(topic));
  assert(rc == 0);

  signal(SIGINT,  onSigIntOrTerm);
  signal(SIGTERM, onSigIntOrTerm);

  SteadyTime last_output = std::chrono::steady_clock::now();

  auto registry = std::make_shared<prometheus::Registry>();
  Metrics metrics(registry);
  exposer.RegisterCollectable(registry);

  std::vector<Kv6Record> msg_buf;
  while (!terminate) {
    std::optional<RawMessage> msg = recvMsg(zmq_subscriber);
    if (!msg) {
      if (!terminate)
        perror("recvMsg");
      continue;
    }
    handleMsg(*msg, metrics, last_output, msg_buf);
  }

  std::cout << "Terminating" << std::endl;
  if (msg_buf.size() > 0) {
    arrow::Status status = writeParquet(msg_buf, metrics);
    if (!status.ok()) std::cout << "Writing final Parquet file failed: " << status << std::endl;
    else std::cout << "Final data written" << std::endl;
    msg_buf.clear();
  }

  if (zmq_close(zmq_subscriber))
    perror("zmq_close");
  if (zmq_ctx_destroy(zmq_context))
    perror("zmq_ctx_destroy");

  std::cout << "Bye" << std::endl;

  return 0;
}
