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
};

struct monitor_args {
  volatile int *a;
  volatile int *b;
  volatile int *fin;  
};

void *incrementar(void *arg) {
  struct args *ar= arg;
    
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = ESPERA_NS;

  for (int i = 0; i < ar->iteraciones; i++) {

    int v1 = *(ar->sum);
    v1++;
    *(ar->sum) = v1;

    int v2 = *(ar->res);
    v2--;
    *(ar->res) = v2;
   
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
    int va = *(m->a);
    int vb = *(m->b);
    printf("[MON] a=%d b=%d a+b=%d\n", va, vb, va + vb);
    nanosleep(&ts, NULL);
  }

  int va = *(m->a);
  int vb = *(m->b);
  printf("[MON] FIN a=%d b=%d a+b=%d\n", va, vb, va + vb);

  return NULL;
}

int lanzar_thread(pthread_t *t, struct args *args) {
   return pthread_create(t, NULL, incrementar, args);
}

int main() {
  pthread_t tha[NUM_THREADS_A];
  pthread_t thb[NUM_THREADS_B];
  pthread_t thm;

  volatile int a = 0;
  volatile int b = 0;
  volatile int fin = 0;
  
  struct monitor_args margs;
  margs.a = &a;
  margs.b = &b;
  margs.fin = &fin;

  if (pthread_create(&thm, NULL, monitor, &margs) != 0) {
    perror("pthread_create monitor");
    exit(1);
  }

  struct args *arg_a = malloc(sizeof(*arg_a));
  if (!arg_a) { perror("malloc"); exit(1); }
  
  arg_a->sum = &a;
  arg_a->res = &b;
  arg_a->iteraciones = ITERACIONES;
  
  struct args arg_b;
  arg_b.sum = &b;        
  arg_b.res = &a;        
  arg_b.iteraciones = ITERACIONES;


  for (int i = 0; i < NUM_THREADS_A; i++) {
      if (lanzar_thread(&tha[i], arg_a) != 0) {        
        perror("pthread_create");
           exit(1);
      }
  }

  for (int i = 0; i < NUM_THREADS_B; i++) {
      if (lanzar_thread(&thb[i], &arg_b) != 0) {
        perror("pthread_create B");
        exit(1);
      }
  }

  for (int i = 0; i < NUM_THREADS_A; i++) {
      pthread_join(tha[i], NULL);
  }

  for (int i = 0; i < NUM_THREADS_B; i++) {
      pthread_join(thb[i], NULL);
  }

  fin = 1;
  pthread_join(thm, NULL);

  printf("FINAL: a=%d b=%d a+b=%d\n", a, b, a + b);
  
  free(arg_a); 
  return 0;
} 


