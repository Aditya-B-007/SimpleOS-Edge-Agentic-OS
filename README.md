# Agentic Edge OS

An agent-centric, microkernel-based operating system designed for
industrial edge controllers, IoT devices, sensor nodes, and edge gateways.

This OS is **not a desktop OS** and **not a Linux derivative**.
It is built for deterministic behavior, fault isolation, long uptime,
and safe operation in constrained, real-world environments.
<img width="1918" height="802" alt="Screenshot 2026-01-23 235031" src="https://github.com/user-attachments/assets/18de7fac-2cfc-40b5-8bc8-3f7477a682a9" />

<img width="1116" height="862" alt="Screenshot 2026-01-23 234748" src="https://github.com/user-attachments/assets/576f0596-475c-4722-a5fd-1dda3d1fd384" />


---

## Motivation

Modern edge devices operate under conditions that traditional operating
systems handle poorly:

- Intermittent power and networking
- Long deployment lifetimes (10–20 years)
- Safety-critical control paths
- Limited memory and energy budgets
- Direct human interaction in the field

Existing systems force a trade-off:
RTOSes are deterministic but rigid; Linux is flexible but heavy.

This project explores a third path:
an **agent-first operating system** that combines determinism,
safety, and adaptability without inheriting desktop assumptions.

---

## Core Principles

- **Agents are first-class OS entities**
- **Determinism over throughput**
- **Capability-based security**
- **Event-driven execution**
- **Safe degradation on failure**
- **UI is optional, never required**
- **Cloud is optional, never required**

---

## What This OS Is

- A microkernel with a minimal trusted computing base
- An agent runtime enforcing strict resource and authority limits
- A platform for industrial controllers, IoT devices, and gateways
- Designed to scale **down to MCUs** and **up to industrial SoCs**

---

## What This OS Is Not

- Not a POSIX-compatible OS
- Not a Linux clone
- Not a desktop or mobile OS
- Not a general-purpose app platform

---

## Target Device Classes

- Industrial edge controllers (primary target)
- Battery-powered IoT devices
- Environmental sensor nodes
- Edge AI / vision devices (control-plane focus)
- Edge gateways and mesh coordinators

---

## High-Level Architecture

Hardware
↓
Microkernel
↓
Agent Runtime
↓
Agents (control, sensing, analytics, comms, UI)


---

## Agents (Key Concept)

An agent is an OS-managed, event-driven, capability-bounded entity
responsible for a specific intent within a device.

Agents:
- Wake only on events
- Run to completion
- Operate under strict CPU, memory, energy, and authority limits
- Can be updated, throttled, or killed independently

---

## Minimal On-Device UI (Optional)

When enabled, the OS provides a single-screen diagnostic UI showing:

- CPU / memory usage
- Temperature and power state
- Network connectivity
- Uptime and last reboot reason
- Live sensor or agent outputs
- Critical alerts
- Recent system events

UI runs as a privileged agent and cannot affect safety-critical logic.

---

## Project Status

🚧 Early-stage architecture and kernel bring-up.

Current focus:
- Microkernel core
- Agent runtime design
- Capability security model
- Deterministic scheduling
- Failure and power handling

---

## Roadmap (High Level)

- Phase 1: Microkernel + agent runtime
- Phase 2: IPC, power management, failure recovery
- Phase 3: Hardware bring-up on real devices
- Phase 4: OTA updates and observability
- Phase 5: Minimal UI agent

---

## Disclaimer

This project is experimental and not production-ready.
No safety certification is claimed.
