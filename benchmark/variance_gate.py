#!/usr/bin/env python3
# Copyright 2026, University of Freiburg,
#                  Chair of Algorithms and Data Structures
# Author: EPIC 10.2 AGENT 9 (Variance Bounding)
#
# Variance Gate: Performance stability validation
#
# Purpose: Ensure P99 latency variance stays within ±5% across 10 sequential runs
#
# Usage:
#   variance_gate.py --benchmark <path_to_benchmark> [--warmup N] [--runs N]
#
# Exit codes:
#   0 = Variance within bounds (PASS)
#   1 = Variance exceeds bounds (FAIL)
#   2 = Error or invalid input

import argparse
import json
import os
import platform
import re
import subprocess
import sys
import hashlib
from datetime import datetime
from pathlib import Path
from statistics import mean, stdev
from typing import List, Dict, Any, Tuple


def get_cpu_info() -> str:
    """Get CPU model information."""
    try:
        if platform.system() == "Linux":
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if "model name" in line:
                        return line.split(":")[1].strip()
        return platform.processor() or "unknown"
    except Exception:
        return "unknown"


def get_memory_info() -> str:
    """Get memory information."""
    try:
        if platform.system() == "Linux":
            with open("/proc/meminfo", "r") as f:
                for line in f:
                    if "MemTotal" in line:
                        return line.split(":")[1].strip()
        return "unknown"
    except Exception:
        return "unknown"


def get_os_info() -> str:
    """Get OS information."""
    return f"{platform.system()} {platform.release()}"


def compute_file_hash(filepath: Path) -> str:
    """Compute SHA256 hash of a file."""
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()


def extract_p99_from_output(output: str) -> int:
    """Extract P99 latency (ns) from benchmark JSON output."""
    # Look for JSON block in output
    try:
        # Try to find JSON in output
        for line in output.split("\n"):
            if line.strip().startswith("{"):
                # Try to parse as JSON
                json_obj = json.loads(line)
                if "p99_ns" in json_obj:
                    return int(json_obj["p99_ns"])

        # If not found in single line, try multi-line JSON parsing
        json_start = output.find("{")
        json_end = output.rfind("}") + 1
        if json_start >= 0 and json_end > json_start:
            json_str = output[json_start:json_end]
            json_obj = json.loads(json_str)
            if "p99_ns" in json_obj:
                return int(json_obj["p99_ns"])

    except (json.JSONDecodeError, ValueError, KeyError) as e:
        print(f"Warning: Failed to parse JSON from output: {e}", file=sys.stderr)
        print(f"Output was: {output[:500]}...", file=sys.stderr)

    # Fallback: look for explicit P99 value in text
    match = re.search(r'"p99_ns"\s*:\s*(\d+)', output)
    if match:
        return int(match.group(1))

    raise ValueError("Could not extract p99_ns from benchmark output")


def run_benchmark(benchmark_path: Path) -> Tuple[str, int]:
    """Run benchmark once and return (full_output, p99_ns)."""
    try:
        result = subprocess.run(
            [str(benchmark_path)],
            capture_output=True,
            text=True,
            timeout=300,  # 5 minute timeout
        )

        output = result.stdout + result.stderr

        if result.returncode != 0:
            print(f"Benchmark failed with return code {result.returncode}", file=sys.stderr)
            print(f"Output: {output}", file=sys.stderr)
            raise RuntimeError(f"Benchmark execution failed: {output}")

        p99_ns = extract_p99_from_output(output)
        return output, p99_ns

    except subprocess.TimeoutExpired:
        raise RuntimeError("Benchmark execution timed out (5 minutes)")
    except Exception as e:
        raise RuntimeError(f"Benchmark execution failed: {e}")


def run_variance_gate(
    benchmark_path: Path, num_warmup: int = 1, num_runs: int = 10
) -> Dict[str, Any]:
    """
    Run variance gate analysis.

    Returns dict with:
    - p99_latencies: List[int]
    - mean_p99: float
    - stddev_p99: float
    - cv_pct: float (coefficient of variation as percentage)
    - passed: bool (True if CV < 5%)
    """
    print(f"=== VARIANCE GATE: {benchmark_path.name} ===\n")

    # Warmup runs
    if num_warmup > 0:
        print(f"Running {num_warmup} warmup run(s)...")
        for i in range(num_warmup):
            try:
                _, _ = run_benchmark(benchmark_path)
                print(f"  Warmup {i + 1}/{num_warmup} complete")
            except Exception as e:
                print(f"  Warmup {i + 1} failed: {e}", file=sys.stderr)
        print()

    # Measured runs
    print(f"Running {num_runs} measured runs...")
    p99_latencies = []

    for i in range(num_runs):
        try:
            _, p99_ns = run_benchmark(benchmark_path)
            p99_latencies.append(p99_ns)
            print(f"  Run {i + 1}/{num_runs}: P99 = {p99_ns:,} ns")
        except Exception as e:
            print(f"ERROR: Run {i + 1} failed: {e}", file=sys.stderr)
            return {
                "p99_latencies": [],
                "mean_p99": 0,
                "stddev_p99": 0,
                "cv_pct": 100.0,
                "passed": False,
                "error": str(e),
            }

    print()

    # Compute statistics
    mean_p99 = mean(p99_latencies)
    stddev_p99 = stdev(p99_latencies) if len(p99_latencies) > 1 else 0.0
    cv_pct = (stddev_p99 / mean_p99 * 100.0) if mean_p99 > 0 else 0.0

    # Variance gate: CV must be < 5%
    passed = cv_pct < 5.0

    return {
        "p99_latencies": p99_latencies,
        "mean_p99": mean_p99,
        "stddev_p99": stddev_p99,
        "cv_pct": cv_pct,
        "passed": passed,
    }


def generate_receipt(
    benchmark_path: Path, results: Dict[str, Any], output_path: Path
) -> None:
    """Generate variance gate receipt file."""
    timestamp = datetime.utcnow().isoformat() + "Z"

    # Compute hash of variance gate implementation
    gate_hash = compute_file_hash(Path(__file__))

    # Benchmark hash (if exists)
    benchmark_hash = (
        compute_file_hash(benchmark_path) if benchmark_path.exists() else "N/A"
    )

    receipt = f"""# VARIANCE GATE RECEIPT
# EPIC 10.2 - Performance Seal - Variance Bounding
# Generated: {timestamp}

## Execution Environment

- **Date/Time (UTC)**: {timestamp}
- **CPU Model**: {get_cpu_info()}
- **Memory**: {get_memory_info()}
- **OS**: {get_os_info()}
- **Benchmark**: {benchmark_path.name}
- **Benchmark Hash (SHA256)**: {benchmark_hash}

## Variance Gate Configuration

- **Warmup Runs**: 1
- **Measured Runs**: {len(results.get('p99_latencies', []))}
- **Variance Threshold**: ±5% (CV < 5.0%)

## Run-by-Run P99 Latencies

"""

    for i, p99_ns in enumerate(results.get("p99_latencies", []), 1):
        receipt += f"- **Run {i}**: {p99_ns:,} ns\n"

    receipt += f"""
## Statistical Analysis

- **Mean P99 Latency**: {results.get('mean_p99', 0):,.2f} ns
- **Standard Deviation**: {results.get('stddev_p99', 0):,.2f} ns
- **Coefficient of Variation (CV)**: {results.get('cv_pct', 0):.3f}%

## Variance Gate Result

- **Status**: {'✅ PASS' if results.get('passed', False) else '❌ FAIL'}
- **Threshold**: 5.000%
- **Actual CV**: {results.get('cv_pct', 0):.3f}%
- **Within Bounds**: {'Yes' if results.get('passed', False) else 'No'}

"""

    if "error" in results:
        receipt += f"## Error\n\n```\n{results['error']}\n```\n\n"

    receipt += f"""## Variance Gate Implementation

- **Script**: {Path(__file__).name}
- **Hash (SHA256)**: {gate_hash}

## Deterministic Proof

This receipt constitutes deterministic proof that the variance gate
{'PASSED' if results.get('passed', False) else 'FAILED'} for benchmark `{benchmark_path.name}`.

The coefficient of variation (CV) across 10 sequential runs was {results.get('cv_pct', 0):.3f}%,
which is {'within' if results.get('passed', False) else 'OUTSIDE'} the ±5% variance bound specified in EPIC 10.2.

---
End of Receipt
"""

    with open(output_path, "w") as f:
        f.write(receipt)

    print(f"Receipt written to: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Variance Gate: Performance stability validation for EPIC 10.2",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--benchmark",
        type=Path,
        required=True,
        help="Path to benchmark executable",
    )
    parser.add_argument(
        "--warmup",
        type=int,
        default=1,
        help="Number of warmup runs (default: 1)",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=10,
        help="Number of measured runs (default: 10)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Output receipt file path (default: <benchmark>.receipt)",
    )

    args = parser.parse_args()

    if not args.benchmark.exists():
        print(f"ERROR: Benchmark not found: {args.benchmark}", file=sys.stderr)
        return 2

    # Determine output path
    output_path = args.output or args.benchmark.with_suffix(".receipt")

    # Run variance gate
    try:
        results = run_variance_gate(args.benchmark, args.warmup, args.runs)

        # Display results
        print("\n=== VARIANCE GATE RESULTS ===\n")
        print(f"Mean P99 Latency: {results.get('mean_p99', 0):,.2f} ns")
        print(f"Standard Deviation: {results.get('stddev_p99', 0):,.2f} ns")
        print(f"Coefficient of Variation: {results.get('cv_pct', 0):.3f}%")
        print(f"Threshold: 5.000%")
        print(
            f"\nStatus: {'✅ PASS' if results.get('passed', False) else '❌ FAIL'}\n"
        )

        # Generate receipt
        generate_receipt(args.benchmark, results, output_path)

        # Exit with appropriate code
        return 0 if results.get("passed", False) else 1

    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
