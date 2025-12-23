# PER-C: Prioritized Experience Replay Buffer

PER-C is a simple and naive implementation of a Prioritized Experience Replay (PER) buffer, accelerated with a Sum Tree. This appeared to me as weekend project since the replay buffers are a critical component in reinforcement learning applications. This project aims to address performance bottlenecks caused by Python's obvious slowness. I am not claiming that this is the fastest implementation but maybe someone will see it and use the code as a starting point or this code will spark some ideas to push the RL in the direction of efficient implementation along side model and method research.

- We have enough scientists.
- We have enough visionares.
- We have enough compute power.
- But lacking true engineers.

---

## Features

- **Prioritized Experience Replay (PER):** Efficient sampling of experiences based on priority.
- **Sum Tree Acceleration:** Optimized data structure for fast priority updates and sampling.
- **Lightweight Build System:** Uses [NOB](https://github.com/tsoding/nob.h), a minimal build system.

---

## Build and Run Instructions

### Prerequisites

Ensure you have a C compiler installed, such as `gcc` or `clang`.

### Steps

1. **Create the Build Directory**
   ```bash
   mkdir build
   ```

2. **Build the NOB Build System**
   Compile the `nob.c` file to create the `nob` executable:
   ```bash
   gcc nob.c -o nob
   ```
   *(Alternatively, you can use `make` if preferred.)*

3. **Build the Main Program**
   Use the `nob` executable to build the main program:
   ```bash
   ./nob
   ```

4. **Run the Program**
   Execute the compiled binary to see the PER buffer in action:
   ```bash
   ./build/main
   ```

---

## What’s Next?

- [ ] Convert to a single-header library for easier integration.
- [ ] Add Python bindings (likely as a separate library).
- [ ] Implement further optimizations for better performance.

---

## Learn More

- **NOB Build System:** [https://github.com/tsoding/nob.h](https://github.com/tsoding/nob.h)
- **Sum Tree:** A data structure used for efficient sampling and updates in PER.

---

## License

This project is licensed under the [Apache License 2.0](LICENSE).

---

Feel free to contribute, report issues, or suggest improvements!
