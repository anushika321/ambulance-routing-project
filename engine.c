#include "engine.h"

DijkstraResult dijkstra(int graph[MAX_NODES][MAX_NODES], int source) {
    DijkstraResult r; int visited[MAX_NODES] = {0}; r.explored = 0;
    for (int i=0;i<MAX_NODES;i++){ r.dist[i]=INF; r.prev[i]=-1; }
    r.dist[source]=0;
    for(int step=0;step<MAX_NODES;step++){
        int u=-1,best=INF;
        for(int i=0;i<MAX_NODES;i++) if(!visited[i] && r.dist[i]<best){best=r.dist[i];u=i;}
        if(u==-1) break;
        visited[u]=1; r.explored++;
        for(int v=0;v<MAX_NODES;v++){
            if(graph[u][v]>0 && !visited[v] && r.dist[u]+graph[u][v]<r.dist[v]){
                r.dist[v]=r.dist[u]+graph[u][v]; r.prev[v]=u;
            }
        }
    }
    return r;
}

int reconstruct_path(DijkstraResult *r, int source, int target, int path[MAX_NODES]){
    int tmp[MAX_NODES], n=0, cur=target;
    while(cur!=-1 && n<MAX_NODES){tmp[n++]=cur; if(cur==source) break; cur=r->prev[cur];}
    if(n==0 || tmp[n-1]!=source) return 0;
    for(int i=0;i<n;i++) path[i]=tmp[n-1-i];
    return n;
}

// Insertion Sort: ranks candidate IDs by increasing route cost.
void insertion_sort_candidates(int ids[], int scores[], int n){
    for(int i=1;i<n;i++){
        int id=ids[i], score=scores[i], j=i-1;
        while(j>=0 && scores[j]>score){scores[j+1]=scores[j];ids[j+1]=ids[j];j--;}
        scores[j+1]=score; ids[j+1]=id;
    }
}

// Greedy selection: choose the currently closest valid available ambulance.
int select_ambulance(int graph[MAX_NODES][MAX_NODES], Ambulance a[], int n, int patientNode, int *distance){
    int ids[16], scores[16], count=0;
    for(int i=0;i<n;i++) if(a[i].available){
        DijkstraResult r=dijkstra(graph,a[i].currentNode);
        if(r.dist[patientNode]<INF){ids[count]=i;scores[count]=r.dist[patientNode];count++;}
    }
    if(!count) return -1;
    insertion_sort_candidates(ids,scores,count); *distance=scores[0]; return ids[0];
}

// Hospital selection rejects full/closed hospitals before ranking by shortest route.
int select_hospital(int graph[MAX_NODES][MAX_NODES], Hospital h[], int n, int patientNode, int *distance){
    int ids[16], scores[16], count=0;
    DijkstraResult r=dijkstra(graph,patientNode);
    for(int i=0;i<n;i++) if(h[i].availableBeds>0 && h[i].emergencyOpen && r.dist[h[i].node]<INF){
        ids[count]=i;scores[count]=r.dist[h[i].node];count++;
    }
    if(!count) return -1;
    insertion_sort_candidates(ids,scores,count); *distance=scores[0]; return ids[0];
}
