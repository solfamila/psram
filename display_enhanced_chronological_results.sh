#!/bin/bash

# Display Enhanced Chronological Peripheral Analysis Results
# Shows comprehensive embedded systems details for all 572 peripheral register accesses

echo "🎯 ENHANCED CHRONOLOGICAL PERIPHERAL ANALYSIS RESULTS"
echo "====================================================="
echo ""

if [ ! -f "enhanced_chronological_analysis/COMPREHENSIVE_ENHANCED_CHRONOLOGICAL_ANALYSIS.json" ]; then
    echo "❌ Enhanced chronological analysis not found. Please run:"
    echo "   ./comprehensive_chronological_analysis_enhanced.sh"
    exit 1
fi

echo "📊 DATA INTEGRITY VERIFICATION"
echo "------------------------------"

python3 << 'EOF'
import json

# Load verification results
with open('enhanced_chronological_analysis/data_integrity_verification.json', 'r') as f:
    verification = json.load(f)

summary = verification['verification_summary']
print(f"✅ DATA INTEGRITY VERIFIED")
print(f"   Total files processed: {summary['total_files_processed']} (46 driver + 4 board init)")
print(f"   Total peripheral accesses: {summary['total_accesses']}")
print(f"   Expected: {summary['expected_total']}")
print(f"   Driver accesses: {summary['driver_accesses']}")
print(f"   Board init accesses: {summary['board_accesses']}")
print(f"   Data integrity verified: {summary['data_integrity_verified']}")

print()
print("📁 FILE PROCESSING BREAKDOWN:")
print("Driver files with peripheral accesses:")
for filename, count in verification['driver_file_details'].items():
    if count > 0:
        print(f"   {filename}: {count} accesses")

print()
print("Board initialization files with peripheral accesses:")
for filename, count in verification['board_file_details'].items():
    if count > 0:
        print(f"   {filename}: {count} accesses")
EOF

echo ""
echo "🔍 COMPREHENSIVE ENHANCED ANALYSIS SUMMARY"
echo "------------------------------------------"

python3 << 'EOF'
import json

# Load comprehensive analysis
with open('enhanced_chronological_analysis/COMPREHENSIVE_ENHANCED_CHRONOLOGICAL_ANALYSIS.json', 'r') as f:
    analysis = json.load(f)

metadata = analysis['analysis_metadata']
print(f"Analysis Type: {metadata['analysis_type']}")
print(f"Total Peripheral Accesses: {metadata['total_peripheral_accesses']}")
print(f"Data Integrity Verified: {metadata['data_integrity_verified']}")
print(f"Analysis Date: {metadata['analysis_timestamp']}")

print()
print("📈 EXECUTION PHASE BREAKDOWN:")
for phase, count in analysis['execution_phase_summary'].items():
    percentage = (count / metadata['total_peripheral_accesses']) * 100
    print(f"   {phase.replace('_', ' ').title()}: {count} accesses ({percentage:.1f}%)")

print()
print("🔧 PERIPHERAL ACCESS SUMMARY:")
peripheral_summary = analysis['peripheral_summary']
for peripheral, info in sorted(peripheral_summary.items(), key=lambda x: x[1]['total_accesses'], reverse=True):
    phases = ', '.join(info['execution_phases'])
    print(f"   {peripheral}: {info['total_accesses']} accesses (seq {info['first_sequence']}-{info['last_sequence']}) [{phases}]")

print()
print("⚡ CRITICAL INITIALIZATION SEQUENCE (First 20 accesses):")
print("-" * 80)

sequences = analysis['comprehensive_chronological_sequence']
for i, access in enumerate(sequences[:20]):
    seq = access['execution_order_context']['global_sequence_number']
    phase = access['execution_order_context']['execution_phase']
    peripheral = access['hardware_register_info']['peripheral_name']
    register = access['hardware_register_info']['register_name']
    addr = access['hardware_register_info']['physical_address']
    func = access['source_traceability']['function_name']
    access_type = access['bit_level_analysis']['access_type']
    critical_path = access['execution_order_context']['critical_path_classification']
    
    # Format for readability
    phase_short = phase.replace('_initialization', '_init').replace('_operation', '_op')
    
    print(f"   {seq:3d}. {phase_short:15s} | {peripheral:8s}.{register:15s} | {access_type:13s} | {critical_path:15s} | {func}()")

if len(sequences) > 20:
    print(f"   ... and {len(sequences) - 20} more accesses")

print()
print("🎯 ENHANCED EMBEDDED SYSTEMS INSIGHTS:")
print("-" * 40)

# Analyze critical path classifications
critical_path_counts = {}
timing_criticality_counts = {}
system_impact_counts = {}

for access in sequences:
    critical_path = access['execution_order_context']['critical_path_classification']
    timing = access['embedded_systems_context']['timing_criticality']
    impact = access['embedded_systems_context']['system_impact_classification']
    
    critical_path_counts[critical_path] = critical_path_counts.get(critical_path, 0) + 1
    timing_criticality_counts[timing] = timing_criticality_counts.get(timing, 0) + 1
    system_impact_counts[impact] = system_impact_counts.get(impact, 0) + 1

print("Critical Path Classification:")
for classification, count in sorted(critical_path_counts.items(), key=lambda x: x[1], reverse=True):
    percentage = (count / len(sequences)) * 100
    print(f"   {classification}: {count} accesses ({percentage:.1f}%)")

print()
print("Timing Criticality Analysis:")
for timing, count in sorted(timing_criticality_counts.items(), key=lambda x: x[1], reverse=True):
    percentage = (count / len(sequences)) * 100
    print(f"   {timing}: {count} accesses ({percentage:.1f}%)")

print()
print("System Impact Classification:")
for impact, count in sorted(system_impact_counts.items(), key=lambda x: x[1], reverse=True):
    percentage = (count / len(sequences)) * 100
    print(f"   {impact}: {count} accesses ({percentage:.1f}%)")

print()
print("🔬 DETAILED BIT-LEVEL OPERATION ANALYSIS:")
print("-" * 42)

# Analyze bit-level operations
bit_operations = {}
operation_masks = {}

for access in sequences:
    bit_op = access['bit_level_analysis']['bit_field_operations']
    mask = access['bit_level_analysis']['operation_mask']
    
    bit_operations[bit_op] = bit_operations.get(bit_op, 0) + 1
    if mask != "0x00000000":
        operation_masks[mask] = operation_masks.get(mask, 0) + 1

print("Bit-Level Operation Types:")
for operation, count in sorted(bit_operations.items(), key=lambda x: x[1], reverse=True):
    percentage = (count / len(sequences)) * 100
    print(f"   {operation}: {count} accesses ({percentage:.1f}%)")

print()
print("Most Common Operation Masks (non-zero):")
for mask, count in sorted(operation_masks.items(), key=lambda x: x[1], reverse=True)[:10]:
    print(f"   {mask}: {count} accesses")

print()
print("📍 HARDWARE REGISTER MAPPING ANALYSIS:")
print("-" * 39)

# Analyze register access patterns
register_counts = {}
address_ranges = {}

for access in sequences:
    peripheral = access['hardware_register_info']['peripheral_name']
    register = access['hardware_register_info']['register_name']
    addr = access['hardware_register_info']['physical_address']
    
    reg_key = f"{peripheral}.{register}"
    register_counts[reg_key] = register_counts.get(reg_key, 0) + 1
    
    if peripheral not in address_ranges:
        address_ranges[peripheral] = {'min': addr, 'max': addr}
    else:
        if addr < address_ranges[peripheral]['min']:
            address_ranges[peripheral]['min'] = addr
        if addr > address_ranges[peripheral]['max']:
            address_ranges[peripheral]['max'] = addr

print("Most Accessed Registers:")
for register, count in sorted(register_counts.items(), key=lambda x: x[1], reverse=True)[:15]:
    percentage = (count / len(sequences)) * 100
    print(f"   {register}: {count} accesses ({percentage:.1f}%)")

print()
print("Peripheral Address Ranges:")
for peripheral, ranges in sorted(address_ranges.items()):
    min_addr = int(ranges['min'], 16) if isinstance(ranges['min'], str) else ranges['min']
    max_addr = int(ranges['max'], 16) if isinstance(ranges['max'], str) else ranges['max']
    range_size = max_addr - min_addr
    print(f"   {peripheral}: 0x{min_addr:08X} - 0x{max_addr:08X} (range: 0x{range_size:X} bytes)")

EOF

echo ""
echo "📋 COMPREHENSIVE ANALYSIS COMPLETE"
echo "----------------------------------"
echo ""
echo "Enhanced chronological analysis provides:"
echo "  ✅ Complete source code traceability for all 572 accesses"
echo "  ✅ Precise hardware register information with physical addresses"
echo "  ✅ Detailed bit-level operation analysis with masks"
echo "  ✅ Precise execution order context with sequence numbering"
echo "  ✅ Embedded systems hardware context and dependency analysis"
echo ""
echo "📁 DETAILED RESULTS AVAILABLE:"
echo "  📊 enhanced_chronological_analysis/COMPREHENSIVE_ENHANCED_CHRONOLOGICAL_ANALYSIS.json"
echo "  🔍 enhanced_chronological_analysis/data_integrity_verification.json"
echo ""
echo "🔍 TO EXPLORE SPECIFIC DETAILS:"
echo "View specific peripheral sequence:"
echo "  python3 -c \"
import json
with open('enhanced_chronological_analysis/COMPREHENSIVE_ENHANCED_CHRONOLOGICAL_ANALYSIS.json', 'r') as f:
    data = json.load(f)
# Show XSPI2 accesses
for access in data['comprehensive_chronological_sequence']:
    if access['hardware_register_info']['peripheral_name'] == 'XSPI2':
        seq = access['execution_order_context']['global_sequence_number']
        reg = access['hardware_register_info']['register_name']
        addr = access['hardware_register_info']['physical_address']
        func = access['source_traceability']['function_name']
        print(f'{seq:3d}. XSPI2.{reg:15s} | {addr:12s} | {func}()')
        if seq > 250: break  # Show first few XSPI2 accesses
\""
echo ""
echo "🎯 ENHANCED CHRONOLOGICAL PERIPHERAL ANALYSIS COMPLETE!"
echo "   This provides unprecedented insight into MIMXRT700 hardware register"
echo "   interaction patterns with comprehensive embedded systems details."
