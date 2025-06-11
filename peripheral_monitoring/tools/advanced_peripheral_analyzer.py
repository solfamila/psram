#!/usr/bin/env python3
"""
Advanced Peripheral Register Analyzer for MIMXRT700
==================================================

This script provides advanced analysis capabilities for peripheral register
access patterns, including statistical analysis, pattern detection, and
performance optimization recommendations.

Features:
1. Statistical analysis of register access patterns
2. Hotspot identification for frequently accessed registers
3. Timing analysis and performance bottleneck detection
4. Register dependency analysis
5. Optimization recommendations
6. Export capabilities for various formats

Usage:
    python3 advanced_peripheral_analyzer.py [options]
"""

import json
import sys
import os
import argparse
import statistics
from collections import defaultdict, Counter
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
# Try to import matplotlib, use fallback if not available
try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("⚠️  Matplotlib not available, skipping plot generation")

@dataclass
class RegisterAccessPattern:
    """Represents a register access pattern"""
    address: str
    peripheral: str
    register: str
    access_count: int
    read_count: int
    write_count: int
    execution_phases: List[str]
    timing_distribution: List[float]
    dependencies: List[str]

class AdvancedPeripheralAnalyzer:
    """Advanced analyzer for peripheral register access patterns"""
    
    def __init__(self):
        self.analysis_data = None
        self.register_patterns = {}
        self.peripheral_stats = {}
        self.timing_analysis = {}
        self.dependencies = defaultdict(list)
        
    def load_analysis_data(self, json_file: str) -> bool:
        """Load peripheral analysis data"""
        try:
            with open(json_file, 'r') as f:
                self.analysis_data = json.load(f)
            
            print(f"📊 Loaded analysis data: {self.analysis_data['total_accesses']} accesses")
            return True
        except Exception as e:
            print(f"❌ Failed to load analysis data: {e}")
            return False
    
    def analyze_access_patterns(self):
        """Analyze register access patterns"""
        print(f"\n🔍 ANALYZING ACCESS PATTERNS")
        print("=" * 50)
        
        # Group accesses by register
        register_groups = defaultdict(list)
        for access in self.analysis_data['chronological_sequence']:
            key = f"{access['peripheral_name']}_{access['register_name']}"
            register_groups[key].append(access)
        
        # Analyze each register
        for reg_key, accesses in register_groups.items():
            pattern = self._analyze_register_pattern(reg_key, accesses)
            self.register_patterns[reg_key] = pattern
        
        print(f"✅ Analyzed {len(self.register_patterns)} unique registers")
    
    def _analyze_register_pattern(self, reg_key: str, accesses: List[Dict]) -> RegisterAccessPattern:
        """Analyze pattern for a specific register"""
        if not accesses:
            return None
        
        first_access = accesses[0]
        address = first_access['address']
        peripheral = first_access['peripheral_name']
        register = first_access['register_name']
        
        # Count access types
        read_count = sum(1 for a in accesses if 'read' in a['access_type'])
        write_count = sum(1 for a in accesses if 'write' in a['access_type'])
        total_count = len(accesses)
        
        # Execution phases
        phases = list(set(a['execution_phase'] for a in accesses))
        
        # Simulate timing distribution (in real implementation, this would come from actual measurements)
        timing_dist = [float(a['sequence_number']) * 0.1 for a in accesses]
        
        # Dependencies (registers accessed before/after this one)
        dependencies = self._find_dependencies(accesses)
        
        return RegisterAccessPattern(
            address=address,
            peripheral=peripheral,
            register=register,
            access_count=total_count,
            read_count=read_count,
            write_count=write_count,
            execution_phases=phases,
            timing_distribution=timing_dist,
            dependencies=dependencies
        )
    
    def _find_dependencies(self, accesses: List[Dict]) -> List[str]:
        """Find register dependencies based on access sequence"""
        dependencies = set()
        
        for access in accesses:
            seq_num = access['sequence_number']
            
            # Look for registers accessed within ±5 sequence positions
            for other_access in self.analysis_data['chronological_sequence']:
                other_seq = other_access['sequence_number']
                if abs(other_seq - seq_num) <= 5 and other_seq != seq_num:
                    dep_key = f"{other_access['peripheral_name']}_{other_access['register_name']}"
                    dependencies.add(dep_key)
        
        return list(dependencies)[:10]  # Limit to top 10 dependencies
    
    def identify_hotspots(self) -> List[Tuple[str, RegisterAccessPattern]]:
        """Identify register access hotspots"""
        print(f"\n🔥 IDENTIFYING ACCESS HOTSPOTS")
        print("=" * 50)
        
        # Sort registers by access count
        hotspots = sorted(
            self.register_patterns.items(),
            key=lambda x: x[1].access_count,
            reverse=True
        )
        
        print(f"Top 10 Most Accessed Registers:")
        for i, (reg_key, pattern) in enumerate(hotspots[:10], 1):
            print(f"   {i:2d}. {pattern.peripheral:8} {pattern.register:12} - {pattern.access_count:3d} accesses")
        
        return hotspots[:20]  # Return top 20 hotspots
    
    def analyze_peripheral_distribution(self):
        """Analyze distribution across peripherals"""
        print(f"\n📊 PERIPHERAL DISTRIBUTION ANALYSIS")
        print("=" * 50)
        
        peripheral_stats = defaultdict(lambda: {
            'total_accesses': 0,
            'unique_registers': 0,
            'read_write_ratio': 0,
            'execution_phases': set(),
            'registers': set()
        })
        
        for pattern in self.register_patterns.values():
            stats = peripheral_stats[pattern.peripheral]
            stats['total_accesses'] += pattern.access_count
            stats['registers'].add(pattern.register)
            stats['execution_phases'].update(pattern.execution_phases)
            
            if pattern.write_count > 0:
                stats['read_write_ratio'] = pattern.read_count / pattern.write_count
        
        # Convert sets to counts
        for peripheral, stats in peripheral_stats.items():
            stats['unique_registers'] = len(stats['registers'])
            stats['execution_phases'] = len(stats['execution_phases'])
            del stats['registers']  # Remove set for JSON serialization
        
        self.peripheral_stats = dict(peripheral_stats)
        
        # Display results
        print(f"Peripheral Statistics:")
        for peripheral, stats in sorted(peripheral_stats.items(), 
                                      key=lambda x: x[1]['total_accesses'], 
                                      reverse=True):
            print(f"   {peripheral:10}: {stats['total_accesses']:3d} accesses, "
                  f"{stats['unique_registers']:2d} registers, "
                  f"{stats['execution_phases']} phases")
    
    def analyze_execution_phases(self):
        """Analyze register access patterns by execution phase"""
        print(f"\n⏱️  EXECUTION PHASE ANALYSIS")
        print("=" * 50)
        
        phase_analysis = defaultdict(lambda: {
            'total_accesses': 0,
            'peripherals': set(),
            'registers': set(),
            'critical_registers': []
        })
        
        for pattern in self.register_patterns.values():
            for phase in pattern.execution_phases:
                analysis = phase_analysis[phase]
                analysis['total_accesses'] += pattern.access_count
                analysis['peripherals'].add(pattern.peripheral)
                analysis['registers'].add(f"{pattern.peripheral}_{pattern.register}")
                
                # Mark as critical if accessed frequently
                if pattern.access_count > 10:
                    analysis['critical_registers'].append(
                        f"{pattern.peripheral}_{pattern.register}"
                    )
        
        # Convert sets to counts for display
        for phase, analysis in phase_analysis.items():
            print(f"\n{phase.upper()} Phase:")
            print(f"   Total accesses: {analysis['total_accesses']}")
            print(f"   Peripherals involved: {len(analysis['peripherals'])}")
            print(f"   Unique registers: {len(analysis['registers'])}")
            print(f"   Critical registers: {len(analysis['critical_registers'])}")
            
            if analysis['critical_registers']:
                print(f"   Top critical registers:")
                for reg in analysis['critical_registers'][:5]:
                    print(f"      - {reg}")
    
    def detect_optimization_opportunities(self) -> List[Dict]:
        """Detect potential optimization opportunities"""
        print(f"\n🚀 OPTIMIZATION OPPORTUNITY DETECTION")
        print("=" * 50)
        
        opportunities = []
        
        # 1. Frequently read registers (potential for caching)
        frequent_reads = [
            (key, pattern) for key, pattern in self.register_patterns.items()
            if pattern.read_count > 20 and pattern.write_count < 5
        ]
        
        if frequent_reads:
            opportunities.append({
                'type': 'register_caching',
                'description': 'Frequently read registers could benefit from caching',
                'registers': [key for key, _ in frequent_reads[:5]],
                'potential_savings': f"{sum(p.read_count for _, p in frequent_reads)} register reads"
            })
        
        # 2. Redundant register writes
        redundant_writes = [
            (key, pattern) for key, pattern in self.register_patterns.items()
            if pattern.write_count > 10 and pattern.read_count == 0
        ]
        
        if redundant_writes:
            opportunities.append({
                'type': 'redundant_writes',
                'description': 'Write-only registers with multiple writes may have redundancy',
                'registers': [key for key, _ in redundant_writes[:5]],
                'potential_savings': f"{sum(p.write_count for _, p in redundant_writes)} register writes"
            })
        
        # 3. Register access clustering
        scattered_accesses = [
            (key, pattern) for key, pattern in self.register_patterns.items()
            if len(pattern.execution_phases) > 2 and pattern.access_count > 5
        ]
        
        if scattered_accesses:
            opportunities.append({
                'type': 'access_clustering',
                'description': 'Registers accessed across multiple phases could be clustered',
                'registers': [key for key, _ in scattered_accesses[:5]],
                'potential_savings': 'Reduced context switching overhead'
            })
        
        # Display opportunities
        for i, opp in enumerate(opportunities, 1):
            print(f"\n{i}. {opp['type'].upper()}")
            print(f"   Description: {opp['description']}")
            print(f"   Affected registers: {len(opp['registers'])}")
            print(f"   Potential savings: {opp['potential_savings']}")
            if len(opp['registers']) <= 5:
                for reg in opp['registers']:
                    print(f"      - {reg}")
            else:
                for reg in opp['registers'][:3]:
                    print(f"      - {reg}")
                print(f"      ... and {len(opp['registers']) - 3} more")
        
        return opportunities
    
    def generate_performance_report(self) -> Dict:
        """Generate comprehensive performance analysis report"""
        print(f"\n📈 GENERATING PERFORMANCE REPORT")
        print("=" * 50)
        
        # Calculate statistics
        access_counts = [p.access_count for p in self.register_patterns.values()]
        read_counts = [p.read_count for p in self.register_patterns.values()]
        write_counts = [p.write_count for p in self.register_patterns.values()]
        
        report = {
            'summary': {
                'total_registers': len(self.register_patterns),
                'total_accesses': sum(access_counts),
                'total_reads': sum(read_counts),
                'total_writes': sum(write_counts),
                'avg_accesses_per_register': statistics.mean(access_counts),
                'median_accesses_per_register': statistics.median(access_counts),
                'max_accesses_per_register': max(access_counts),
                'min_accesses_per_register': min(access_counts)
            },
            'peripheral_stats': self.peripheral_stats,
            'hotspots': [(k, {
                'peripheral': p.peripheral,
                'register': p.register,
                'access_count': p.access_count,
                'read_count': p.read_count,
                'write_count': p.write_count
            }) for k, p in sorted(self.register_patterns.items(), 
                                key=lambda x: x[1].access_count, reverse=True)[:10]],
            'optimization_opportunities': self.detect_optimization_opportunities()
        }
        
        print(f"✅ Performance report generated")
        print(f"   Total registers analyzed: {report['summary']['total_registers']}")
        print(f"   Total accesses: {report['summary']['total_accesses']}")
        print(f"   Average accesses per register: {report['summary']['avg_accesses_per_register']:.1f}")
        
        return report
    
    def export_results(self, output_format: str = 'json', filename: str = None):
        """Export analysis results in various formats"""
        if not filename:
            filename = f"peripheral_analysis_results.{output_format}"
        
        print(f"\n💾 EXPORTING RESULTS")
        print("=" * 50)
        
        if output_format == 'json':
            self._export_json(filename)
        elif output_format == 'csv':
            self._export_csv(filename)
        elif output_format == 'html':
            self._export_html(filename)
        else:
            print(f"❌ Unsupported format: {output_format}")
            return False
        
        print(f"✅ Results exported to: {filename}")
        return True
    
    def _export_json(self, filename: str):
        """Export results as JSON"""
        results = {
            'analysis_summary': {
                'total_registers': len(self.register_patterns),
                'total_accesses': sum(p.access_count for p in self.register_patterns.values())
            },
            'register_patterns': {
                key: {
                    'address': pattern.address,
                    'peripheral': pattern.peripheral,
                    'register': pattern.register,
                    'access_count': pattern.access_count,
                    'read_count': pattern.read_count,
                    'write_count': pattern.write_count,
                    'execution_phases': pattern.execution_phases,
                    'dependencies': pattern.dependencies
                } for key, pattern in self.register_patterns.items()
            },
            'peripheral_stats': self.peripheral_stats
        }
        
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
    
    def _export_csv(self, filename: str):
        """Export results as CSV"""
        import csv
        
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                'Register_Key', 'Address', 'Peripheral', 'Register', 
                'Total_Accesses', 'Read_Count', 'Write_Count', 
                'Execution_Phases', 'Dependencies'
            ])
            
            for key, pattern in self.register_patterns.items():
                writer.writerow([
                    key, pattern.address, pattern.peripheral, pattern.register,
                    pattern.access_count, pattern.read_count, pattern.write_count,
                    ';'.join(pattern.execution_phases),
                    ';'.join(pattern.dependencies[:5])  # Limit dependencies
                ])
    
    def _export_html(self, filename: str):
        """Export results as HTML report"""
        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <title>MIMXRT700 Peripheral Analysis Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        table {{ border-collapse: collapse; width: 100%; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
        .summary {{ background-color: #e7f3ff; padding: 15px; margin: 10px 0; }}
    </style>
</head>
<body>
    <h1>MIMXRT700 Peripheral Register Analysis Report</h1>
    
    <div class="summary">
        <h2>Summary</h2>
        <p>Total Registers: {len(self.register_patterns)}</p>
        <p>Total Accesses: {sum(p.access_count for p in self.register_patterns.values())}</p>
    </div>
    
    <h2>Register Access Patterns</h2>
    <table>
        <tr>
            <th>Peripheral</th>
            <th>Register</th>
            <th>Address</th>
            <th>Total Accesses</th>
            <th>Reads</th>
            <th>Writes</th>
            <th>Phases</th>
        </tr>
"""
        
        for pattern in sorted(self.register_patterns.values(), 
                            key=lambda x: x.access_count, reverse=True):
            html_content += f"""
        <tr>
            <td>{pattern.peripheral}</td>
            <td>{pattern.register}</td>
            <td>{pattern.address}</td>
            <td>{pattern.access_count}</td>
            <td>{pattern.read_count}</td>
            <td>{pattern.write_count}</td>
            <td>{', '.join(pattern.execution_phases)}</td>
        </tr>
"""
        
        html_content += """
    </table>
</body>
</html>
"""
        
        with open(filename, 'w') as f:
            f.write(html_content)

def main():
    """Main function"""
    parser = argparse.ArgumentParser(
        description="Advanced peripheral register analysis for MIMXRT700"
    )
    
    parser.add_argument(
        "--analysis-file", "-a",
        default="complete_enhanced_peripheral_analysis.json",
        help="Peripheral analysis JSON file"
    )
    
    parser.add_argument(
        "--export-format", "-f",
        choices=['json', 'csv', 'html'],
        default='json',
        help="Export format for results"
    )
    
    parser.add_argument(
        "--output", "-o",
        help="Output filename"
    )
    
    args = parser.parse_args()
    
    # Initialize analyzer
    analyzer = AdvancedPeripheralAnalyzer()
    
    # Load data
    if not analyzer.load_analysis_data(args.analysis_file):
        return 1
    
    # Run analysis
    analyzer.analyze_access_patterns()
    analyzer.identify_hotspots()
    analyzer.analyze_peripheral_distribution()
    analyzer.analyze_execution_phases()
    
    # Generate performance report
    report = analyzer.generate_performance_report()
    
    # Export results
    analyzer.export_results(args.export_format, args.output)
    
    print(f"\n🎉 Advanced analysis completed successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
