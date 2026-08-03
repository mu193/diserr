# HA Solutions for IBM MQ on Linux x87 VMs

## Overview

### IBM MQ Native HA Options

| Solution | Notes |
|---|---|
| **Multi-instance queue manager** | Active/standby, shared filesystem (NFS/GPFS/GFS2), manual or automatic failover |
| **Native HA** | MQ 9.2+, built-in Raft-based replication across 3 nodes, no shared storage |
| **RDQM** (Replicated Data Queue Manager) | Pacemaker + DRBD replication, 3 nodes, DR variant available |

---

### Infrastructure / Platform HA Options

| Solution | Notes |
|---|---|
| **Pacemaker + shared storage (GFS2/SAN)** | Classic Linux HA cluster, active/passive, requires MQ resource agent |
| **VMware HA / vSphere HA** | Hypervisor-level restart on host failure, not MQ-aware |
| **Veritas InfoScale Availability** (formerly VCS) | Enterprise HA cluster with MQ agent, active/passive, supports shared and replicated storage |

---


### Summary Comparison Matrix

| Solution | Shared Storage | Replication | Nodes | RPO | RTO | Complexity |
|---|---|---|---|---|---|---|
| Multi-instance QM | Yes (NFS/GPFS/GFS2) | No | 2 | Near zero | Seconds–minutes | Low |
| Native HA | No | Yes (Raft) | 3 | Near zero | Seconds | Low |
| RDQM | No | Yes (DRBD) | 3 | Near zero | Seconds | Medium |
| Pacemaker + shared storage | Yes (GFS2/SAN) | No | 2+ | Near zero | Seconds | Medium |
| VMware HA | Depends | No | 2+ hosts | Minutes (VM restart) | Minutes | Low (hypervisor) |
| Veritas InfoScale Availability | Yes (VxFS/SAN) | Optional (VVR) | 2+ | Near zero | Seconds | Medium–High |

## Multi-Instance Queue Manager

### Overview

A multi-instance queue manager runs as an active instance on one node and a standby instance on a second node. Both instances share the same queue manager data and logs via a shared filesystem. When the active instance fails, the standby automatically takes over.

### Architecture

```
Node A (active)                Node B (standby)
  mqm process running            mqm process waiting
        \                              /
         \                            /
          +----------------------------+
          |   Shared Filesystem        |
          |   (NFS / GPFS / GFS2)      |
          |   - queue manager data     |
          |   - transaction logs       |
          +----------------------------+
```

### Requirements

- Shared filesystem accessible from both nodes simultaneously with full POSIX locking support.
- Supported shared filesystems:
  - NFS v4 with locking enabled
  - GPFS (IBM Spectrum Scale)
  - GFS2
  - plain NFS v3 is not sufficient 
  — also see [IBM Doc](https://www.ibm.com/support/pages/testing-statement-ibm-mq-multi-instance-queue-manager-file-systems?view=full#fs_not_meeting_requirements)
- Both nodes must run the same MQ version and fix pack level.
- Shared filesystem must be mounted on both nodes before starting either instance.

### Failover Behaviour

- Standby polls the shared lock file at a configurable interval (default 30 seconds).
- On active failure, standby detects the released lock and promotes itself.
- MQ clients with reconnect enabled reconnect automatically after failover.
- Failover time is typically 30–90 seconds depending on polling interval and log replay.

### Strengths

- Simple to configure — no third-party cluster software required.
- Supported on all IBM MQ editions.
- Automatic failover with no manual intervention needed.
- Works with existing NFS infrastructure.

### Weaknesses

- Shared filesystem is a single point of failure unless the filesystem itself is HA (GPFS/GFS2).
- No replication — if the shared storage fails, both nodes are affected.
- Failover time is longer than Native HA or RDQM due to polling interval.
- Only 2 nodes supported (1 active + 1 standby).

### Split-Brain

Split-brain occurs when both nodes lose communication with each other but both remain running. Each node believes the other has failed and attempts to become the active instance — resulting in two active queue managers writing to the same shared filesystem simultaneously, which causes data corruption.

**How multi-instance prevents split-brain:**

Multi-instance queue manager relies entirely on the shared filesystem's lock mechanism to prevent this. The active instance holds an exclusive lock on a lock file in the shared data directory. The standby can only take over by acquiring that lock. As long as the shared filesystem enforces POSIX locks correctly, only one instance can hold the lock at a time — split-brain is not possible at the MQ level.

**When split-brain can still occur:**

- If the shared filesystem itself has a bug or misconfiguration that grants the lock to both nodes.
- GFS2 and GPFS are safer because their locking is cluster-aware, not server-dependent.

**Key conclusion:** do not use NFS for production multi-instance deployments — use GPFS or GFS2.

---

### Data Center Requirements

Two MQ nodes are required (active + standby), each in a separate DC. The shared filesystem must span both DCs.

**With GPFS (Spectrum Scale):**
- GPFS also requires quorum among its quorum nodes to survive a DC failure without split-brain.
- With 1 node per DC, a tie-breaker quorum node in a third location is needed — the same constraint as GFS2.

**With GFS2:**
- GFS2 requires cluster quorum, which a 2-node stretched cluster cannot achieve alone after a DC failure.
- Either a **third DC** hosting a quorum device (`corosync-qnetd`) or a **third full cluster node** is required.

| Filesystem | Min DCs | Quorum device needed |
|---|---|---|
| GPFS / Spectrum Scale | 2 + quorum node (tie-breaker) | Yes (3rd location) |
| GFS2 | 2 + quorum device | Yes (3rd location) |
| GFS2 with 3 nodes | 3 | No |

Network latency between DCs directly affects performance — every MQ write to the shared filesystem is synchronous, so latency above ~5ms will impact throughput.

