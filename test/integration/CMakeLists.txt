# Copyright 2025, University of Freiburg,
# Chair of Algorithms and Data Structures.
#
# Integration tests for QLever components.

# EpochIntegrationTest - End-to-end test for the epoch system
# Exercises full lifecycle: INIT -> INGEST -> SEAL -> SERVE -> RESTART
# Validates all acceptance criteria for EPIC 1: Epoch-based Immutability
addLinkAndDiscoverTestSerial(EpochIntegrationTest)

# EpochContractIntegrationTest - EPIC 1.1 Integration Tests
# Comprehensive end-to-end tests for Contract + Receipt + Atomic Promote
# Tests deterministic caching, manifest reproducibility, and atomic promotion
# Includes simulated ingress pipeline, cache system, and real-world data
addLinkAndDiscoverTestSerial(EpochContractIntegrationTest)
