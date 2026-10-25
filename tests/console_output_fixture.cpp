#include "ConsoleOutput.h"
#include <atomic>
#include <thread>
#include <vector>

std::string g_ReportFileName;

// Keep producers in separate translation units, as they are in the CLI/core.
void EmitFrameRecords(int worker, int count);
void EmitReportRecords(int worker, int count);

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    g_ReportFileName = argv[1];
    OVERRIDE_TO_REPORT_LOG("Record framing regression\n");
    constexpr int count = 1500;
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    for (int worker = 0; worker < 6; ++worker) {
        workers.emplace_back([&, worker] {
            while (!start.load()) std::this_thread::yield();
            if (worker < 2) {
                for (int i = 0; i < count; ++i) {
                    vsf::WriteConsoleRecord(stderr, "Progress: " + std::to_string(i % 100) +
                        "% elapsed=" + std::to_string(worker * count + i) + "s");
                    std::this_thread::yield();
                }
            } else if (worker < 4) {
                EmitFrameRecords(worker, count);
            } else {
                EmitReportRecords(worker, count);
            }
        });
    }
    start.store(true);
    for (auto& worker : workers) worker.join();
    vsf::WriteConsoleRecord(stderr, "Progress: 100%\n");
    vsf::WriteConsoleRecord(stderr, "FINISH");
}
