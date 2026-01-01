#!/usr/bin/env python3
"""
CONSTRUCT Benchmark Analysis Script

This script processes benchmark results from ConstructBenchmark and generates
analysis, statistics, and visualizations suitable for a PhD thesis.

Usage:
    python3 analyze_construct_benchmarks.py --input results.json
    python3 analyze_construct_benchmarks.py --input results.json --output thesis_figures/
    python3 analyze_construct_benchmarks.py --input results*.json --compare  # Multiple runs
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple
import statistics
import re

def load_benchmark_results(filepath: str) -> Dict[str, Any]:
    """Load benchmark results from JSON file."""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found: {filepath}")
        sys.exit(1)
    except json.JSONDecodeError:
        print(f"Error: Invalid JSON in file: {filepath}")
        sys.exit(1)

def extract_measurements(benchmark_data: Dict[str, Any]) -> List[Dict[str, Any]]:
    """Extract individual measurements from benchmark data with validation."""
    measurements = []

    if not isinstance(benchmark_data, dict):
        print(f"Error: Expected dict, got {type(benchmark_data)}")
        return measurements

    # Handle direct measurements at top level
    if 'measurements' in benchmark_data:
        if not isinstance(benchmark_data['measurements'], dict):
            print("Warning: 'measurements' should be a dict")
        else:
            for name, data in benchmark_data['measurements'].items():
                try:
                    time_ms = data.get('time_ms') if isinstance(data, dict) else None
                    if time_ms is None:
                        print(f"Warning: Skipping measurement '{name}' - missing time_ms")
                        continue
                    # Validate and convert time_ms to float
                    time_ms = float(time_ms)
                    measurements.append({
                        'name': str(name),
                        'time_ms': time_ms,
                        'metadata': data.get('metadata', {}) if isinstance(data, dict) else {}
                    })
                except (ValueError, TypeError) as e:
                    print(f"Warning: Skipping measurement '{name}' - invalid time_ms: {e}")
                    continue

    # Handle grouped measurements
    if 'groups' in benchmark_data:
        if not isinstance(benchmark_data['groups'], dict):
            print("Warning: 'groups' should be a dict")
        else:
            for group_name, group_data in benchmark_data['groups'].items():
                if not isinstance(group_data, dict):
                    print(f"Warning: Group '{group_name}' should be a dict")
                    continue
                if 'measurements' in group_data:
                    if not isinstance(group_data['measurements'], dict):
                        print(f"Warning: Group '{group_name}' measurements should be a dict")
                        continue
                    for name, data in group_data['measurements'].items():
                        try:
                            time_ms = data.get('time_ms') if isinstance(data, dict) else None
                            if time_ms is None:
                                continue
                            time_ms = float(time_ms)
                            measurements.append({
                                'name': f"{group_name}/{name}",
                                'time_ms': time_ms,
                                'group': str(group_name),
                                'metadata': data.get('metadata', {}) if isinstance(data, dict) else {}
                            })
                        except (ValueError, TypeError) as e:
                            print(f"Warning: Skipping measurement '{group_name}/{name}' - invalid time_ms: {e}")
                            continue

    if not measurements:
        print("Warning: No valid measurements found in data")

    return measurements

def generate_performance_summary(measurements: List[Dict[str, Any]]) -> str:
    """Generate summary statistics of all measurements."""
    if not measurements:
        return "No measurements found."

    times = [m['time_ms'] for m in measurements]

    output = []
    output.append("=" * 70)
    output.append("CONSTRUCT BENCHMARK PERFORMANCE SUMMARY")
    output.append("=" * 70)
    output.append("")
    output.append("Overall Statistics:")
    output.append(f"  Total benchmarks run:        {len(measurements)}")
    output.append(f"  Mean execution time:         {statistics.mean(times):.3f} ms")
    output.append(f"  Median execution time:       {statistics.median(times):.3f} ms")
    output.append(f"  Min execution time:          {min(times):.3f} ms")
    output.append(f"  Max execution time:          {max(times):.3f} ms")

    if len(times) > 1:
        output.append(f"  Std deviation:               {statistics.stdev(times):.3f} ms")

    output.append("")
    return "\n".join(output)

def analyze_format_performance(measurements: List[Dict[str, Any]]) -> str:
    """Analyze performance differences between export formats."""
    output = []
    output.append("=" * 70)
    output.append("EXPORT FORMAT ANALYSIS")
    output.append("=" * 70)
    output.append("")

    # Group by format
    formats = {}
    for m in measurements:
        fmt = m['metadata'].get('format', 'unknown')
        if fmt not in formats:
            formats[fmt] = []
        formats[fmt].append(m['time_ms'])

    if not formats:
        return "No format-specific data found."

    # Calculate statistics per format
    format_stats = []
    for fmt, times in sorted(formats.items()):
        stats = {
            'format': fmt,
            'count': len(times),
            'mean': statistics.mean(times),
            'min': min(times),
            'max': max(times),
            'std': statistics.stdev(times) if len(times) > 1 else 0
        }
        format_stats.append(stats)

    # Find baseline (minimum mean time)
    baseline_mean = min(s['mean'] for s in format_stats)

    # Report
    output.append("Format Performance Comparison:")
    output.append("")
    output.append(f"{'Format':<15} {'Mean (ms)':<12} {'Min':<10} {'Max':<10} {'Overhead':<12}")
    output.append("-" * 70)

    for stats in sorted(format_stats, key=lambda x: x['mean']):
        overhead = ((stats['mean'] - baseline_mean) / baseline_mean * 100) if baseline_mean > 0 else 0
        output.append(
            f"{stats['format']:<15} {stats['mean']:>10.3f}  "
            f"{stats['min']:>8.3f}  {stats['max']:>8.3f}  {overhead:>10.1f}%"
        )

    output.append("")
    return "\n".join(output)

def analyze_scaling_behavior(measurements: List[Dict[str, Any]]) -> str:
    """Analyze how performance scales with knowledge graph size."""
    output = []
    output.append("=" * 70)
    output.append("SCALING ANALYSIS (KG Size Impact)")
    output.append("=" * 70)
    output.append("")

    # Extract KG size data
    kg_sizes = {}
    for m in measurements:
        kg_size = m['metadata'].get('kg_size') or m['metadata'].get('kg_entity_count')
        if kg_size is not None:
            if kg_size not in kg_sizes:
                kg_sizes[kg_size] = []
            kg_sizes[kg_size].append(m['time_ms'])

    if not kg_sizes:
        return "No KG size scaling data found."

    # Sort by size and analyze trend
    sorted_sizes = sorted(kg_sizes.keys())
    output.append(f"{'KG Size':<12} {'Mean Time':<15} {'Growth Factor':<15} {'Approx. Complexity':<20}")
    output.append("-" * 70)

    baseline = None
    baseline_size = None

    for kg_size in sorted_sizes:
        mean_time = statistics.mean(kg_sizes[kg_size])

        if baseline is None:
            baseline = mean_time
            baseline_size = kg_size
            output.append(f"{kg_size:<12} {mean_time:>13.3f}   {'(baseline)':<15}")
        else:
            # Calculate growth factor
            growth = mean_time / baseline
            size_ratio = kg_size / baseline_size

            # Estimate complexity
            if size_ratio > 0:
                log_growth = growth / size_ratio if size_ratio > 0 else 0
                if log_growth < 1.05:
                    complexity = "O(1) or O(log n)"
                elif 0.95 <= log_growth <= 1.05:
                    complexity = "O(n)"
                elif 1.95 <= log_growth <= 2.05:
                    complexity = "O(n log n)"
                elif log_growth <= 2.1:
                    complexity = "O(n^1.x)"
                else:
                    complexity = "O(n^2) or worse"
            else:
                complexity = "Unknown"

            output.append(
                f"{kg_size:<12} {mean_time:>13.3f}   {growth:>13.2f}x   {complexity:<20}"
            )

    output.append("")
    return "\n".join(output)

def analyze_template_complexity(measurements: List[Dict[str, Any]]) -> str:
    """Analyze performance impact of template complexity."""
    output = []
    output.append("=" * 70)
    output.append("TEMPLATE COMPLEXITY ANALYSIS")
    output.append("=" * 70)
    output.append("")

    # Extract template data
    templates = {}
    for m in measurements:
        triple_count = m['metadata'].get('template_triple_count')
        if triple_count is not None:
            if triple_count not in templates:
                templates[triple_count] = []
            templates[triple_count].append(m['time_ms'])

    if not templates:
        return "No template complexity data found."

    # Analyze
    sorted_counts = sorted(templates.keys())
    output.append(f"{'Triples':<10} {'Mean Time':<15} {'Per-Triple Cost':<20} {'Overhead':<15}")
    output.append("-" * 70)

    baseline = None
    baseline_count = None
    baseline_time = None

    for count in sorted_counts:
        mean_time = statistics.mean(templates[count])

        if baseline is None:
            baseline = mean_time
            baseline_count = count
            baseline_time = mean_time
            output.append(f"{count:<10} {mean_time:>13.3f}   {'(baseline)':<20}")
        else:
            # Calculate per-triple cost
            added_triples = count - baseline_count
            added_time = mean_time - baseline_time
            per_triple = added_time / added_triples if added_triples > 0 else 0

            # Calculate overhead
            overhead = ((mean_time - baseline) / baseline * 100) if baseline > 0 else 0

            output.append(
                f"{count:<10} {mean_time:>13.3f}   {per_triple:>18.3f} ms   {overhead:>13.1f}%"
            )

    output.append("")
    return "\n".join(output)

def analyze_query_complexity(measurements: List[Dict[str, Any]]) -> str:
    """Analyze performance impact of query complexity."""
    output = []
    output.append("=" * 70)
    output.append("QUERY COMPLEXITY ANALYSIS")
    output.append("=" * 70)
    output.append("")

    # Extract query type data
    query_types = {}
    for m in measurements:
        query_type = m['metadata'].get('query_type', 'unknown')
        if query_type not in query_types:
            query_types[query_type] = []
        query_types[query_type].append(m)

    if not query_types:
        return "No query complexity data found."

    output.append(f"{'Query Type':<30} {'Count':<8} {'Mean (ms)':<12} {'Std Dev':<12}")
    output.append("-" * 70)

    for query_type in sorted(query_types.keys()):
        measurements_for_type = query_types[query_type]
        times = [m['time_ms'] for m in measurements_for_type]
        mean = statistics.mean(times)
        std = statistics.stdev(times) if len(times) > 1 else 0

        output.append(
            f"{query_type:<30} {len(times):<8} {mean:>10.3f}   {std:>10.3f}"
        )

    output.append("")
    return "\n".join(output)

def generate_latex_table(measurements: List[Dict[str, Any]], table_name: str = "construct_performance") -> str:
    """Generate LaTeX table for thesis publication."""
    output = []
    output.append(f"% Auto-generated LaTeX table from benchmark results")
    output.append(f"\\begin{{table}}[h]")
    output.append(f"\\centering")
    output.append(f"\\begin{{tabular}}{{|l|r|r|r|}}")
    output.append(f"\\hline")
    output.append(f"Benchmark & Mean (ms) & Std Dev (ms) & Count \\\\")
    output.append(f"\\hline")

    # Group by benchmark
    benchmarks = {}
    for m in measurements:
        name = m['name']
        if name not in benchmarks:
            benchmarks[name] = []
        benchmarks[name].append(m['time_ms'])

    for name in sorted(benchmarks.keys()):
        times = benchmarks[name]
        mean = statistics.mean(times)
        std = statistics.stdev(times) if len(times) > 1 else 0
        count = len(times)

        # Escape LaTeX special characters
        safe_name = name.replace('_', '\\_').replace('&', '\\&')
        output.append(f"{safe_name} & {mean:.3f} & {std:.3f} & {count} \\\\")

    output.append(f"\\hline")
    output.append(f"\\end{{tabular}}")
    output.append(f"\\caption{{CONSTRUCT query performance measurements}}")
    output.append(f"\\label{{tab:{table_name}}}")
    output.append(f"\\end{{table}}")
    output.append(f"")

    return "\n".join(output)

def generate_all_analysis(results_file: str, output_dir: str = None) -> None:
    """Generate complete analysis report."""
    print(f"Loading benchmark results from {results_file}...")
    data = load_benchmark_results(results_file)

    print("Extracting measurements...")
    measurements = extract_measurements(data)

    if not measurements:
        print("Error: No measurements found in benchmark data.")
        sys.exit(1)

    print(f"Found {len(measurements)} measurements.\n")

    # Generate all analyses
    analyses = [
        generate_performance_summary(measurements),
        analyze_format_performance(measurements),
        analyze_scaling_behavior(measurements),
        analyze_template_complexity(measurements),
        analyze_query_complexity(measurements),
        generate_latex_table(measurements),
    ]

    # Combine and output
    full_report = "\n".join(analyses)
    print(full_report)

    # Save to file if output directory specified
    if output_dir:
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        report_file = output_path / "benchmark_analysis_report.txt"
        with open(report_file, 'w') as f:
            f.write(full_report)
        print(f"\nReport saved to: {report_file}")

        # Also save LaTeX table separately
        latex_table = generate_latex_table(measurements)
        latex_file = output_path / "benchmark_table.tex"
        with open(latex_file, 'w') as f:
            f.write(latex_table)
        print(f"LaTeX table saved to: {latex_file}")

def main():
    parser = argparse.ArgumentParser(
        description="Analyze CONSTRUCT benchmark results for PhD thesis"
    )
    parser.add_argument(
        "--input", "-i",
        required=True,
        help="Input benchmark results JSON file (or pattern for multiple files)"
    )
    parser.add_argument(
        "--output", "-o",
        default=None,
        help="Output directory for analysis reports and figures"
    )
    parser.add_argument(
        "--compare",
        action="store_true",
        help="Compare multiple benchmark runs"
    )

    args = parser.parse_args()

    # Handle single file
    if not args.compare or '*' not in args.input:
        generate_all_analysis(args.input, args.output)
    else:
        # Handle multiple files (wildcard)
        from glob import glob
        files = sorted(glob(args.input))
        if not files:
            print(f"Error: No files match pattern: {args.input}")
            sys.exit(1)

        print(f"Found {len(files)} files matching pattern.")

        # Combine results from all files
        all_measurements = []
        for filepath in files:
            print(f"  Processing: {filepath}")
            data = load_benchmark_results(filepath)
            measurements = extract_measurements(data)
            all_measurements.extend(measurements)

        # Generate analysis on combined data
        if all_measurements:
            print(f"\nCombined {len(all_measurements)} measurements from {len(files)} runs.\n")

            # Create temporary combined data structure
            combined_data = {
                'measurements': {
                    f"{m['name']}_{i}": {
                        'time_ms': m['time_ms'],
                        'metadata': m.get('metadata', {})
                    }
                    for i, m in enumerate(all_measurements)
                }
            }

            # Run analysis on combined data
            # Note: This is a simplified version; for production use,
            # implement proper statistical comparison
            print("Statistical Comparison of Runs:")
            print("-" * 70)

            # Group by benchmark name (without index)
            by_name = {}
            for m in all_measurements:
                name = re.sub(r'_\d+$', '', m['name'])
                if name not in by_name:
                    by_name[name] = []
                by_name[name].append(m['time_ms'])

            for name in sorted(by_name.keys()):
                times = by_name[name]
                mean = statistics.mean(times)
                std = statistics.stdev(times) if len(times) > 1 else 0
                min_t = min(times)
                max_t = max(times)

                print(f"{name:<40} μ={mean:.3f}ms σ={std:.3f}ms [{min_t:.3f}-{max_t:.3f}]")

        else:
            print("Error: No measurements found in any files.")
            sys.exit(1)

if __name__ == "__main__":
    main()
