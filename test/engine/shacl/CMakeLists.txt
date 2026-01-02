# SHACL test suite
# W3C SHACL 1.0 compliance testing for QLever

# Core SHACL tests
addLinkAndDiscoverTest(ShaclConstraintEvaluatorTest engine)
addLinkAndDiscoverTest(ShaclShapeParserTest engine)
addLinkAndDiscoverTest(ShaclShapeRegistryTest engine)

# W3C SHACL 1.0 Compliance tests
addLinkAndDiscoverTest(ShaclComplianceTest engine)
addLinkAndDiscoverTest(W3CShaclTestSuiteTest engine)
addLinkAndDiscoverTest(ShapesGraphValidationTest engine)

# Advanced SHACL features (beyond 80/20)
addLinkAndDiscoverTest(LogicalShapesTest engine)
addLinkAndDiscoverTest(AdvancedConstraintsTest engine)
addLinkAndDiscoverTest(RecursiveShapeValidatorTest engine)
addLinkAndDiscoverTest(ShapeCompositionTest engine)
addLinkAndDiscoverTest(ComplexPropertyPathsTest engine)
addLinkAndDiscoverTest(PathResolverTest engine)
addLinkAndDiscoverTest(SparqlBasedConstraintTest engine)
addLinkAndDiscoverTest(ShaclPlanningStrategyTest engine)

# EPIC 10.3 - Focus-Node Injection optimization tests
# addLinkAndDiscoverTest(FocusNodeInjectionTest engine)  # EPIC 10 WIP - file not yet present
