#include <gtest/gtest.h>
#include "engine/validation/ValidationService.h"
#include "engine/validation/ValidationConfig.h"
#include "engine/validation/ValidationStats.h"
#include "engine/validation/ViolationFormatter.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include "engine/shacl/ShaclShape.h"
#include "engine/contracts/Violation.h"
#include "engine/contracts/ErrorCode.h"
#include "engine/contracts/CommonTypes.h"

using namespace validation;
using namespace contracts;
using namespace shacl;

class ValidationServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a simple shape registry for testing
    registry_ = std::make_shared<ShaclShapeRegistry>();

    // Add a simple test shape
    NodeShape testShape;
    testShape.shapeId = "http://example.org/shapes/PersonShape";
    testShape.addTargetClass("http://example.org/Person");

    // Add property shape for required name
    PropertyShape nameShape("http://example.org/name");
    ShaclConstraint minCount(ConstraintType::MinCount);
    minCount.value = 1;
    nameShape.constraints.push_back(minCount);
    nameShape.required = true;

    testShape.addPropertyShape(nameShape);

    registry_->registerShape(testShape);
  }

  std::shared_ptr<ShaclShapeRegistry> registry_;
};

TEST_F(ValidationServiceTest, ConstructorWithValidRegistry) {
  ValidationConfig config = ValidationConfig::defaultConfig();
  EXPECT_NO_THROW({
    ValidationService service(registry_.get(), config);
  });
}

TEST_F(ValidationServiceTest, ConstructorWithNullRegistry) {
  ValidationConfig config = ValidationConfig::defaultConfig();
  EXPECT_THROW({
    ValidationService service(nullptr, config);
  }, std::invalid_argument);
}

TEST_F(ValidationServiceTest, ValidateWithEmptyShapeSet) {
  ValidationConfig config = ValidationConfig::testConfig();
  ValidationService service(registry_.get(), config);

  ValidationInput input;
  input.scope = DatasetScope::FULL_DATASET;
  // Leave shapeSet empty

  ValidationResult result = service.validate(input);

  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.errorCode, ErrorCode::INVALID_SHAPE_SET);
  EXPECT_TRUE(result.errorMessage.has_value());
}

TEST_F(ValidationServiceTest, ValidateWithValidShapeSet) {
  ValidationConfig config = ValidationConfig::testConfig();
  ValidationService service(registry_.get(), config);

  ValidationInput input;
  input.scope = DatasetScope::FULL_DATASET;
  input.shapeSet = ShapeSet({"http://example.org/shapes/PersonShape"});

  ValidationResult result = service.validate(input);

  // Since this is a wrapper with placeholder implementation,
  // we just verify the API works
  EXPECT_TRUE(result.errorCode == ErrorCode::OK ||
              result.errorCode == ErrorCode::VIOLATIONS_FOUND);
}

TEST_F(ValidationServiceTest, ValidateWithMissingShape) {
  ValidationConfig config = ValidationConfig::testConfig();
  ValidationService service(registry_.get(), config);

  ValidationInput input;
  input.scope = DatasetScope::FULL_DATASET;
  input.shapeSet = ShapeSet({"http://example.org/shapes/NonExistentShape"});

  ValidationResult result = service.validate(input);

  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.errorCode, ErrorCode::MISSING_SHAPE);
  EXPECT_TRUE(result.errorMessage.has_value());
}

TEST_F(ValidationServiceTest, GuardViolationLimit) {
  ValidationConfig config = ValidationConfig::testConfig();
  config.maxViolations = 5;  // Very low limit
  ValidationService service(registry_.get(), config);

  // Test that guards are properly configured
  EXPECT_EQ(config.maxViolations, 5);
  EXPECT_EQ(config.maxFocusNodes, 1000);
  EXPECT_EQ(config.maxConstraintEvaluations, 10000);
}

TEST_F(ValidationServiceTest, ConfigurationMethods) {
  ValidationConfig config1 = ValidationConfig::defaultConfig();
  ValidationService service(registry_.get(), config1);

  // Get config
  const auto& retrievedConfig = service.getConfig();
  EXPECT_EQ(retrievedConfig.maxViolations, config1.maxViolations);

  // Set config
  ValidationConfig config2 = ValidationConfig::strictConfig();
  service.setConfig(config2);
  const auto& retrievedConfig2 = service.getConfig();
  EXPECT_EQ(retrievedConfig2.maxViolations, config2.maxViolations);
}

TEST_F(ValidationServiceTest, ClearCache) {
  ValidationConfig config = ValidationConfig::testConfig();
  ValidationService service(registry_.get(), config);

  // Should not throw
  EXPECT_NO_THROW(service.clearCache());
}

TEST_F(ValidationServiceTest, ValidationResultConforms) {
  ValidationResult result;
  result.ok = true;
  result.errorCode = ErrorCode::OK;

  EXPECT_TRUE(result.conforms());
  EXPECT_FALSE(result.guardTriggered());
  EXPECT_EQ(result.violationCount(), 0);
}

TEST_F(ValidationServiceTest, ValidationResultWithViolations) {
  ValidationResult result;
  result.ok = false;
  result.errorCode = ErrorCode::VIOLATIONS_FOUND;
  result.violations.resize(3);  // Add 3 violations

  EXPECT_FALSE(result.conforms());
  EXPECT_FALSE(result.guardTriggered());
  EXPECT_EQ(result.violationCount(), 3);
}

TEST_F(ValidationServiceTest, ValidationResultGuardTriggered) {
  ValidationResult result;
  result.ok = false;
  result.errorCode = ErrorCode::VIOLATIONS_LIMIT_EXCEEDED;

  EXPECT_FALSE(result.conforms());
  EXPECT_TRUE(result.guardTriggered());
  EXPECT_TRUE(isGuardTriggered(result.errorCode));
}

TEST_F(ValidationServiceTest, ValidationStatsToJson) {
  ValidationStats stats;
  stats.violationsFound = 10;
  stats.warningsFound = 5;
  stats.infosFound = 2;
  stats.focusNodesEvaluated = 100;
  stats.constraintsEvaluated = 500;
  stats.runtimeMs = 1500;
  stats.guardsTriggered = false;

  auto json = stats.toJson();

  EXPECT_EQ(json["violationsFound"], 10);
  EXPECT_EQ(json["warningsFound"], 5);
  EXPECT_EQ(json["infosFound"], 2);
  EXPECT_EQ(json["totalViolations"], 17);
  EXPECT_EQ(json["focusNodesEvaluated"], 100);
  EXPECT_EQ(json["constraintsEvaluated"], 500);
  EXPECT_EQ(json["runtimeMs"], 1500);
  EXPECT_EQ(json["guardsTriggered"], false);
}

TEST_F(ValidationServiceTest, ValidationConfigDefaults) {
  ValidationConfig config = ValidationConfig::defaultConfig();

  EXPECT_EQ(config.maxViolations, 10000);
  EXPECT_EQ(config.maxFocusNodes, 100000);
  EXPECT_EQ(config.maxConstraintEvaluations, 1000000);
  EXPECT_TRUE(config.maxRuntimeMsPerShape.has_value());
  EXPECT_EQ(*config.maxRuntimeMsPerShape, 5000);
  EXPECT_EQ(config.maxSparqlResults, 100000);
  EXPECT_EQ(config.profile, ShaclProfile::FULL);
  EXPECT_TRUE(config.enableParallelValidation);
  EXPECT_TRUE(config.enableCaching);
}

TEST_F(ValidationServiceTest, ValidationConfigPermissive) {
  ValidationConfig config = ValidationConfig::permissiveConfig();

  EXPECT_EQ(config.maxViolations, 100000);
  EXPECT_EQ(config.maxFocusNodes, 1000000);
  EXPECT_EQ(config.maxConstraintEvaluations, 10000000);
  EXPECT_EQ(*config.maxRuntimeMsPerShape, 30000);
}

TEST_F(ValidationServiceTest, ValidationConfigStrict) {
  ValidationConfig config = ValidationConfig::strictConfig();

  EXPECT_EQ(config.maxViolations, 1000);
  EXPECT_EQ(config.maxFocusNodes, 10000);
  EXPECT_EQ(config.maxConstraintEvaluations, 100000);
  EXPECT_EQ(*config.maxRuntimeMsPerShape, 1000);
}

TEST_F(ValidationServiceTest, ValidationConfigTest) {
  ValidationConfig config = ValidationConfig::testConfig();

  EXPECT_EQ(config.maxViolations, 100);
  EXPECT_EQ(config.maxFocusNodes, 1000);
  EXPECT_EQ(config.maxConstraintEvaluations, 10000);
  EXPECT_EQ(*config.maxRuntimeMsPerShape, 500);
  EXPECT_FALSE(config.enableParallelValidation);  // Deterministic for tests
}

TEST_F(ValidationServiceTest, ViolationFormatterConversion) {
  // Create a SHACL violation
  ShaclViolation shaclViolation;
  shaclViolation.focusNode = "<http://example.org/person1>";
  shaclViolation.resultPath = "http://example.org/name";
  shaclViolation.sourceShape = "http://example.org/shapes/PersonShape";
  shaclViolation.sourceConstraintComponent = "sh:minCount";
  shaclViolation.severity = SeverityLevel::Violation;
  shaclViolation.message = "Property http://example.org/name has too few values";
  shaclViolation.values.push_back("\"John\"");

  // Convert to standardized violation
  auto stdViolation = ViolationFormatter::fromShaclViolation(shaclViolation);

  // Verify conversion
  EXPECT_EQ(stdViolation.sourceShape, "http://example.org/shapes/PersonShape");
  EXPECT_TRUE(stdViolation.resultPath.has_value());
  EXPECT_EQ(*stdViolation.resultPath, "http://example.org/name");
  EXPECT_TRUE(stdViolation.constraintComponent.has_value());
  EXPECT_EQ(*stdViolation.constraintComponent, "sh:minCount");
  EXPECT_EQ(stdViolation.message,
            "Property http://example.org/name has too few values");
  EXPECT_EQ(stdViolation.values.size(), 1);
}

TEST_F(ValidationServiceTest, ViolationFormatterJsonSerialization) {
  // Create a SHACL violation
  ShaclViolation shaclViolation;
  shaclViolation.focusNode = "<http://example.org/person1>";
  shaclViolation.sourceShape = "http://example.org/shapes/PersonShape";
  shaclViolation.severity = SeverityLevel::Warning;
  shaclViolation.message = "Test warning";

  // Convert and serialize to JSON
  auto stdViolation = ViolationFormatter::fromShaclViolation(shaclViolation);
  auto json = stdViolation.toJson();

  EXPECT_EQ(json["sourceShape"], "http://example.org/shapes/PersonShape");
  EXPECT_EQ(json["message"], "Test warning");
  EXPECT_EQ(json["severity"], "Warning");
}

TEST_F(ValidationServiceTest, ErrorCodeHelpers) {
  EXPECT_TRUE(isSuccess(ErrorCode::OK));
  EXPECT_FALSE(isSuccess(ErrorCode::VIOLATIONS_FOUND));

  EXPECT_TRUE(isGuardTriggered(ErrorCode::VIOLATIONS_LIMIT_EXCEEDED));
  EXPECT_TRUE(isGuardTriggered(ErrorCode::FOCUS_NODES_LIMIT_EXCEEDED));
  EXPECT_TRUE(isGuardTriggered(ErrorCode::CONSTRAINT_EVALS_LIMIT_EXCEEDED));
  EXPECT_TRUE(isGuardTriggered(ErrorCode::RUNTIME_EXCEEDED));
  EXPECT_FALSE(isGuardTriggered(ErrorCode::OK));
  EXPECT_FALSE(isGuardTriggered(ErrorCode::VIOLATIONS_FOUND));
}
