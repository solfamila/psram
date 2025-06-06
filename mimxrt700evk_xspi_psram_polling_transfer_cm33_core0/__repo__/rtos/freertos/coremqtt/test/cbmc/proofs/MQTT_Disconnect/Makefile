#
# Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

HARNESS_ENTRY=harness
HARNESS_FILE=MQTT_Disconnect_harness
PROOF_UID=MQTT_Disconnect

# Please see test/cbmc/stubs/network_interface_subs.c for
# more information on MAX_NETWORK_SEND_TRIES.
MAX_NETWORK_SEND_TRIES=3
DEFINES += -DMAX_NETWORK_SEND_TRIES=$(MAX_NETWORK_SEND_TRIES)
INCLUDES +=

REMOVE_FUNCTION_BODY +=
# Unlike recvExact, sendBuffer is not bounded by the timeout. The loop in
# sendBuffer will continue until all the bytes are sent or a network error
# occurs. Please see NetworkInterfaceReceiveStub in
# libraries\standard\mqtt\cbmc\stubs\network_interface_stubs.c for more
# information.
UNWINDSET += __CPROVER_file_local_core_mqtt_c_sendBuffer.0:$(MAX_NETWORK_SEND_TRIES)

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c
PROOF_SOURCES += $(SRCDIR)/test/cbmc/sources/mqtt_cbmc_state.c
PROOF_SOURCES += $(SRCDIR)/test/cbmc/stubs/network_interface_stubs.c
PROOF_SOURCES += $(SRCDIR)/test/cbmc/stubs/get_time_stub.c
PROOF_SOURCES += $(SRCDIR)/test/cbmc/stubs/event_callback_stub.c
PROJECT_SOURCES += $(SRCDIR)/source/core_mqtt.c
PROJECT_SOURCES += $(SRCDIR)/source/core_mqtt_serializer.c
PROJECT_SOURCES += $(SRCDIR)/source/core_mqtt_state.c

include ../Makefile.common
