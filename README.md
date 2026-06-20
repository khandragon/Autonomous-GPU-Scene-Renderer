# Autonomous-GPU-Scene-Renderer

Building a renderer where the CPU primarily manages resources, frame setup, descriptor heaps, and high-level orchestration, while the GPU performs visibility testing, LOD selection, visible-list compaction, draw generation, and optional mesh-shader or Work Graph execution.

The renderer will include a baseline CPU-submitted path so that GPU-driven paths can be measured against a simple reference implementation.