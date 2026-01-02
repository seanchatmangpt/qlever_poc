# Formal Property Verification (FPV) Test Suite

**EPIC 10.3 - Agent 2: FPV Auditor**

This directory contains the formal verification suite for QLever kernel operations.

## Components

### RapidCheck Property-Based Tests
- `rapidcheck_join_properties.cpp` - Join kernel semantic equivalence
- `rapidcheck_filter_properties.cpp` - Filter kernel semantic equivalence
- `rapidcheck_indexscan_properties.cpp` - IndexScan kernel semantic equivalence
- `rapidcheck_generators.h` - Custom generators for IdTable, TripleComponent, etc.

### Kani Bounded Model Checking
- `kani/` - Rust crate with Kani harnesses for arithmetic safety proofs

### MC/DC Coverage
- `mcdc_instrumentation.sh` - Script to enable MC/DC coverage measurement
- `mcdc_report.py` - Parser for MC/DC coverage reports

## Running Tests

### RapidCheck (12-hour saturation)
```bash
./build/test/fpv/rapidcheck_join --rc-seed=42 --rc-max-success=1000000000
./build/test/fpv/rapidcheck_filter --rc-seed=42 --rc-max-success=1000000000
./build/test/fpv/rapidcheck_indexscan --rc-seed=42 --rc-max-success=1000000000
```

### Kani (2-hour verification)
```bash
cd test/fpv/kani
cargo kani --harness verify_all
```

### MC/DC Coverage
```bash
./test/fpv/mcdc_instrumentation.sh
make test
./test/fpv/mcdc_report.py build/coverage.info
```

## Success Criteria

- All RapidCheck properties pass 1 billion tests
- All Kani harnesses verify without counterexamples
- MC/DC coverage == 100% for Join/Filter/IndexScan kernels

## Output

Upon success, generates `fpv_witness.receipt` with signed hashes of all verification results.
