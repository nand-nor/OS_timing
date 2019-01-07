/*
* Allie Clifford
* Comp111 Assignment 1: Timing System Calls
* Date created: 9/16/2018
* Last modified: 9/20/2018
*/

#define _POSIX_C_SOURCE 199309 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <stdint.h> 
#include <time.h>
#include <sys/time.h> 
#include <sys/resource.h> 
#include <pthread.h> 
#include <semaphore.h> 
#include <fcntl.h> 
#include<signal.h>
#include<errno.h>
#include<string.h>
#include<sys/sysinfo.h>
#include<sys/times.h>
#include<math.h>

#define CLOCKIDREAL CLOCK_REALTIME
#define CLOCKIDCPU CLOCK_PROCESS_CPUTIME_ID

#define SMALL 8024 //1 page
#define MED 8024 * 4 //4 pages
#define LARGE 8024 * 10 //10 pages
#define PTRIAL 1
#define STRIAL 2
#define OTRIAL 3
#define SBTRIAL 12
#define NUM_PTRIALS 10000000
#define NUM_STRIALS 100000000
#define NUM_OTRIALS 1000
#define NUM_SBTRIALS 100000

/*structs for ease of experimentation*/
typedef struct rtimers {
    struct rusage start;
    struct rusage end;
    double sresult;
    double uresult;
}rtimers;

typedef struct ttimers {
    struct timespec start;
    struct timespec end;
    double result;
}ttimers;


/*
 * global arrays for storing data for later comparison-- 
 * exclusively for experimentation purposes
 */
struct ttimers completed_ttime[15]; //15 total timing tests run
struct rtimers completed_ruse[15];
struct ttimers empty_ttime[4];
struct rtimers empty_ruse[4];

/*more global variables 0;)*/
double loop_ttime_p;
double loop_srtime_p;
double loop_urtime_p;
double loop_ttime_s;
double loop_srtime_s;
double loop_urtime_s;
double loop_ttime_o;
double loop_srtime_o;
double loop_urtime_o;
double loop_ttime_sb;
double loop_srtime_sb;
double loop_urtime_sb;
 
/*not declared in standard headers*/
extern void *sbrk(intptr_t); 

/*get the resolution of the real time clock*/
static double seconds_per_tick(){
    struct timespec res; 
    clock_getres(CLOCK_REALTIME, &res);
    double resolution = res.tv_sec + (((double)res.tv_nsec)/1.0e9);
    return resolution; 
}

/*time an empty loop using the ruse method*/
static inline void time_emptyloop_rtime(int arg){
    int i, trials;
    if(arg == PTRIAL){
        trials = NUM_PTRIALS;
    }else if(arg == STRIAL){
        trials = NUM_STRIALS;
    }else if(arg == OTRIAL){
        trials = NUM_OTRIALS;
    } else {
        trials = NUM_SBTRIALS;
    }
    struct rtimers test;
    getrusage(RUSAGE_SELF, &test.start); 
    for(i = 0; i < trials; ++i){
        //emptyloop
    }
    getrusage(RUSAGE_SELF, &test.end); 
    empty_ruse[arg] = test;
}
/*time an empty loop usng the clock_gettime() method*/
static inline void time_emptyloop_ttime(int arg){ 
    int i, trials;
    if(arg == PTRIAL){
        trials = NUM_PTRIALS;
    }else if(arg == STRIAL){
        trials = NUM_STRIALS;
    }else if(arg == OTRIAL){
        trials = NUM_OTRIALS;
    } else {
        trials = NUM_SBTRIALS;
    }
    struct ttimers test;
    clock_gettime(CLOCKIDREAL, &test.start); 
    for(i = 0; i < trials; ++i){
        //emptyloop
    }
    clock_gettime(CLOCKIDREAL, &test.end);   
    empty_ttime[arg] = test;
}

/*function to test calls to pthread_mutex_lock using ruse*/
static inline void time_pthread_rtime(int arg){     
    struct rtimers test;
    int i;
    static pthread_mutex_t mutex[NUM_PTRIALS]; 
    for(i = 0; i < NUM_PTRIALS; i++){  
        pthread_mutex_init(&mutex[i], NULL);
    } 
    getrusage(RUSAGE_SELF, &test.start); 
    for(i = 0; i < NUM_PTRIALS; ++i){
        pthread_mutex_lock(&mutex[i]); 
    }
    getrusage(RUSAGE_SELF, &test.end); 
    completed_ruse[arg] = test;
    for(i = 0; i < NUM_PTRIALS; i++){  
        pthread_mutex_unlock(&mutex[i]);
    } 
}
/*function for timing pthread_mutex_lock using clock_gettime()*/
static inline void time_pthread_ttime(int arg){     
    struct ttimers test;
    int i;
    static pthread_mutex_t mutex[NUM_PTRIALS];
    for(i = 0; i < NUM_PTRIALS; i++){  
        pthread_mutex_init(&mutex[i], NULL);
    } 
    clock_gettime(CLOCKIDREAL, &test.start); 
    for(i = 0; i < NUM_PTRIALS; ++i){
        pthread_mutex_lock(&mutex[i]);   
    }
    clock_gettime(CLOCKIDREAL, &test.end); 
    completed_ttime[arg] = test;
    for(i = 0; i < NUM_PTRIALS; i++){  
        pthread_mutex_unlock(&mutex[i]);
    } 
}
/*function timing sempost using clock_gettime()*/
static inline void time_sempost_ttime(int arg){
    int i, success;
    sem_t onesem;  
    success = sem_init(&onesem,0,0); 
    struct ttimers test;
    clock_gettime(CLOCKIDREAL, &test.start);  
    for(i =0; i < NUM_STRIALS; ++i){
        success = sem_post(&onesem); 
    }
    clock_gettime(CLOCKIDREAL, &test.end);  
    completed_ttime[arg] = test;
    if(sem_destroy(&onesem) < 0) {
        fprintf(stderr,"Error description is : %s\n",strerror(errno));
        exit(errno);
    }
}
/*function timing sempost using ruse*/
static inline void time_sempost_rtime(int arg){
    int i, success;
    sem_t onesem;  
    success = sem_init(&onesem,0,0); 
    struct rtimers test;
    getrusage(RUSAGE_SELF, &test.start);  
    for(i =0; i < NUM_STRIALS; ++i){
        success = sem_post(&onesem); 
    }
    getrusage(RUSAGE_SELF, &test.end);  
    completed_ruse[arg] = test;
    if(sem_destroy(&onesem) < 0) {
        fprintf(stderr,"Error description is : %s\n",strerror(errno));
        exit(errno);
    }
}
/*function to time open calls using clock_gettime()*/
static inline void time_open_ttime(int arg, char *filepath){
    struct ttimers test;      
    int i, fd, sfd;
    sfd = open(filepath, O_RDONLY);
    clock_gettime(CLOCKIDREAL, &test.start); 
    for(i =0; i < NUM_OTRIALS; ++i){
        fd = open(filepath, O_RDONLY);
    }
    clock_gettime(CLOCKIDREAL, &test.end); 
    completed_ttime[arg] = test; 
    for(i = fd; i >= sfd; i--){
        if(close(i) != 0) {perror("ttime fd wont close\n"); printf("fd: %d\n", i);}
    }
}
/*function ti time open calls using ruse*/
static inline void time_open_rtime(int arg, char *filepath){
    struct rtimers test;      
    int i, fd, sfd;  
    sfd = open(filepath, O_RDONLY);
    getrusage(RUSAGE_SELF, &test.start); 
    for(i =0; i < NUM_OTRIALS; ++i){
        fd = open(filepath, O_RDONLY); 
    }
    getrusage(RUSAGE_SELF, &test.end); 
    completed_ruse[arg] = test; 
    for(i = fd; i >= sfd;i--){
        if(close(i) != 0) {perror("rtime fd wont close\n"); printf("fd: %d\n", i); }
    }
}

/*function to time sbrk calls using clock_gettime()*/
static inline void time_sbrk_ttime(int arg, int size){
    struct ttimers test;
    void *p;
    clock_gettime(CLOCKIDREAL, &test.start);  
    for(int i =0; i < NUM_SBTRIALS; ++i){ 
        p=sbrk(size); 
    }
    clock_gettime(CLOCKIDREAL, &test.end);
    completed_ttime[arg] = test; 
}
/*function to time sbrk calls using ruse*/
static inline void time_sbrk_rtime(int arg, int size){
    struct rtimers test;
    void *p;
    getrusage(RUSAGE_SELF, &test.start);  
    for(int i =0; i < NUM_SBTRIALS; ++i){ 
        p=sbrk(size); 
    }
    getrusage(RUSAGE_SELF, &test.end);
    completed_ruse[arg] = test; 
}

/*function to caluclate the returned total time from
 * a specified experimental run's ruse stats */
static inline void calc_rtime(char *function, int i){
   // printf("i: %d\n");
    double avg_s = 0;
    double avg_u = 0;
    struct rusage *sbuf = &completed_ruse[i].start;
    struct rusage *ebuf = &completed_ruse[i].end;
    double su=(double)sbuf->ru_utime.tv_sec 
        + ((double)sbuf->ru_utime.tv_usec)/1e6; 
    double ss=(double)sbuf->ru_stime.tv_sec 
        + ((double)sbuf->ru_stime.tv_usec)/1e6; 
    double eu=(double)ebuf->ru_utime.tv_sec 
        + ((double)ebuf->ru_utime.tv_usec)/1e6; 
    double es=(double)ebuf->ru_stime.tv_sec 
        + ((double)ebuf->ru_stime.tv_usec)/1e6; 
    if(i <= PTRIAL){
        avg_s += ((es - ss) - loop_srtime_p)/NUM_PTRIALS;
        avg_u += ((eu - su)- loop_urtime_p)/NUM_PTRIALS;
    } else if(i == STRIAL){
        avg_s += ((es - ss) - loop_srtime_s)/NUM_STRIALS;
        avg_u += ((eu - su)- loop_urtime_s)/NUM_STRIALS;
    } else if ((i <= OTRIAL) && (i < SBTRIAL)){
        avg_s += ((es - ss) - loop_srtime_o)/NUM_OTRIALS;
        avg_u += ((eu - su)- loop_urtime_o)/NUM_OTRIALS;
    } else if (i >= SBTRIAL){
        avg_s += ((es - ss) - loop_srtime_sb)/NUM_SBTRIALS;
        avg_u += ((eu - su)- loop_urtime_sb)/NUM_SBTRIALS;
    } 
    //printf("Average user time (nanos) for %s: %e\n",function, avg_u); 
    //printf("Average sys time (nanos) for %s: %e\titer %d\n", function, avg_s, i); 
    completed_ruse[i].sresult = avg_s;
    completed_ruse[i].uresult = avg_u;
}

/*function to caluclate the returned total time from
 * a specified experimental run's clock_gettime() stats */
static inline void calc_ttime(char *function, int i){
    //printf("i: %d\n");
    double avg_time = 0; 
    struct timespec *sbuf = &completed_ttime[i].start;
    struct timespec *ebuf = &completed_ttime[i].end;
    double s=(double)sbuf->tv_sec 
        + ((double)sbuf->tv_nsec)/1e6; 
    double e=(double)ebuf->tv_sec 
        + ((double)ebuf->tv_nsec)/1e6; 
    if(i <= PTRIAL){
        avg_time += ((e - s) - loop_ttime_p)/NUM_PTRIALS;  
    } else if(i == STRIAL){
        avg_time += ((e - s) - loop_ttime_s)/NUM_STRIALS;  
    } else if ((i <= OTRIAL) && (i < SBTRIAL)){
        avg_time += ((e - s) - loop_ttime_o)/NUM_OTRIALS;  
    } else if (i >= SBTRIAL){
        avg_time += ((e - s) - loop_ttime_sb)/NUM_SBTRIALS;  
    }
    //printf("Average realtime (nanos) for %s: %e\titer: %d\n", function, avg_time, i); 
    completed_ttime[i].result = avg_time;
}
/*Parent function call to call pthread test*/
static inline void test_pthread(int i){
    printf("\n\nStarting pthread_mutex_lock() tests\n");
    int j;
    double srtime = 0; 
    double urtime = 0;
    double ttime = 0; 
    for(j = 0; j < 10; j++){
        time_pthread_rtime(i);
        time_pthread_ttime(i);
        calc_rtime("pthread", i);
        calc_ttime("pthread", i);
        srtime += completed_ruse[PTRIAL].sresult;
        urtime += completed_ruse[PTRIAL].uresult;
        ttime += completed_ttime[PTRIAL].result;
    }
    //printf("Average times for pthread_mutex_lock: sys rtime %e, user rtime %e, realtime %e\n",\
    //     srtime/10, urtime/10, ttime/10);
    printf("user=%e system=%e\n",urtime/10,srtime/10); 
    printf("end pthread_mutex_lock tests\n\n");
}

/*Parent function to call sempost test*/
static inline void test_sempost(int i){
    printf("\n\nStarting sem_post() tests\n");
    int j;
    double srtime = 0; 
    double urtime = 0;
    double ttime = 0; 
    for(j = 0; j < 10; j++){
        time_sempost_rtime(i);
        time_sempost_ttime(i);
        calc_rtime("sempost", i);
        calc_ttime("sempost", i);
        srtime += completed_ruse[STRIAL].sresult;
        urtime += completed_ruse[STRIAL].uresult;
        ttime += completed_ttime[STRIAL].result;
    }
    //printf("Average times for pthread_mutex_lock: sys rtime %e, user rtime %e, realtime %e\n",\
    //     srtime/10, urtime/10, ttime/10);
    printf("user=%e system=%e\n",urtime/10,srtime/10); 
    printf("end sempost tests\n\n");   

}



/*Function to create a system of subdiretcories and files*/
static inline void make_dirs(void){
    system("mkdir one");
    system("touch one/small");
    system("echo 'small file just a few bytes' >> one/small");
    system("touch one/medium");
    system("touch one/large");
    int bytes = 0;
    while(bytes != 3000){
        bytes += 100;
        system("echo 'medium file just a few bytes' >> one/medium");
    }
    bytes = 0;
    while(bytes != 8000){
        bytes += 50;
        system("cat one/medium  >> one/large");
    }   
    system("mkdir one/two");   
    system("mkdir one/two/three");
    system("mkdir one/two/three/four");
    system("cp one/small one/two/three/four/small");
    system("cp one/medium one/two/three/four/medium");
    system("cp one/large one/two/three/four/large");
    system("mkdir one/two/three/four/five");
    system("mkdir one/two/three/four/five/six");
    system("mkdir one/two/three/four/five/six/seven");
    system("mkdir one/two/three/four/five/six/seven/eight");
    system("mkdir one/two/three/four/five/six/seven/eight/nine");
    system("mkdir one/two/three/four/five/six/seven/eight/nine/ten");
    system("cp one/small one/two/three/four/five/six/seven/eight/nine/ten/small");
    system("cp one/medium one/two/three/four/five/six/seven/eight/nine/ten/medium");
    system("cp one/large one/two/three/four/five/six/seven/eight/nine/ten/large");
}

/*Function to remove created directories*/
static inline void rm_dirs(void){
    system("rm -rf one");
}

/* test_open calls 9 different subroutines:
 * three tests on a small subdirectory of dpeth 1, for a small, medium and large file size
 * three tests on a medium subdirectory of depth 4 on the same files, and
 * three tests on a large subdirectory of depth 10 on the same 3 files
 */
static inline void test_open(int i){
    printf("\nStarting open() tests\n");
    double srtime = 0; 
    double urtime = 0;
    double ttime = 0; 
    //printf("First test set: 3 varying file sizes in a subdirectory of depth 1\n");
    make_dirs();
    char *filepath = "one/small";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_smallsmall_subdirs", i);
    calc_ttime("open_smallsmall_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    filepath = "one/medium";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_medsmall_subdirs", i);
    calc_ttime("open_medsmall_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    filepath = "one/large";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_largesmall_subdirs", i);
    calc_ttime("open_largesmall_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    filepath = "one/two/three/four/small";
   // printf("\nSecond test set: subdirectory of depth 4 on same 3 file sizes\n");
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_smallmed_subdirs", i);
    calc_ttime("open_smallmed_subdirs", i); 
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    filepath = "one/two/three/four/medium";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_medmed_subdirs", i);
    calc_ttime("open_medmed_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    filepath = "one/two/three/four/large";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_largemed_subdirs", i);
    calc_ttime("open_largemed_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    //printf("\nFinal test set: subdirectory of depth 10 on same 3 file sizes\n");
    filepath = "one/two/three/four/five/six/seven/eight/nine/ten/small";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_smalllarge_subdirs", i);
    calc_ttime("open_smalllarge_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    filepath = "one/two/three/four/five/six/seven/eight/nine/ten/medium";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_medlarge_subdirs", i);
    calc_ttime("open_medlarge_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    filepath = "one/two/three/four/five/six/seven/eight/nine/ten/large";
    time_open_rtime(i, filepath);
    time_open_ttime(i, filepath);
    calc_rtime("open_largelarge_subdirs", i);
    calc_ttime("open_largelarge_subdirs", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    printf("user=%e system=%e\n",urtime/9,srtime/9); 
    printf("End open tests\n\n");
    rm_dirs();
    printf("Reasons for variance: size of file and location of file \
will impact length of system time\n");
}

/*Parent function call to test sbrk, calls 3 test parameters:
 * sbrk requesting 1 page of virtual memory,
 * sbrk requesting 4 pages of virtual memory,
 * and sbrk requesting 10 pages of virtual memory
 */
static inline void test_sbrk(int i){
    double srtime = 0; 
    double urtime = 0;
    double ttime = 0; 
 
    printf("\n\nStarting sbrk tests\n");
    //printf("First test: small (1 page) request\n");

    time_sbrk_rtime(i, SMALL);
    time_sbrk_ttime(i, SMALL);
    calc_rtime("sbrk small", i);
    calc_ttime("sbrk small", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    //printf("Second test: medium (4 page) request\n");
    time_sbrk_rtime(i, MED);
    time_sbrk_ttime(i, MED);
    calc_rtime("sbrk medium", i);
    calc_ttime("sbrk medium", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    ++i;
    //printf("Final sbrk test: large (10 page) request\n");
    time_sbrk_rtime(i, LARGE);
    time_sbrk_ttime(i, LARGE);
    calc_rtime("sbrk large", i);
    calc_ttime("sbrk large", i);
    srtime += completed_ruse[i].sresult;
    urtime += completed_ruse[i].uresult;
    ttime += completed_ttime[i].result;
    printf("End sbrk tests\n\n");
    printf("user=%e system=%e\n",urtime/3,srtime/3); 
} 

static inline void calc_empty_ttime(int i){ 
    struct timespec *sbuf = &empty_ttime[i].start;
    struct timespec *ebuf = &empty_ttime[i].end;
    double s=(double)sbuf->tv_sec 
        + ((double)sbuf->tv_nsec)/1e6;  
    double e=(double)ebuf->tv_sec 
        + ((double)ebuf->tv_nsec)/1e6; 
    if(i == PTRIAL){
 
        loop_ttime_p = (e - s);
    } else if(i == STRIAL){
  
        loop_ttime_s = (e - s);
    } else if ((i <= OTRIAL) && (i < SBTRIAL)){
   
        loop_ttime_o = (e - s);
    } else if(i == SBTRIAL){
    
        loop_ttime_sb = (e - s);
    }
    empty_ttime[i].result = (e - s);
}

static inline void calc_empty_rtime(int i){
    struct rusage *sbuf = &empty_ruse[i].start;
    struct rusage *ebuf = &empty_ruse[i].end;
    double su=(double)sbuf->ru_utime.tv_sec 
        + ((double)sbuf->ru_utime.tv_usec)/1e6; 
    double ss=(double)sbuf->ru_stime.tv_sec 
        + ((double)sbuf->ru_stime.tv_usec)/1e6; 
    double eu=(double)ebuf->ru_utime.tv_sec 
        + ((double)ebuf->ru_utime.tv_usec)/1e6; 
    double es=(double)ebuf->ru_stime.tv_sec 
        + ((double)ebuf->ru_stime.tv_usec)/1e6; 
   
    if(i == PTRIAL){

        loop_srtime_p = (es - ss);
        loop_urtime_p = (eu - su);
    } else if(i == STRIAL){

        loop_srtime_s = (es - ss);
        loop_urtime_s = (eu - su);
    } else if ((i <= OTRIAL) && (i < SBTRIAL)){

        loop_srtime_o = (es - ss);
        loop_urtime_o = (eu - su);
    } else if(i == SBTRIAL){

        loop_srtime_sb = (es - ss);
        loop_urtime_sb = (eu - su);
    }
    empty_ruse[i].sresult = (es - ss);
    empty_ruse[i].uresult = (eu - su);
}


int main() {
    struct timespec start,end;
    time_t start1;
    time(&start1); 

    printf("Start time: %s\n", ctime(&start1));
    printf("\n~~~~~~~~~~~~BEGIN TESTS~~~~~~~~~~~~\n");  
    int exp;
    int time_exps[4] = {PTRIAL, STRIAL, OTRIAL, SBTRIAL};
    for(int i = 0; i < 4; i++){
        exp = time_exps[i];
        time_emptyloop_rtime(exp);
        calc_empty_rtime(exp);
        time_emptyloop_ttime(exp);
        calc_empty_ttime(exp); 
    }
    
    test_pthread(PTRIAL);
    test_sempost(STRIAL);
    test_open(OTRIAL);
    test_sbrk(STRIAL);

    printf("~~~~~~~~~~~~~END TEST~~~~~~~~~~~~~\n"); 
    clock_gettime(CLOCK_REALTIME, &end);
  
    const time_t time_it = start.tv_sec + end.tv_sec;
    printf("End time: %s\n", ctime(&time_it));  

    exit(EXIT_SUCCESS);
} 

