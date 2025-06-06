# lpspi_edma_b2b_transfer_slave

## Overview
The lpspi_edma_b2b_transfer example shows how to use LPSPI driver in edma way:

In this example , we need two boards, one board used as LPSPI master and another board used as LPSPI slave.
The file 'lpspi_edma_b2b_transfer_slave.c' includes the LPSPI slave code.

1. LPSPI master send/received data to/from LPSPI slave in edma . (LPSPI Slave using edma to receive/send the data)

## Supported Boards
- [EVK9-MIMX8ULP](../../../../_boards/evk9mimx8ulp/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [EVKB-IMXRT1050](../../../../_boards/evkbimxrt1050/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT1060-EVKB](../../../../_boards/evkbmimxrt1060/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT1170-EVKB](../../../../_boards/evkbmimxrt1170/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT1060-EVKC](../../../../_boards/evkcmimxrt1060/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [EVK-MCIMX7ULP](../../../../_boards/evkmcimx7ulp/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [EVK-MIMX8ULP](../../../../_boards/evkmimx8ulp/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [EVK-MIMXRT1010](../../../../_boards/evkmimxrt1010/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [EVK-MIMXRT1015](../../../../_boards/evkmimxrt1015/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [EVK-MIMXRT1020](../../../../_boards/evkmimxrt1020/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT1024-EVK](../../../../_boards/evkmimxrt1024/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT1040-EVK](../../../../_boards/evkmimxrt1040/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [EVK-MIMXRT1064](../../../../_boards/evkmimxrt1064/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT1160-EVK](../../../../_boards/evkmimxrt1160/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT1180-EVK](../../../../_boards/evkmimxrt1180/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-K32L2A4S](../../../../_boards/frdmk32l2a4s/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-K32L3A6](../../../../_boards/frdmk32l3a6/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-KE15Z](../../../../_boards/frdmke15z/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-KE17Z](../../../../_boards/frdmke17z/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-KE17Z512](../../../../_boards/frdmke17z512/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXA153](../../../../_boards/frdmmcxa153/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXA156](../../../../_boards/frdmmcxa156/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXA166](../../../../_boards/frdmmcxa166/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXA276](../../../../_boards/frdmmcxa276/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXN236](../../../../_boards/frdmmcxn236/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXE247](../../../../_boards/frdmmcxe247/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXN947](../../../../_boards/frdmmcxn947/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXW71](../../../../_boards/frdmmcxw71/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MCX-W71-EVK](../../../../_boards/mcxw71evk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [FRDM-MCXW72](../../../../_boards/frdmmcxw72/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [K32W148-EVK](../../../../_boards/k32w148evk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [KW45B41Z-EVK](../../../../_boards/kw45b41zevk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [KW47-EVK](../../../../_boards/kw47evk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MCX-N5XX-EVK](../../../../_boards/mcxn5xxevk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MCX-N9XX-EVK](../../../../_boards/mcxn9xxevk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MCX-W72-EVK](../../../../_boards/mcxw72evk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
- [MIMXRT700-EVK](../../../../_boards/mimxrt700evk/driver_examples/lpspi/edma_b2b_transfer/slave/example_board_readme.md)
