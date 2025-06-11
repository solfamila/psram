#!/usr/bin/env python3
"""
Peripheral Register Access Visualizer for MIMXRT700
==================================================

This script creates comprehensive visualizations of peripheral register
access patterns, including charts, graphs, and interactive plots.

Features:
1. Peripheral access distribution charts
2. Timeline visualization of register accesses
3. Heatmaps of register usage
4. Execution phase analysis plots
5. Interactive dashboards
6. Export capabilities for presentations

Usage:
    python3 peripheral_visualizer.py [options]
"""

import json
import sys
import os
import argparse
from collections import defaultdict, Counter
from typing import Dict, List, Tuple
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
from datetime import datetime

# Try to import seaborn, use fallback if not available
try:
    import seaborn as sns
    HAS_SEABORN = True
except ImportError:
    HAS_SEABORN = False
    print("⚠️  Seaborn not available, using matplotlib defaults")

class PeripheralVisualizer:
    """Visualizer for peripheral register access patterns"""
    
    def __init__(self):
        self.analysis_data = None
        self.fig_count = 0
        
        # Set up plotting style
        if HAS_SEABORN:
            plt.style.use('seaborn-v0_8')
            sns.set_palette("husl")
        else:
            plt.style.use('default')
            plt.rcParams['figure.facecolor'] = 'white'
        
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
    
    def create_peripheral_distribution_chart(self, save_path: str = None):
        """Create peripheral access distribution pie chart"""
        print("📊 Creating peripheral distribution chart...")
        
        # Count accesses by peripheral
        peripheral_counts = Counter()
        for access in self.analysis_data['chronological_sequence']:
            peripheral_counts[access['peripheral_name']] += 1
        
        # Prepare data
        peripherals = list(peripheral_counts.keys())
        counts = list(peripheral_counts.values())
        
        # Create figure
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 8))
        
        # Pie chart
        colors = plt.cm.Set3(np.linspace(0, 1, len(peripherals)))
        wedges, texts, autotexts = ax1.pie(counts, labels=peripherals, autopct='%1.1f%%',
                                          colors=colors, startangle=90)
        ax1.set_title('Peripheral Access Distribution', fontsize=14, fontweight='bold')
        
        # Bar chart
        bars = ax2.bar(peripherals, counts, color=colors)
        ax2.set_title('Peripheral Access Counts', fontsize=14, fontweight='bold')
        ax2.set_xlabel('Peripheral')
        ax2.set_ylabel('Number of Accesses')
        ax2.tick_params(axis='x', rotation=45)
        
        # Add value labels on bars
        for bar in bars:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}', ha='center', va='bottom')
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"✅ Saved: {save_path}")
        
        self.fig_count += 1
        return fig
    
    def create_execution_phase_timeline(self, save_path: str = None):
        """Create timeline visualization of execution phases"""
        print("⏱️ Creating execution phase timeline...")
        
        # Group accesses by phase
        phase_data = defaultdict(list)
        for access in self.analysis_data['chronological_sequence']:
            phase_data[access['execution_phase']].append(access['sequence_number'])
        
        # Create figure
        fig, ax = plt.subplots(figsize=(14, 8))
        
        # Define colors for phases
        phase_colors = {
            'board_init': '#FF6B6B',
            'driver_init': '#4ECDC4', 
            'runtime': '#45B7D1'
        }
        
        y_pos = 0
        phase_positions = {}
        
        for phase, sequences in phase_data.items():
            color = phase_colors.get(phase, '#95A5A6')
            
            # Create timeline bars
            for seq in sequences:
                ax.barh(y_pos, 1, left=seq, height=0.8, color=color, alpha=0.7)
            
            phase_positions[phase] = y_pos
            y_pos += 1
        
        # Customize plot
        ax.set_yticks(list(phase_positions.values()))
        ax.set_yticklabels(list(phase_positions.keys()))
        ax.set_xlabel('Sequence Number')
        ax.set_title('Register Access Timeline by Execution Phase', 
                    fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        
        # Add legend
        legend_elements = [patches.Patch(color=color, label=phase) 
                          for phase, color in phase_colors.items() 
                          if phase in phase_data]
        ax.legend(handles=legend_elements, loc='upper right')
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"✅ Saved: {save_path}")
        
        self.fig_count += 1
        return fig
    
    def create_register_heatmap(self, save_path: str = None):
        """Create heatmap of register access patterns"""
        print("🔥 Creating register access heatmap...")
        
        # Prepare data for heatmap
        peripheral_register_counts = defaultdict(lambda: defaultdict(int))
        
        for access in self.analysis_data['chronological_sequence']:
            peripheral = access['peripheral_name']
            register = access['register_name']
            peripheral_register_counts[peripheral][register] += 1
        
        # Get top peripherals and registers
        top_peripherals = sorted(peripheral_register_counts.keys(), 
                               key=lambda p: sum(peripheral_register_counts[p].values()),
                               reverse=True)[:8]
        
        # Create matrix
        all_registers = set()
        for peripheral in top_peripherals:
            all_registers.update(peripheral_register_counts[peripheral].keys())
        
        top_registers = sorted(all_registers, 
                             key=lambda r: sum(peripheral_register_counts[p][r] 
                                             for p in top_peripherals),
                             reverse=True)[:15]
        
        # Build matrix
        matrix = np.zeros((len(top_peripherals), len(top_registers)))
        for i, peripheral in enumerate(top_peripherals):
            for j, register in enumerate(top_registers):
                matrix[i, j] = peripheral_register_counts[peripheral][register]
        
        # Create heatmap
        fig, ax = plt.subplots(figsize=(14, 8))
        
        im = ax.imshow(matrix, cmap='YlOrRd', aspect='auto')
        
        # Set ticks and labels
        ax.set_xticks(np.arange(len(top_registers)))
        ax.set_yticks(np.arange(len(top_peripherals)))
        ax.set_xticklabels(top_registers, rotation=45, ha='right')
        ax.set_yticklabels(top_peripherals)
        
        # Add colorbar
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label('Number of Accesses', rotation=270, labelpad=15)
        
        # Add text annotations
        for i in range(len(top_peripherals)):
            for j in range(len(top_registers)):
                if matrix[i, j] > 0:
                    text = ax.text(j, i, int(matrix[i, j]),
                                 ha="center", va="center", color="black" if matrix[i, j] < matrix.max()/2 else "white")
        
        ax.set_title('Register Access Heatmap by Peripheral', 
                    fontsize=14, fontweight='bold')
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"✅ Saved: {save_path}")
        
        self.fig_count += 1
        return fig
    
    def create_access_type_analysis(self, save_path: str = None):
        """Create analysis of access types (read/write)"""
        print("📈 Creating access type analysis...")
        
        # Count access types
        access_type_counts = Counter()
        peripheral_access_types = defaultdict(lambda: defaultdict(int))
        
        for access in self.analysis_data['chronological_sequence']:
            access_type = access['access_type']
            peripheral = access['peripheral_name']
            
            access_type_counts[access_type] += 1
            peripheral_access_types[peripheral][access_type] += 1
        
        # Create subplots
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
        
        # 1. Overall access type distribution
        access_types = list(access_type_counts.keys())
        counts = list(access_type_counts.values())
        colors = ['#FF6B6B', '#4ECDC4', '#45B7D1']
        
        ax1.pie(counts, labels=access_types, autopct='%1.1f%%', colors=colors)
        ax1.set_title('Overall Access Type Distribution')
        
        # 2. Access types by peripheral (stacked bar)
        peripherals = list(peripheral_access_types.keys())[:8]  # Top 8 peripherals
        access_type_list = list(access_type_counts.keys())
        
        bottom = np.zeros(len(peripherals))
        for i, access_type in enumerate(access_type_list):
            values = [peripheral_access_types[p][access_type] for p in peripherals]
            ax2.bar(peripherals, values, bottom=bottom, label=access_type, color=colors[i % len(colors)])
            bottom += values
        
        ax2.set_title('Access Types by Peripheral')
        ax2.set_xlabel('Peripheral')
        ax2.set_ylabel('Number of Accesses')
        ax2.legend()
        ax2.tick_params(axis='x', rotation=45)
        
        # 3. Read vs Write ratio
        read_counts = []
        write_counts = []
        peripheral_names = []
        
        for peripheral in peripherals:
            reads = (peripheral_access_types[peripheral]['volatile_read'] + 
                    peripheral_access_types[peripheral].get('function_call_read', 0))
            writes = (peripheral_access_types[peripheral]['volatile_write'] + 
                     peripheral_access_types[peripheral]['function_call_write'])
            
            if reads + writes > 0:  # Only include peripherals with accesses
                read_counts.append(reads)
                write_counts.append(writes)
                peripheral_names.append(peripheral)
        
        x = np.arange(len(peripheral_names))
        width = 0.35
        
        ax3.bar(x - width/2, read_counts, width, label='Reads', color='#4ECDC4')
        ax3.bar(x + width/2, write_counts, width, label='Writes', color='#FF6B6B')
        
        ax3.set_title('Read vs Write Accesses by Peripheral')
        ax3.set_xlabel('Peripheral')
        ax3.set_ylabel('Number of Accesses')
        ax3.set_xticks(x)
        ax3.set_xticklabels(peripheral_names, rotation=45)
        ax3.legend()
        
        # 4. Access frequency over sequence
        sequence_numbers = [access['sequence_number'] for access in self.analysis_data['chronological_sequence']]
        
        # Create bins for sequence numbers
        bins = np.linspace(min(sequence_numbers), max(sequence_numbers), 50)
        ax4.hist(sequence_numbers, bins=bins, alpha=0.7, color='#45B7D1')
        ax4.set_title('Register Access Frequency Over Time')
        ax4.set_xlabel('Sequence Number')
        ax4.set_ylabel('Number of Accesses')
        ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"✅ Saved: {save_path}")
        
        self.fig_count += 1
        return fig
    
    def create_peripheral_dependency_graph(self, save_path: str = None):
        """Create dependency graph showing peripheral relationships"""
        print("🔗 Creating peripheral dependency graph...")
        
        # Analyze sequence dependencies
        peripheral_sequences = defaultdict(list)
        for access in self.analysis_data['chronological_sequence']:
            peripheral_sequences[access['peripheral_name']].append(access['sequence_number'])
        
        # Find dependencies (peripherals accessed close in sequence)
        dependencies = defaultdict(set)
        window_size = 10  # Look within 10 sequence positions
        
        for access in self.analysis_data['chronological_sequence']:
            current_peripheral = access['peripheral_name']
            current_seq = access['sequence_number']
            
            # Find other peripherals accessed within window
            for other_access in self.analysis_data['chronological_sequence']:
                other_peripheral = other_access['peripheral_name']
                other_seq = other_access['sequence_number']
                
                if (current_peripheral != other_peripheral and 
                    abs(current_seq - other_seq) <= window_size):
                    dependencies[current_peripheral].add(other_peripheral)
        
        # Create network-style visualization
        fig, ax = plt.subplots(figsize=(12, 10))
        
        peripherals = list(peripheral_sequences.keys())
        n_peripherals = len(peripherals)
        
        # Position peripherals in a circle
        angles = np.linspace(0, 2*np.pi, n_peripherals, endpoint=False)
        positions = {peripheral: (np.cos(angle), np.sin(angle)) 
                    for peripheral, angle in zip(peripherals, angles)}
        
        # Draw connections
        for peripheral, deps in dependencies.items():
            if peripheral in positions:
                x1, y1 = positions[peripheral]
                for dep in deps:
                    if dep in positions:
                        x2, y2 = positions[dep]
                        ax.plot([x1, x2], [y1, y2], 'gray', alpha=0.3, linewidth=0.5)
        
        # Draw peripheral nodes
        for peripheral, (x, y) in positions.items():
            # Size based on number of accesses
            size = len(peripheral_sequences[peripheral]) * 2
            ax.scatter(x, y, s=size, alpha=0.7, label=peripheral)
            ax.text(x, y+0.15, peripheral, ha='center', va='bottom', fontsize=8)
        
        ax.set_xlim(-1.5, 1.5)
        ax.set_ylim(-1.5, 1.5)
        ax.set_aspect('equal')
        ax.set_title('Peripheral Dependency Network\n(Node size = access count, Lines = sequence proximity)', 
                    fontsize=12, fontweight='bold')
        ax.axis('off')
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"✅ Saved: {save_path}")
        
        self.fig_count += 1
        return fig
    
    def create_comprehensive_dashboard(self, output_dir: str = "visualization_output"):
        """Create comprehensive visualization dashboard"""
        print(f"\n📊 CREATING COMPREHENSIVE VISUALIZATION DASHBOARD")
        print("=" * 60)
        
        # Create output directory
        os.makedirs(output_dir, exist_ok=True)
        
        # Generate all visualizations
        figures = []
        
        # 1. Peripheral distribution
        fig1 = self.create_peripheral_distribution_chart(
            os.path.join(output_dir, "01_peripheral_distribution.png")
        )
        figures.append(("Peripheral Distribution", fig1))
        
        # 2. Execution timeline
        fig2 = self.create_execution_phase_timeline(
            os.path.join(output_dir, "02_execution_timeline.png")
        )
        figures.append(("Execution Timeline", fig2))
        
        # 3. Register heatmap
        fig3 = self.create_register_heatmap(
            os.path.join(output_dir, "03_register_heatmap.png")
        )
        figures.append(("Register Heatmap", fig3))
        
        # 4. Access type analysis
        fig4 = self.create_access_type_analysis(
            os.path.join(output_dir, "04_access_type_analysis.png")
        )
        figures.append(("Access Type Analysis", fig4))
        
        # 5. Dependency graph
        fig5 = self.create_peripheral_dependency_graph(
            os.path.join(output_dir, "05_dependency_graph.png")
        )
        figures.append(("Dependency Graph", fig5))
        
        # Create HTML dashboard
        self._create_html_dashboard(output_dir, figures)
        
        print(f"\n✅ Dashboard created successfully!")
        print(f"   Output directory: {output_dir}")
        print(f"   Generated {len(figures)} visualizations")
        print(f"   Open {output_dir}/dashboard.html to view")
        
        return figures
    
    def _create_html_dashboard(self, output_dir: str, figures: List[Tuple]):
        """Create HTML dashboard with all visualizations"""
        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <title>MIMXRT700 Peripheral Analysis Dashboard</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }}
        .header {{ background-color: #2c3e50; color: white; padding: 20px; text-align: center; }}
        .container {{ max-width: 1200px; margin: 0 auto; }}
        .visualization {{ background-color: white; margin: 20px 0; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        .visualization img {{ max-width: 100%; height: auto; }}
        .summary {{ background-color: #ecf0f1; padding: 15px; margin: 20px 0; border-radius: 5px; }}
        .stats {{ display: flex; justify-content: space-around; margin: 20px 0; }}
        .stat {{ text-align: center; }}
        .stat-number {{ font-size: 2em; font-weight: bold; color: #3498db; }}
        .stat-label {{ color: #7f8c8d; }}
    </style>
</head>
<body>
    <div class="header">
        <h1>MIMXRT700 Peripheral Register Analysis Dashboard</h1>
        <p>Generated on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
    </div>
    
    <div class="container">
        <div class="summary">
            <h2>Analysis Summary</h2>
            <div class="stats">
                <div class="stat">
                    <div class="stat-number">{self.analysis_data['total_accesses']}</div>
                    <div class="stat-label">Total Accesses</div>
                </div>
                <div class="stat">
                    <div class="stat-number">{self.analysis_data['files_analyzed']}</div>
                    <div class="stat-label">Files Analyzed</div>
                </div>
                <div class="stat">
                    <div class="stat-number">{len(set(a['peripheral_name'] for a in self.analysis_data['chronological_sequence']))}</div>
                    <div class="stat-label">Peripherals</div>
                </div>
                <div class="stat">
                    <div class="stat-number">{len(figures)}</div>
                    <div class="stat-label">Visualizations</div>
                </div>
            </div>
        </div>
"""
        
        # Add each visualization
        for i, (title, _) in enumerate(figures, 1):
            image_filename = f"{i:02d}_{title.lower().replace(' ', '_')}.png"
            html_content += f"""
        <div class="visualization">
            <h2>{title}</h2>
            <img src="{image_filename}" alt="{title}">
        </div>
"""
        
        html_content += """
    </div>
</body>
</html>
"""
        
        dashboard_path = os.path.join(output_dir, "dashboard.html")
        with open(dashboard_path, 'w') as f:
            f.write(html_content)

def main():
    """Main function"""
    parser = argparse.ArgumentParser(
        description="Create visualizations for MIMXRT700 peripheral analysis"
    )
    
    parser.add_argument(
        "--analysis-file", "-a",
        default="complete_enhanced_peripheral_analysis.json",
        help="Peripheral analysis JSON file"
    )
    
    parser.add_argument(
        "--output-dir", "-o",
        default="visualization_output",
        help="Output directory for visualizations"
    )
    
    parser.add_argument(
        "--dashboard", "-d",
        action="store_true",
        help="Create comprehensive dashboard"
    )
    
    args = parser.parse_args()
    
    # Initialize visualizer
    visualizer = PeripheralVisualizer()
    
    # Load data
    if not visualizer.load_analysis_data(args.analysis_file):
        return 1
    
    if args.dashboard:
        # Create comprehensive dashboard
        visualizer.create_comprehensive_dashboard(args.output_dir)
    else:
        # Create individual visualizations
        os.makedirs(args.output_dir, exist_ok=True)
        
        visualizer.create_peripheral_distribution_chart(
            os.path.join(args.output_dir, "peripheral_distribution.png")
        )
        visualizer.create_execution_phase_timeline(
            os.path.join(args.output_dir, "execution_timeline.png")
        )
        visualizer.create_register_heatmap(
            os.path.join(args.output_dir, "register_heatmap.png")
        )
    
    print(f"\n🎉 Visualization completed successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
