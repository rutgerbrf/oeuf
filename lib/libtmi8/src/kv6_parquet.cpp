// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <tmi8/kv6_parquet.hpp>

ParquetBuilder::ParquetBuilder() {
  std::shared_ptr<arrow::Field> field_type, field_data_owner_code, field_line_planning_number, field_operating_day,
                                field_journey_number, field_reinforcement_number, field_timestamp, field_source,
                                field_punctuality, field_user_stop_code, field_passage_sequence_number,
                                field_vehicle_number, field_block_code, field_wheelchair_accessible,
                                field_number_of_coaches, field_rd_y, field_rd_x, field_distance_since_last_user_stop;
  field_type                          = arrow::field("type", arrow::utf8());
  field_data_owner_code               = arrow::field("data_owner_code", arrow::utf8());
  field_line_planning_number          = arrow::field("line_planning_number", arrow::utf8());
  field_operating_day                 = arrow::field("operating_day", arrow::date32());
  field_journey_number                = arrow::field("journey_number", arrow::uint32());
  field_reinforcement_number          = arrow::field("reinforcement_number", arrow::uint8());
  field_timestamp                     = arrow::field("timestamp", arrow::timestamp(arrow::TimeUnit::SECOND));
  field_source                        = arrow::field("source", arrow::utf8());
  field_punctuality                   = arrow::field("punctuality", arrow::int16());
  field_user_stop_code                = arrow::field("user_stop_code", arrow::utf8());
  field_passage_sequence_number       = arrow::field("passage_sequence_number", arrow::uint16());
  field_vehicle_number                = arrow::field("vehicle_number", arrow::uint32());
  field_block_code                    = arrow::field("block_code", arrow::uint32());
  field_wheelchair_accessible         = arrow::field("wheelchair_accessible", arrow::utf8());
  field_number_of_coaches             = arrow::field("number_of_coaches", arrow::uint8());
  field_rd_y                          = arrow::field("rd_y", arrow::int32());
  field_rd_x                          = arrow::field("rd_x", arrow::int32());
  field_distance_since_last_user_stop = arrow::field("distance_since_last_user_stop", arrow::uint32());

  schema = arrow::schema({ field_type, field_data_owner_code, field_line_planning_number,
                           field_operating_day, field_journey_number,
                           field_reinforcement_number, field_timestamp, field_source,
                           field_punctuality, field_user_stop_code,
                           field_passage_sequence_number, field_vehicle_number,
                           field_block_code, field_wheelchair_accessible,
                           field_number_of_coaches, field_rd_y, field_rd_x,
                           field_distance_since_last_user_stop });
}

arrow::Result<std::shared_ptr<arrow::Table>> ParquetBuilder::getTable() {
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> types,                          types.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> data_owner_codes,               data_owner_codes.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> line_planning_numbers,          line_planning_numbers.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> operating_days,                 operating_days.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> journey_numbers,                journey_numbers.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> reinforcement_numbers,          reinforcement_numbers.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> timestamps,                     timestamps.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> sources,                        sources.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> punctualities,                  punctualities.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> user_stop_codes,                user_stop_codes.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> passage_sequence_numbers,       passage_sequence_numbers.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> vehicle_numbers,                vehicle_numbers.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> block_codes,                    block_codes.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> wheelchair_accessibles,         wheelchair_accessibles.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> number_of_coaches,              number_of_coaches.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> rd_ys,                          rd_ys.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> rd_xs,                          rd_xs.Finish());
  ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Array> distance_since_last_user_stops, distance_since_last_user_stops.Finish());

  std::vector<std::shared_ptr<arrow::Array>> columns = { types, data_owner_codes, line_planning_numbers, operating_days,
                                                         journey_numbers, reinforcement_numbers, timestamps, sources,
                                                         punctualities, user_stop_codes, passage_sequence_numbers,
                                                         vehicle_numbers, block_codes, wheelchair_accessibles,
                                                         number_of_coaches, rd_ys, rd_xs,
                                                         distance_since_last_user_stops };
  return arrow::Result(arrow::Table::Make(schema, columns));
}

arrow::Status writeArrowRecordsAsParquetFile(arrow::RecordBatchReader &rbr, std::filesystem::path filename) {
  std::shared_ptr<parquet::WriterProperties> props = parquet::WriterProperties::Builder()
    .compression(arrow::Compression::ZSTD)
    ->created_by("oeuf-libtmi8")
    ->version(parquet::ParquetVersion::PARQUET_2_6)
    ->data_page_version(parquet::ParquetDataPageVersion::V2)
    ->max_row_group_length(MAX_PARQUET_CHUNK)
    ->build();

  std::shared_ptr<parquet::ArrowWriterProperties> arrow_props = parquet::ArrowWriterProperties::Builder()
    .store_schema()->build();

  std::shared_ptr<arrow::io::FileOutputStream> out_file;
  std::string filename_str = filename;
  ARROW_ASSIGN_OR_RAISE(out_file, arrow::io::FileOutputStream::Open(filename_str + ".part"));

  ARROW_ASSIGN_OR_RAISE(auto writer,
    parquet::arrow::FileWriter::Open(*rbr.schema(), arrow::default_memory_pool(), out_file, props, arrow_props));
  for (const auto &batchr : rbr) {
    ARROW_ASSIGN_OR_RAISE(auto batch, batchr);
    ARROW_RETURN_NOT_OK(writer->WriteRecordBatch(*batch));
  }
  ARROW_RETURN_NOT_OK(writer->Close());
  ARROW_RETURN_NOT_OK(out_file->Close());

  std::filesystem::rename(filename_str + ".part", filename);

  return arrow::Status::OK();
}

arrow::Status writeArrowTableAsParquetFile(const arrow::Table &table, std::filesystem::path filename) {
  auto tbr = arrow::TableBatchReader(table);
  return writeArrowRecordsAsParquetFile(tbr, filename);
}
