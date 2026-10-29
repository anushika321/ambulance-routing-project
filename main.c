#include <stdio.h>
#include "engine.h"

static void edge(int g[MAX_NODES][MAX_NODES],int a,int b,int w){g[a][b]=g[b][a]=w;}
int main(void){
    int g[MAX_NODES][MAX_NODES]={{0}};
    edge(g,0,1,4); edge(g,0,2,7); edge(g,1,3,3); edge(g,1,4,6); edge(g,2,3,2);
    edge(g,2,5,5); edge(g,3,4,2); edge(g,3,6,4); edge(g,4,7,3); edge(g,5,6,2);
    edge(g,5,8,6); edge(g,6,7,2); edge(g,6,9,5); edge(g,7,10,4); edge(g,8,9,2);
    edge(g,9,10,3); edge(g,10,11,2); edge(g,7,11,6);
    Ambulance a[]={{101,"AMB-101",1,0},{102,"AMB-102",0,3},{103,"AMB-103",1,5},{104,"AMB-104",1,8},{105,"AMB-105",0,10},{106,"AMB-106",1,11}};
    Hospital h[]={{1,"City Hospital",7,12,1},{2,"Central Medical Center",9,0,1},{3,"University Hospital",10,7,1},{4,"North Trauma Center",2,5,1}};
    int patient=4, da=0, dh=0;
    int ai=select_ambulance(g,a,6,patient,&da), hi=select_hospital(g,h,4,patient,&dh);
    if(ai<0 || hi<0){puts("No valid dispatch available.");return 1;}
    DijkstraResult ar=dijkstra(g,a[ai].currentNode), hr=dijkstra(g,patient);
    int p1[MAX_NODES],p2[MAX_NODES]; int n1=reconstruct_path(&ar,a[ai].currentNode,patient,p1); int n2=reconstruct_path(&hr,patient,h[hi].node,p2);
    printf("Smart Emergency Ambulance Routing - C DAA Engine\n");
    printf("Selected ambulance: %s | distance: %d km\n",a[ai].name,da);
    printf("Ambulance -> Patient path: "); for(int i=0;i<n1;i++) printf("%c%s",'A'+p1[i],i+1<n1?" -> ":"\n");
    printf("Selected hospital: %s | distance: %d km\n",h[hi].name,dh);
    printf("Patient -> Hospital path: "); for(int i=0;i<n2;i++) printf("%c%s",'A'+p2[i],i+1<n2?" -> ":"\n");
    printf("Total route: %d km | Nodes explored: %d\n",da+dh,ar.explored+hr.explored);
    printf("Algorithms: Dijkstra + Greedy Selection + Insertion Sort\n");
    return 0;
}
