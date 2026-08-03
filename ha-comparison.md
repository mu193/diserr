# HA Solutions for IBM MQ on Linux x86 VMs

## IBM MQ Native HA Options

| Solution | Notes |
|---|---|
| **Multi-instance queue manager** | Active/standby, shared filesystem (NFS/GPFS/GFS2), manual or automatic failover |
| **Native HA** | MQ 9.2+, built-in Raft-based replication across 3 nodes, no shared storage |
| **RDQM** (Replicated Data Queue Manager) | Pacemaker + DRBD replication, 3 nodes, DR variant available |

---

## Infrastructure / Platform HA Options

| Solution | Notes |
|---|---|
| **Pacemaker + shared storage (GFS2/SAN)** | Classic Linux HA cluster, active/passive, not MQ-specific |
| **Pacemaker + DRBD (without RDQM)** | Block-level replication, active/passive, manual MQ resource agent |
| **VMware HA / vSphere HA** | Hypervisor-level restart on host failure, not MQ-aware |
| **VMware FT (Fault Tolerance)** | Lockstep VM mirroring, near-zero downtime, significant overhead |
| **Hyperscaler** (AWS/Azure/GCP) | MQ on cloud VMs with managed storage, availability zones, autoscaling |

---

## Container / Orchestration Options

| Solution | Notes |
|---|---|
| **IBM MQ on OpenShift / Kubernetes** | Native HA or RDQM in containers, persistent volumes, pod rescheduling |
| **IBM MQ Appliance HA pair** | Hardware appliance, not x86 VMs — included for completeness |

---

## Less Common but Valid Options

| Solution | Notes |
|---|---|
| **IBM MQ + Keepalived/VRRP** | VIP failover only, no data replication — very limited, not recommended |
| **Stretch cluster (GFS2 across sites)** | Multi-site active/active filesystem, DLM latency constraints apply |
| **IBM Spectrum Scale (GPFS)** | Enterprise shared filesystem, replaces NFS for multi-instance QM, strong performance |

---

## Summary Comparison Matrix

| Solution | Shared Storage | Replication | Nodes | RPO | RTO | Complexity |
|---|---|---|---|---|---|---|
| Multi-instance QM | Yes (NFS/GPFS/GFS2) | No | 2 | Near zero | Seconds–minutes | Low |
| Native HA | No | Yes (Raft) | 3 | Near zero | Seconds | Low |
| RDQM | No | Yes (DRBD) | 3 | Near zero | Seconds | Medium |
| Pacemaker + DRBD | No | Yes (DRBD) | 2 + qdevice | Near zero | Seconds | Medium |
| VMware HA | Depends | No | 2+ hosts | Minutes | Minutes | Low (hypervisor) |
| VMware FT | Yes | Yes (lockstep) | 2 hosts | Zero | Zero | High |
| Hyperscaler | Managed | Managed | Varies | Varies | Varies | Low–Medium |
| MQ on OpenShift | PV | Optional | 3+ | Near zero | Seconds | High |
