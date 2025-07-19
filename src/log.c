#include "log.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#define LOG_FILE "logs/server.log"

void write_log(const char *level , const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) return;

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = '\0'; // Remove newline

    fprintf(log_file, "[%s] [%s] %s\n",level, timestamp, message);
    fclose(log_file);  // close the file
}   
