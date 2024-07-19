#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int pcount = getpcount();
  printf("Initial number of my processes: %d\n", pcount);


//create process
for(int i=0;i<5;i++){
    if(fork()==0){
        //the subproc sleep for a while and the exit
        sleep(10);
        exit(0);
    }
}
printf("create 5 subprocess\n");

sleep(5); //process wait to make sure that all the subprocesses have started

int new_count=getpcount();
printf("New number of my processes: %d\n", new_count);

for (int i=0;i<5;i++){
    wait(0);
}

int final_count=getpcount();
printf("Final number of my processes: %d\n", final_count);
  exit(0);
}
