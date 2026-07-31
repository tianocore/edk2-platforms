# Qualcomm edk2-platforms structure
### UEFI Platform - Glymur & Nord Families

```
edk2-platforms/
|-- Silicon/
|   `-- Qualcomm/
|       |-- Common/
|       |   |-- QualcommCommonSiliconPkg/
|       |   `-- QualcommProtocolPkg/
|       |-- GlymurFamily/
|       |   `-- GlymurSiliconPkg/
|       |       `-- Settings/
|       |           |-- Common/
|       |           |-- Glymur/
|       |           `-- Mahua/
|       `-- NordFamily/
|           `-- NordSiliconPkg/
|               `-- Settings/
|                   |-- Common/
|                   `-- Nord/
`-- Platform/
    |-- MinPlatform/                        (Arch-neutral - tianocore-shared)
    |   `-- Sec, PEI, DxeCore
    `-- Qualcomm/
        |-- QualcommMinPlatformPkg/
        |-- Common/
        |   `-- QualcommCommonPlatformPkg/
        |-- GlymurFamily/
        |   |-- GlymurMinPlatformPkg/
        |   |-- GlymurOpenBoardPkg/
        |   |-- GlymurAbcOpenBoardPkg/
        |   `-- GlymurXyzOpenBoardPkg/
        `-- NordFamily/
            |-- NordMinPlatformPkg/
            |-- NordAuOpenBoardPkg/
            |-- NordDcOpenBoardPkg/
            `-- NordOpenBoardPkg/
```

Legend: [S] = shared, [B] = board-specific, [T] = tianocore-shared

---

### `Silicon/Qualcomm/`

| Directory | Description |
|-----------|-------------|
| `Common/` | Cross-family silicon libraries and protocols shared across all Qualcomm platforms |
| `Common/QualcommCommonSiliconPkg/` | [S] Common silicon-level libraries, e.g. `ChipInfoLib`, shared HW abstraction |
| `Common/QualcommProtocolPkg/` | [S] Qualcomm-specific UEFI protocol definitions shared across silicon families |
| `GlymurFamily/` | Silicon package tree scoped to the **Glymur** SoC family |
| `GlymurFamily/GlymurSiliconPkg/` | [S] Glymur silicon drivers, libraries, and hardware init modules |
| `GlymurFamily/GlymurSiliconPkg/Settings/Common/` | [S] Board-agnostic silicon settings (clocks, regulators, memory configs) shared across Glymur boards |
| `GlymurFamily/GlymurSiliconPkg/Settings/Glymur/` | [S] Silicon settings specific to the Glymur SoC variant |
| `GlymurFamily/GlymurSiliconPkg/Settings/Mahua/` | [S] Silicon settings specific to the Mahua SoC variant |
| `NordFamily/` | Silicon package tree scoped to the **Nord** SoC family |
| `NordFamily/NordSiliconPkg/` | [S] Nord silicon drivers, libraries, and hardware init modules |
| `NordFamily/NordSiliconPkg/Settings/Common/` | [S] Board-agnostic silicon settings shared across all Nord boards |
| `NordFamily/NordSiliconPkg/Settings/Nord/` | [S] Silicon settings specific to the Nord SoC variant |

---

### `Platform/`

| Directory | Description |
|-----------|-------------|
| `MinPlatform/` | [T] **Tianocore-shared**. MinPlatform core: `Sec`, `PEI`, `DxeCore`. Shared across silicon vendors. |
| `Qualcomm/Common/` | Qualcomm platform packages shared across all SoC families |
| `Qualcomm/QualcommMinPlatformPkg/` | [S] Qualcomm-layer MinPlatform
| `Qualcomm/Common/QualcommCommonPlatformPkg/` | [S] Common platform libraries across families, e.g. `BaseDtFrameworkLib`, `PlatformInfoLib` |

---

### `Platform/Qualcomm/GlymurFamily/` [S][B]

| Directory | Description |
|-----------|-------------|
| `GlymurDefaultPkg/` | [B] **Open-source downstream closed-board full-feature package.** Top-level DSC/FDF for full UEFI on Glymur closed boards (CDP, MTP, QRD). Aggregates all layers. |
| `GlymurMinPlatformPkg/` | [S] Board-agnostic Glymur family platform layer. Provides family-wide PEIMs, DXE drivers, and PCDs sitting between `QualcommMinPlatformPkg` and board-specific packages. |
| `GlymurOpenBoardPkg/` | [B] Open-board support package for a Glymur board, built as a thin board layer on top of `GlymurMinPlatformPkg`. |
| `GlymurAbcOpenBoardPkg/` | [B] Open-board support package for a specific Glymur board variant (e.g. RB-series, community boards). |
| `GlymurXyzOpenBoardPkg/` | [B] Additional open-board support package for another Glymur board variant. |

---

### `Platform/Qualcomm/NordFamily/` [S][B]

| Directory | Description |
|-----------|-------------|
| `NordDefaultPkg/` | [B] **Full UEFI closed-board package for Nord.** Top-level DSC/FDF aggregating all Nord layers for closed boards (CDP, MTP, QRD). Hosted at `www.github.com/qualcomm/`. |
| `NordMinPlatformPkg/` | [S] Board-agnostic Nord family platform layer. Provides family-wide PEIMs, DXE drivers, and PCDs. Hosted at `www.github.com/tianocore/`. |
| `NordAuOpenBoardPkg/` | [B] Open-board support package for Nord **AU** (Automotive) product line boards. |
| `NordDcOpenBoardPkg/` | [B] Open-board support package for Nord **DC** (Data Center / Server) product line boards. |
| `Nord*OpenBoardPkg/` | [B] Template / wildcard entry representing additional Nord open-board packages for other BU product lines (e.g. IoT, RBx, Arduino-class boards). |

---
