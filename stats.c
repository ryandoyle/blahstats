#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    float time;
    char *transaction_id;
} status_record;

int pump_record(float the_time, char *the_transaction_id){
    printf("%f - %s", the_time, the_transaction_id);
}

int read_lines_from_stdin(){
    char line[64];
    char *the_time;
    char *the_transaction_id;
    char *p_line= line;
    char *p_remainding;

    status_record record;

    while(fgets(line, sizeof line, stdin) != NULL) {
        the_time = strtok_r(p_line, " ", &p_remainding);
        // Strip newline character
        p_remainding[strlen(p_remainding) - 1] = '\0';
        the_transaction_id = p_remainding;


        //pump_record(strtof(the_time),the_transaction_id);
        printf("%f - %s", strtof(the_time, (char **) NULL), the_transaction_id);
    }
}

int main(void){
    read_lines_from_stdin();
}

