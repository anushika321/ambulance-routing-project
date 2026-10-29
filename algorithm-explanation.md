# Algorithm Explanation

## Dijkstra
Each location is a vertex and each road is an edge with a non-negative distance. Dijkstra repeatedly selects the unvisited vertex with the smallest known distance and relaxes its neighbors.

## Greedy Allocation
Among all currently valid candidates, choose the one with the minimum computed route cost. This makes the locally optimal dispatch choice.

## Insertion Sort
The C engine sorts candidate IDs alongside their route scores. Each new score is inserted into its correct place among already sorted scores.

## Constraints
Busy ambulances and hospitals with no beds are removed before selection. Unreachable candidates are also rejected.
