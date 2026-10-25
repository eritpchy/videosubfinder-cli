#include "ConsoleOutput.h"
#include <thread>

void EmitFrameRecords(int worker, int count) {
    for (int i = 0; i < count; ++i) {
        vsf::WriteConsoleRecord(stderr, "Frame: 0_00_04_000__0_00_06_039_" +
            std::to_string(worker) + "_" + std::to_string(i));
        std::this_thread::yield();
    }
}

void EmitReportRecords(int worker, int count) {
    for (int i = 0; i < count; ++i) {
        // Exceed typical stdio/pipe buffers as well as exercising short EOF logs.
        const std::string padding(i % 25 == 0 ? 8192 : 0, 'x');
        APPEND_TO_REPORT_LOG("Video ended or cannot read new frames. worker=%d record=%d %s\n",
            worker, i, padding.c_str());
        std::this_thread::yield();
    }
}
