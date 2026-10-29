# Ambulance Routing Project

A data-structures-and-algorithms-based emergency response system that assigns ambulances to incidents and hospitals using graph algorithms, greedy strategies, maximum flow, matching, sorting, searching, and benchmarking.

## Project Overview

The system simulates a city as a weighted grid graph. Ambulances, hospitals, and emergency incidents are placed on the graph. The backend calculates routes, assigns ambulances, selects hospitals, and reports response performance through a web dashboard.

The project compares two dispatch strategies:

1. **Greedy nearest-first dispatch**
2. **Maximum-flow joint optimization**

## Architecture

```text
Browser Dashboard
       |
       | HTTP request: /api/run
       v
C HTTP Server
       |
       v
Scenario Generator
       |
       v
Routing and Assignment Engine
       |
       +-- Graph representation
       +-- Shortest-path algorithms
       +-- Greedy dispatch
       +-- Maximum-flow optimization
       +-- Bipartite matching
       +-- Sorting and searching
       +-- Benchmarking and validation
```

## Main Components

### `engine.h`

Contains the complete algorithm engine and domain model:

- Weighted graph representation using adjacency arrays
- Custom arena memory allocator
- Ambulance, hospital, incident, and city models
- Shortest-path algorithms
- Dispatch strategies
- Flow and matching algorithms
- Sorting and searching algorithms
- Operation counting for algorithm comparison

### `server.c`

Provides:

- Scenario generation
- JSON API responses
- Algorithm benchmarks
- Self-tests
- Command-line execution
- Lightweight HTTP server

### `index.html`

Provides the browser dashboard:

- Live city network visualization
- Ambulance, hospital, and incident controls
- Dispatch assignment table
- Route visualization
- Capacity and response metrics
- Critical incident alerts
- Algorithm benchmark table
- Capacity shock simulation

### `Makefile`

Provides build, test, run, and clean commands.

## DAA Algorithms Used

### 1. Graph Representation

The city is modeled as a weighted graph.

- Vertices represent locations in the city grid.
- Edges represent roads.
- Edge weights represent travel time.
- Adjacency arrays are used for efficient graph traversal.

### 2. Dijkstra's Algorithm

Used for calculating shortest travel paths from ambulances and incidents.

- Strategy: Greedy
- Priority queue: Hand-written binary min-heap
- Complexity: `O((V + E) log V)`

### 3. Bellman-Ford Algorithm

Implemented as a dynamic-programming shortest-path algorithm.

- Complexity: `O(VE)`
- Useful for comparing shortest-path approaches.

### 4. Floyd-Warshall Algorithm

Calculates all-pairs shortest paths.

- Strategy: Dynamic programming
- Complexity: `O(V^3)`
- Produces a complete distance matrix.

### 5. Merge Sort

A stable divide-and-conquer sorting algorithm.

- Complexity: `O(n log n)`
- Used for ordering and comparing data.

### 6. Quick Sort

An in-place divide-and-conquer sorting algorithm.

- Average complexity: `O(n log n)`
- Worst-case complexity: `O(n^2)`

### 7. Binary Search

Implemented in both iterative and recursive forms.

- Complexity: `O(log n)`
- Used to search sorted data.

### 8. Ford-Fulkerson Maximum Flow

Used to model joint ambulance-to-incident-to-hospital assignment.

- Uses DFS-based augmenting paths.
- Hospital capacities are represented as flow capacities.
- Provides a globally optimized assignment strategy.

### 9. Bipartite Matching

Matches ambulances and incidents using augmenting paths.

- Models assignment as a bipartite graph.
- Used for assignment benchmarking.
- Complexity: `O(VE)` for the implemented approach.

### 10. String Matching

Two dispatch-log search algorithms are implemented:

- Naive string matching: `O(nm)`
- Knuth-Morris-Pratt (KMP): `O(n + m)`

## Dispatch Strategies

### Greedy Nearest-First

The greedy strategy:

1. Sorts incidents by severity.
2. Selects the highest-priority unserved incident.
3. Assigns the nearest available ambulance.
4. Selects the nearest hospital with available capacity.
5. Builds routes for ambulance-to-incident and incident-to-hospital travel.

This approach is fast and simple but may not produce the best global assignment.

### Maximum-Flow Joint Optimization

The optimized strategy builds a flow network:

```text
Source
  |
  v
Ambulances
  |
  v
Incidents
  |
  v
Hospitals
  |
  v
Sink
```

Capacities enforce:

- Each ambulance can serve one incident.
- Each incident can be served once.
- Each hospital has a limited number of beds.

This strategy attempts to maximize the number of served incidents while respecting ambulance and hospital capacity constraints.

## Website Features

The dashboard includes:

- Live weighted city map
- Ambulance, hospital, and incident markers
- Animated route visualization
- Dispatch assignment table
- Incident severity levels
- Hospital capacity tracking
- Average response-time calculation
- Critical incident alerts
- Greedy versus maximum-flow comparison
- Algorithm runtime and operation benchmarks
- Normal operating mode
- Hospital capacity shock simulation
- Configurable grid size, fleet size,
