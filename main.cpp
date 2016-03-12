#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

const int N = 3;
const int M = N * N;
pthread_mutex_t p = PTHREAD_MUTEX_INITIALIZER;
struct Sudoku {
  int data[M * M];
  unsigned long long mask[M * M];
  int current;
  Sudoku() {}
  Sudoku(const char* c, int i = 0) {
    for (int i = 0; i < M * M; i++) {
      data[i] = 0;
      mask[i] = 0;
      for (int j = 0; j < M; j++) {
        mask[i] |= 1 << j;
      }
    }
    for (int i = 0; i < M * M; i++) {
      if (c[i] != '0') set(i, c[i] - '0', "init");
    }
    current = i;
  }
  bool allowed(int v, int i) {
    return mask[i] & (1 << (v - 1));
    int x = i % M;
    int y = i / M;
    int bx = x / N * N;
    int by = y / N * N;
    for (int j = 0; j < M; j++) {
      int ox = j % N;
      int oy = j / N;
      if (x != j && v == data[y * M + j]) return false;
      if (y != j && v == data[j * M + x]) return false;
      if (i != (by + oy) * M + bx + ox && v == data[(by + oy) * M + bx + ox]) return false;
    }
    return true;
  }
  bool solved() {
    for (int i = 0; i < M * M; i++) {
      if (!data[i]) return false;
      if (!allowed(data[i], i)) return false;
    }
    return true;
  }
  bool solvable() {
    for (int i = 0; i < M * M; i++) {
      if (!mask[i]) {
        /* printf("not solvable at %d (%d, %d)\n", i, i % M, i / M); */
        /* print(); */
        return false;
      }
    }
    return true;
  }
  bool set(int i, int v, const char* phase) {
    unsigned long long m = ~(1 << (v - 1));
    int x = i % M;
    int y = i / M;
    int bx = x / N * N;
    int by = y / N * N;
    if (SLOW) {
      printf("\033[%d;1H\033[2KSet (%d, %d) to %d in phase %s\n\n", (N + 1) * N + 4, x + 1, y + 1, v, phase);
      print(i);
      print_b();
      usleep(1000 * SLOW);
    }
    data[i] = v;
    for (int j = 0; j < M; j++) {
      int ox = j % N;
      int oy = j / N;
      if (i != y * M + j) mask[y * M + j] &= m;
      if (i != j * M + x) mask[j * M + x] &= m;
      if (i != (by + oy) * M + bx + ox) mask[(by + oy) * M + bx + ox] &= m;
      if (!mask[y * M + j] || !mask[j * M + x] || !mask[(by + oy) * M + bx + ox]) {
        return false;
      }
    }
    mask[i] = ~m;
    return true;
  }
  bool backtrack(Sudoku* w, int* n);
  bool eliminate();
  bool loner();
  void print(int i = -1) {
    if (SLOW) {
      print_(i);
      return;
    }
    for (int i = 0; i < M * M; i++) {
      printf("%d", data[i]);
    }
  }
  void print_(int i) {
    for (int y = 0; y < M; y++) {
      for (int x = 0; x < M; x++) {
        int j = y * M + x;
        int c = data[j];
        printf("%c", i == j ? '<' : (current == j ? '[' : ' '));
        if (!c) printf(".");
        else printf("%d", c);
        printf("%c", i == j ? '>' : (current == j ? ']' : ' '));
        if (x % N == N - 1) printf(" ");
      }
      printf("\n");
      if (y % N == N - 1) printf("\n");
    }
    printf("\n");
  }
  void print_b() {
    for (int y = 0; y < M; y++) {
      for (int x = 0; x < M; x++) {
        int c = data[y * M + x];
        if (current == y * M + x) printf("["); else printf(" ");
        if (!mask[y * M + x]) {
          for (int i = M - 1; i >= 0; i--) {
            printf("X");
          }
        } else if (c) {
          for (int i = M - 1; i >= 0; i--) {
            printf("%c", c == (i + 1) ? 'O' : ' ');
          }
        } else {
          for (int i = M - 1; i >= 0; i--) {
            bool allow = mask[y * M + x] >> i & 1;
            printf("%c", allow ? '.' : ' ');
          }
        }
        if (current == y * M + x) printf("]"); else printf(" ");
        if (x % N == N - 1) printf(" ");
      }
      printf("\n");
      if (y % N == N - 1) printf("\n");
    }
    printf("\n");
  }
};

Sudoku stack[100000];
int curr = -1;
int waiting = 0;
unsigned long long pushes = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t d = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;

void waitForItems() {
  pthread_mutex_lock(&wait);
  waiting++;
  if (waiting >= T) {
    pthread_cond_signal(&done);
  }
  pthread_mutex_unlock(&wait);
  pthread_cond_wait(&cond, &mutex);
  pthread_mutex_lock(&wait);
  waiting--;
  pthread_mutex_unlock(&wait);
}

void pop(Sudoku* w, int* n) {
  pthread_mutex_lock(&mutex);
  while(curr < 0) waitForItems();
  int i = 0;
  while (i < W && curr >= 0) {
    w[i++] = stack[curr--];
  }
  *n = i;
  pthread_mutex_unlock(&mutex);
}

void push(Sudoku* w, int n) {
  pthread_mutex_lock(&mutex);
  for (int i = 0; i < n; i++) {
    stack[++curr] = w[i];
    pushes++;
  }
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

bool Sudoku::loner() {
  bool again;
  do {
    if(!eliminate()) return false;
    again = false;
    for (int y = 0; y < M; y++) {
      int h[M] = {};
      int p[M] = {};
      for (int x = 0; x < M; x++) {
        for (int i = 0; i < M; i++) {
          int c = (mask[y * M + x] >> i) & 1;
          h[i] += c;
          if (c) p[i] = x;
        }
      }
      for (int i = 0; i < M; i++) {
        if (h[i] == 1 && !data[y * M + p[i]]) {
          bool ok = set(y * M + p[i], i + 1, "hidden single in row");
          if (!ok) return false;
          again = true;
          break;
        }
      }
    }
    for (int x = 0; x < M; x++) {
      int h[M] = {};
      int p[M] = {};
      for (int y = 0; y < M; y++) {
        for (int i = 0; i < M; i++) {
          int c = (mask[y * M + x] >> i) & 1;
          h[i] += c;
          if (c) p[i] = y;
        }
      }
      for (int i = 0; i < M; i++) {
        if (h[i] == 1 && !data[p[i] * M + x]) {
          bool ok = set(p[i] * M + x, i + 1, "hidden single in col");
          if (!ok) return false;
          again = true;
          break;
        }
      }
    }
    for (int i = 0; i < M; i++) {
      int h[M] = {};
      int p[M] = {};
      int bx = i % 3 * 3;
      int by = i / 3 * 3;
      for (int j = 0; j < M; j++) {
        int x = bx + j % 3;
        int y = by + j / 3;
        for (int k = 0; k < M; k++) {
          int c = (mask[y * M + x] >> k) & 1;
          h[k] += c;
          if (c) p[k] = j;
        }
      }
      for (int k = 0; k < M; k++) {
        int x = bx + p[k] % 3;
        int y = by + p[k] / 3;
        if (h[k] == 1 && !data[y * M + x]) {
          bool ok = set(y * M + x, k + 1, "hidden single in block");
          if (!ok) return false;
          again = true;
          break;
        }
      }
    }
  } while (again);
  return true;
}

bool Sudoku::eliminate() {
  bool again;
  do {
    again = false;
    for (int i = 0; i < M * M; i++) {
      for (int j = 0; j < M; j++) {
        if (data[i] || mask[i] >> j > 1) break;
        else if (!data[i]) {
          bool ok = set(i, j + 1, "naked single");
          if (!ok) return false;
          again = true;
          break;
        }
      }
    }
  }
  while (again);
  return true;
}

bool Sudoku::backtrack(Sudoku* w, int* n) {
  int i = 0;
  if (!solvable()) return false;
  if (!loner()) return false;
  if (!solvable()) return false;
  while (current < M * M && data[current]) current++;
  if (current >= M * M) return true;
  for (int j = 1; j <= M; j++) {
    if (!allowed(j, current)) continue;
    Sudoku tmp = *this;
    tmp.set(current, j, "backtrace");
    tmp.current++;
    w[i++] = tmp;
  }
  *n = i;
  return false;
}


void* worker(void* data) {
  int id = *(int*)data;
  Sudoku w[W];
  Sudoku w_n[W * M];
  for (;;) {
    int n = 0;
    pop(w, &n);
    int j = 0;
    for (int i = 0; i < n; i++) {
      int k = 0;
      if (w[i].backtrack(&w_n[j], &k)) {
        if (SLOW) {
          printf("\033[16;1H");
          printf("\033[2KSolution:\n\n");
          w[i].print(); printf("\n");
          usleep(1000 * 1000);
        }
      }
      j += k;
    }
    push(w_n, j);
  }
}

void solve(Sudoku s) {
  stack[curr = pushes = 0] = s;
  pthread_mutex_lock(&d);
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
  pthread_cond_wait(&done, &mutex);
  pthread_mutex_unlock(&mutex);
  pthread_mutex_unlock(&d);
}

int main() {
  srand(time(NULL));
  int ids[T];
  setbuf(stdout, NULL);
  pthread_t threads[T];
  for (int i = 0; i < T; i++) {
    ids[i] = i;
    int s = pthread_create(&threads[i], NULL, worker, (void*)&ids[i]);
  }

  Sudoku s;
  for (int i = 0; i < M * M; i++) {
    s.data[i] = 0;
    s.mask[i] = 0;
    for (int j = 0; j < M; j++) {
      s.mask[i] |= 1 << j;
    }
  }

  FILE* f = fopen("./puzzles.txt", "r");
  char buf[100] = {};
  int i = 0;
  while (fgets(buf, 100, f) && !feof(f)) {
    if (SLOW) printf("\033[2J\033[1;1HNext puzzle: %s", buf);
    if (SLOW) usleep(1000 * 1000);
    if (SLOW) printf("\033[2J\033[1;1HPuzzle:\n\nInit...");
    Sudoku s(buf);
    if (SLOW) printf("\033[1;1HPuzzle:\n\n");
    if (SLOW) s.print();
    solve(s);
    if (i++ > 5000) return 0;
    printf("%d\r", i);
  }

  return 0;
}
