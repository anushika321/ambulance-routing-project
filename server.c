/* server.c - backend: scenario runner, benchmark, self-tests and a tiny HTTP server (C17, no libraries).
 *   ./ambulance serve [port] [web/index.html]      start the dashboard backend
 *   ./ambulance run side amb hosp inc seed shock   print scenario result as JSON (CLI export)
 *   ./ambulance test                               run self-tests                                  */
#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <time.h>
#include "engine.h"
#ifdef _WIN32
#include <winsock2.h>
#define CLOSESOCK closesocket
typedef SOCKET sock_t;
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSESOCK close
typedef int sock_t;
#define INVALID_SOCKET (-1)
#endif

static double now_ms(void) {
#ifdef _WIN32
  return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
#else
  struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)t.tv_sec * 1e3 + (double)t.tv_nsec / 1e6;
#endif
}
#define MEASURE(t, code) do { OPS = 0; double _s = now_ms(); code; (t) = now_ms() - _s; } while (0)

/* growable string buffer for JSON output (heap, freed by caller) */
typedef struct { char *b; size_t n, cap; } SB;
static void sb(SB *s, const char *fmt, ...) {
  for (;;) {
    va_list ap; va_start(ap, fmt); int k = vsnprintf(s->b + s->n, s->cap - s->n, fmt, ap); va_end(ap);
    if (k < 0) return;
    if ((size_t)k < s->cap - s->n) { s->n += (size_t)k; return; }
    s->cap = s->cap * 2 + (size_t)k + 1; s->b = (char *)realloc(s->b, s->cap);
  }
}
static void arr(SB *o, const int *v, int n) { sb(o, "["); for (int i = 0; i < n; i++) sb(o, "%s%d", i ? "," : "", v[i]); sb(o, "]"); }
static void row(SB *o, int *k, const char *task, const char *algo, const char *cx, double ms, ll ops, const char *note) {
  sb(o, "%s{\"task\":\"%s\",\"algo\":\"%s\",\"cx\":\"%s\",\"ms\":%.3f,\"ops\":%lld,\"note\":\"%s\"}", (*k)++ ? "," : "", task, algo, cx, ms, ops, note);
}

static void bench(SB *o, const City *c, const Ctx *x) {
  int k = 0, n = c->g.n, s = c->na ? c->amb[0] : 0; double t; char note[160];
  int *d1 = NEW(int, n), *d2 = NEW(int, n), *par = NEW(int, n);
  MEASURE(t, dijkstra(&c->g, s, d1, par)); row(o, &k, "Shortest path", "Dijkstra (heap)", "O((V+E) log V)", t, OPS, "reference result");
  MEASURE(t, bellmanFord(&c->g, s, d2)); row(o, &k, "Shortest path", "Bellman-Ford", "O(VE)", t, OPS, memcmp(d1, d2, sizeof(int) * (size_t)n) == 0 ? "matches Dijkstra" : "MISMATCH");
  if (n <= 150) { int *D; MEASURE(t, D = floydWarshall(&c->g)); row(o, &k, "Shortest path", "Floyd-Warshall", "O(V^3)", t, OPS, memcmp(D + s * n, d1, sizeof(int) * (size_t)n) == 0 ? "matches Dijkstra (all pairs)" : "MISMATCH"); }
  else row(o, &k, "Shortest path", "Floyd-Warshall", "O(V^3)", -1, 0, "skipped: V>150 (too slow - shows why Dijkstra wins)");
  Rng r = {99}; int N = 20000, *v = NEW(int, N), *A = NEW(int, N), *B = NEW(int, N);
  for (int i = 0; i < N; i++) v[i] = rnext(&r, 1000000);
  memcpy(A, v, sizeof(int) * N); memcpy(B, v, sizeof(int) * N);
  MEASURE(t, mergeSort(A, N, cmpInt, NULL)); int ok = 1; for (int i = 1; i < N; i++) ok &= A[i - 1] <= A[i];
  row(o, &k, "Sort 20000 keys", "Merge sort", "O(n log n) always", t, OPS, ok ? "sorted OK" : "FAIL");
  MEASURE(t, quickSort(B, 0, N, cmpInt, NULL));
  row(o, &k, "Sort 20000 keys", "Quick sort", "O(n log n) avg, O(n^2) worst", t, OPS, memcmp(A, B, sizeof(int) * N) == 0 ? "same as merge sort" : "FAIL");
  int *ids = NEW(int, c->ni), f1 = 0; for (int i = 0; i < c->ni; i++) ids[i] = c->inc[i].id;
  MEASURE(t, for (int i = 0; i < c->ni; i++) f1 += bsIter(ids, c->ni, ids[i]) >= 0);
  snprintf(note, sizeof note, "%d/%d found", f1, c->ni); row(o, &k, "Incident lookup by id", "Binary search (iterative)", "O(log n)", t, OPS, note);
  f1 = 0; MEASURE(t, for (int i = 0; i < c->ni; i++) f1 += bsRec(ids, ids[i], 0, c->ni - 1) >= 0);
  snprintf(note, sizeof note, "%d/%d found", f1, c->ni); row(o, &k, "Incident lookup by id", "Binary search (recursive)", "O(log n)", t, OPS, note);
  Match m = mnew(c->ni, c->na); int mm = 0;
  for (int i = 0; i < c->ni; i++) for (int a = 0; a < c->na; a++) if (x->dA[a * n + c->inc[i].node] <= LIM[c->inc[i].sev]) m.adj[i * c->na + a] = 1;
  MEASURE(t, mm = mrun(&m)); snprintf(note, sizeof note, "%d of %d incidents coverable (upper bound)", mm, c->ni);
  row(o, &k, "Ambulance-incident assignment", "Bipartite matching", "O(V^3) adj. matrix", t, OPS, note);
  char *text = NEW(char, 20000 + 128); int tn = 0; const char *pat = "CARDIAC";
  while (tn < 20000 && c->ni > 0) for (int i = 0; i < c->ni && tn < 20000; i++) tn += snprintf(text + tn, 128, "INC%d %s sev%d node%d; ", c->inc[i].id, c->inc[i].type, c->inc[i].sev, c->inc[i].node);
  int c1 = 0, c2 = 0;
  MEASURE(t, c1 = naiveSearch(text, tn, pat, 7)); snprintf(note, sizeof note, "%d hits", c1); row(o, &k, "Dispatch-log search", "Naive matching", "O(nm)", t, OPS, note);
  MEASURE(t, c2 = kmpSearch(text, tn, pat, 7)); snprintf(note, sizeof note, "%d hits%s", c2, c1 == c2 ? "" : " MISMATCH"); row(o, &k, "Dispatch-log search", "KMP", "O(n+m)", t, OPS, note);
}

static char *runJson(int side, int na, int nh, int ni, unsigned seed, int shock) {
  ARU = 0; City c = makeCity(side, na, nh, ni, seed, shock); Ctx x = buildCtx(&c); int *ord = priorityOrder(&c); Result rs[2];
  for (int m = 0; m < 2; m++) { double t; MEASURE(t, rs[m] = m ? optimal(&c, &x, ord) : greedy(&c, &x, ord)); rs[m].ms = t; rs[m].ops = OPS; }
  SB o; o.cap = 1 << 16; o.n = 0; o.b = (char *)malloc(o.cap);
  sb(&o, "{\"side\":%d,\"edges\":[", c.side); int f = 1;
  for (int u = 0; u < c.g.n; u++) EACH(&c.g, u, e) if (u < c.g.to[e]) { sb(&o, "%s[%d,%d,%d]", f ? "" : ",", u, c.g.to[e], c.g.w[e]); f = 0; }
  sb(&o, "],\"amb\":["); for (int i = 0; i < c.na; i++) sb(&o, "%s{\"id\":%d,\"node\":%d}", i ? "," : "", i, c.amb[i]);
  sb(&o, "],\"hosp\":["); for (int i = 0; i < c.nh; i++) sb(&o, "%s{\"id\":%d,\"node\":%d,\"cap\":%d}", i ? "," : "", i, c.hosp[i].node, c.hosp[i].cap);
  sb(&o, "],\"inc\":["); for (int i = 0; i < c.ni; i++) sb(&o, "%s{\"id\":%d,\"node\":%d,\"sev\":%d,\"type\":\"%s\"}", i ? "," : "", i, c.inc[i].node, c.inc[i].sev, c.inc[i].type);
  int beds = 0, crit = 0; for (int h = 0; h < c.nh; h++) beds += c.hosp[h].cap; for (int i = 0; i < c.ni; i++) crit += c.inc[i].sev == 3;
  sb(&o, "],\"beds\":%d,\"crit\":%d,\"strategies\":[", beds, crit);
  for (int k = 0; k < 2; k++) {
    Result *r = &rs[k]; int eta = 0, th = 0, cs = 0, *used = NEW(int, c.nh), nal = 0;
    for (int i = 0; i < r->na; i++) { Assign *a = &r->a[i]; eta += a->eta; th += a->toHosp; cs += c.inc[a->inc].sev == 3; used[a->hosp]++; }
    sb(&o, "%s{\"name\":\"%s\",\"served\":%d,\"total\":%d,\"eta\":%d,\"avgEta\":%.3f,\"toHosp\":%d,\"critServed\":%d,\"bedsUsed\":%d,\"ms\":%.3f,\"ops\":%lld,\"unserved\":",
       k ? "," : "", r->name, r->na, c.ni, eta, r->na ? (double)eta / r->na : 0.0, th, cs, r->na, r->ms, r->ops);
    arr(&o, r->unserved, r->nu); sb(&o, ",\"alerts\":[");
#define AL(...) do { sb(&o, "%s\"", nal++ ? "," : ""); sb(&o, __VA_ARGS__); sb(&o, "\""); } while (0)
    for (int h = 0; h < c.nh; h++) {
      if (c.hosp[h].cap == 0) AL("Hospital H%d DIVERTED (0 beds available)", h);
      else if (used[h] >= c.hosp[h].cap) AL("OVERLOAD: Hospital H%d full (%d/%d beds)", h, used[h], c.hosp[h].cap);
    }
    if (c.ni > c.na) AL("AMBULANCE SHORTAGE: %d incidents vs %d ambulances", c.ni, c.na);
    for (int i = 0; i < r->nu; i++) { Inc *n = &c.inc[r->unserved[i]]; AL("%sincident #%d (%s, sev %d) NOT served", n->sev == 3 ? "CRITICAL " : "", n->id, n->type, n->sev); }
    sb(&o, "],\"assign\":[");
    for (int i = 0; i < r->na; i++) {
      Assign *a = &r->a[i]; sb(&o, "%s{\"inc\":%d,\"amb\":%d,\"hosp\":%d,\"eta\":%d,\"toHosp\":%d,\"r1\":", i ? "," : "", a->inc, a->amb, a->hosp, a->eta, a->toHosp);
      arr(&o, a->r1, a->n1); sb(&o, ",\"r2\":"); arr(&o, a->r2, a->n2); sb(&o, "}");
    }
    sb(&o, "]}");
  }
  sb(&o, "],\"bench\":["); bench(&o, &c, &x); sb(&o, "]}"); return o.b;
}

/* ------------------------------------------------------------ self-tests */
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static int cmpQ(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
static int selfTest(void) {
  Rng r = {5};
  for (int t = 0; t < 20; t++) { /* shortest paths agree on random graphs */
    ARU = 0; Graph g = gnew(25, 400);
    for (int i = 1; i < 25; i++) { int p = rnext(&r, i), w = 1 + rnext(&r, 9); gadd(&g, i, p, w); }
    for (int j = 0; j < 30; j++) { int a = rnext(&r, 25), b = rnext(&r, 25), w = 1 + rnext(&r, 9); gadd(&g, a, b, w); }
    int d1[25], d2[25], par[25]; dijkstra(&g, 0, d1, par); bellmanFord(&g, 0, d2); int *D = floydWarshall(&g);
    CHECK(memcmp(d1, d2, sizeof d1) == 0); CHECK(memcmp(D, d1, sizeof d1) == 0);
  }
  ARU = 0; int v[500], a[500], b[500], s[500];
  for (int i = 0; i < 500; i++) v[i] = a[i] = b[i] = s[i] = rnext(&r, 100);
  qsort(s, 500, sizeof(int), cmpQ); mergeSort(a, 500, cmpInt, NULL); quickSort(b, 0, 500, cmpInt, NULL);
  CHECK(memcmp(a, s, sizeof s) == 0); CHECK(memcmp(b, s, sizeof s) == 0);
  int ids[50]; for (int i = 0; i < 50; i++) ids[i] = i;
  for (int i = 0; i < 50; i++) { CHECK(bsIter(ids, 50, i) == i); CHECK(bsRec(ids, i, 0, 49) == i); }
  CHECK(bsIter(ids, 50, 99) == -1);
  Flow f = fnew(4, 20); fadd(&f, 0, 1, 3); fadd(&f, 0, 2, 2); fadd(&f, 1, 3, 2); fadd(&f, 2, 3, 3); fadd(&f, 1, 2, 1); CHECK(maxFlow(&f, 0, 3) == 5);
  Match m = mnew(3, 3); m.adj[0 * 3 + 0] = m.adj[0 * 3 + 1] = 1; m.adj[1 * 3 + 0] = 1; m.adj[2 * 3 + 1] = m.adj[2 * 3 + 2] = 1; CHECK(mrun(&m) == 3);
  CHECK(naiveSearch("abababab", 8, "abab", 4) == 3); CHECK(kmpSearch("abababab", 8, "abab", 4) == 3); CHECK(kmpSearch("aaaa", 4, "b", 1) == 0);
  for (unsigned seed = 1; seed <= 30; seed++) for (int shock = 0; shock < 2; shock++) { /* dispatch validity + optimal >= greedy */
    ARU = 0; City c = makeCity(6, 6, 3, 9, seed, shock); Ctx x = buildCtx(&c); int *ord = priorityOrder(&c); Result R[2]; R[0] = greedy(&c, &x, ord); R[1] = optimal(&c, &x, ord);
    for (int k = 0; k < 2; k++) {
      int *ua = NEW(int, c.na), *uh = NEW(int, c.nh);
      for (int i = 0; i < R[k].na; i++) { Assign *q = &R[k].a[i]; ua[q->amb]++; uh[q->hosp]++; CHECK(q->eta <= LIM[c.inc[q->inc].sev]); CHECK(q->toHosp <= HLIM); }
      for (int i = 0; i < c.na; i++) CHECK(ua[i] <= 1);
      for (int h = 0; h < c.nh; h++) CHECK(uh[h] <= c.hosp[h].cap);
      CHECK(R[k].na + R[k].nu == c.ni);
    }
    CHECK(R[1].na >= R[0].na);
  }
  if (fails) printf("%d test(s) FAILED\n", fails); else puts("All tests passed");
  return fails ? 1 : 0;
}

/* ------------------------------------------------------------ HTTP */
static int qint(const char *q, const char *key, int def, int lo, int hi) {
  char pat[32]; snprintf(pat, sizeof pat, "%s=", key); size_t L = strlen(pat);
  for (const char *p = strstr(q, pat); p; p = strstr(p + 1, pat)) if (p == q || p[-1] == '&') { def = atoi(p + L); break; }
  return def < lo ? lo : def > hi ? hi : def;
}
static void sendAll(sock_t cl, const char *s, size_t n) { size_t o = 0; while (o < n) { int k = (int)send(cl, s + o, (int)(n - o), 0); if (k <= 0) return; o += (size_t)k; } }
static char *readFile(const char *path) {
  FILE *f = fopen(path, "rb"); if (!f) return NULL; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  char *b = (char *)malloc((size_t)n + 1); size_t got = fread(b, 1, (size_t)n, f); b[got] = 0; fclose(f); return b;
}
static int serve(int port, const char *page) {
#ifdef _WIN32
  WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
#endif
  sock_t srv = socket(AF_INET, SOCK_STREAM, 0); int one = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one);
  struct sockaddr_in a; memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons((unsigned short)port);
  if (bind(srv, (struct sockaddr *)&a, sizeof a) < 0 || listen(srv, 16) < 0) { perror("bind/listen"); return 1; }
  printf("Dispatcher running at http://localhost:%d  (Ctrl+C to stop)\n", port); fflush(stdout);
  for (;;) {
    sock_t cl = accept(srv, NULL, NULL); if (cl == INVALID_SOCKET) continue;
    char buf[4096], method[8], target[1024]; int n = (int)recv(cl, buf, sizeof buf - 1, 0);
    if (n > 0) {
      buf[n] = 0; target[0] = 0;
      if (sscanf(buf, "%7s %1023s", method, target) == 2) {
        char *qm = strchr(target, '?'); const char *q = qm ? qm + 1 : ""; if (qm) *qm = 0;
        const char *type = "application/json", *code = "200 OK"; char *body = NULL;
        if (!strcmp(target, "/") || !strcmp(target, "/index.html")) {
          type = "text/html"; body = readFile(page); if (!body) { code = "404 Not Found"; body = strdup("index.html not found - run from the project folder"); }
        } else if (!strcmp(target, "/api/run")) {
          body = runJson(qint(q, "side", 7, 3, 40), qint(q, "amb", 8, 1, 200), qint(q, "hosp", 4, 1, 40), qint(q, "inc", 6, 1, 400), (unsigned)qint(q, "seed", 1, 0, 99999), qint(q, "shock", 0, 0, 1));
        } else { code = "404 Not Found"; body = strdup("{\"error\":\"not found\"}"); }
        char head[256]; int hl = snprintf(head, sizeof head, "HTTP/1.1 %s\r\nContent-Type: %s; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", code, type, strlen(body));
        sendAll(cl, head, (size_t)hl); sendAll(cl, body, strlen(body)); free(body);
      }
    }
    CLOSESOCK(cl);
  }
}

int main(int argc, char **argv) {
  ar_init((size_t)192 << 20); const char *cmd = argc > 1 ? argv[1] : "serve";
  if (!strcmp(cmd, "test")) return selfTest();
  if (!strcmp(cmd, "run")) {
    int v[6] = {7, 8, 4, 6, 1, 0}; for (int i = 0; i < 6 && argc > i + 2; i++) v[i] = atoi(argv[i + 2]);
    char *j = runJson(v[0], v[1], v[2], v[3], (unsigned)v[4], v[5]); puts(j); free(j); return 0;
  }
  return serve(argc > 2 ? atoi(argv[2]) : 8080, argc > 3 ? argv[3] : "web/index.html");
}
