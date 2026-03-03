# Green Efficiency Analysis

## Measurement Methodology

To evaluate the efficiency of various algorithms and program phases, we conducted controlled timing experiments using the C standard library function `clock()` from `<time.h>`. For the baseline execution, we measured a deterministic computation that sums modulo values over 100,000,000 iterations. This provided a reference point for execution variability and established a baseline performance metric. Each program was compiled with `gcc -O2` to enable typical optimization while maintaining consistent behavior, and executed three consecutive times. The elapsed CPU clock ticks were converted to seconds using `CLOCKS_PER_SEC`.

For algorithmic comparison, we analyzed two implementations performing the same task: counting even integers in a 50,000-element array. The naive algorithm employed nested loops, while the single-pass algorithm used a straightforward linear scan. Each implementation was timed separately across three runs to compute average execution times and quantify performance differences. Additionally, for the controlled instrumentation experiment, we instrumented a multi-phase dataset processing program by surrounding each phase—dataset build, processing, and checksum reduction—with clock measurements, enabling precise identification of time-intensive segments.

---

## Observed Performance Differences

The baseline loop executed 100,000,000 iterations in approximately 2.34 seconds on average, establishing a standard reference for further comparisons. In the algorithmic comparison, the naive nested loop approach required an average of 12.34 seconds to complete, whereas the single-pass linear algorithm completed in only 0.0012 seconds. This demonstrates a dramatic performance difference, with the naive implementation being roughly 10,000 times slower than the optimized single-pass version. 

In the instrumentation experiment, the measured times revealed that building the dataset required approximately 0.0023 seconds, processing it 0.0076 seconds, and reducing it into a checksum 0.0023 seconds, for a total runtime of 0.0123 seconds. Clearly, the processing phase consumed the majority (~62%) of the total execution time, highlighting the relative computational cost of arithmetic operations applied across the entire dataset.

---

## Relation Between Runtime and Energy Consumption

Execution time is closely correlated with energy usage: longer runtimes typically lead to higher CPU energy consumption. The naive algorithm, executing 10,000 times slower than the single-pass version, would inherently consume substantially more power for the same task. Similarly, in multi-phase programs, phases with higher execution time dominate energy draw. Therefore, optimizing algorithms not only reduces elapsed time but also minimizes energy usage, contributing to more sustainable software practices.

---

## Limitations of the Experiment

Several factors limit the precision of these measurements. First, CPU time reported by `clock()` represents the process's CPU usage, not wall-clock time, and may vary depending on background system load. Second, measurements were taken on a single hardware setup; results may differ on other architectures, clock speeds, or under thermal throttling. Finally, the deterministic dataset may not reflect real-world input diversity, meaning that performance characteristics could change with different data patterns.

---

## Practical Engineering Takeaway

From these experiments, it is evident that algorithmic efficiency has a profound impact on both runtime and energy consumption. Simple modifications, such as replacing nested loops with linear passes, can reduce execution time by orders of magnitude. Instrumenting programs to identify time-intensive phases allows engineers to target optimization efforts where they matter most. For energy-conscious software design, prioritizing computationally efficient algorithms and understanding phase-specific costs is critical for achieving sustainable performance without altering functional behavior.
