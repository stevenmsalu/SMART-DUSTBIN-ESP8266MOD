# Audit addendum — Buzzer classification correction

**Case ID:** `CASE-20261112-SMARTDUSTBIN-END-TO-END`  
**Date:** 2026-11-12  
**Work package:** WP-0  
**Authorised by:** Steven (Product Owner)

---

## 1. Purpose

This addendum **corrects** buzzer-related statements in the Stage-1 audit report:

[`SMARTDUSTBIN_END_TO_END_AUDIT_CASE-20261112.md`](SMARTDUSTBIN_END_TO_END_AUDIT_CASE-20261112.md)

The original audit (executed before final hardware confirmation) described the buzzer as **unverified**, recommended keeping it **optional and disabled**, and flagged policy mismatch with the buzzer test sketch.

**This addendum supersedes that classification only.** The original audit file is **unchanged** and remains the historical record.

---

## 2. Corrected buzzer baseline (authoritative as of WP-0)

| Item | Corrected statement |
|------|---------------------|
| Operational status | Buzzer is **connected and working** on the bench |
| Wiring | **Positive → D4 / GPIO2**; **negative → GND** |
| Drive | **Direct GPIO** connection; **no separate driver circuit** currently documented |
| Activation | **GPIO2 HIGH** for active alert (firmware may also use short `tone()` patterns where appropriate) |
| Architecture | Buzzer **is included** as a subsystem — short beep on **transition into warning** fill state with **cooldown** (per approved functional behaviour) |

---

## 3. Engineering notes unchanged in spirit

The audit’s **technical** warnings about **GPIO2 boot-strapping** and GPIO current limits remain valid:

- GPIO2 must be **HIGH** during boot for normal flash boot.
- Cold-start reliability with the buzzer attached, and electrical suitability of direct GPIO drive, must still be **verified and recorded** in a hardware validation work package.

These items are **verification tasks**, not reasons to exclude the buzzer from the design.

---

## 4. Impact on audit gap analysis

| Audit finding (historical) | WP-0 / PO position |
|--------------------------|-------------------|
| Buzzer “unverified / optional” | **Superseded** — treat as working hardware per §2 |
| Buzzer test sketch vs policy | Bring-up sketch remains valid for D4; integrated warning logic is **planned firmware**, not yet implemented |
| WP-9 “optional gated buzzer” in audit | Reinterpret for planning: implement buzzer with **HV-2 / HV-3** validation gates, not as a removed feature |

---

## 5. Cross-references

- Hardware detail: [`HARDWARE_GPIO_MAP.md`](HARDWARE_GPIO_MAP.md) §7  
- Calibration (alert timing): [`CALIBRATION_LOG.md`](CALIBRATION_LOG.md) §7  
- Project overview: [`../README.md`](../README.md)

---

*End of addendum*
