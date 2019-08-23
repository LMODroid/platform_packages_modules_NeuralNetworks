// Generated from space_to_depth_float_3_relaxed.mod.py
// DO NOT EDIT
// clang-format off
#include "TestHarness.h"

using namespace test_helper;

namespace generated_tests::space_to_depth_float_3_relaxed {

const TestModel& get_test_model() {
    static TestModel model = {
        .expectedMultinomialDistributionTolerance = 0,
        .inputIndexes = {0},
        .isRelaxed = true,
        .operands = {{
                .channelQuant = {},
                .data = TestBuffer::createFromVector<float>({10.0f, 20.0f, 11.0f, 21.0f, 12.0f, 22.0f, 13.0f, 23.0f, 14.0f, 24.0f, 15.0f, 25.0f, 16.0f, 26.0f, 17.0f, 27.0f, 18.0f, 28.0f, 19.0f, 29.0f, 110.0f, 210.0f, 111.0f, 211.0f, 112.0f, 212.0f, 113.0f, 213.0f, 114.0f, 214.0f, 115.0f, 215.0f}),
                .dimensions = {1, 4, 4, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::TENSOR_FLOAT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({2}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<float>({10.0f, 20.0f, 11.0f, 21.0f, 14.0f, 24.0f, 15.0f, 25.0f, 12.0f, 22.0f, 13.0f, 23.0f, 16.0f, 26.0f, 17.0f, 27.0f, 18.0f, 28.0f, 19.0f, 29.0f, 112.0f, 212.0f, 113.0f, 213.0f, 110.0f, 210.0f, 111.0f, 211.0f, 114.0f, 214.0f, 115.0f, 215.0f}),
                .dimensions = {1, 2, 2, 8},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_OUTPUT,
                .numberOfConsumers = 0,
                .scale = 0.0f,
                .type = TestOperandType::TENSOR_FLOAT32,
                .zeroPoint = 0
            }},
        .operations = {{
                .inputs = {0, 1},
                .outputs = {2},
                .type = TestOperationType::SPACE_TO_DEPTH
            }},
        .outputIndexes = {2}
    };
    return model;
}

}  // namespace generated_tests::space_to_depth_float_3_relaxed

namespace generated_tests::space_to_depth_float_3_relaxed {

const TestModel& get_test_model_all_inputs_as_internal() {
    static TestModel model = {
        .expectedMultinomialDistributionTolerance = 0,
        .inputIndexes = {3},
        .isRelaxed = true,
        .operands = {{
                .channelQuant = {},
                .data = TestBuffer::createFromVector<float>({}),
                .dimensions = {1, 4, 4, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::TEMPORARY_VARIABLE,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::TENSOR_FLOAT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({2}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<float>({10.0f, 20.0f, 11.0f, 21.0f, 14.0f, 24.0f, 15.0f, 25.0f, 12.0f, 22.0f, 13.0f, 23.0f, 16.0f, 26.0f, 17.0f, 27.0f, 18.0f, 28.0f, 19.0f, 29.0f, 112.0f, 212.0f, 113.0f, 213.0f, 110.0f, 210.0f, 111.0f, 211.0f, 114.0f, 214.0f, 115.0f, 215.0f}),
                .dimensions = {1, 2, 2, 8},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_OUTPUT,
                .numberOfConsumers = 0,
                .scale = 0.0f,
                .type = TestOperandType::TENSOR_FLOAT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<float>({10.0f, 20.0f, 11.0f, 21.0f, 12.0f, 22.0f, 13.0f, 23.0f, 14.0f, 24.0f, 15.0f, 25.0f, 16.0f, 26.0f, 17.0f, 27.0f, 18.0f, 28.0f, 19.0f, 29.0f, 110.0f, 210.0f, 111.0f, 211.0f, 112.0f, 212.0f, 113.0f, 213.0f, 114.0f, 214.0f, 115.0f, 215.0f}),
                .dimensions = {1, 4, 4, 2},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::MODEL_INPUT,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::TENSOR_FLOAT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<float>({0.0f}),
                .dimensions = {1},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::TENSOR_FLOAT32,
                .zeroPoint = 0
            }, {
                .channelQuant = {},
                .data = TestBuffer::createFromVector<int32_t>({0}),
                .dimensions = {},
                .isIgnored = false,
                .lifetime = TestOperandLifeTime::CONSTANT_COPY,
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .type = TestOperandType::INT32,
                .zeroPoint = 0
            }},
        .operations = {{
                .inputs = {3, 4, 5},
                .outputs = {0},
                .type = TestOperationType::ADD
            }, {
                .inputs = {0, 1},
                .outputs = {2},
                .type = TestOperationType::SPACE_TO_DEPTH
            }},
        .outputIndexes = {2}
    };
    return model;
}

}  // namespace generated_tests::space_to_depth_float_3_relaxed
