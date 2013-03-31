#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ERR_RING_NOT_FULL -1
#define RECORD_OK 1


#define MAX_RECORDS 1000
#define STDIN_LINES_SIZE 64
#define TRANSACTION_ID_SIZE 20
#define LISTEN_PORT 2020


typedef struct {
    float time;
    char transaction_id[TRANSACTION_ID_SIZE];
} status_record;

typedef struct {
    float *time;
    char *transaction_id;
} status_record_ptr;

typedef struct {
    status_record record[MAX_RECORDS];
    int current_index;
    int has_overlapped;
} ring_buffer;

ring_buffer ring;
pthread_t thread_stdin; /* Thread for the stdin read loop */
pthread_mutex_t lock;

int pump_record(status_record this_record){
    printf("pushing to ring buffer: %f - %s\n", this_record.time, this_record.transaction_id);

    /* We need exclusive access to ring data structure and we _could_ be asking for the percentile
       from this data structure. Lock it so we don't corrupt the data
     */
    pthread_mutex_lock(&lock);
    /* Pop it onto the ring buffer */
    ring.record[ring.current_index] = this_record;
    /* Increment our index */
    ring.current_index++;
    /* Have we overflowed? */
    if (ring.current_index >= MAX_RECORDS){
        ring.current_index = 0;
        ring.has_overlapped = 1;
    }
    pthread_mutex_unlock(&lock);
}


int get_percentile(float percentile, status_record *percentile_record){
    int i;
    int percentile_element;

    /* We need exclusive access to the ring buffer. We are populating another data 
       structure with pointers to the data in the original ring buffer. If we don't lock,
       we will overwrite the original data and we will be pointing to data that is not 
       sorted. We are assuming the time taken to sort is less the time taken for the 
       pipe buffer to fill up.
      
     */
    pthread_mutex_lock(&lock);

    if(!ring.has_overlapped){
        printf("Ring buffer not full enough: Need %i, have %i\n", MAX_RECORDS, ring.current_index);
        pthread_mutex_unlock(&lock);
        return ERR_RING_NOT_FULL;
    }
    /* For pointer to sorted index */
    status_record_ptr pointed_records[MAX_RECORDS];
    /* Populate pointer to struct - we could copy the data and this would allow us to 
       keep the ring buffer unlocked while we are sorting. Instead, lock the ring from
       updating so we don't use as much space, only pointers to the real values
     */
    for (i=0; i<MAX_RECORDS; i++){
        pointed_records[i].time = &ring.record[i].time;
        pointed_records[i].transaction_id = ring.record[i].transaction_id;
    }
    
    /* Insertion sort */
    for (i=0; i<MAX_RECORDS; i++){
        int j;
        float *v = pointed_records[i].time;
        char *transaction_id = pointed_records[i].transaction_id;
        for (j=i-1; j>=0; j--)
        {
            if (*pointed_records[j].time <= *v) break;
            pointed_records[j + 1].time = pointed_records[j].time;
            pointed_records[j + 1].transaction_id = pointed_records[j].transaction_id;
        }
        pointed_records[j + 1].time = v;
        pointed_records[j + 1].transaction_id = transaction_id;
    }

    /* Print out results */
    for (i=0;i<MAX_RECORDS;i++){
        printf("sorted: %f - %s\n", *pointed_records[i].time, pointed_records[i].transaction_id);
    }

    /* Get the 95th percent */
    percentile_element = (int)floor(((float)MAX_RECORDS) * percentile);
    printf("%f percentile: element: %i, time: %f, reference: %s\n", 
            percentile,
            percentile_element, 
            *pointed_records[percentile_element].time, 
            pointed_records[percentile_element].transaction_id);
    percentile_record->time = *pointed_records[percentile_element].time;
    strcpy(percentile_record->transaction_id,pointed_records[percentile_element].transaction_id);
    /* Finally, we can unlock */
    pthread_mutex_unlock(&lock);
    return RECORD_OK;
}   

void* read_lines_from_stdin(){
    char line[STDIN_LINES_SIZE];
    char *p_line = line;
    char *p_remainding;
    status_record record;

    while(fgets(line, sizeof line, stdin) != NULL) {
        /* Split by new line and then coverter to float */
        record.time = strtof(strtok_r(p_line, " ", &p_remainding), (char **) NULL);
        /* Strip newline character */
        p_remainding[strlen(p_remainding) - 1] = '\0';
        strcpy(record.transaction_id, p_remainding);
        /* Push it onto the stack */
        pump_record(record);
    }
}

int listen_for_queries(){

    int listenfd = 0;
    int connfd = 0;
    int socket_reuse = 1;
    struct sockaddr_in serv_addr;
    char send_buf[128];
    status_record percentile_record;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(send_buf, 0, sizeof(send_buf));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(LISTEN_PORT);

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &socket_reuse, sizeof(int));

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 1);

    while(1){
        int record_ret;
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        record_ret = get_percentile(.95, &percentile_record);
        /* Work out what to send */
        switch(record_ret){
            case RECORD_OK: 
                snprintf(send_buf, sizeof(send_buf), ".95 percentile - time: %f, transaction_id: %s\n", 
                        percentile_record.time, percentile_record.transaction_id);
                break;
            case ERR_RING_NOT_FULL:
                snprintf(send_buf, sizeof(send_buf), "ERR: ring buffer not full, try again later\n");
                break;
            default:
                snprintf(send_buf, sizeof(send_buf), "ERR: unknown error\n");
        }

        /* write it out */
        write(connfd, send_buf, sizeof(send_buf));
        close(connfd);

    }



}

int main(void){
    /* Setup the ring buffer */
    ring.current_index = 0;
    ring.has_overlapped = 0;
    /* And the mutex */
    pthread_mutex_init(&lock, NULL);
    /* Keep reading stdin on another thread */
    pthread_create(&thread_stdin, NULL, &read_lines_from_stdin, NULL);
    listen_for_queries();
}

