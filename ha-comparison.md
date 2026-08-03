# HA Solutions for IBM MQ on Linux x86 VMs

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

