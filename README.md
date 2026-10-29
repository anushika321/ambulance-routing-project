# Smart Emergency Ambulance Routing & Hospital Allocation System

College DAA project demonstrating a working emergency-routing dashboard and a standalone C algorithm engine.

## Algorithms
- Dijkstra's shortest path algorithm: route calculation on a weighted city graph.
- Greedy selection: chooses the closest valid available ambulance and hospital after shortest-path costs are computed.
- Insertion Sort: ranks candidate costs in the C engine.
- Constraint checks: excludes busy ambulances and full/closed hospitals.

## Run the website on macOS
```bash
cd Smart-Ambulance-Routing
python3 -m http.server 8787
```
Open: http://localhost:8787

## Compile and run the C DAA engine
```bash
cd algorithm
make
./ambulance_engine
```
Or without Make:
```bash
cc -std=c11 -Wall -Wextra -pedantic -O2 main.c engine.c -o ambulance_engine
./ambulance_engine
```

## Viva summary
The city is modeled as a weighted graph: vertices are locations and edges are roads. Dijkstra computes shortest routes. A greedy strategy selects the currently best valid ambulance/hospital based on those route costs. Insertion Sort ranks candidates in the C implementation. Capacity and availability checks prevent invalid assignments.

## Main frontend features
Dashboard, city graph, emergency creation, deterministic dispatch, ambulance management view, hospital capacity view, Dijkstra visualizer, and DAA analysis.

## Notes
The browser uses JavaScript to visualize the same scheduling/routing workflow interactively. The official standalone DAA implementation is in C under `algorithm/`.
