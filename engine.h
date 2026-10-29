#ifndef ENGINE_H
#define ENGINE_H
#define MAX_NODES 12
#define INF 1000000000

typedef struct { int id; char name[32]; int available; int currentNode; } Ambulance;
typedef struct { int id; char name[48]; int node; int availableBeds; int emergencyOpen; } Hospital;
typedef struct { int dist[MAX_NODES]; int prev[MAX_NODES]; int explored; } DijkstraResult;

DijkstraResult dijkstra(int graph[MAX_NODES][MAX_NODES], int source);
int reconstruct_path(DijkstraResult *r, int source, int target, int path[MAX_NODES]);
void insertion_sort_candidates(int ids[], int scores[], int n);
int select_ambulance(int graph[MAX_NODES][MAX_NODES], Ambulance a[], int n, int patientNode, int *distance);
int select_hospital(int graph[MAX_NODES][MAX_NODES], Hospital h[], int n, int patientNode, int *distance);
#endif
