# Tiny Monitor - Low-Level Linux System Monitor

A learning project to build a comprehensive low-level Linux monitoring tool in C++ from scratch.

## Project Goals

1. **Level up C++ skills** - Learn modern C++ (C++17) through practical systems programming
2. **Understand system internals** - Deep dive into Linux kernel interfaces, performance counters, and hardware monitoring
3. **Build useful tools** - Create a monitoring tool that reveals bottlenecks beyond basic CPU/memory usage

## What Makes This Different?

Unlike typical monitoring tools, this will provide:

- **Storage I/O deep analysis** - Queue depths, latency distributions, I/O patterns
- **Hardware performance counters** - Cache misses, branch predictions, IPC
- **Interrupt analysis** - Per-CPU interrupt distribution and overhead
- **Root cause identification** - Not just "what's high" but "why it's slow"

## Current Status

**Phase 1 (In Progress)**: Foundation - Basic /proc parsing

- [ ] CPU monitoring (`/proc/stat`)
- [ ] Memory monitoring (`/proc/meminfo`)
- [ ] Disk I/O basics (`/proc/diskstats`)
- [ ] Interrupt tracking (`/proc/interrupts`)

## Building

```bash
mkdir build
cd build
cmake ..
make
./tiny_monitor
```

## Roadmap

See [low-level-linux-monitor.plan.md](low-level-linux-monitor.plan.md) for detailed phase breakdown.

**Phases**:

1. ✅ Foundation - /proc filesystem parsing
2. Storage Deep Dive - /sys/block analysis, queue depths
3. Advanced CPU Profiling - perf_event_open(), hardware counters
4. Memory System Analysis - NUMA, TLB, memory bandwidth
5. Process-Level Analysis - Per-process I/O attribution
6. Visualization - ncurses TUI with real-time updates

## Learning Resources

- Linux `/proc` filesystem: [kernel.org documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
- Performance monitoring: Brendan Gregg's "Systems Performance"
- C++17 features: [cppreference.com](https://en.cppreference.com/)

## Requirements

- Linux kernel 4.0+
- GCC 7+ or Clang 5+ (C++17 support)
- CMake 3.10+
- Root access for some features (perf_event, hardware counters)

## License

MIT - Feel free to learn from and adapt this code.
