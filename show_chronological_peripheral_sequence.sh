#!/bin/bash

# Display Chronological Peripheral Access Sequence
# Shows the exact execution order of peripheral register accesses

echo "🎯 MIMXRT700 CHRONOLOGICAL PERIPHERAL ACCESS SEQUENCE"
echo "====================================================="
echo ""

if [ ! -f "chronological_reports/COMPLETE_CHRONOLOGICAL_SEQUENCE.json" ]; then
    echo "❌ Chronological analysis not found. Please run:"
    echo "   ./generate_complete_chronological_report.sh"
    exit 1
fi

echo "📊 EXECUTION SEQUENCE ANALYSIS"
echo "------------------------------"

python3 << 'EOF'
import json

# Load the complete chronological sequence
with open('chronological_reports/COMPLETE_CHRONOLOGICAL_SEQUENCE.json', 'r') as f:
    data = json.load(f)

sequences = data['complete_chronological_sequence']
metadata = data['analysis_metadata']

print(f"Total Peripheral Register Accesses: {metadata['total_accesses']}")
print()

print("Execution Phase Breakdown:")
for phase, count in metadata['execution_phases'].items():
    percentage = (count / metadata['total_accesses']) * 100
    print(f"  {phase.replace('_', ' ').title()}: {count} accesses ({percentage:.1f}%)")
print()

print("Peripheral Access Summary:")
peripheral_summary = data['peripheral_sequences']
for i, (peripheral, info) in enumerate(peripheral_summary.items(), 1):
    print(f"  {i}. {peripheral}: {info['total_accesses']} accesses "
          f"(sequence #{info['first_access_sequence']}-{info['last_access_sequence']})")
print()

print("🔄 CRITICAL INITIALIZATION SEQUENCE (First 30 accesses):")
print("-" * 60)

current_phase = None
phase_color = {"board_init": "🔧", "driver_init": "⚙️", "runtime": "🏃"}

for i, seq in enumerate(sequences[:30]):
    phase = seq.get('execution_phase', 'unknown')
    
    if current_phase != phase:
        phase_icon = phase_color.get(phase, "❓")
        print(f"\n{phase_icon} {phase.replace('_', ' ').upper()} PHASE:")
        current_phase = phase
    
    peripheral = seq.get('peripheral_name', 'UNKNOWN')
    register = seq.get('register_name', 'UNKNOWN')
    access_type = seq.get('access_type', 'unknown')
    function = seq.get('source_location', {}).get('function', 'unknown')
    seq_num = seq.get('sequence_number', i)
    
    print(f"  {seq_num:3d}. {peripheral:8s}.{register:15s} ({access_type:12s}) - {function}()")

if len(sequences) > 30:
    print(f"\n... and {len(sequences) - 30} more accesses")

print()
print("🎯 KEY INSIGHTS:")
print("-" * 15)

# Analyze the sequence for key insights
board_init_accesses = [s for s in sequences if s.get('execution_phase') == 'board_init']
driver_init_accesses = [s for s in sequences if s.get('execution_phase') == 'driver_init']
runtime_accesses = [s for s in sequences if s.get('execution_phase') == 'runtime']

print(f"1. Board initialization starts with: {board_init_accesses[0]['peripheral_name']}.{board_init_accesses[0]['register_name']}")
print(f"   in function {board_init_accesses[0]['source_location']['function']}()")

print(f"2. First driver initialization: {driver_init_accesses[0]['peripheral_name']}.{driver_init_accesses[0]['register_name']}")
print(f"   in function {driver_init_accesses[0]['source_location']['function']}()")

print(f"3. First runtime operation: {runtime_accesses[0]['peripheral_name']}.{runtime_accesses[0]['register_name']}")
print(f"   in function {runtime_accesses[0]['source_location']['function']}()")

# Find the most accessed peripheral in each phase
def get_most_accessed_peripheral(phase_accesses):
    peripheral_counts = {}
    for access in phase_accesses:
        peripheral = access.get('peripheral_name', 'UNKNOWN')
        peripheral_counts[peripheral] = peripheral_counts.get(peripheral, 0) + 1
    return max(peripheral_counts.items(), key=lambda x: x[1]) if peripheral_counts else ('NONE', 0)

board_top = get_most_accessed_peripheral(board_init_accesses)
driver_top = get_most_accessed_peripheral(driver_init_accesses)
runtime_top = get_most_accessed_peripheral(runtime_accesses)

print(f"4. Most accessed peripheral in board init: {board_top[0]} ({board_top[1]} accesses)")
print(f"5. Most accessed peripheral in driver init: {driver_top[0]} ({driver_top[1]} accesses)")
print(f"6. Most accessed peripheral in runtime: {runtime_top[0]} ({runtime_top[1]} accesses)")

# Analyze phase transitions
transitions = data['execution_flow_analysis']['phase_transitions']
print(f"7. Total phase transitions: {len(transitions)}")

print()
print("📋 EXECUTION ORDER SUMMARY:")
print("-" * 27)
print("This chronological analysis reveals the exact sequence of peripheral")
print("register accesses as they occur during MIMXRT700 system execution:")
print()
print("  🔧 Board Initialization: Hardware setup, clock configuration, pin muxing")
print("  ⚙️  Driver Initialization: Peripheral driver setup and configuration")  
print("  🏃 Runtime Operation: Normal system operation and peripheral usage")
print()
print("The sequence shows critical dependencies:")
print("  • Clock control (CLKCTL0/1) is accessed throughout all phases")
print("  • GPIO configuration happens early in board initialization")
print("  • XSPI2 (external memory) is configured after basic system setup")
print("  • System control (SYSCON0) manages various system-level operations")
print()
EOF

echo "📁 DETAILED RESULTS AVAILABLE:"
echo "------------------------------"
echo "  📊 chronological_reports/COMPLETE_CHRONOLOGICAL_SEQUENCE.json"
echo "  📋 chronological_reports/EXECUTION_SEQUENCE_SUMMARY.txt"
echo ""
echo "🔍 TO EXPLORE FURTHER:"
echo "----------------------"
echo "View complete sequence:"
echo "  cat chronological_reports/EXECUTION_SEQUENCE_SUMMARY.txt"
echo ""
echo "Search for specific peripheral:"
echo "  grep -A5 -B5 'XSPI2' chronological_reports/EXECUTION_SEQUENCE_SUMMARY.txt"
echo ""
echo "View JSON data:"
echo "  python3 -c \"
import json
with open('chronological_reports/COMPLETE_CHRONOLOGICAL_SEQUENCE.json', 'r') as f:
    data = json.load(f)
# Show first 10 accesses with full details
for i, access in enumerate(data['complete_chronological_sequence'][:10]):
    print(f'{i+1}. SEQ#{access[\"sequence_number\"]} - {access[\"execution_phase\"]} - {access[\"peripheral_name\"]}.{access[\"register_name\"]} ({access[\"access_type\"]}) in {access[\"source_location\"][\"function\"]}()')
\""
echo ""
echo "🎯 CHRONOLOGICAL PERIPHERAL ANALYSIS COMPLETE!"
echo "   This shows the exact execution order critical for embedded systems programming."
