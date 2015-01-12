#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

pthread_mutex_t lock;
pthread_mutex_t slock;
pthread_mutex_t tlock;
typedef struct my_data{
  double balance;
  double amount;


} data;

void *withdraw(void *arg);
void *deposit(void *arg);
void  mainFunc();
void ShreyasLock(pthread_mutex_t *m);
void ShreyasUnlock(pthread_mutex_t *m);
void ShreyasInit(pthread_mutex_t m);
void ShreyasAcquired(pthread_mutex_t *m);

double balance = 0;
double cash = 1000;

void ShreyasAcquired(pthread_mutex_t *m){
    
  printf("acquired\n");

}


void ShreyasLock(pthread_mutex_t *m){
   int err = pthread_mutex_lock(m);
   if(err==0){
     
     ShreyasAcquired(m);
   }
   else if(err!=0){
     printf("lock was NOT successful,error: %s\n",strerror(err));
   }

}


void ShreyasUnlock(pthread_mutex_t *m){

  int err = pthread_mutex_unlock(m);
   if(err!=0){
    printf("unlock was NOT successful,error: %s\n",strerror(err));
  }

}

void ShreyasInit(pthread_mutex_t m){
  pthread_mutex_init(&m,NULL);
}


void mainFunc(){
    ShreyasLock(&slock);
  
  balance+=200;
  printf("after main thread: %f\n",balance);
  ShreyasUnlock(&slock);

}


void *withdraw(void *arg){

  ShreyasLock(&lock);
 
  balance-=100;
  printf("after withdraw: %f\n",balance);
  ShreyasUnlock(&lock);
  
  return NULL;

}


void *deposit(void *arg){
  
  ShreyasLock(&lock);
 
  balance+=100;
  printf("after deposit: %f\n",balance);
  ShreyasUnlock(&lock);
  balance+=5;  
  return NULL;

}



int main(){

 int status =  pthread_mutex_init(&lock,NULL);
 if(status!=0){
     printf("mutex init failed\n");
        }
 
 int status2 =  pthread_mutex_init(&slock,NULL);
 if(status2!=0){
    printf("mutex2 init failed\n");
     
 }

 int status3 =  pthread_mutex_init(&tlock,NULL);
 if(status3!=0){
   printf("mutex3 init failed\n");
     
 }
   
  pthread_t ft,st;
  
  pthread_create(&ft,NULL,deposit,NULL);  
  pthread_create(&st,NULL,withdraw,NULL);
  
  pthread_join(ft,NULL);
  pthread_join(st,NULL);
  pthread_mutex_destroy(&lock);
  pthread_mutex_destroy(&slock);
  pthread_mutex_destroy(&tlock);
  printf("done\n");

  return 0;

}
