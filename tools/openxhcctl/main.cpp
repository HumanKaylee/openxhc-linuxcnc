// SPDX-License-Identifier: GPL-2.0-or-later
#include "openxhc/hid_probe.hpp"
#include "openxhc/hid_transport.hpp"
#include "openxhc/status_report.hpp"
#include "openxhc/trace.hpp"

#include <cerrno>
#include <chrono>
#include <memory>
#include <thread>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <variant>
#include <vector>

namespace {
constexpr std::string_view kTraceHeader = "OPENXHC_TRACE_V1";

enum class ExitCode : int { Success = 0, Usage = 2, Open = 3, Parse = 4, Unsupported = 5 };
enum class ReadResult { Success, Open, Parse };

void print_usage() {
  std::cerr << "usage: openxhcctl trace validate <input.xhctrace>\n"
            << "       openxhcctl trace summary <input.xhctrace>\n"
            << "       openxhcctl trace import-tshark <input.tsv> <output.xhctrace>\n"
            << "       openxhcctl device list [--show-paths]\n"
            << "       openxhcctl status [--seconds N] [--interface N]\n";
}

#ifndef OPENXHCCTL_TEST_HID_FIXTURE
ExitCode print_hid_error(const openxhc::Error& error) {
  std::cerr << "error: " << error.message << '\n';
  return error.code == openxhc::ErrorCode::UnsupportedDevice ? ExitCode::Unsupported
                                                             : ExitCode::Open;
}
#endif

void print_device(const openxhc::DeviceIdentity& identity, std::string_view path) {
  const auto caller_flags = std::cout.flags();
  const char caller_fill = std::cout.fill();
  std::cout << std::hex << std::setfill('0') << std::setw(4) << identity.vendor_id << ':'
            << std::setw(4) << identity.product_id << std::dec << std::setfill(' ')
            << " interface=" << identity.interface_number << " product=\""
            << identity.product_string << "\" release=0x" << std::hex << std::setfill('0')
            << std::setw(4) << identity.release_number << std::dec << std::setfill(' ')
            << " path=" << path << '\n';
  std::cout.flags(caller_flags);
  std::cout.fill(caller_fill);
}

#ifdef OPENXHCCTL_TEST_HID_FIXTURE
std::vector<openxhc::HidDeviceIdentity> fixture_hid_devices() {
  return {{{0x10ce, 0xeb73, 0, "XHC MACH3 CARD", 0x0100}, "/dev/hidraw-fixture-0"},
          {{0x10ce, 0xeb73, 1, "XHC MACH3 CARD", 0x0100}, "/dev/hidraw-fixture-1"}};
}
#endif

ExitCode list_devices(bool show_paths) {
#ifdef OPENXHCCTL_TEST_HID_FIXTURE
  const auto devices = fixture_hid_devices();
  for (const auto& device : devices) {
    print_device(device.identity, show_paths ? std::string_view(device.path) : "<redacted>");
  }
  return ExitCode::Success;
#else
  if (show_paths) {
    const auto devices = openxhc::enumerate_supported_hid_with_paths();
    if (std::holds_alternative<openxhc::Error>(devices)) {
      return print_hid_error(std::get<openxhc::Error>(devices));
    }
    for (const auto& device : std::get<std::vector<openxhc::HidDeviceIdentity>>(devices)) {
      print_device(device.identity, device.path);
    }
    return ExitCode::Success;
  }

  // The controller resets itself roughly every 2.8 s on this bus, so a single enumeration
  // can legitimately land in a window where the device is absent and report "not found".
  // Retry briefly before believing a negative.
  for (int attempt = 0; attempt < 15; ++attempt) {
    auto devices = openxhc::enumerate_supported_hid();
    if (!std::holds_alternative<openxhc::Error>(devices)) {
      for (const auto& identity : std::get<std::vector<openxhc::DeviceIdentity>>(devices)) {
        print_device(identity, "<redacted>");
      }
      return ExitCode::Success;
    }
    if (attempt == 14) {
      return print_hid_error(std::get<openxhc::Error>(devices));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{300});
  }
  return ExitCode::Open;
#endif
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

bool same_existing_file(const char* input_path, const char* output_path) {
  struct stat input_status {};
  struct stat output_status {};
  return ::stat(input_path, &input_status) == 0 && ::stat(output_path, &output_status) == 0 &&
         input_status.st_dev == output_status.st_dev && input_status.st_ino == output_status.st_ino;
}

bool write_all(int file_descriptor, const std::string& contents) {
  const char* current = contents.data();
  std::size_t remaining = contents.size();
  while (remaining != 0U) {
    const ssize_t written = ::write(file_descriptor, current, remaining);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    if (written == 0) {
      return false;
    }
    const auto count = static_cast<std::size_t>(written);
    current += count;
    remaining -= count;
  }
  return true;
}

std::string temporary_output_template(const char* output_path) {
  const std::filesystem::path destination(output_path);
  const std::filesystem::path parent =
      destination.has_parent_path() ? destination.parent_path() : std::filesystem::path{"."};
  return (parent / ("." + destination.filename().string() + ".openxhcctl-XXXXXX")).string();
}

int create_temporary_output(std::string& temporary_path) {
  return ::mkstemp(temporary_path.data());
}

#ifdef OPENXHCCTL_TEST_FAULT_INJECTION
bool staging_fault_requested() {
  return std::getenv("OPENXHCCTL_TEST_FAIL_STAGING") != nullptr;
}
#endif

void remove_temporary_file(const std::string& temporary_path) {
  if (!temporary_path.empty()) {
    static_cast<void>(::unlink(temporary_path.c_str()));
  }
}

ExitCode import_tshark(const char* input_path, const char* output_path) {
  std::vector<openxhc::TraceRecord> records;
  const ReadResult result = read_tshark_trace(input_path, records);
  if (result != ReadResult::Success) {
    return exit_code(result);
  }

  if (same_existing_file(input_path, output_path)) {
    std::cerr << "error: input and output refer to the same file\n";
    return ExitCode::Open;
  }

  std::ostringstream serialized;
  serialized << kTraceHeader << '\n';
  if (!serialized) {
    std::cerr << "error: unable to serialize trace output\n";
    return ExitCode::Open;
  }
  for (const openxhc::TraceRecord& record : records) {
    if (!openxhc::write_trace_record(serialized, record).success) {
      std::cerr << "error: unable to serialize trace output\n";
      return ExitCode::Open;
    }
  }
  if (!serialized) {
    std::cerr << "error: unable to serialize trace output\n";
    return ExitCode::Open;
  }

  std::string temporary_path = temporary_output_template(output_path);
  const int temporary_descriptor = create_temporary_output(temporary_path);
  if (temporary_descriptor == -1) {
    std::cerr << "error: unable to open trace output\n";
    return ExitCode::Open;
  }

  bool injected_failure = false;
#ifdef OPENXHCCTL_TEST_FAULT_INJECTION
  injected_failure = staging_fault_requested();
#endif
  const bool wrote = !injected_failure && write_all(temporary_descriptor, serialized.str());
  const bool flushed = wrote && ::fsync(temporary_descriptor) == 0;
  const bool closed = ::close(temporary_descriptor) == 0;
  if (!wrote || !flushed || !closed) {
    remove_temporary_file(temporary_path);
    std::cerr << "error: unable to write trace output\n";
    return ExitCode::Open;
  }
  if (::rename(temporary_path.c_str(), output_path) != 0) {
    remove_temporary_file(temporary_path);
    std::cerr << "error: unable to finalize trace output\n";
    return ExitCode::Open;
  }
  return ExitCode::Success;
}
}  // namespace

namespace {
constexpr int kStatusMaxSecondsWithoutOverride = 120;

// Read-only listen on the device-initiated interrupt IN stream. Bounded by construction:
// it never writes, never loops unbounded, and reports where the record is dynamic rather
// than interpreting any byte.
ExitCode status_listen(int seconds, int interface_number, bool allow_long) {
  if (seconds <= 0) {
    std::cerr << "error: --seconds must be positive\n";
    return ExitCode::Usage;
  }
  if (seconds > kStatusMaxSecondsWithoutOverride && !allow_long) {
    std::cerr << "error: --seconds above " << kStatusMaxSecondsWithoutOverride
              << " requires --allow-long\n";
    return ExitCode::Usage;
  }

  openxhc::FieldActivity activity;
  std::uint64_t records = 0U;
  std::uint64_t timeouts = 0U;
  std::uint64_t malformed = 0U;
  std::uint64_t reconnects = 0U;
  std::uint64_t open_failures = 0U;

  const auto started = std::chrono::steady_clock::now();
  const auto deadline = started + std::chrono::seconds(seconds);
  std::unique_ptr<openxhc::HidTransport> transport;

  while (std::chrono::steady_clock::now() < deadline) {
    if (transport == nullptr) {
      auto opened = openxhc::open_supported_hid_transport(interface_number);
      if (std::holds_alternative<openxhc::Error>(opened)) {
        // The controller resets itself periodically on this bus, so a failed open is
        // expected during a reset window. Keep trying until the deadline.
        ++open_failures;
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
        continue;
      }
      transport = std::get<std::unique_ptr<openxhc::HidTransport>>(std::move(opened));
      if (records > 0U || reconnects > 0U) {
        ++reconnects;
      }
    }

    auto incoming = transport->read(std::chrono::milliseconds{500});
    if (std::holds_alternative<openxhc::Error>(incoming)) {
      const auto& error = std::get<openxhc::Error>(incoming);
      if (error.code == openxhc::ErrorCode::Timeout) {
        ++timeouts;
        continue;
      }
      if (openxhc::is_recoverable_read_error(error.code)) {
        transport->close();
        transport.reset();
        continue;
      }
      std::cerr << "error: " << error.message << '\n';
      transport->close();
      return ExitCode::Open;
    }

    const auto& raw = std::get<openxhc::RawReport>(incoming);
    auto parsed = openxhc::parse_status_record(raw);
    if (std::holds_alternative<openxhc::Error>(parsed)) {
      ++malformed;
      std::cout << "record bytes=" << raw.size << " UNRECOGNISED\n";
      continue;
    }
    ++records;
    activity.observe(std::get<openxhc::StatusRecord>(parsed));
  }
  if (transport != nullptr) {
    transport->close();
  }

  if (records == 0U) {
    std::cerr << "error: no records received (open failures=" << open_failures << ")\n";
    return ExitCode::Open;
  }

  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - started)
                           .count();
  std::cout << "records=" << records << '\n'
            << "unrecognised=" << malformed << '\n'
            << "timeouts=" << timeouts << '\n'
            << "reconnects=" << reconnects << '\n'
            << "open_failures=" << open_failures << '\n'
            << "elapsed_ms=" << elapsed << '\n'
            << "changing_offsets=" << activity.changed_count() << '\n';
  std::cout << "changed=";
  for (std::size_t offset = 0U; offset < openxhc::kStatusRecordBytes; ++offset) {
    if (activity.changed(offset)) {
      std::cout << offset << ' ';
    }
  }
  std::cout << "\nnote: offsets only; no field meaning is claimed\n";
  return ExitCode::Success;
}
}  // namespace

int main(int argc, char* argv[]) {
  if (argc >= 2 && std::string_view(argv[1]) == "status") {
    int seconds = 10;
    int interface_number = 0;
    bool allow_long = false;
    for (int index = 2; index < argc; ++index) {
      const std::string_view option(argv[index]);
      if (option == "--allow-long") {
        allow_long = true;
        continue;
      }
      if (index + 1 >= argc) {
        print_usage();
        return static_cast<int>(ExitCode::Usage);
      }
      const std::string value(argv[++index]);
      if (option == "--seconds") {
        seconds = std::atoi(value.c_str());
      } else if (option == "--interface") {
        interface_number = std::atoi(value.c_str());
      } else {
        print_usage();
        return static_cast<int>(ExitCode::Usage);
      }
    }
    return static_cast<int>(status_listen(seconds, interface_number, allow_long));
  }

  if (argc >= 3 && std::string_view(argv[1]) == "device" &&
      std::string_view(argv[2]) == "list") {
    if (argc == 3) {
      return static_cast<int>(list_devices(false));
    }
    if (argc == 4 && std::string_view(argv[3]) == "--show-paths") {
      return static_cast<int>(list_devices(true));
    }
    print_usage();
    return static_cast<int>(ExitCode::Usage);
  }

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
