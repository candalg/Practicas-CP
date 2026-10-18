
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS_A 4
#define NUM_THREADS_B 4
#define ITERACIONES 1000
#define ESPERA_NS 1000
#define MONITOR_MS 10

struct args {
  volatile int *sum;
  volatile int *res;
  int iteraciones;
  pthread_mutex_t *mtx_sum;  // mutex suma
  pthread_mutex_t *mtx_res;  // mutex resta
};

struct monitor_args {
  volatile int *a;
  volatile int *b;
  volatile int *fin;
  pthread_mutex_t *mtx_a;
  pthread_mutex_t *mtx_b;
};

static inline void lock_two(pthread_mutex_t *m1, pthread_mutex_t *m2) {
  if (m1 < m2) {
    pthread_mutex_lock(m1);
    pthread_mutex_lock(m2);
  } else if (m2 < m1) {
    pthread_mutex_lock(m2);
    pthread_mutex_lock(m1);
  } else {
    pthread_mutex_lock(m1);
  }
}

static inline void unlock_two(pthread_mutex_t *m1, pthread_mutex_t *m2) {
  if (m1 == m2) {
    pthread_mutex_unlock(m1);
    return;
  }
  if (m1 < m2) {
    pthread_mutex_unlock(m2);
    pthread_mutex_unlock(m1);
  } else {
    pthread_mutex_unlock(m1);
    pthread_mutex_unlock(m2);
  }
}

void *incrementar(void *arg) {
  struct args *ar = (struct args *)arg;

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = ESPERA_NS;

  for (int i = 0; i < ar->iteraciones; i++) {

    lock_two(ar->mtx_sum, ar->mtx_res);

    int v1 = *(ar->sum);
    v1++;
    *(ar->sum) = v1;

    int v2 = *(ar->res);
    v2--;
    *(ar->res) = v2;

    unlock_two(ar->mtx_sum, ar->mtx_res);

    if (ESPERA_NS > 0) nanosleep(&ts, NULL);
  }

  return NULL;
}

void *monitor(void *arg) {
  struct monitor_args *m = (struct monitor_args *)arg;

  struct timespec ts;
  ts.tv_sec = MONITOR_MS / 1000;
  ts.tv_nsec = (MONITOR_MS % 1000) * 1000000L;

  while (*(m->fin) == 0) {
    lock_two(m->mtx_a, m->mtx_b);
    int va = *(m->a);
    int vb = *(m->b);
    unlock_two(m->mtx_a, m->mtx_b);

    printf("[MON] a=%d b=%d a+b=%d\n", va, vb, va + vb);
    nanosleep(&ts, NULL);
  }

  lock_two(m->mtx_a, m->mtx_b);
  int va = *(m->a);
  int vb = *(m->b);
  unlock_two(m->mtx_a, m->mtx_b);

  printf("[MON] FIN a=%d b=%d a+b=%d\n", va, vb, va + vb);
  return NULL;
}

int lanzar_thread(pthread_t *t, struct args *args) {
  return pthread_create(t, NULL, incrementar, args);
}

int main(void) {
  pthread_t tha[NUM_THREADS_A];
  pthread_t thb[NUM_THREADS_B];
  pthread_t thm;

  volatile int a = 0;
  volatile int b = 0;
  volatile int fin = 0;

  pthread_mutex_t mtx_a, mtx_b;
  pthread_mutex_init(&mtx_a, NULL);
  pthread_mutex_init(&mtx_b, NULL);

  struct monitor_args margs;
  margs.a = &a;
  margs.b = &b;
  margs.fin = &fin;
  margs.mtx_a = &mtx_a;
  margs.mtx_b = &mtx_b;

  if (pthread_create(&thm, NULL, monitor, &margs) != 0) {
    perror("pthread_create monitor");
    exit(1);
  }

  struct args *arg_a = malloc(sizeof(*arg_a));
  if (!arg_a) { perror("malloc"); exit(1); }

  arg_a->sum = &a;              // a++
  arg_a->res = &b;              // b--
  arg_a->iteraciones = ITERACIONES;
  arg_a->mtx_sum = &mtx_a;       // mutex de a
  arg_a->mtx_res = &mtx_b;       // mutex de b

  struct args arg_b;
  arg_b.sum = &b;               // b++
  arg_b.res = &a;               // a--
  arg_b.iteraciones = ITERACIONES;
  arg_b.mtx_sum = &mtx_b;        // mutex de b
  arg_b.mtx_res = &mtx_a;        // mutex de a

  // Lanzar A y B concurrentemente
  for (int i = 0; i < NUM_THREADS_A; i++) {
    if (lanzar_thread(&tha[i], arg_a) != 0) { perror("pthread_create A"); exit(1); }
  }
  for (int i = 0; i < NUM_THREADS_B; i++) {
    if (lanzar_thread(&thb[i], &arg_b) != 0) { perror("pthread_create B"); exit(1); }
  }

  for (int i = 0; i < NUM_THREADS_A; i++) pthread_join(tha[i], NULL);
  for (int i = 0; i < NUM_THREADS_B; i++) pthread_join(thb[i], NULL);

  fin = 1;
  pthread_join(thm, NULL);

  printf("FINAL: a=%d b=%d a+b=%d\n", a, b, a + b);

  free(arg_a);
  pthread_mutex_destroy(&mtx_a);
  pthread_mutex_destroy(&mtx_b);
  return 0;
}
