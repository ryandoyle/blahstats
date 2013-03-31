#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_RECORDS 50
#define STDIN_LINES_SIZE 64
#define TRANSACTION_ID_SIZE 20

FILE * read_in;

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

int pump_record(status_record this_record){
    printf("pushing to ring buffer: %f - %s\n", this_record.time, this_record.transaction_id);

    /* Pop it onto the ring buffer */
    ring.record[ring.current_index] = this_record;
    /* Increment our index */
    ring.current_index++;
    /* Have we overflowed? */
    if (ring.current_index >= MAX_RECORDS){
        ring.current_index = 0;
        ring.has_overlapped = 1;
    }
}

void print_buffer() {
    int i;
    for (i=0; i < MAX_RECORDS; i++ ){
        printf("buffer @ %d: %f - %s\n", i, ring.record[i].time, ring.record[i].transaction_id);
    }
}

int get_95th_percentile(){
    int i;
    int ninety_fifth_element;

    if(!ring.has_overlapped){
        printf("Ring buffer not full enough: Need %i, have %i\n", MAX_RECORDS, ring.current_index);
        return -1;
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
    ninety_fifth_element = (int)floor(((float)MAX_RECORDS) * .95);
    printf("95th percentile: element: %i, time: %f, reference: %s\n", 
            ninety_fifth_element, 
            *pointed_records[ninety_fifth_element].time, 
            pointed_records[ninety_fifth_element].transaction_id);

}   

int read_lines_from_stdin(){
    char line[STDIN_LINES_SIZE];
    char *p_line = line;
    char *p_remainding;
    status_record record;

    /* file for testing */
    read_in = fopen("data.txt", "r");
    /* end file for testing */

    while(fgets(line, sizeof line, read_in) != NULL) {
        /* Split by new line and then coverter to float */
        record.time = strtof(strtok_r(p_line, " ", &p_remainding), (char **) NULL);
        /* Strip newline character */
        p_remainding[strlen(p_remainding) - 1] = '\0';
        strcpy(record.transaction_id, p_remainding);
        /* Push it onto the stack */
        pump_record(record);
    }
}

int main(void){
    ring.current_index = 0;
    ring.has_overlapped = 0;
    read_lines_from_stdin();
    get_95th_percentile();
}

