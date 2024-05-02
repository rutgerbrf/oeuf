// vim:set sw=2 ts=2 sts et:

#ifndef OEUF_LIBTMI8_KV6_PARQUET_HPP
#define OEUF_LIBTMI8_KV6_PARQUET_HPP

#include <filesystem>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>

static const size_t MAX_PARQUET_CHUNK = 10000;

struct ParquetBuilder {
  ParquetBuilder();
  arrow::Result<std::shared_ptr<arrow::Table>> getTable();

  std::shared_ptr<arrow::Schema> schema;

  arrow::StringBuilder    types;
  arrow::StringBuilder    data_owner_codes;
  arrow::StringBuilder    line_planning_numbers;
  arrow::Date32Builder    operating_days;
  arrow::UInt32Builder    journey_numbers;
  arrow::UInt8Builder     reinforcement_numbers;
  arrow::TimestampBuilder timestamps{arrow::timestamp(arrow::TimeUnit::SECOND), arrow::default_memory_pool()};
  arrow::StringBuilder    sources;
  arrow::Int16Builder     punctualities;
  arrow::StringBuilder    user_stop_codes;
  arrow::UInt16Builder    passage_sequence_numbers;
  arrow::UInt32Builder    vehicle_numbers;
  arrow::UInt32Builder    block_codes;
  arrow::StringBuilder    wheelchair_accessibles;
  arrow::UInt8Builder     number_of_coaches;
  arrow::Int32Builder     rd_ys;
  arrow::Int32Builder     rd_xs;
  arrow::UInt32Builder    distance_since_last_user_stops;
};

[[nodiscard]]
arrow::Status writeArrowRecordsAsParquetFile(arrow::RecordBatchReader &rbr, std::filesystem::path filename);

[[nodiscard]]
arrow::Status writeArrowTableAsParquetFile(const arrow::Table &table, std::filesystem::path filename);

#endif // OEUF_LIBTMI8_KV6_PARQUET_HPP
