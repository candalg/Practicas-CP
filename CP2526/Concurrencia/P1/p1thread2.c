#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS_A 4
#define NUM_THREADS_B 4
#define ITERACIONES 1000
#define ESPERA_NS 1000

struct args {
  volatile int *sum;
  volatile int *res;
  int iteraciones;
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

int lanzar_thread(pthread_t *t, struct args *args) {
   return pthread_create(t, NULL, incrementar, args);
}

int main() {
  pthread_t tha[NUM_THREADS_A];
  pthread_t thb[NUM_THREADS_B];

  volatile int a = 0;
  volatile int b = 0;

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

  printf("a=%d b=%d a+b=%d\n", a, b, a + b);
  
  free(arg_a); 
  return 0;
} 


