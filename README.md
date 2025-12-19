# Verlet-based Particle Simulator

2D particle simulation using C++ with SFML rendering. Designed as a concurrency/performance showcase: stable physics via Verlet integration, broad-phase acceleration via spatial hashing, and a multithreaded collision solver implemented with a persistent worker pool and slice scheduling.

This work was inspired by the work of Keyframe Codes & John Buffer (Pezzza's Work) but built irrespective of their implementation. Respective GitHub repos: [Keyframe](https://github.com/keyframe41/ParticleSimulation/commits?author=keyframe41), [Pezzza](https://github.com/johnBuffer/VerletSFML-Multithread).

---

![Particle Sim Demo](assets/SimulationDemo.gif)
> Particle simulator demonstation.

---

## Key Features

- **Verlet Integration (stability at scale)**  
  Started with Euler-style integration but encountered jitter/instability and energy gain in dense, highly constrained scenes. Switched to Verlet integration (position + previous position), improving stability for higher particle counts and frequent collision constraints.

- **Spatial Hashing Grid (broad-phase collision acceleration)**  
  Particles are inserted into a uniform grid cell based on position. Collision checks are limited to the current cell and a small neighborhood, reducing the naive all-pairs cost from **O(n²)** to **near O(n)** in typical distributions (bounded local density).

- **Multithreaded Collision Solver (core performance work)**  
  Profiling showed collision resolution dominated the update loop. The solver was parallelized using:
  - a **flat, fixed-capacity grid** (reduces allocations and improves locality)
  - **column-major layout** to make column ranges contiguous in memory
  - **vertical slicing** so each worker processes independent column ranges
  - an **even/odd two-pass schedule** to avoid adjacent-slice contention during neighbor checks
  - a persistent **worker pool** (condition variables + atomic job index) to avoid per-frame thread overhead

- **Deterministic Image Colouring Mode (optional)**  
  If `assets/image.(png|jpg|jpeg|bmp|tga)` exists, it is resized to the window dimensions and sampled to assign colours deterministically by particle index. This enables “image reconstruction” effects when particles converge to a predetermined final configuration.

---

## Build & Run

### Dependencies

- SFML 2.5
- C++ (17 or later)
- CMake

### Build

From project root:

```bash
mkdir build
cd build
cmake ..
cmake --build . -j
```

### Run

From build directory

`./particle-sim`

---

## Controls

- **Mouse (hold left)**: attract/accelerate nearby particles
- **Arrow keys**: change gravity direction
- **Esc**: exit
