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
int commentator_count;
int speaker_count;
int answer_list[1024] = {0}; //we assume that n<1024

// Thread handling conditions
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t commentator_process = PTHREAD_COND_INITIALIZER;
pthread_cond_t question_asked = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue[1024];
pthread_cond_t speak[1024];

int pthread_sleep(double seconds);

char *currentTime() //ADD UP TO MILISECONDS
{
    char *time;
    struct timeval tp;
    gettimeofday(&tp, NULL);
    sprintf(time, "%d", tp.tv_sec);
    return time;
}
void *commentator(void *args)
{
    const int id = (long) args;
    for (int i = 0; i < q; i++)
    {
        // Wait for question asked broadcast
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&question_asked, &mutex);
	
	// Generate answer with probability p
	double prob = (double)random() / RAND_MAX;
	int answer = 0;
        if (prob < p)
        {
            answer = 1;
        }
        else
        {
            answer = 0;
        }

	// Lock for commentator process
        if (!answer)
            answer_list[commentator_count] = 0;
        else
        {
            speaker_count++;
            answer_list[commentator_count] = id;
            printf(" Commentator #%d generates answer, position in queue: %d\n", id, speaker_count);
        }

        commentator_count++;
        if (commentator_count == n)
        {
            	// Signal that all commentators are finished
		pthread_cond_signal(&commentator_process);
        }

        if (answer == 1)
        {
	    // Make the commentator wait untill the turn comes
	    pthread_cond_wait(&queue[id], &mutex);

            double speak_time = (double)random() / (double)(RAND_MAX / t);
            printf(" Commentator #%d's turn to speak for %f seconds\n", id, speak_time);
            pthread_sleep(speak_time);
            printf(" Commentator #%d finished speaking\n", id);

	    // Signal that this commentator is done speaking
	    pthread_cond_signal(&speak[id]);
       	}

	pthread_mutex_unlock(&mutex);
    }
}

void *moderator(void *args)
{
    for (int i = 1; i < q + 1; i++)
    {
	pthread_mutex_lock(&mutex);

        commentator_count = 0;
        speaker_count = 0;
	printf(": Moderator asks question %d\n", i);	
	// Signal all commentators to calculate probabilty and get in queue
	pthread_cond_broadcast(&question_asked);

	// Wait untill all commentator processes are finished
	pthread_cond_wait(&commentator_process, &mutex);

        for (int j = 0; j < n; j++)
        {
            if (answer_list[j] != 0) //note that 0 is not a valid id for any commentator
            {
		    // Give commentator with id answer_list[j] turn to speak
		    pthread_cond_signal(&queue[answer_list[j]]);
		    // Wait untill commentator is finished speaking
		    pthread_cond_wait(&speak[answer_list[j]], &mutex);
            }
        }
	pthread_mutex_unlock(&mutex);
    }
}

int main()
{
    p = 0.75; // probability of a commentator speaks
    q = 5;    // number of questions
    n = 4;    // number of commentators
    t = 3;    // max time for a commentator to speak

    printf("\n===============================\n");
    printf("Starting task\n");
    // Initiliaze waiting conditions
    for (int i=0; i<1024; i++) 
    {
    	pthread_cond_init(&queue[i], NULL);
	pthread_cond_init(&speak[i], NULL);
    }

    printf("After every cond init in main\n");

    pthread_create(&tid[0], NULL, moderator, NULL);
    for (int i = 1; i < n + 1; i++)
    {
        pthread_create(&tid[i], NULL, commentator, (void*)(long) i);
    }
    printf("After every thread create\n");

    printf("Main pthread join\n");
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
    pthread_mutex_t mutex2;
    pthread_cond_t conditionvar;
    if (pthread_mutex_init(&mutex2, NULL))
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

    pthread_mutex_lock(&mutex2);
    int res = pthread_cond_timedwait(&conditionvar, &mutex2, &timetoexpire);
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_destroy(&mutex2);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}
