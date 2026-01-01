# Copyright 2025, University of Freiburg,
# Chair of Algorithms and Data Structures.
#
# Integration tests for QLever components.

# EpochIntegrationTest - End-to-end test for the epoch system
# Exercises full lifecycle: INIT -> INGEST -> SEAL -> SERVE -> RESTART
# Validates all acceptance criteria for EPIC 1: Epoch-based Immutability
addLinkAndDiscoverTestSerial(EpochIntegrationTest)
