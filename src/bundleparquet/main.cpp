// vim:set sw=2 ts=2 sts et:

#include <chrono>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include <nlohmann/json.hpp>

#include <prometheus/counter.h>
#include <prometheus/gateway.h>
#include <prometheus/registry.h>

#include <tmi8/kv6_parquet.hpp>

#include "spliturl.hpp"

static const int MIN_COMBINED_ROWS = 1000000;  // one million
static const int MAX_COMBINED_ROWS = 2000000;  // two million

struct FileMetadata {
  int64_t min_timestamp = 0;
  int64_t max_timestamp = 0;
  int64_t rows_written  = 0;
};

struct File {
  FileMetadata metadata;
  std::filesystem::path filename;
};

FileMetadata readMetadataOf(std::filesystem::path filename) {
  std::string meta_filename = std::string(filename) + ".meta.json";
  std::ifstream meta_file = std::ifstream(meta_filename, std::ifstream::in|std::ifstream::binary);
  nlohmann::json meta_json;
  meta_file >> meta_json;
  FileMetadata meta = {
    .min_timestamp = meta_json["min_timestamp"],
    .max_timestamp = meta_json["max_timestamp"],
    .rows_written  = meta_json["rows_written"],
  };
  return meta;
}

arrow::Status processFirstTables(std::deque<File> &files, prometheus::Counter &rows_written) {
  if (files.size() == 0) {
    std::cerr << "Did not find any files" << std::endl;
    return arrow::Status::OK();
  }

  int64_t rows = 0;

  std::vector<std::shared_ptr<arrow::Table>> tables;
  std::vector<std::filesystem::path> processed;
  int64_t min_timestamp = std::numeric_limits<int64_t>::max();
  int64_t max_timestamp = 0;

  bool over_capacity_risk = false;
  auto it = files.begin();
  while (it != files.end()) {
    const std::filesystem::path &filename = it->filename;
    const FileMetadata &metadata = it->metadata;

    std::shared_ptr<arrow::io::RandomAccessFile> input;
    ARROW_ASSIGN_OR_RAISE(input, arrow::io::ReadableFile::Open(filename));

    std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
    ARROW_RETURN_NOT_OK(parquet::arrow::OpenFile(input, arrow::default_memory_pool(), &arrow_reader));

    if (metadata.min_timestamp < min_timestamp)
      min_timestamp = metadata.min_timestamp;
    if (metadata.max_timestamp > max_timestamp)
      max_timestamp = metadata.max_timestamp;

    if (rows + metadata.rows_written > MAX_COMBINED_ROWS) {
      over_capacity_risk = true;
      break;
    }

    std::shared_ptr<arrow::Table> table;
    ARROW_RETURN_NOT_OK(arrow_reader->ReadTable(&table));
    tables.push_back(table);
    processed.push_back(filename);
    rows += metadata.rows_written;
    it = files.erase(it);
  }

  if (rows < MIN_COMBINED_ROWS && !over_capacity_risk) {
    std::cerr << "Found files, but not enough to satisfy the minimum amount of rows for the combined file" << std::endl;
    std::cerr << "(We have " << rows << "/" << MIN_COMBINED_ROWS << " rows at the moment, so "
              << static_cast<float>(rows)/static_cast<float>(MIN_COMBINED_ROWS)*100.f << "%)" << std::endl;
    return arrow::Status::OK();
  } else if (rows == 0 && over_capacity_risk) {
    const std::filesystem::path &filename = files.front().filename;
    std::filesystem::rename(filename, "merged" / filename);
    std::filesystem::rename(std::string(filename) + ".meta.json", std::string("merged" / filename) + ".meta.json");
    rows_written.Increment(static_cast<double>(files.front().metadata.rows_written));
    files.pop_front();
    return arrow::Status::OK();
  }

  // Default options specify that the schemas are not unified, which is
  // luckliy exactly what we want :)
  std::shared_ptr<arrow::Table> merged_table;
  ARROW_ASSIGN_OR_RAISE(merged_table, arrow::ConcatenateTables(tables));

  auto timestamp = std::chrono::round<std::chrono::seconds>(std::chrono::system_clock::now());
  std::string filename = std::format("merged/oeuf-{:%FT%T%Ez}.parquet", timestamp);
  ARROW_RETURN_NOT_OK(writeArrowTableAsParquetFile(*merged_table, filename));
  
  std::cerr << "Wrote merged table to " << filename << std::endl;

  std::ofstream metaf(filename + ".meta.json.part", std::ios::binary);
  nlohmann::json meta{
    { "min_timestamp", min_timestamp },
    { "max_timestamp", max_timestamp },
    { "rows_written",  rows          },
  };
  metaf << meta;
  metaf.close();
  std::filesystem::rename(filename + ".meta.json.part", filename + ".meta.json");

  std::cerr << "Wrote merged table metadata" << std::endl;
  rows_written.Increment(static_cast<double>(rows));

  for (const std::filesystem::path &filename : processed) {
    std::filesystem::remove(filename);
    std::filesystem::remove(std::string(filename) + ".meta.json");
  }

  std::cerr << "Successfully wrote merged table, metadata and deleted old files" << std::endl;

  return arrow::Status::OK();
}

arrow::Status processTables(std::deque<File> &files, prometheus::Counter &rows_written) {
  while (!files.empty())
    ARROW_RETURN_NOT_OK(processFirstTables(files, rows_written));
  return arrow::Status::OK();
}

int main(int argc, char *argv[]) {
  std::filesystem::path cwd = std::filesystem::current_path();
  std::filesystem::create_directory(cwd / "merged");

  const char *prom_push_url = getenv("PROMETHEUS_PUSH_URL");
  if (!prom_push_url || strlen(prom_push_url) == 0) {
    std::cerr << "Error: no PROMETHEUS_PUSH_URL set!" << std::endl;
    return EXIT_FAILURE;
  }

  std::string split_err;
  auto split_prom_push_url = splitUrl(prom_push_url, &split_err);
  if (!split_prom_push_url) {
    std::cerr << "Could not process URL in environment variable PROMETHEUS_PUSH_URL: "
              << split_err << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Prometheus Push URL: " << split_prom_push_url->schemehost << ":"
                                       << split_prom_push_url->portpath << std::endl;

  prometheus::Gateway gateway{split_prom_push_url->schemehost,
                              split_prom_push_url->portpath,
                              "oeuf-archiver"};

  auto registry = std::make_shared<prometheus::Registry>();
  prometheus::Gauge &rows_available = prometheus::BuildGauge()
    .Name("archiver_rows_available")
    .Help("Number of rows available to the archiver")
    .Register(*registry)
    .Add({});
  prometheus::Counter &rows_written = prometheus::BuildCounter()
    .Name("archiver_rows_written")
    .Help("Number of rows written by the archiver")
    .Register(*registry)
    .Add({});
  gateway.RegisterCollectable(registry);

  std::deque<File> files;
  for (auto const &dir_entry : std::filesystem::directory_iterator{cwd}) {
    if (!dir_entry.is_regular_file()) continue;
    std::filesystem::path filename = dir_entry.path().filename();
    const std::string &filename_str = filename;
    if (filename_str.starts_with("oeuf-") && filename_str.ends_with("+00:00.parquet")) {
      try {
        FileMetadata meta = readMetadataOf(filename);
        File file = { .metadata = meta, .filename = filename };
        files.push_back(file);

        rows_available.Increment(static_cast<double>(meta.rows_written));
      } catch (const std::exception &e) {
        std::cerr << "Failed to read metadata of file " << filename << ": " << e.what() << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  std::sort(files.begin(), files.end(),
            [](const File &f1, const File &f2) { return f1.filename < f2.filename; });
  arrow::Status st = processTables(files, rows_written);
  if (!st.ok()) {
    std::cerr << "Failed to process tables: " << st << std::endl;
    return EXIT_FAILURE;
  }

  gateway.Push();
}
