#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>

pthread_t tid[1024] = {0}; // Max number of threads, note: we assume that n<1024

double p; // probability of a commentator speaks
int q;    // number of questions
int n;    // number of commentators
double t; // max time for a commentator to speak
pthread_cond_t *question_asked;
sem_t *commentator_process;
sem_t *commentators_done;
sem_t *speak[1024];
sem_t *speak_done;
pthread_mutex_t lock_qa;
pthread_mutex_t lock_speak[1024];
pthread_mutex_t lock_speak_done;
int commentator_count;
int speaker_count;
int answer_list[1024] = {0}; //we assume that n<1024

int pthread_sleep(double seconds);
char current_time[10] = "time";

char *currentTime() //ADD UP TO MILISECONDS
{
    char time[10];
    struct timeval tp;
    gettimeofday(&tp, NULL);
    sprintf(time, "%d", tp.tv_sec);
    return time;
    //return "time";
}
void *commentator(void *args)
{
    printf("commentator cp1\n");
    int id = (int)args;
    printf("commentator cp2\n");
    for (int i = 0; i < q; i++)
    {
        printf("commentator cp3\n");
        pthread_mutex_lock(&lock_qa);
        printf("commentator cp3.5\n");
        pthread_cond_wait(&question_asked, &lock_qa); //????
        pthread_mutex_unlock(&lock_qa);
        printf("commentator cp4\n");
        //generate answer with probability p
        double answer = (double)rand() / RAND_MAX;
        if (answer < p)
        {
            answer = 1;
        }
        else
        {
            answer = 0;
        }

        sem_wait(&commentator_process);
        if (answer == 0)
            answer_list[commentator_count] = 0;
        else
        {
            speaker_count++;
            answer_list[commentator_count] = id;
            printf("%s Commentator #%d generates answer, position in queue: %d\n", currentTime(), id, speaker_count);
            printf("question %d\n", i + 1);
        }
        commentator_count++;
        if (commentator_count == n)
        {
            sem_post(&commentators_done);
        }
        sem_post(&commentator_process);

        if (answer == 1)
        {
            //pthread_cond_wait(&speak[id], &lock_speak[id]);
            sem_wait(&speak[id]);
            double speak_time = (double)rand() / (double)(RAND_MAX / t);
            printf("question %d\n", i + 1);

            printf("%s Commentator #%d's turn to speak for %f seconds\n", currentTime(), id, speak_time);
            pthread_sleep(speak_time);
            printf("%s Commentator #%d finished speaking\n", currentTime(), id);
            //  pthread_cond_signal(&speak_done);
            sem_post(&speak_done);
        }
    }
}

void *moderator(void *args)
{
    //printf("moderatorcp1 %s\n", currentTime);
    for (int i = 1; i < q + 1; i++)
    {
         printf("moderatorcp1.5\n");
        commentator_count = 0;
        speaker_count = 0;
        pthread_cond_broadcast(&question_asked);
          printf("moderatorcp2\n");
        printf("%s: Moderator asks question %d\n", currentTime(), i);
        sem_wait(&commentators_done);
        // printf("moderatorcp3\n");
        for (int j = 0; j < n; j++)
        {
            if (answer_list[j] != 0) //note that 0 is not a valid id for any commentator
            {
                printf("waiting for speaking..\n");
              //  pthread_cond_signal(&speak[answer_list[j]]);
               // pthread_mutex_lock(&lock_speak_done);
                sem_post(&speak[answer_list[j]]);
                sem_wait(&speak_done);
               // pthread_cond_wait(&speak_done, &lock_speak_done);
                printf("waiting done..\n");
            }
        }
    }
}

int main()
{
    p = 0.75; // probability of a commentator speaks
    q = 5;    // number of questions
    n = 1;    // number of commentators
    t = 3;    // max time for a commentator to speak

    pthread_cond_init(&question_asked, NULL);
    sem_init(&commentator_process, 0, 1);
    sem_init(&commentators_done, 0, 1);

    for (int i = 1; i < n + 1; i++)
    {
        //pthread_cond_init(&speak[i], NULL);
        sem_init(&speak[i], 0,1);
    }
    //pthread_cond_init(&speak_done, NULL);
sem_init(&speak_done,0,1);
    pthread_create(&tid[0], NULL, moderator, NULL);

    for (int i = 1; i < n + 1; i++)
    {
        pthread_create(&tid[i], NULL, commentator, i);
    }

    for (int i = 0; i < n + 1; ++i)
        pthread_join(tid[i], NULL);
}

/**
 * pthread_sleep takes an integer number of seconds to pause the current thread
 * original by Yingwu Zhu
 * updated by Muhammed Nufail Farooqi
 * updated by Fahrican Kosar
 */
int pthread_sleep(double seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }

    struct timeval tp;
    struct timespec timetoexpire;
    // When to expire is an absolute time, so get the current time and add
    // it to our delay time
    gettimeofday(&tp, NULL);
    long new_nsec = tp.tv_usec * 1000 + (seconds - (long)seconds) * 1e9;
    timetoexpire.tv_sec = tp.tv_sec + (long)seconds + (new_nsec / (long)1e9);
    timetoexpire.tv_nsec = new_nsec % (long)1e9;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}
