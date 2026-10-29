/* engine.h - algorithms + domain model for Smart Ambulance Routing (C17, no dependencies).
 * Every assessed algorithm is hand-written: heap, sorts, searches, flow, matching, string matching.
 * Memory comes from one arena that is reset per request (no leaks, no free() calls). */
#ifndef ENGINE_H
#define ENGINE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef long long ll;
#define INF 1000000000
static ll OPS = 0; /* operation counter: reset before each measured run */

/* ------------------------------------------------------------ arena allocator */
static char *AR = NULL; static size_t ARN = 0, ARU = 0;
static inline void ar_init(size_t n) { AR = (char *)malloc(n); if (!AR) { fputs("out of memory\n", stderr); exit(1); } ARN = n; ARU = 0; }
static inline void *ar(size_t n) {
  n = (n + 15) & ~(size_t)15;
  if (ARU + n > ARN) { fputs("arena exhausted\n", stderr); exit(1); }
  void *p = AR + ARU; ARU += n; memset(p, 0, n); return p;
}
#define NEW(T, n) ((T *)ar(sizeof(T) * (size_t)((n) > 0 ? (n) : 1)))

/* ------------------------------------------------------------ graph + shortest paths */
typedef struct { int n, m; int *head, *nxt, *to, *w; } Graph;
static inline Graph gnew(int n, int ecap) {
  Graph g; g.n = n; g.m = 0; g.head = NEW(int, n); g.nxt = NEW(int, ecap); g.to = NEW(int, ecap); g.w = NEW(int, ecap);
  for (int i = 0; i < n; i++) g.head[i] = -1; return g;
}
static inline void gedge(Graph *g, int a, int b, int w) { g->to[g->m] = b; g->w[g->m] = w; g->nxt[g->m] = g->head[a]; g->head[a] = g->m++; }
static inline void gadd(Graph *g, int a, int b, int w) { gedge(g, a, b, w); gedge(g, b, a, w); }
#define EACH(g, u, e) for (int e = (g)->head[u]; e >= 0; e = (g)->nxt[e])

/* Greedy: Dijkstra with a hand-written binary min-heap, O((V+E) log V) */
static inline void hpush(int *hd, int *hv, int *hn, int d, int v) {
  int i = (*hn)++; while (i > 0 && hd[(i - 1) / 2] > d) { hd[i] = hd[(i - 1) / 2]; hv[i] = hv[(i - 1) / 2]; i = (i - 1) / 2; } hd[i] = d; hv[i] = v;
}
static inline void hpop(int *hd, int *hv, int *hn, int *d, int *v) {
  *d = hd[0]; *v = hv[0]; int ld = hd[--(*hn)], lv = hv[*hn], i = 0;
  for (;;) { int c = 2 * i + 1; if (c >= *hn) break; if (c + 1 < *hn && hd[c + 1] < hd[c]) c++; if (hd[c] >= ld) break; hd[i] = hd[c]; hv[i] = hv[c]; i = c; }
  if (*hn > 0) { hd[i] = ld; hv[i] = lv; }
}
static inline void dijkstra(const Graph *g, int s, int *d, int *par) {
  size_t mk = ARU; int cap = g->m + 2, hn = 0, *hd = NEW(int, cap), *hv = NEW(int, cap);
  for (int i = 0; i < g->n; i++) { d[i] = INF; par[i] = -1; }
  d[s] = 0; hpush(hd, hv, &hn, 0, s);
  while (hn > 0) {
    int du, u; hpop(hd, hv, &hn, &du, &u); if (du > d[u]) continue;
    EACH(g, u, e) { OPS++; int v = g->to[e]; if (d[u] + g->w[e] < d[v]) { d[v] = d[u] + g->w[e]; par[v] = u; hpush(hd, hv, &hn, d[v], v); } }
  }
  ARU = mk;
}
/* DP: Bellman-Ford, O(VE) */
static inline void bellmanFord(const Graph *g, int s, int *d) {
  for (int i = 0; i < g->n; i++) d[i] = INF; d[s] = 0;
  for (int it = 0; it < g->n - 1; it++) {
    int ch = 0;
    for (int u = 0; u < g->n; u++) if (d[u] < INF) EACH(g, u, e) { OPS++; if (d[u] + g->w[e] < d[g->to[e]]) { d[g->to[e]] = d[u] + g->w[e]; ch = 1; } }
    if (!ch) break;
  }
}
/* DP: Floyd-Warshall, O(V^3); returns n*n matrix */
static inline int *floydWarshall(const Graph *g) {
  int n = g->n, *D = NEW(int, n * n);
  for (int i = 0; i < n * n; i++) D[i] = INF;
  for (int i = 0; i < n; i++) { D[i * n + i] = 0; EACH(g, i, e) if (g->w[e] < D[i * n + g->to[e]]) D[i * n + g->to[e]] = g->w[e]; }
  for (int k = 0; k < n; k++) for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) {
    OPS++; if (D[i * n + k] < INF && D[k * n + j] < INF && D[i * n + k] + D[k * n + j] < D[i * n + j]) D[i * n + j] = D[i * n + k] + D[k * n + j];
  }
  return D;
}
static inline int pathTo(const int *par, int t, int **out) {
  int len = 0; for (int v = t; v != -1; v = par[v]) len++;
  int *p = NEW(int, len), k = len; for (int v = t; v != -1; v = par[v]) p[--k] = v; *out = p; return len;
}

/* ------------------------------------------------------------ divide & conquer: sorting + search */
typedef int (*Cmp)(int a, int b, void *ctx); /* <0 when a must come before b */
static inline int cmpInt(int a, int b, void *c) { (void)c; return a < b ? -1 : a > b; }
static inline void msortRec(int *a, int *t, int l, int r, Cmp f, void *c) { /* [l,r) stable, O(n log n) */
  if (r - l < 2) return;
  int m = (l + r) / 2, i = l, j = m, k = 0; msortRec(a, t, l, m, f, c); msortRec(a, t, m, r, f, c);
  while (i < m && j < r) { OPS++; if (f(a[j], a[i], c) < 0) t[k++] = a[j++]; else t[k++] = a[i++]; }
  while (i < m) t[k++] = a[i++];
  while (j < r) t[k++] = a[j++];
  memcpy(a + l, t, sizeof(int) * (size_t)(r - l));
}
static inline void mergeSort(int *a, int n, Cmp f, void *c) { size_t mk = ARU; int *t = NEW(int, n); msortRec(a, t, 0, n, f, c); ARU = mk; }
static inline void quickSort(int *a, int l, int r, Cmp f, void *c) { /* [l,r) avg O(n log n), worst O(n^2) */
  while (r - l > 1) {
    int m = l + (r - l) / 2, tmp = a[m]; a[m] = a[r - 1]; a[r - 1] = tmp; int p = a[r - 1], i = l;
    for (int j = l; j < r - 1; j++) { OPS++; if (f(a[j], p, c) < 0) { tmp = a[i]; a[i] = a[j]; a[j] = tmp; i++; } }
    tmp = a[i]; a[i] = a[r - 1]; a[r - 1] = tmp;
    if (i - l < r - i - 1) { quickSort(a, l, i, f, c); l = i + 1; } else { quickSort(a, i + 1, r, f, c); r = i; }
  }
}
static inline int bsIter(const int *a, int n, int x) {
  int l = 0, r = n - 1;
  while (l <= r) { OPS++; int m = (l + r) / 2; if (a[m] == x) return m; if (a[m] < x) l = m + 1; else r = m - 1; }
  return -1;
}
static inline int bsRec(const int *a, int x, int l, int r) {
  if (l > r) return -1;
  OPS++; int m = (l + r) / 2;
  if (a[m] == x) return m;
  return a[m] < x ? bsRec(a, x, m + 1, r) : bsRec(a, x, l, m - 1);
}

/* ------------------------------------------------------------ flow + matching */
typedef struct { int n, m; int *head, *nxt, *to, *cap; char *vis; } Flow; /* Ford-Fulkerson, DFS augmenting paths */
static inline Flow fnew(int n, int ecap) {
  Flow f; f.n = n; f.m = 0; f.head = NEW(int, n); f.nxt = NEW(int, ecap); f.to = NEW(int, ecap); f.cap = NEW(int, ecap); f.vis = NEW(char, n);
  for (int i = 0; i < n; i++) f.head[i] = -1; return f;
}
static inline int fadd(Flow *f, int a, int b, int c) { /* returns id of forward edge; reverse is id^1 */
  int id = f->m;
  f->to[id] = b; f->cap[id] = c; f->nxt[id] = f->head[a]; f->head[a] = id;
  f->to[id + 1] = a; f->cap[id + 1] = 0; f->nxt[id + 1] = f->head[b]; f->head[b] = id + 1; f->m += 2; return id;
}
static inline int fdfs(Flow *f, int u, int t, int fl) {
  if (u == t) return fl;
  f->vis[u] = 1;
  for (int id = f->head[u]; id >= 0; id = f->nxt[id]) {
    OPS++; int v = f->to[id];
    if (!f->vis[v] && f->cap[id] > 0) { int d = fdfs(f, v, t, fl < f->cap[id] ? fl : f->cap[id]); if (d > 0) { f->cap[id] -= d; f->cap[id ^ 1] += d; return d; } }
  }
  return 0;
}
static inline int maxFlow(Flow *f, int s, int t) { int tot = 0; for (;;) { memset(f->vis, 0, (size_t)f->n); int d = fdfs(f, s, t, INF); if (!d) break; tot += d; } return tot; }

typedef struct { int L, R; char *adj; int *mr, *ml; char *vis; } Match; /* maximum bipartite matching, augmenting paths */
static inline Match mnew(int L, int R) {
  Match m; m.L = L; m.R = R; m.adj = NEW(char, L * R); m.mr = NEW(int, R); m.ml = NEW(int, L); m.vis = NEW(char, R);
  for (int i = 0; i < R; i++) m.mr[i] = -1;
  for (int i = 0; i < L; i++) m.ml[i] = -1;
  return m;
}
static inline int mtry(Match *m, int u) {
  for (int v = 0; v < m->R; v++) {
    if (!m->adj[u * m->R + v] || m->vis[v]) continue;
    OPS++; m->vis[v] = 1;
    if (m->mr[v] < 0 || mtry(m, m->mr[v])) { m->mr[v] = u; m->ml[u] = v; return 1; }
  }
  return 0;
}
static inline int mrun(Match *m) { int c = 0; for (int u = 0; u < m->L; u++) { memset(m->vis, 0, (size_t)m->R); c += mtry(m, u); } return c; }

/* ------------------------------------------------------------ string matching (dispatch-log search) */
static inline int naiveSearch(const char *t, int tn, const char *p, int pn) {
  int c = 0;
  for (int i = 0; i + pn <= tn; i++) { int j = 0; while (j < pn) { OPS++; if (t[i + j] != p[j]) break; j++; } if (j == pn) c++; }
  return c;
}
static inline int kmpSearch(const char *t, int tn, const char *p, int pn) {
  size_t mk = ARU; int *f = NEW(int, pn), c = 0, k = 0;
  for (int i = 1, q = 0; i < pn; i++) { while (q && p[i] != p[q]) q = f[q - 1]; if (p[i] == p[q]) q++; f[i] = q; }
  for (int i = 0; i < tn; i++) {
    OPS++; while (k && t[i] != p[k]) { k = f[k - 1]; OPS++; }
    if (t[i] == p[k]) k++;
    if (k == pn) { c++; k = f[k - 1]; }
  }
  ARU = mk; return c;
}

/* ------------------------------------------------------------ domain model */
typedef struct { unsigned long long s; } Rng;
static inline int rnext(Rng *r, int m) { r->s = r->s * 6364136223846793005ULL + 1442695040888963407ULL; return (int)((r->s >> 33) % (unsigned long long)m); }
typedef struct { int id, node, cap; } Hosp;
typedef struct { int id, node, sev; const char *type; } Inc;
typedef struct { int side; Graph g; int na, nh, ni; int *amb; Hosp *hosp; Inc *inc; } City;
static const int LIM[4] = {0, 40, 25, 15}; /* max ambulance ETA (min) by severity 1..3 */
#define HLIM 30                            /* max incident->hospital travel (min) */

static inline City makeCity(int side, int na, int nh, int ni, unsigned seed, int shock) {
  Rng r = {(unsigned long long)seed * 2654435761ULL + 12345ULL}; City c; int n = side * side; c.side = side; c.g = gnew(n, 6 * n + 6);
  for (int y = 0; y < side; y++) for (int x = 0; x < side; x++) {
    int u = y * side + x, w;
    if (x + 1 < side) { w = 2 + rnext(&r, 8); gadd(&c.g, u, u + 1, w); }
    if (y + 1 < side) { w = 2 + rnext(&r, 8); gadd(&c.g, u, u + side, w); }
    if (x + 1 < side && y + 1 < side && rnext(&r, 5) == 0) { w = 3 + rnext(&r, 6); gadd(&c.g, u, u + side + 1, w); }
  }
  int *pool = NEW(int, n); for (int i = 0; i < n; i++) pool[i] = i;
  for (int i = n - 1; i > 0; i--) { int j = rnext(&r, i + 1), t = pool[i]; pool[i] = pool[j]; pool[j] = t; }
  if (nh > n / 3) nh = n / 3;
  if (na > (n - nh) / 2) na = (n - nh) / 2;
  if (ni > n - nh - na) ni = n - nh - na;
  c.na = na; c.nh = nh; c.ni = ni; c.hosp = NEW(Hosp, nh); c.amb = NEW(int, na); c.inc = NEW(Inc, ni);
  int p = 0;
  for (int h = 0; h < nh; h++) { int cap = 2 + rnext(&r, 3); if (shock) cap = (h == 0) ? 0 : (cap / 2 < 1 ? 1 : cap / 2); c.hosp[h].id = h; c.hosp[h].node = pool[p++]; c.hosp[h].cap = cap; }
  for (int a = 0; a < na; a++) c.amb[a] = pool[p++];
  static const char *T[] = {"CARDIAC", "TRAUMA", "STROKE", "BURN", "FRACTURE", "FEVER"};
  for (int i = 0; i < ni; i++) { int ty = rnext(&r, 6), sev = 1 + rnext(&r, 3); c.inc[i].id = i; c.inc[i].node = pool[p++]; c.inc[i].sev = sev; c.inc[i].type = T[ty]; }
  return c;
}

typedef struct { int inc, amb, hosp, eta, toHosp; int *r1, *r2; int n1, n2; } Assign;
typedef struct { const char *name; Assign *a; int na; int *unserved; int nu; double ms; ll ops; } Result;
typedef struct { int n; int *dA, *pA, *dI, *pI; } Ctx; /* Dijkstra from every ambulance and every incident, row-major n per source */

static inline Ctx buildCtx(const City *c) {
  Ctx x; int n = c->g.n; x.n = n; x.dA = NEW(int, c->na * n); x.pA = NEW(int, c->na * n); x.dI = NEW(int, c->ni * n); x.pI = NEW(int, c->ni * n);
  for (int a = 0; a < c->na; a++) dijkstra(&c->g, c->amb[a], x.dA + a * n, x.pA + a * n);
  for (int i = 0; i < c->ni; i++) dijkstra(&c->g, c->inc[i].node, x.dI + i * n, x.pI + i * n);
  return x;
}
static inline int cmpSev(int a, int b, void *ctx) { const City *c = (const City *)ctx; return c->inc[a].sev != c->inc[b].sev ? c->inc[b].sev - c->inc[a].sev : a - b; }
static inline int *priorityOrder(const City *c) { /* severity desc, then id (merge sort) */
  int *o = NEW(int, c->ni); for (int i = 0; i < c->ni; i++) o[i] = i;
  mergeSort(o, c->ni, cmpSev, (void *)c); return o;
}
static inline Assign mkA(const City *c, const Ctx *x, int i, int a, int h) {
  Assign q; int n = x->n; q.inc = i; q.amb = a; q.hosp = h; q.eta = x->dA[a * n + c->inc[i].node]; q.toHosp = x->dI[i * n + c->hosp[h].node];
  q.n1 = pathTo(x->pA + a * n, c->inc[i].node, &q.r1); q.n2 = pathTo(x->pI + i * n, c->hosp[h].node, &q.r2); return q;
}
static inline Result newResult(const City *c, const char *name) {
  Result R; R.name = name; R.a = NEW(Assign, c->ni); R.na = 0; R.unserved = NEW(int, c->ni); R.nu = 0; R.ms = 0; R.ops = 0; return R;
}
/* Strategy 1: greedy - most critical first, nearest free ambulance, nearest hospital with a free bed */
static inline Result greedy(const City *c, const Ctx *x, const int *ord) {
  Result R = newResult(c, "Greedy nearest-first"); int n = x->n;
  char *used = NEW(char, c->na); int *left = NEW(int, c->nh); for (int h = 0; h < c->nh; h++) left[h] = c->hosp[h].cap;
  for (int k = 0; k < c->ni; k++) {
    int i = ord[k], ba = -1, bh = -1, nd = c->inc[i].node;
    for (int a = 0; a < c->na; a++) { OPS++; int d = x->dA[a * n + nd]; if (!used[a] && d <= LIM[c->inc[i].sev] && (ba < 0 || d < x->dA[ba * n + nd])) ba = a; }
    for (int h = 0; h < c->nh; h++) { OPS++; int d = x->dI[i * n + c->hosp[h].node]; if (left[h] > 0 && d <= HLIM && (bh < 0 || d < x->dI[i * n + c->hosp[bh].node])) bh = h; }
    if (ba < 0 || bh < 0) { R.unserved[R.nu++] = i; continue; }
    used[ba] = 1; left[bh]--; R.a[R.na++] = mkA(c, x, i, ba, bh);
  }
  return R;
}
/* Strategy 2: joint max-flow  S -> ambulance -> incident(in->out, cap 1) -> hospital -> T(cap = beds).
 * Incident gates open in priority order, so critical patients are routed first. */
static inline Result optimal(const City *c, const Ctx *x, const int *ord) {
  Result R = newResult(c, "Max-flow (joint optimal)");
  int A = c->na, I = c->ni, H = c->nh, n = x->n, S = 0, T = 1, a0 = 2, i0 = a0 + A, o0 = i0 + I, h0 = o0 + I;
  Flow f = fnew(h0 + H, 2 * (A + I + A * I + I * H + H) + 8);
  for (int a = 0; a < A; a++) fadd(&f, S, a0 + a, 1);
  int *gate = NEW(int, I); for (int i = 0; i < I; i++) gate[i] = fadd(&f, i0 + i, o0 + i, 0);
  int *aiA = NEW(int, A * I), *aiI = NEW(int, A * I), *aiE = NEW(int, A * I), nai = 0;
  for (int a = 0; a < A; a++) for (int i = 0; i < I; i++) if (x->dA[a * n + c->inc[i].node] <= LIM[c->inc[i].sev]) { aiA[nai] = a; aiI[nai] = i; aiE[nai++] = fadd(&f, a0 + a, i0 + i, 1); }
  int *ihI = NEW(int, I * H), *ihH = NEW(int, I * H), *ihE = NEW(int, I * H), nih = 0, *hs = NEW(int, H);
  for (int i = 0; i < I; i++) {
    int k = 0; for (int h = 0; h < H; h++) { int d = x->dI[i * n + c->hosp[h].node]; if (d <= HLIM) hs[k++] = d * 1000 + h; }
    mergeSort(hs, k, cmpInt, NULL);
    for (int j = k - 1; j >= 0; j--) { int h = hs[j] % 1000; ihI[nih] = i; ihH[nih] = h; ihE[nih++] = fadd(&f, o0 + i, h0 + h, 1); } /* adjacency is LIFO: add farthest first */
  }
  for (int h = 0; h < H; h++) fadd(&f, h0 + h, T, c->hosp[h].cap);
  for (int k = 0; k < I; k++) { f.cap[gate[ord[k]]] = 1; maxFlow(&f, S, T); }
  int *amb = NEW(int, I), *hos = NEW(int, I); for (int i = 0; i < I; i++) amb[i] = hos[i] = -1;
  for (int e = 0; e < nai; e++) if (f.cap[aiE[e] ^ 1] > 0) amb[aiI[e]] = aiA[e];
  for (int e = 0; e < nih; e++) if (f.cap[ihE[e] ^ 1] > 0) hos[ihI[e]] = ihH[e];
  for (int k = 0; k < I; k++) { int i = ord[k]; if (amb[i] >= 0 && hos[i] >= 0) R.a[R.na++] = mkA(c, x, i, amb[i], hos[i]); else R.unserved[R.nu++] = i; }
  return R;
}
#endif
