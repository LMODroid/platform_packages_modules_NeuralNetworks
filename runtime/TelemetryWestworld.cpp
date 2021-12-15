/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "TelemetryWestworld"

#include "TelemetryWestworld.h"

#include <android-base/logging.h>
#include <statslog_neuralnetworks.h>

#include <algorithm>
#include <limits>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "FeatureLevel.h"
#include "Telemetry.h"
#include "Tracing.h"

namespace android::nn::telemetry {
namespace {

constexpr uint64_t kNoTimeReportedRuntime = std::numeric_limits<uint64_t>::max();
constexpr int64_t kNoTimeReportedWestworld = std::numeric_limits<int64_t>::max();

// WestWorld specifies that "Atom logging frequency should not exceed once per 10 milliseconds (i.e.
// consecutive atom calls should be at least 10 milliseconds apart)." A quiet period of 100ms is
// chosen here to reduce the chance that the NNAPI logs too frequently, even from separate
// applications.
constexpr auto kMinimumLoggingQuietPeriod = std::chrono::milliseconds(100);

int32_t getUid() {
    static const int32_t uid = getuid();
    return uid;
}

constexpr int64_t nanosToMillis(uint64_t time) {
    constexpr uint64_t kNanosPerMilli = 1'000'000;
    return time == kNoTimeReportedRuntime ? kNoTimeReportedWestworld : time / kNanosPerMilli;
}

constexpr int64_t nanosToMicros(uint64_t time) {
    constexpr uint64_t kNanosPerMicro = 1'000;
    return time == kNoTimeReportedRuntime ? kNoTimeReportedWestworld : time / kNanosPerMicro;
}

AtomValue::AccumulatedTiming accumulatedTimingFrom(int64_t timing) {
    if (timing == kNoTimeReportedWestworld) {
        return {};
    }
    return {
            .sumTime = timing,
            .minTime = timing,
            .maxTime = timing,
            .sumSquaredTime = timing * timing,
            .count = 1,
    };
}

void combineAccumulatedTiming(AtomValue::AccumulatedTiming* accumulatedTime,
                              const AtomValue::AccumulatedTiming& timing) {
    if (timing.count == 0) {
        return;
    }
    accumulatedTime->sumTime += timing.sumTime;
    accumulatedTime->minTime = std::min(accumulatedTime->minTime, timing.minTime);
    accumulatedTime->maxTime = std::max(accumulatedTime->maxTime, timing.maxTime);
    accumulatedTime->sumSquaredTime += timing.sumSquaredTime;
    accumulatedTime->count += timing.count;
}

stats::BytesField makeBytesField(const ModelArchHash& modelArchHash) {
    return stats::BytesField(reinterpret_cast<const char*>(modelArchHash.begin()),
                             modelArchHash.size());
}

ModelArchHash makeModelArchHash(const uint8_t* modelArchHash) {
    ModelArchHash output;
    std::memcpy(output.data(), modelArchHash, BYTE_SIZE_OF_MODEL_ARCH_HASH);
    return output;
}

#define STATIC_ASSERT_DATA_CLASS_EQ_VALUE(type, inout, value) \
    static_assert(static_cast<int32_t>(DataClass::value) ==   \
                  stats::NEURAL_NETWORKS_##type##__##inout##_DATA_CLASS__DATA_CLASS_##value)

#define STATIC_ASSERT_DATA_CLASS_EQ(type, inout)             \
    STATIC_ASSERT_DATA_CLASS_EQ_VALUE(type, inout, UNKNOWN); \
    STATIC_ASSERT_DATA_CLASS_EQ_VALUE(type, inout, OTHER);   \
    STATIC_ASSERT_DATA_CLASS_EQ_VALUE(type, inout, FLOAT32); \
    STATIC_ASSERT_DATA_CLASS_EQ_VALUE(type, inout, FLOAT16); \
    STATIC_ASSERT_DATA_CLASS_EQ_VALUE(type, inout, QUANT);   \
    STATIC_ASSERT_DATA_CLASS_EQ_VALUE(type, inout, MIXED)

STATIC_ASSERT_DATA_CLASS_EQ(COMPILATION_COMPLETED, INPUT);
STATIC_ASSERT_DATA_CLASS_EQ(COMPILATION_COMPLETED, OUTPUT);
STATIC_ASSERT_DATA_CLASS_EQ(COMPILATION_FAILED, INPUT);
STATIC_ASSERT_DATA_CLASS_EQ(COMPILATION_FAILED, OUTPUT);
STATIC_ASSERT_DATA_CLASS_EQ(EXECUTION_COMPLETED, INPUT);
STATIC_ASSERT_DATA_CLASS_EQ(EXECUTION_COMPLETED, OUTPUT);
STATIC_ASSERT_DATA_CLASS_EQ(EXECUTION_FAILED, INPUT);
STATIC_ASSERT_DATA_CLASS_EQ(EXECUTION_FAILED, OUTPUT);

#define STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, value) \
    static_assert(ANEURALNETWORKS_##value ==            \
                  stats::NEURAL_NETWORKS_##type##__ERROR_CODE__RESULT_CODE_##value)

#define STATIC_ASSERT_RESULT_CODE_EQ(type)                                   \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, NO_ERROR);                      \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, OUT_OF_MEMORY);                 \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, INCOMPLETE);                    \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, UNEXPECTED_NULL);               \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, BAD_DATA);                      \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, OP_FAILED);                     \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, BAD_STATE);                     \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, UNMAPPABLE);                    \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, OUTPUT_INSUFFICIENT_SIZE);      \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, UNAVAILABLE_DEVICE);            \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, MISSED_DEADLINE_TRANSIENT);     \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, MISSED_DEADLINE_PERSISTENT);    \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, RESOURCE_EXHAUSTED_TRANSIENT);  \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, RESOURCE_EXHAUSTED_PERSISTENT); \
    STATIC_ASSERT_RESULT_CODE_EQ_VALUE(type, DEAD_OBJECT)

STATIC_ASSERT_RESULT_CODE_EQ(COMPILATION_FAILED);
STATIC_ASSERT_RESULT_CODE_EQ(EXECUTION_FAILED);

#undef STATIC_ASSERT_DATA_CLASS_EQ_VALUE
#undef STATIC_ASSERT_DATA_CLASS_EQ
#undef STATIC_ASSERT_RESULT_CODE_EQ_VALUE
#undef STATIC_ASSERT_RESULT_CODE_EQ

int32_t convertDataClass(DataClass dataClass) {
    switch (dataClass) {
        case DataClass::UNKNOWN:
        case DataClass::OTHER:
        case DataClass::FLOAT32:
        case DataClass::FLOAT16:
        case DataClass::QUANT:
        case DataClass::MIXED:
            return static_cast<int32_t>(dataClass);
    }
    return static_cast<int32_t>(DataClass::UNKNOWN);
}

int32_t convertExecutionMode(ExecutionMode executionMode) {
    switch (executionMode) {
        case ExecutionMode::ASYNC:
            return stats::NEURAL_NETWORKS_EXECUTION_FAILED__MODE__MODE_ASYNC;
        case ExecutionMode::SYNC:
            return stats::NEURAL_NETWORKS_EXECUTION_FAILED__MODE__MODE_SYNC;
        case ExecutionMode::BURST:
            return stats::NEURAL_NETWORKS_EXECUTION_FAILED__MODE__MODE_BURST;
        case ExecutionMode::ASYNC_WITH_DEPS:
            return stats::NEURAL_NETWORKS_EXECUTION_FAILED__MODE__MODE_ASYNC_WITH_DEPS;
    }
    return stats::NEURAL_NETWORKS_EXECUTION_FAILED__MODE__MODE_UNKNOWN;
}

int32_t convertResultCode(int32_t resultCode) {
    return resultCode >= ANEURALNETWORKS_NO_ERROR && resultCode <= ANEURALNETWORKS_DEAD_OBJECT
                   ? resultCode
                   : ANEURALNETWORKS_OP_FAILED;
}

void logAtomToWestworld(Atom&& atom) {
    NNTRACE_RT(NNTRACE_PHASE_UNSPECIFIED, "logAtomToWestworld");
    const auto& [key, value] = atom;

    if (!key.isExecution) {
        if (key.errorCode == ANEURALNETWORKS_NO_ERROR) {
            stats::stats_write(
                    stats::NEURALNETWORKS_COMPILATION_COMPLETED, getUid(), getSessionId(),
                    kNnapiApexVersion, makeBytesField(key.modelArchHash), key.deviceId.c_str(),
                    convertDataClass(key.inputDataClass), convertDataClass(key.outputDataClass),
                    key.fallbackToCpuFromError, key.introspectionEnabled, key.cacheEnabled,
                    key.hasControlFlow, key.hasDynamicTemporaries,
                    value.compilationTimeMillis.sumTime, value.compilationTimeMillis.minTime,
                    value.compilationTimeMillis.maxTime, value.compilationTimeMillis.sumSquaredTime,
                    value.compilationTimeMillis.count, value.count);
        } else {
            stats::stats_write(
                    stats::NEURALNETWORKS_COMPILATION_FAILED, getUid(), getSessionId(),
                    kNnapiApexVersion, makeBytesField(key.modelArchHash), key.deviceId.c_str(),
                    convertDataClass(key.inputDataClass), convertDataClass(key.outputDataClass),
                    convertResultCode(key.errorCode), key.introspectionEnabled, key.cacheEnabled,
                    key.hasControlFlow, key.hasDynamicTemporaries, value.count);
        }
    } else {
        if (key.errorCode == ANEURALNETWORKS_NO_ERROR) {
            stats::stats_write(
                    stats::NEURALNETWORKS_EXECUTION_COMPLETED, getUid(), getSessionId(),
                    kNnapiApexVersion, makeBytesField(key.modelArchHash), key.deviceId.c_str(),
                    convertExecutionMode(key.executionMode), convertDataClass(key.inputDataClass),
                    convertDataClass(key.outputDataClass), key.introspectionEnabled,
                    key.cacheEnabled, key.hasControlFlow, key.hasDynamicTemporaries,
                    value.durationRuntimeMicros.sumTime, value.durationRuntimeMicros.minTime,
                    value.durationRuntimeMicros.maxTime, value.durationRuntimeMicros.sumSquaredTime,
                    value.durationRuntimeMicros.count, value.durationDriverMicros.sumTime,
                    value.durationDriverMicros.minTime, value.durationDriverMicros.maxTime,
                    value.durationDriverMicros.sumSquaredTime, value.durationDriverMicros.count,
                    value.durationHardwareMicros.sumTime, value.durationHardwareMicros.minTime,
                    value.durationHardwareMicros.maxTime,
                    value.durationHardwareMicros.sumSquaredTime, value.durationHardwareMicros.count,
                    value.count);
        } else {
            stats::stats_write(
                    stats::NEURALNETWORKS_EXECUTION_FAILED, getUid(), getSessionId(),
                    kNnapiApexVersion, makeBytesField(key.modelArchHash), key.deviceId.c_str(),
                    convertExecutionMode(key.executionMode), convertDataClass(key.inputDataClass),
                    convertDataClass(key.outputDataClass), convertResultCode(key.errorCode),
                    key.introspectionEnabled, key.cacheEnabled, key.hasControlFlow,
                    key.hasDynamicTemporaries, value.count);
        }
    }
}

// This class ensures that we are not pushing atoms to Westworld more frequently than
// kMinimumLoggingQuietPeriod ms.
class WestworldLogger {
   public:
    static WestworldLogger& get() {
        static WestworldLogger logger;
        return logger;
    }

    void write(Atom&& atom) {
        std::lock_guard hold(mMutex);
        mLogger.write(std::move(atom));
    }

   private:
    WestworldLogger() = default;

    std::mutex mMutex;
    LoggerWithQuietPeriod mLogger GUARDED_BY(mMutex) =
            LoggerWithQuietPeriod(kMinimumLoggingQuietPeriod, logAtomToWestworld);
};

constexpr auto asTuple(const AtomKey& v) {
    return std::tie(v.isExecution, v.modelArchHash, v.deviceId, v.executionMode, v.errorCode,
                    v.inputDataClass, v.outputDataClass, v.fallbackToCpuFromError,
                    v.introspectionEnabled, v.cacheEnabled, v.hasControlFlow,
                    v.hasDynamicTemporaries);
};

}  // namespace

bool operator==(const AtomKey& lhs, const AtomKey& rhs) {
    return asTuple(lhs) == asTuple(rhs);
}

bool operator<(const AtomKey& lhs, const AtomKey& rhs) {
    return asTuple(lhs) < asTuple(rhs);
}

void combineAtomValues(AtomValue* accumulatedValue, const AtomValue& value) {
    accumulatedValue->count += value.count;
    combineAccumulatedTiming(&accumulatedValue->compilationTimeMillis, value.compilationTimeMillis);
    combineAccumulatedTiming(&accumulatedValue->durationRuntimeMicros, value.durationRuntimeMicros);
    combineAccumulatedTiming(&accumulatedValue->durationDriverMicros, value.durationDriverMicros);
    combineAccumulatedTiming(&accumulatedValue->durationHardwareMicros,
                             value.durationHardwareMicros);
}

bool AtomAggregator::empty() const {
    return mOrder.empty();
}

void AtomAggregator::push(Atom&& atom) {
    const AtomValue& value = atom.second;
    if (const auto [it, inserted] = mAggregate.try_emplace(std::move(atom.first), value);
        !inserted) {
        combineAtomValues(&it->second, value);
    } else {
        mOrder.push(&it->first);
    }
}

std::pair<AtomKey, AtomValue> AtomAggregator::pop() {
    CHECK(!empty());

    // Find the key of the aggregated atom to log and remove it.
    const AtomKey* key = mOrder.front();
    mOrder.pop();

    // Find the value that corresponds to the key and remove the (key,value) from the map.
    auto node = mAggregate.extract(*key);
    CHECK(!node.empty());

    return std::make_pair(std::move(node.key()), node.mapped());
}

LoggerWithQuietPeriod::LoggerWithQuietPeriod(Duration duration, LoggerFn logger,
                                             AtomAggregator data, TimePoint safeToLogTimepoint)
    : kQuietPeriodDuration(duration),
      kLogger(std::move(logger)),
      mData(std::move(data)),
      mSafeToLogTimepoint(safeToLogTimepoint) {
    CHECK(kLogger != nullptr);
}

void LoggerWithQuietPeriod::write(Atom&& atom) {
    NNTRACE_RT(NNTRACE_PHASE_UNSPECIFIED, "LoggerWithQuietPeriod::write");

    const auto currentTime = Clock::now();
    const bool hasLoggedRecently = currentTime < mSafeToLogTimepoint;

    // Quickly log the data without aggregation if possible.
    if (mData.empty() && !hasLoggedRecently) {
        kLogger(std::move(atom));
        mSafeToLogTimepoint = currentTime + kQuietPeriodDuration;
        return;
    }

    // Aggregate the data.
    mData.push(std::move(atom));

    // Don't log any data if we've logged recently.
    if (hasLoggedRecently) {
        return;
    }

    // Retrieve an aggregated atom and send it to WestWorld.
    kLogger(mData.pop());
    mSafeToLogTimepoint = currentTime + kQuietPeriodDuration;
}

Atom createAtomFrom(const DiagnosticCompilationInfo* info) {
    Atom atom = Atom{
            AtomKey{
                    .isExecution = false,
                    .modelArchHash = makeModelArchHash(info->modelArchHash),
                    .deviceId = info->deviceId,
                    .executionMode = ExecutionMode::SYNC,
                    .errorCode = info->errorCode,
                    .inputDataClass = info->inputDataClass,
                    .outputDataClass = info->outputDataClass,
                    .fallbackToCpuFromError = info->fallbackToCpuFromError,
                    .introspectionEnabled = info->introspectionEnabled,
                    .cacheEnabled = info->cacheEnabled,
                    .hasControlFlow = info->hasControlFlow,
                    .hasDynamicTemporaries = info->hasDynamicTemporaries,
            },
            AtomValue{
                    .count = 1,
            },
    };

    // Timing information is only relevant for the "Completed" path.
    if (info->errorCode == ANEURALNETWORKS_NO_ERROR) {
        auto& value = atom.second;
        const auto compilationTimeMillis = nanosToMillis(info->compilationTimeNanos);
        value.compilationTimeMillis = accumulatedTimingFrom(compilationTimeMillis);
    }

    return atom;
}

Atom createAtomFrom(const DiagnosticExecutionInfo* info) {
    Atom atom = Atom{
            AtomKey{
                    .isExecution = true,
                    .modelArchHash = makeModelArchHash(info->modelArchHash),
                    .deviceId = info->deviceId,
                    .executionMode = info->executionMode,
                    .errorCode = info->errorCode,
                    .inputDataClass = info->inputDataClass,
                    .outputDataClass = info->outputDataClass,
                    .fallbackToCpuFromError = false,
                    .introspectionEnabled = info->introspectionEnabled,
                    .cacheEnabled = info->cacheEnabled,
                    .hasControlFlow = info->hasControlFlow,
                    .hasDynamicTemporaries = info->hasDynamicTemporaries,
            },
            AtomValue{
                    .count = 1,
            },
    };

    // Timing information is only relevant for the "Completed" path.
    if (info->errorCode == ANEURALNETWORKS_NO_ERROR) {
        auto& value = atom.second;
        const auto durationRuntimeMicros = nanosToMicros(info->durationRuntimeNanos);
        const auto durationDriverMicros = nanosToMicros(info->durationDriverNanos);
        const auto durationHardwareMicros = nanosToMicros(info->durationHardwareNanos);
        value.durationRuntimeMicros = accumulatedTimingFrom(durationRuntimeMicros);
        value.durationDriverMicros = accumulatedTimingFrom(durationDriverMicros);
        value.durationHardwareMicros = accumulatedTimingFrom(durationHardwareMicros);
    };

    return atom;
}

void logCompilationToWestworld(const DiagnosticCompilationInfo* info) {
    NNTRACE_RT(NNTRACE_PHASE_UNSPECIFIED, "logCompilationWestworld");
    WestworldLogger::get().write(createAtomFrom(info));
}

void logExecutionToWestworld(const DiagnosticExecutionInfo* info) {
    NNTRACE_RT(NNTRACE_PHASE_UNSPECIFIED, "logExecutionWestworld");
    WestworldLogger::get().write(createAtomFrom(info));
}

}  // namespace android::nn::telemetry
