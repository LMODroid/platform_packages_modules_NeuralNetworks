// clang-format off
// Generated file (from: strided_slice_quant8_5.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace strided_slice_quant8_5 {
// Generated strided_slice_quant8_5 test
#include "generated/examples/strided_slice_quant8_5.example.cpp"
// Generated model constructor
#include "generated/models/strided_slice_quant8_5.model.cpp"
} // namespace strided_slice_quant8_5

TEST_F(GeneratedTests, strided_slice_quant8_5) {
    execute(strided_slice_quant8_5::CreateModel,
            strided_slice_quant8_5::is_ignored,
            strided_slice_quant8_5::get_examples());
}

TEST_F(DynamicOutputShapeTest, strided_slice_quant8_5_dynamic_output_shape) {
    execute(strided_slice_quant8_5::CreateModel_dynamic_output_shape,
            strided_slice_quant8_5::is_ignored_dynamic_output_shape,
            strided_slice_quant8_5::get_examples_dynamic_output_shape());
}

