// Generated from space_to_depth_float_2_relaxed.mod.py
// DO NOT EDIT
// clang-format off
#include "GeneratedTests.h"

namespace generated_tests::space_to_depth_float_2_relaxed {

std::vector<::test_helper::MixedTypedExample>& get_examples();

} // namespace generated_tests::space_to_depth_float_2_relaxed

namespace android::hardware::neuralnetworks::V1_1::generated_tests::space_to_depth_float_2_relaxed {

Model createTestModel();
bool is_ignored(int);

TEST_F(NeuralnetworksHidlTest, space_to_depth_float_2_relaxed) {
  Execute(device,
          createTestModel,
          is_ignored,
          ::generated_tests::space_to_depth_float_2_relaxed::get_examples());
}

TEST_F(ValidationTest, space_to_depth_float_2_relaxed) {
  const Model model = createTestModel();
  const std::vector<Request> requests = createRequests(::generated_tests::space_to_depth_float_2_relaxed::get_examples());
  validateEverything(model, requests);
}

} // namespace android::hardware::neuralnetworks::V1_1::generated_tests::space_to_depth_float_2_relaxed

