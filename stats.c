#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECORDS 10
#define STDIN_LINES_SIZE 64
#define TRANSACTION_ID_SIZE 20

FILE * read_in;

typedef struct {
    float time;
    char transaction_id[TRANSACTION_ID_SIZE];
} status_record;

typedef struct {
    status_record record[MAX_RECORDS];
    int current_index;
    int has_overlapped;
} ring_buffer;

/* cast as ring_buffer, calloc fills memory with 0, which we want
   1 is the number of members per per size
 */
ring_buffer ring;

int pump_record(status_record this_record){
    printf("%f - %s\n", this_record.time, this_record.transaction_id);

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

void sort_data(){
    
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
    print_buffer();
}

