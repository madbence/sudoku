#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

const int N = 3;
const int M = N * N;
struct Sudoku {
  int data[M * M];
  int current;
  Sudoku() {}
  Sudoku(const char* c, int i = 0) {
    for (int i = 0; i < M * M; i++) {
      data[i] = c[i] - '0';
    }
    current = i;
  }
  bool allowed(int v, int i, bool rec = true) {
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
  bool backtrack(Sudoku* w, int* n);
  void print() {
    for (int i = 0; i < M * M; i++) {
      printf("%d", data[i]);
    }
  }
};

Sudoku stack[10000];
int curr = -1;
int waiting = 0;
const int T = 4;
const int W = 100;
int pushes = 0;

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

bool Sudoku::backtrack(Sudoku* w, int* n) {
  int i = 0;
  while (current < M * M && data[current]) current++;
  if (current >= M * M) return true;
  for (int j = 1; j <= M; j++) {
    if (!allowed(j, current)) continue;
    Sudoku tmp = *this;
    tmp.data[current] = j;
    tmp.current++;
    w[i++] = tmp;
  }
  *n = i;
  return false;
}

pthread_mutex_t p = PTHREAD_MUTEX_INITIALIZER;

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
        pthread_mutex_lock(&p);
        w[i].print();
        printf(" found by worker %d (after %d pushes to the stack)\n", id, pushes);
        pthread_mutex_unlock(&p);
      }
      j += k;
    }
    push(w_n, j);
  }
}

void solve(Sudoku s) {
  stack[curr = pushes = 0] = s;
  // printf("searching for solutions on %d worker threads (workload size: %d)...\n", T, W);
  stack[0].print();
  printf(" is the problem\n");
  pthread_cond_signal(&cond);
  pthread_mutex_lock(&d);
  pthread_cond_wait(&done, &d);
  pthread_mutex_unlock(&d);
  // printf("no more solutions (job stack is empty, %d items were processed)\n\n", pushes);
}

int main() {
  int ids[T];
  pthread_t threads[T];
  for (int i = 0; i < T; i++) {
    ids[i] = i;
    int s = pthread_create(&threads[i], NULL, worker, (void*)&ids[i]);
  }

  // Sudoku s("000801000000000043500000000000070800020030000000000100600000075003400000000200600");
  // solve(s);

  FILE* f = fopen("./puzzles.txt", "r");
  char buf[100] = {};
  while (fgets(buf, 100, f) && !feof(f)) {
    Sudoku s(buf);
    solve(s);
  }

  return 0;
}
