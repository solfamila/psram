# Changelog Multicore Manager

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [4.1.7]

### Fixed

- `mcmgr_stop_core_internal()` function now returns `kStatus_MCMGR_NotImplemented` status code instead  
  of `kStatus_MCMGR_Success` when device does not support stop of secondary core.  
  Ports affected: `kw32w1`, `kw45b41`, `kw45b42`, `mcxw716`, `mcxw727`.

## [v4.1.6]

### Added

- Multicore Manager moved to standalone repository.
- Add porting layers for imxrt700, mcmxw727, kw47b42.
- New MCMGR_ProcessDeferredRxIsr() API added.

## [v4.1.5]

### Added

- Add notification into MCMGR_EarlyInit and mcmgr_early_init_internal functions to avoid using uninitialized data in their implementations.

## [v4.1.4]

### Fixed

- Avoid calling tx isr callbacks when respective Messaging Unit Transmit Interrupt Enable flag is not set in the CR/TCR register.
- Messaging Unit RX and status registers are cleared after the initialization.

## [v4.1.3]

### Added

- Add porting layers for imxrt1180.

### Fixed

- mu_isr() updated to avoid calling tx isr callbacks when respective Transmit Interrupt Enable flag is not set in the CR/TCR register.
- mcmgr_mu_internal.c code adaptation to new supported SoCs.

## [v4.1.2]

### Fixed

- Update mcmgr_stop_core_internal() implementations to set core state to kMCMGR_ResetCoreState.

## [v4.1.0]

### Fixed

- Code adjustments to address MISRA C-2012 Rules

## [v4.0.3]

### Fixed

- Documentation updated to describe handshaking in a graphic form.
- Minor code adjustments based on static analysis tool findings

## [v4.0.2]

### Fixed

- Align porting layers to the updated MCUXpressoSDK feature files.

## [v4.0.1]

### Fixed

- Code formatting, removed unused code

## [v4.0.0]

### Added

- Add new MCMGR_TriggerEventForce() API.

## [v3.0.0]

### Removed

- Removed MCMGR_LoadApp(), MCMGR_MapAddress() and MCMGR_SignalReady()

### Modified

- Modified MCMGR_GetStartupData()

### Added

- Added MCMGR_EarlyInit(), MCMGR_RegisterEvent() and MCMGR_TriggerEvent()
- Added the ability for remote core monitoring and event handling

## [v2.0.1]

### Fixed

- Updated to be Misra compliant.

## [v2.0.0]

### Added

- Support for lpcxpresso54114 board.

## [v1.1.0]

### Fixed

- Ported to KSDK 2.0.0.

## [v1.0.0]

### Added

- Initial release.

[unreleased]: https://github.com/nxp-mcuxpresso/mcmgr
