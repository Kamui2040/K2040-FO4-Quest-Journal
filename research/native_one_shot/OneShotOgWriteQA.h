#pragma once
// Build ONLY with K2040_TRACKING_NATIVE_WRITE_QA. This is not a production
// feature. One engine-owned SetActive call maximum per process.
#include "OneShotOgDryRun.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
namespace K2040::OneShotResearch {
    enum class WriteTestResult : std::uint8_t {
        kDenied, kAlready, kUnverified, kCalledProvisional
    };
    [[nodiscard]] bool ReadExplicitWriteTestConsent();
    [[nodiscard]] bool QueueWriteTestOnce(
        const std::shared_ptr<Gate>& gate, const char* borrowedUiPayload,
        std::uint64_t saveEpoch, std::uint64_t uiEpoch,
        std::atomic_bool& processAttempted,
        ReadContext readFresh, std::function<void(WriteTestResult)> report);
}
