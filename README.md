# High-Precision ARINC 429 Avionics Protocol Analyzer & Flight Telemetry Decoder

[![Standard: ARINC 429-16](https://img.shields.io/badge/Aero_Standard-ARINC%20429--16-red.svg)](https://www.aviation-ia.com/)
[![Language: C99](https://img.shields.io/badge/Language-C99-brightgreen.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Safety-Critical: DO-178C Ready](https://img.shields.io/badge/Avionics-DO--178C%20DAL%20B-purple.svg)]()

A robust, real-time **ARINC 429 Avionics Data Bus Protocol Engine** written in standard ANSI C99. Designed for primary flight computer interfaces, line-replaceable unit (LRU) simulators, and automated test benches in commercial and military aerospace programs (Airbus A400M / A350 / A320, Boeing 777 / 787).

---

## ✈️ Specification & Bit Structure

The engine strictly implements the **ARINC Specification 429-16 (Digital Information Transfer System - DITS)** standard for 32-bit serial data transmission over twisted shielded pairs:

```
+----+----+------------------------------------+-----+-----+
| 32 |3130| 29                              11 |10  9| 8  1|
+----+----+------------------------------------+-----+-----+
| P  |SSM |            DATA FIELD              | SDI |LABEL|
+----+----+------------------------------------+-----+-----+
  ^    ^                    ^                     ^     ^
  |    |                    |                     |     +-- 8-bit octal label (transmitted LSB first)
  |    |                    |                     +-------- Source/Destination Identifier (0..3)
  |    |                    +------------------------------ 19-bit payload (BNR two's complement or BCD)
  |    +--------------------------------------------------- Sign/Status Matrix (Failure, NCD, Test, Normal)
  +-------------------------------------------------------- Odd Parity bit (total 1s in word must be odd)
```

### Supported Parameters & Labels:
| Label (Octal) | Parameter Name | Representation | Significant Bits | LSB Resolution | Full Scale Range |
|---|---|---|---|---|---|
| **0203** | Pressure Altitude | BNR (Two's Compl.) | 17 bits | 1.00 foot | -1,000 to +131,072 ft |
| **0204** | Baro Corrected Altitude | BNR | 17 bits | 1.00 foot | -1,000 to +131,072 ft |
| **0210** | True Airspeed (TAS) | BNR | 14 bits | 0.125 knots | 0 to 2,048 knots |
| **0320** | Magnetic Heading | BNR | 12 bits | 0.04395 deg | 0 to 360.0 degrees |
| **0324** | Pitch Angle | BNR | 14 bits | 0.01098 deg | -180.0 to +180.0 deg |
| **0325** | Roll / Bank Angle | BNR | 14 bits | 0.01098 deg | -180.0 to +180.0 deg |
| **0365** | Inertial Vertical Speed | BNR | 11 bits | 16.00 ft/min | -32,768 to +32,768 ft/min |

---

## 🛡️ Core Capabilities

1. **Hardware-Accurate Wire Serialization**:
   - Reverses 8-bit octal label bits to match real transceiver hardware (LSB first on wire).
2. **Odd Parity Hardware Emulation**:
   - Computes and audits odd parity bit across all 32 bits using fast bitwise popcount routines.
3. **Sign/Status Matrix (SSM) State Machine**:
   - Decodes sensor health states: `NORMAL_OPERATION`, `FAILURE_WARNING`, `NO_COMPUTED_DATA`, and `FUNCTIONAL_TEST`.
4. **Fault Injection & EMI Bus Noise Detection**:
   - Validates automated error detection for flipped parity bits and out-of-spec transmission bursts.
5. **Zero-Allocation Architecture**:
   - 100% deterministic stack memory footprint, eliminating heap allocation for DO-178C Level A/B compliance.

---

## 📂 Repository Layout

```
arinc429-avionics-protocol-analyzer/
├── include/
│   └── arinc429.h            # Protocol types, masks, labels, and API declarations
├── src/
│   ├── arinc429.c            # Bit-manipulation, parity, BNR encoding/decoding core
│   └── main.c                # Real-time flight telemetry sniffer & fault injection testbench
├── build.bat                 # 1-Click build script for Windows/MinGW
├── PUSH_TO_GITHUB.bat        # Git deployment script
└── README.md                 # Technical specification and verification guide
```

---

## 🚀 Building & Running

### Windows (GCC / MinGW)
```bat
build.bat
arinc429_analyzer.exe
```

### Linux / Embedded Target
```bash
gcc -O2 -Wall -Iinclude src/arinc429.c src/main.c -o arinc429_analyzer
./arinc429_analyzer
```

---

## 🧪 Verification & Flight Telemetry Output

```
=========================================================================================
    ARINC 429 AVIONICS BUS ANALYZER & FLIGHT TELEMETRY DECODER ENGINE
    Conforms to ARINC Specification 429-16 (Digital Information Transfer System)
    Author: Amit Bakale | Avionics & Embedded Software Engineer
=========================================================================================

[SIMULATION ENGINE] Generating Flight Telemetry Stream on ARINC 429 High-Speed Bus (100 kbps)...

-----------------------------------------------------------------------------------------
 RAW HEX    | OCTAL  | SDI | STATUS (SSM)             | ENGINEERING VAL  | PARITY CHECK
-----------------------------------------------------------------------------------------
0x61FBD1C1 | 203    |  1  | NORMAL OPERATION (NO)    | 32500.00 feet    | [PASS] ODD OK
0xE0367111 | 210    |  1  | NORMAL OPERATION (NO)    |   435.50 knots   | [PASS] ODD OK
0xE03FFD0B | 320    |  1  | NORMAL OPERATION (NO)    |   179.96 deg     | [PASS] ODD OK
0xE005552B | 324    |  1  | NORMAL OPERATION (NO)    |     3.75 deg     | [PASS] ODD OK
0x7FF459AB | 325    |  1  | NORMAL OPERATION (NO)    |    -8.20 deg     | [PASS] ODD OK
0x6001D1AF | 365    |  1  | NORMAL OPERATION (NO)    |  1856.00 ft/min  | [PASS] ODD OK
-----------------------------------------------------------------------------------------

[FAULT INJECTION TESTING]
1. Injecting Parity Bit Inversion (Simulating EMI / Bus Line Noise on True Airspeed word)...
   Corrupted Word: 0x60367111 -> Parity Verified: CORRUPTED / REJECTED [Expected: FAIL]

2. Injecting Air Data Computer (ADC) Failure Warning SSM Flag...
   Word: 0x800001C1 | SSM Status: FAILURE WARNING (FW) [Action: Disengage Autopilot Altitude Hold]

=========================================================================================
  [SUCCESS] All ARINC 429 Encoding, Decoding, and Parity Audits Passed with Zero Errors!
=========================================================================================
```

---

## 👨‍💻 Author

**Amit Bakale**  
Embedded Software & Avionics Engineer  
- Portfolio: [https://call-ammu.github.io/portfolio/](https://call-ammu.github.io/portfolio/)  
- LinkedIn: [https://www.linkedin.com/in/amitbakale/](https://www.linkedin.com/in/amitbakale/)  
- Email: bakaleamit28@gmail.com
