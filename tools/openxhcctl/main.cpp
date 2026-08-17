// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/trace.hpp"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {
constexpr std::string_view kTraceHeader = "OPENXHC_TRACE_V1";

enum class ExitCode : int { Success = 0, Usage = 2, Open = 3, Parse = 4 };
enum class ReadResult { Success, Open, Parse };

void print_usage() {
  std::cerr << "usage: openxhcctl trace validate <input.xhctrace>\n"
            << "       openxhcctl trace summary <input.xhctrace>\n"
            << "       openxhcctl trace import-tshark <input.tsv> <output.xhctrace>\n";
}

ReadResult read_native_trace(const char* input_path, std::vector<openxhc::TraceRecord>& records) {
  std::ifstream input(input_path);
  if (!input.is_open()) {
    std::cerr << "error: unable to open trace input\n";
    return ReadResult::Open;
  }

  std::string line;
  if (!std::getline(input, line) || line != kTraceHeader) {
    std::cerr << "error: invalid trace header\n";
    return ReadResult::Parse;
  }
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    const auto parsed = openxhc::parse_trace_line(line);
    if (!std::holds_alternative<openxhc::TraceRecord>(parsed)) {
      std::cerr << "error: invalid trace record\n";
      return ReadResult::Parse;
    }
    records.push_back(std::get<openxhc::TraceRecord>(parsed));
  }
  if (input.bad()) {
    std::cerr << "error: unable to read trace input\n";
    return ReadResult::Open;
  }
  return ReadResult::Success;
}

ReadResult read_tshark_trace(const char* input_path, std::vector<openxhc::TraceRecord>& records) {
  std::ifstream input(input_path);
  if (!input.is_open()) {
    std::cerr << "error: unable to open TShark input\n";
    return ReadResult::Open;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    const auto parsed = openxhc::parse_tshark_line(line);
    if (!std::holds_alternative<openxhc::TraceRecord>(parsed)) {
      std::cerr << "error: invalid TShark record\n";
      return ReadResult::Parse;
    }
    records.push_back(std::get<openxhc::TraceRecord>(parsed));
  }
  if (input.bad()) {
    std::cerr << "error: unable to read TShark input\n";
    return ReadResult::Open;
  }
  return ReadResult::Success;
}

ExitCode exit_code(ReadResult result) {
  return result == ReadResult::Open ? ExitCode::Open : ExitCode::Parse;
}

ExitCode validate(const char* input_path) {
  std::vector<openxhc::TraceRecord> records;
  const ReadResult result = read_native_trace(input_path, records);
  return result == ReadResult::Success ? ExitCode::Success : exit_code(result);
}

ExitCode summary(const char* input_path) {
  std::vector<openxhc::TraceRecord> records;
  const ReadResult result = read_native_trace(input_path, records);
  if (result != ReadResult::Success) {
    return exit_code(result);
  }

  const openxhc::TraceSummary trace_summary = openxhc::summarize_trace(records);
  std::cout << "records=" << trace_summary.records << '\n'
            << "in=" << trace_summary.device_to_host << '\n'
            << "out=" << trace_summary.host_to_device << '\n';
  for (std::size_t report_id = 0U; report_id < trace_summary.report_ids.size(); ++report_id) {
    const std::uint64_t count = trace_summary.report_ids[report_id];
    if (count == 0U) {
      continue;
    }
    std::cout << "report_id[0x" << std::hex << std::setw(2) << std::setfill('0') << report_id
              << std::dec << std::setfill(' ') << "]=" << count << '\n';
  }
  return ExitCode::Success;
}

ExitCode import_tshark(const char* input_path, const char* output_path) {
  std::vector<openxhc::TraceRecord> records;
  const ReadResult result = read_tshark_trace(input_path, records);
  if (result != ReadResult::Success) {
    return exit_code(result);
  }

  std::ofstream output(output_path);
  if (!output.is_open()) {
    std::cerr << "error: unable to open trace output\n";
    return ExitCode::Open;
  }
  output << kTraceHeader << '\n';
  for (const openxhc::TraceRecord& record : records) {
    if (!openxhc::write_trace_record(output, record).success) {
      std::cerr << "error: unable to write trace output\n";
      return ExitCode::Open;
    }
  }
  if (!output) {
    std::cerr << "error: unable to write trace output\n";
    return ExitCode::Open;
  }
  return ExitCode::Success;
}
}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 3 || std::string_view(argv[1]) != "trace") {
    print_usage();
    return static_cast<int>(ExitCode::Usage);
  }

  const std::string_view command(argv[2]);
  if (command == "validate" && argc == 4) {
    return static_cast<int>(validate(argv[3]));
  }
  if (command == "summary" && argc == 4) {
    return static_cast<int>(summary(argv[3]));
  }
  if (command == "import-tshark" && argc == 5) {
    return static_cast<int>(import_tshark(argv[3], argv[4]));
  }

  print_usage();
  return static_cast<int>(ExitCode::Usage);
}
