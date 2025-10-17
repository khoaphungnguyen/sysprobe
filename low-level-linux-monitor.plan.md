# Low-Level Linux System Monitor - Progressive Learning Plan

## Project Goals

I'm building `tiny_monitor/` to level up my C++ skills and understand Linux system internals deeply. This is a progressive learning project where I'll implement each phase myself, learning by doing.

**What I'll Learn**:

- Modern C++ (C++17) - RAII, smart pointers, templates, threading
- Linux kernel interfaces - /proc, /sys, perf_event, eBPF
- System performance - CPU, memory, storage, and their interactions
- Low-level debugging - Finding root causes, not just symptoms

**My Approach**:

- Build each feature myself from scratch
- Start simple and add complexity progressively
- Understand the "why" behind each metric
- Document what I learn along the way

---

## Phase 1: Foundation - Process & System Basics (Week 1-2)

**C++ Skills**: File I/O, string parsing, RAII, smart pointers, basic classes

**System Skills**: /proc filesystem, /sys filesystem basics

### What I'll Build:

- Parse `/proc/stat` for CPU usage (user, system, idle, iowait, irq, softirq)
- Parse `/proc/meminfo` for memory metrics (available, cached, buffers, dirty pages)
- Parse `/proc/diskstats` for basic I/O counters (reads, writes, sectors)
- Parse `/proc/interrupts` for interrupt counts per CPU
- Create `SystemStats` class with RAII for file handles
- Implement basic text output (1-second intervals)

**Key Learning**: Understand how kernel exposes metrics, CPU time accounting, page cache mechanics

---

## Phase 2: Storage Deep Dive (Week 3-4)

**C++ Skills**: STL containers (maps, vectors), templates, operator overloading

**System Skills**: Block layer internals, I/O schedulers, queue depth

### What I'll Build:

- Parse `/sys/block/{device}/stat` for detailed per-device metrics
- Track I/O queue depth via `/sys/block/{device}/queue/`
- Monitor I/O scheduler (`/sys/block/{device}/queue/scheduler`)
- Calculate latency metrics (await, service time estimates)
- Parse `/proc/diskstats` deltas for throughput (MB/s, IOPS)
- Detect hot devices (highest I/O rate or queue buildup)
- Create `StorageMonitor` class with device discovery

**Key Learning**: Linux block layer, I/O merging, read-ahead, device queue management

---

## Phase 3: Advanced CPU Profiling (Week 5-6)

**C++ Skills**: System calls, error handling, low-level APIs

**System Skills**: perf_event_open(), hardware performance counters

### What I'll Build:

- Use `perf_event_open()` syscall to access hardware counters:
- CPU cycles, instructions, cache references, cache misses
- Branch instructions, branch misses
- Context switches, page faults
- Monitor per-CPU statistics
- Calculate IPC (instructions per cycle), cache hit rates
- Detect CPU bottlenecks (cache thrashing, branch mispredictions)
- Create `PerfMonitor` class wrapping perf_event APIs

**Key Learning**: CPU microarchitecture, performance counters, cache hierarchies, branch prediction

---

## Phase 4: Memory System Analysis (Week 7-8)

**C++ Skills**: Multi-threading, synchronization, atomic operations

**System Skills**: Memory bandwidth, NUMA, TLB, page faults

### What I'll Build:

- Parse `/proc/vmstat` for detailed memory events:
- Page faults (major/minor)
- Page cache hits/misses
- Swap in/out activity
- Dirty page writeback
- Read `/sys/devices/system/node/` for NUMA topology
- Monitor memory bandwidth using perf_event (if supported)
- Track TLB misses via performance counters
- Create `MemoryMonitor` with NUMA awareness
- Use threads for parallel monitoring without blocking

**Key Learning**: Virtual memory, NUMA architecture, memory pressure indicators, TLB mechanics

---

## Phase 5: Process-Level Deep Analysis (Week 9-10)

**C++ Skills**: Process management, signal handling, ptrace basics

**System Skills**: Per-process profiling, I/O accounting

### What I'll Build:

- Parse `/proc/{pid}/io` for per-process I/O accounting:
- Read/write syscalls vs actual I/O
- Page cache efficiency per process
- Parse `/proc/{pid}/stat` and `/proc/{pid}/status` for:
- CPU time, context switches
- Memory maps, page faults
- I/O wait time
- Track file descriptors via `/proc/{pid}/fd/`
- Correlate process behavior with system-wide bottlenecks
- Create `ProcessMonitor` class that can attach to specific PIDs (fio, databases, etc.)

**Key Learning**: Process accounting, I/O attribution, context switch overhead

---

## Phase 6: Real-Time Visualization & Integration (Week 11-12)

**C++ Skills**: ncurses library, data structures for time-series, performance optimization

**System Skills**: Efficient polling strategies, minimal overhead monitoring

### What I'll Build:

- Implement ncurses-based TUI with multiple views:
- Overview dashboard (CPU, memory, storage summary)
- Storage detail view (per-device hot spot analysis)
- Performance counter view (cache, branch prediction stats)
- Process drill-down view
- Add historical graphs (sparklines for trends)
- Color-coded alerts (red for bottlenecks: high queue depth, cache thrashing)
- Optimize monitoring loop to minimize overhead
- Add command-line options: `-p <pid>` (focus on process), `-d <device>` (focus on storage)

**Key Learning**: Efficient system monitoring, data visualization, performance-aware coding

---

## Advanced Extensions (Optional - Week 13+)

### eBPF Integration:

- Use BCC/libbpf to trace block I/O latency distributions
- Track I/O patterns (sequential vs random) at kernel level
- Capture stack traces on slow I/O operations

### Kernel Tracing:

- Read `/sys/kernel/debug/tracing/` (ftrace)
- Monitor block I/O trace events
- Capture I/O scheduler decisions

### Export & Analysis:

- JSON/CSV output for integration with other tools
- Prometheus exporter format
- Real-time streaming to web dashboard

---

## Learning Resources Per Phase

**Phase 1-2**: Understanding the Linux /proc filesystem (kernel.org documentation)

**Phase 3**: Intel/AMD Software Developer Manuals (performance counters), Brendan Gregg's systems performance

**Phase 4**: NUMA architecture guides, Linux memory management internals

**Phase 5**: Linux process management, I/O accounting internals

**Phase 6**: ncurses programming guide, efficient polling strategies

---

## Success Criteria

By the end, I'll be able to answer:

- **Why is fio slow?** → High queue depth? Cache misses? NUMA imbalance? Context switches?
- **Where is the bottleneck?** → CPU compute? Memory bandwidth? Storage I/O? Kernel overhead?
- **What's the root cause?** → See interrupt distribution, I/O scheduler behavior, cache hit rates

My tool will answer: "Performance is slow because device nvme0n1 has queue depth at 128 with 50% cache misses and 2000 interrupts/sec concentrated on CPU 0, indicating interrupt affinity misconfiguration."
