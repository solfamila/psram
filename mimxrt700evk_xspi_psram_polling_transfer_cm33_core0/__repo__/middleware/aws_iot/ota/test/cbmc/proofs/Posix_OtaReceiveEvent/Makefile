
HARNESS_ENTRY = Posix_OtaReceiveEvent_harness
HARNESS_FILE = $(HARNESS_ENTRY)

# This should be a unique identifier for this proof, and will appear on the
# Litani dashboard. It can be human-readable and contain spaces if you wish.
PROOF_UID = Posix_OtaReceiveEvent_harness

INCLUDES += -I$(SRCDIR)/source/portable/os/

DEFINES += -DBOUND=10

UNWINDSET += Posix_OtaReceiveEvent.0:10

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

PROJECT_SOURCES += $(SRCDIR)/source/portable/os/ota_os_posix.c

# If this proof is found to consume huge amounts of RAM, you can set the
# EXPENSIVE variable. With new enough versions of the proof tools, this will
# restrict the number of EXPENSIVE CBMC jobs running at once. See the
# documentation in Makefile.common under the "Job Pools" heading for details.
# EXPENSIVE = true

include ../Makefile.common
