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
double b; //probability of a breaking event happens
int commentator_count;
int speaker_count;
int answer_list[1024] = {0}; //we assume that n<1024
int simulation_done = 0;
int current_speaker = -1;
int breaking_event_happening;

// Thread handling conditions
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t simulation_done_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t current_speaker_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t breaking_event_happening_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t breaking_event_ends_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t commentators_done = PTHREAD_COND_INITIALIZER;
pthread_cond_t breaking_event_ends = PTHREAD_COND_INITIALIZER;
sem_t commentator_process;
sem_t breaking_event;
pthread_cond_t question_asked = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue[1024];
pthread_cond_t speak[1024];
pthread_cond_t conditionvar;

int pthread_sleep_updated(double seconds);
char currentTime[12];
struct timeval initial_time;
char *getCurrentTime() //ADD UP TO MILISECONDS
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    int m = (tp.tv_sec - initial_time.tv_sec) / 60;
    int s = tp.tv_sec - initial_time.tv_sec - m * 60;
    int ms = tp.tv_usec / 1000;
    if (ms < 0)
        ms += 1000;
    sprintf(currentTime, "[%02d:%02d.%03d]", m, s, ms);
    return currentTime;
}
void *commentator(void *args)
{
    const int id = (long)args;
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
        sem_wait(&commentator_process);
        if (!answer)
            answer_list[commentator_count] = 0;
        else
        {
            speaker_count++;
            answer_list[commentator_count] = id;
            printf("%s Commentator #%d generates answer, position in queue: %d\n", getCurrentTime(), id, speaker_count);
        }

        commentator_count++;
        if (commentator_count == n)
        {
            // Signal that all commentators are finished
            pthread_cond_signal(&commentators_done);
        }
        sem_post(&commentator_process);
        if (answer == 1)
        {
            // Make the commentator wait untill the turn comes
            pthread_cond_wait(&queue[id], &mutex);

            double speak_time = (double)random() / (double)(RAND_MAX / t);
            printf("%s Commentator #%d's turn to speak for %f seconds\n", getCurrentTime(), id, speak_time);
            pthread_sleep_updated(speak_time);
            printf("%s Commentator #%d finished speaking\n", getCurrentTime(), id);

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
        printf("%s Moderator asks question %d\n", getCurrentTime(), i);
        // Signal all commentators to calculate probabilty and get in queue
        pthread_cond_broadcast(&question_asked);

        // Wait untill all commentator processes are finished
        pthread_cond_wait(&commentators_done, &mutex);

        for (int j = 0; j < n; j++)
        {
            if (answer_list[j] != 0) //note that 0 is not a valid id for any commentator
            {
                pthread_mutex_lock(&breaking_event_happening_mutex);
                if (breaking_event_happening)
                {
                    pthread_mutex_unlock(&breaking_event_happening_mutex);
                    pthread_mutex_lock(&breaking_event_ends_mutex);
                    pthread_cond_wait(&breaking_event_ends, &breaking_event_ends_mutex);
                    pthread_mutex_unlock(&breaking_event_ends_mutex);
                }
                else
                {
                    pthread_mutex_unlock(&breaking_event_happening_mutex);
                }
                // Give commentator with id answer_list[j] turn to speak
                pthread_cond_signal(&queue[answer_list[j]]);

                pthread_mutex_lock(&current_speaker_mutex);
                current_speaker = answer_list[j];
                pthread_mutex_unlock(&current_speaker_mutex);
                // Wait untill commentator is finished speaking
                pthread_cond_wait(&speak[answer_list[j]], &mutex);
            }
        }

        pthread_mutex_lock(&current_speaker_mutex);
        current_speaker = -1;
        pthread_mutex_unlock(&current_speaker_mutex);
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_lock(&simulation_done_mutex);
    simulation_done = 1;
    //   printf("Now simulation_done is 1\n");
    pthread_mutex_unlock(&simulation_done_mutex);
}

void *breaking_event_observer(void *args)
{
    while (1)
    {
        sem_wait(&breaking_event);
        pthread_mutex_lock(&breaking_event_happening_mutex);
        breaking_event_happening = 1;
        // pthread_mutex_unlock(&breaking_event_happening_mutex);

        printf("%s Breaking news!\n", getCurrentTime());
        pthread_cond_signal(&conditionvar);
        pthread_mutex_lock(&current_speaker_mutex);
        if (current_speaker != -1)
            printf("%s Commentator #%d is cut short due to breaking event\n", getCurrentTime(), current_speaker);
        pthread_mutex_unlock(&current_speaker_mutex);
        //pthread_sleep_updated(0.5); //CHANGE BACK TO 5
        printf("%s Breaking news ends\n", getCurrentTime());


        //   pthread_mutex_lock(&breaking_event_ends_mutex);
        pthread_cond_broadcast(&breaking_event_ends);
        printf("observer broadcast\n");
        // pthread_mutex_lock(&breaking_event_happening_mutex);
        printf("observer changed breaking_event_happening\n");
        breaking_event_happening = 0;
        pthread_mutex_unlock(&breaking_event_happening_mutex);
        //  pthread_mutex_unlock(&breaking_event_ends_mutex);
    }
}
int main()
{
    p = 0.75; // probability of a commentator speaks
    q = 5;    // number of questions
    n = 5;    // number of commentators
    t = 0.5;  // max time for a commentator to speak
    b = 0.5;  //probability of a breaking event happens

    gettimeofday(&initial_time, NULL);

    printf("\n===============================\n");
    printf("Starting task\n");
    // Initiliaze waiting conditions
    for (int i = 0; i < 1024; i++)
    {
        pthread_cond_init(&queue[i], NULL);
        pthread_cond_init(&speak[i], NULL);
    }

    sem_init(&commentator_process, 0, 1);
    sem_init(&breaking_event, 0, 1);
    printf("After every cond init in main\n");

    pthread_create(&tid[0], NULL, moderator, NULL);
    for (int i = 1; i < n + 1; i++)
    {
        pthread_create(&tid[i], NULL, commentator, (void *)(long)i);
    }
    pthread_create(&tid[n + 1], NULL, breaking_event_observer, NULL);
    printf("After every thread create\n");

    while (1)
    {
          printf("CP1:simulation done is %d\n",simulation_done);

        pthread_mutex_lock(&simulation_done_mutex);
         printf("CP2:simulation done is %d\n",simulation_done);
        if (simulation_done)
            break;
        pthread_mutex_unlock(&simulation_done_mutex);
        double prob = (double)random() / RAND_MAX;
        if (prob < b) //a breaking event occurs
        {
            sem_post(&breaking_event);
                pthread_mutex_lock(&breaking_event_ends_mutex);
            pthread_cond_wait(&breaking_event_ends, &breaking_event_ends_mutex);
               pthread_mutex_unlock(&breaking_event_ends_mutex);
        }
        //pthread_sleep_updated(1);
    }
    printf("Main pthread join\n");

    pthread_cancel(tid[n + 1]);
    for (int i = 0; i <= n + 1; ++i)
        pthread_join(tid[i], NULL);
}

/**
 * pthread_sleep takes an integer number of seconds to pause the current thread
 * original by Yingwu Zhu
 * updated by Muhammed Nufail Farooqi
 * updated by Fahrican Kosar
 * updated by Burcu Yıldız and Orhan Ceyhun Aslan
 */
int pthread_sleep_updated(double seconds)
{
    pthread_mutex_t mutex;
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
