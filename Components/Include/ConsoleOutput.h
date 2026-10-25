#pragma once

#include <cstdio>
#include <mutex>
#include <string>

namespace vsf {

// This inline function has one shared mutex across translation units. Both
// console streams use it because CLI consumers commonly merge stdout/stderr.
inline std::mutex& ConsoleOutputMutex() {
    static std::mutex mutex;
    return mutex;
}

inline void WriteConsoleRecord(FILE* stream, std::string record) {
    if (record.empty() || record.back() != '\n') record += '\n';
    const std::lock_guard<std::mutex> lock(ConsoleOutputMutex());
    std::fwrite(record.data(), 1, record.size(), stream);
    std::fflush(stream);
}

} // namespace vsf

extern std::string g_ReportFileName;

// Report messages must end in a newline. Flush while still holding the same
// lock as protocol events so buffered stdout cannot bisect a stderr record.
#define APPEND_TO_REPORT_LOG(...) do { \
    const std::lock_guard<std::mutex> output_lock(vsf::ConsoleOutputMutex()); \
    FILE* fp = fopen(g_ReportFileName.c_str(), "ab"); \
    if (fp != NULL) { \
        fprintf(fp, __VA_ARGS__); \
        fclose(fp); \
    } \
    printf(__VA_ARGS__); \
    fflush(stdout); \
} while(0)

#define OVERRIDE_TO_REPORT_LOG(...) do { \
    const std::lock_guard<std::mutex> output_lock(vsf::ConsoleOutputMutex()); \
    FILE* fp = fopen(g_ReportFileName.c_str(), "wb"); \
    if (fp != NULL) { \
        fprintf(fp, __VA_ARGS__); \
        fclose(fp); \
    } \
    printf(__VA_ARGS__); \
    fflush(stdout); \
} while(0)
