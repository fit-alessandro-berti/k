/*
 * XES Importer/Exporter
 * Only stores the activity of each event (concept:name)
 * Implemented in ANSI C without external dependencies
 *
 * The code implements an XES importer/exporter in ANSI C without external dependencies. It only focuses on storing the activity of each event (`concept:name`) and ignores all other attributes.
 *
 * **Main Components:**

- **Data Structures:**
  - `Case`: Represents a single case (trace) containing a list of activities.
  - `Log`: Represents the entire log containing multiple cases.

- **Functions:**
  - `create_log()`: Allocates and initializes a new log.
  - `free_log(Log *log)`: Frees all memory associated with the log.
  - `add_activity_to_case(Case *c, const char *activity)`: Adds an activity to a case.
  - `add_case(Log *log)`: Adds a new case to the log.
  - `parse_xes(FILE *fp, Log *log)`: Parses an XES file and fills the log data structure.
  - `export_xes(FILE *fp, Log *log)`: Exports the log data structure back into an XES file.

- **Parsing Logic:**
  - Reads the input XES file line by line.
  - Maintains state variables `in_trace` and `in_event` to keep track of where we are in the XML structure.
  - When an `<event>` tag is encountered inside a `<trace>`, it starts collecting `concept:name` values.
  - It searches for lines containing `key="concept:name"` and extracts the `value`.

- **Export Logic:**
  - Writes the XML header and the `<log>` tag with version information.
  - Iterates over each case and each activity within the case.
  - Writes each event back into the XES format with only the `concept:name` attribute.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACTIVITY_LENGTH 256
#define MAX_ACTIVITIES_PER_CASE 1024
#define INITIAL_CASE_CAPACITY 128

/* Data structure to hold activities for each case */
typedef struct {
    char **activities;
    int activity_count;
    int activity_capacity;
} Case;

typedef struct {
    Case *cases;
    int case_count;
    int case_capacity;
} Log;

/* Function prototypes */
Log *create_log();
void free_log(Log *log);
void add_activity_to_case(Case *c, const char *activity);
void add_case(Log *log);
void parse_xes(FILE *fp, Log *log);
void export_xes(FILE *fp, Log *log);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input.xes output.xes\n", argv[0]);
        return 1;
    }

    /* Open input file */
    FILE *fp_in = fopen(argv[1], "r");
    if (!fp_in) {
        perror("Failed to open input file");
        return 1;
    }

    /* Create log structure */
    Log *log = create_log();

    /* Parse XES file */
    parse_xes(fp_in, log);
    fclose(fp_in);

    /* Open output file */
    FILE *fp_out = fopen(argv[2], "w");
    if (!fp_out) {
        perror("Failed to open output file");
        free_log(log);
        return 1;
    }

    /* Export log to XES file */
    export_xes(fp_out, log);
    fclose(fp_out);

    /* Free log structure */
    free_log(log);

    return 0;
}

/* Create an empty log */
Log *create_log() {
    Log *log = (Log *)malloc(sizeof(Log));
    log->case_count = 0;
    log->case_capacity = INITIAL_CASE_CAPACITY;
    log->cases = (Case *)malloc(sizeof(Case) * log->case_capacity);
    return log;
}

/* Free the log and its contents */
void free_log(Log *log) {
    for (int i = 0; i < log->case_count; i++) {
        Case *c = &log->cases[i];
        for (int j = 0; j < c->activity_count; j++) {
            free(c->activities[j]);
        }
        free(c->activities);
    }
    free(log->cases);
    free(log);
}

/* Add a new activity to a case */
void add_activity_to_case(Case *c, const char *activity) {
    if (c->activity_capacity == 0) {
        c->activity_capacity = MAX_ACTIVITIES_PER_CASE;
        c->activities = (char **)malloc(sizeof(char *) * c->activity_capacity);
    } else if (c->activity_count >= c->activity_capacity) {
        c->activity_capacity *= 2;
        c->activities = (char **)realloc(c->activities, sizeof(char *) * c->activity_capacity);
    }
    c->activities[c->activity_count] = strdup(activity);
    c->activity_count++;
}

/* Add a new case to the log */
void add_case(Log *log) {
    if (log->case_count >= log->case_capacity) {
        log->case_capacity *= 2;
        log->cases = (Case *)realloc(log->cases, sizeof(Case) * log->case_capacity);
    }
    Case *c = &log->cases[log->case_count++];
    c->activities = NULL;
    c->activity_count = 0;
    c->activity_capacity = 0;
}

/* Simple XML parsing functions */

char *trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (*str == ' ' || *str == '\n' || *str == '\t' || *str == '\r') str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while ((end > str) && (*end == ' ' || *end == '\n' || *end == '\t' || *end == '\r')) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return str;
}

/* Check if the line starts with a given prefix */
int starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) return 0;
    }
    return 1;
}

/* Simple XML parser to parse XES and extract concept:name attributes */
void parse_xes(FILE *fp, Log *log) {
    char line[1024];
    int in_trace = 0;
    int in_event = 0;
    int has_concept = 0;
    char activity[MAX_ACTIVITY_LENGTH];

    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim_whitespace(line);

        /* Start of a trace */
        if (starts_with(trimmed, "<trace")) {
            add_case(log);
            in_trace = 1;
            continue;
        }

        /* End of a trace */
        if (starts_with(trimmed, "</trace>")) {
            in_trace = 0;
            continue;
        }

        /* Start of an event */
        if (starts_with(trimmed, "<event")) {
            in_event = 1;
            has_concept = 0;
            continue;
        }

        /* End of an event */
        if (starts_with(trimmed, "</event>")) {
            if (in_event && has_concept) {
                Case *current_case = &log->cases[log->case_count - 1];
                add_activity_to_case(current_case, activity);
            }
            in_event = 0;
            continue;
        }

        /* Inside an event, look for concept:name attributes */
        if (in_event) {
            if (strstr(trimmed, "key=\"concept:name\"")) {
                /* Extract the value */
                char *val_start = strstr(trimmed, "value=");
                if (val_start) {
                    val_start += 6;  // Move past 'value='
                    char quote = *val_start++;
                    char *val_end = strchr(val_start, quote);
                    if (val_end) {
                        size_t len = val_end - val_start;
                        if (len >= MAX_ACTIVITY_LENGTH) len = MAX_ACTIVITY_LENGTH - 1;
                        strncpy(activity, val_start, len);
                        activity[len] = '\0';
                        has_concept = 1;
                    }
                }
            }
        }
    }
}

/* Export the log to XES format */
void export_xes(FILE *fp, Log *log) {
    /* Write XML header */
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<log xes.version=\"1.0\" xes.features=\"\">\n");

    /* For each case */
    for (int i = 0; i < log->case_count; i++) {
        fprintf(fp, "  <trace>\n");
        Case *c = &log->cases[i];
        /* For each event */
        for (int j = 0; j < c->activity_count; j++) {
            fprintf(fp, "    <event>\n");
            fprintf(fp, "      <string key=\"concept:name\" value=\"%s\"/>\n", c->activities[j]);
            fprintf(fp, "    </event>\n");
        }
        fprintf(fp, "  </trace>\n");
    }

    fprintf(fp, "</log>\n");
}