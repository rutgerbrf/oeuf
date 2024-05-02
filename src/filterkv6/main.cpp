// vim:set sw=2 ts=2 sts et:

#include <chrono>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/filesystem/api.h>
#include <arrow/dataset/api.h>
#include <arrow/io/api.h>

#include <tmi8/kv6_parquet.hpp>

namespace ds = arrow::dataset;
namespace cp = arrow::compute;
using namespace arrow;

arrow::Status processTables(std::string lineno) {
  auto filesystem = std::make_shared<fs::LocalFileSystem>();

  fs::FileSelector selector;
  selector.base_dir = std::filesystem::current_path();
  selector.recursive = false;

  auto format = std::static_pointer_cast<ds::FileFormat>(std::make_shared<ds::ParquetFileFormat>());

  ARROW_ASSIGN_OR_RAISE(auto factory,
    ds::FileSystemDatasetFactory::Make(filesystem, selector, format,
      ds::FileSystemFactoryOptions()));

  ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());

  printf("Scanning dataset for line %s...\n", lineno.c_str());
  // Read specified columns with a row filter
  ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
  ARROW_RETURN_NOT_OK(scan_builder->Filter(cp::and_({
    cp::equal(cp::field_ref("line_planning_number"), cp::literal(lineno)),
    cp::is_valid(cp::field_ref("rd_x")),
    cp::is_valid(cp::field_ref("rd_y")),
  })));

  ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
  ARROW_ASSIGN_OR_RAISE(auto table, scanner->ToTable());

  puts("Finished loading data, computing stable sort indices...");

  arrow::Datum sort_indices;
  cp::SortOptions sort_options;
  sort_options.sort_keys = { cp::SortKey("timestamp" /* ascending by default */) };
  ARROW_ASSIGN_OR_RAISE(sort_indices, cp::CallFunction("sort_indices", { table }, &sort_options));
  puts("Finished computing stable sort indices, creating sorted table...");

  arrow::Datum sorted;
  ARROW_ASSIGN_OR_RAISE(sorted, cp::CallFunction("take", { table, sort_indices }));

  puts("Writing sorted table to disk...");
  ARROW_RETURN_NOT_OK(writeArrowTableAsParquetFile(*sorted.table(), "merged/oeuf-merged.parquet"));
  puts("Syncing...");
  sync();
  puts("Done. Have a nice day.");

  return arrow::Status::OK();
}

#define NOTICE "Notice: This tool will fail if any non-Parquet files in are present in the\n" \
               "        current working directory. It does not load files which are present in\n" \
               "        any possible subdirectories."

const char help[] =
  "Usage: %s <LINENO>\n"
  "\n"
  "  LINENO  The LinePlanningNumber as in the KV1/KV6 data\n\n"
  NOTICE "\n";

void exitHelp(const char *progname, int code = 1) {
  printf(help, progname);
  exit(code);
}

int main(int argc, char *argv[]) {
  const char *progname = argv[0];
  if (argc != 2) {
    puts("Error: incorrect number of arguments provided\n");
    exitHelp(progname);
  }
  char *lineno = argv[1];
  puts(NOTICE "\n");

  std::filesystem::path cwd = std::filesystem::current_path();
  std::filesystem::create_directory(cwd / "merged");

  puts("Running this program may take a while, especially on big datasets. If you're\n"
       "processing the data of a single bus line over the course of multiple months,\n"
       "you may see memory usage of up to 10 GiB. Make sure that you have sufficient\n"
       "RAM available, to avoid overloading and subsequently freezing your system.\n");

  arrow::Status st = processTables(std::string(lineno));
  if (!st.ok()) {
    std::cerr << "Failed to process tables: " << st << std::endl;
    return EXIT_FAILURE;
  }
}
