/*
 * OCEL 2.0 JSON Importer/Exporter
 *
 * This program implements importers and exporters for the Object-Centric Event Log (OCEL) 2.0
 * JSON standard in process mining.
 *
 * It is written in ANSI C without any dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Define maximum sizes */
#define MAX_ATTRIBUTES       100
#define MAX_RELATIONSHIPS    100
#define MAX_EVENTS           1000
#define MAX_EVENT_TYPES      100
#define MAX_OBJECTS          1000
#define MAX_OBJECT_TYPES     100
#define BUFFER_SIZE          65536

/* Data structures */

typedef struct {
    char name[256];
    char type[64];   /* For EventType and ObjectType attributes */
} TypeAttribute;

typedef struct {
    char name[256];
    char value[256];
    char time[64];   /* ISO format */
} Attribute;

typedef struct {
    char objectId[256];
    char qualifier[256];
} Relationship;

typedef struct {
    char id[256];
    char type[256];
    char time[64];
    Attribute attributes[MAX_ATTRIBUTES];
    int attribute_count;
    Relationship relationships[MAX_RELATIONSHIPS];
    int relationship_count;
} Event;

typedef struct {
    char name[256];
    TypeAttribute attributes[MAX_ATTRIBUTES];
    int attribute_count;
} EventType;

typedef struct {
    char id[256];
    char type[256];
    Attribute attributes[MAX_ATTRIBUTES];
    int attribute_count;
    Relationship relationships[MAX_RELATIONSHIPS];
    int relationship_count;
} Object;

typedef struct {
    char name[256];
    TypeAttribute attributes[MAX_ATTRIBUTES];
    int attribute_count;
} ObjectType;

/* Global arrays */
Event events[MAX_EVENTS];
int event_count = 0;

EventType eventTypes[MAX_EVENT_TYPES];
int eventType_count = 0;

Object objects[MAX_OBJECTS];
int object_count = 0;

ObjectType objectTypes[MAX_OBJECT_TYPES];
int objectType_count = 0;

/* Function prototypes */
void read_ocel(const char* filename);
void write_ocel(const char* filename);

/* Simple JSON parsing functions */
char* read_file(const char* filename);
char* skip_whitespace(char* ptr);
char* parse_string(char* ptr, char* buffer);
char* parse_array(char* ptr, char** endptr);
char* parse_object(char* ptr, char** endptr);
char* parse_value(char* ptr, char* buffer);

/* Parsing functions for specific structures */
char* parse_event_types(char* ptr);
char* parse_object_types(char* ptr);
char* parse_events(char* ptr);
char* parse_objects(char* ptr);

/* Main function */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <input.json> <output.json>\n", argv[0]);
        return 1;
    }

    read_ocel(argv[1]);
    write_ocel(argv[2]);

    return 0;
}

/* Function implementations */

void read_ocel(const char* filename) {
    char* json = read_file(filename);
    if (!json) {
        fprintf(stderr, "Failed to read file %s\n", filename);
        exit(1);
    }

    char* ptr = json;
    ptr = skip_whitespace(ptr);
    if (*ptr != '{') {
        fprintf(stderr, "Expected '{' at the beginning of JSON\n");
        free(json);
        exit(1);
    }
    ptr++;

    while (*ptr) {
        ptr = skip_whitespace(ptr);
        if (*ptr == '}') {
            break;
        }
        char key[256];
        ptr = parse_string(ptr, key);
        ptr = skip_whitespace(ptr);
        if (*ptr != ':') {
            fprintf(stderr, "Expected ':' after key %s\n", key);
            free(json);
            exit(1);
        }
        ptr++;

        ptr = skip_whitespace(ptr);
        if (strcmp(key, "eventTypes") == 0) {
            ptr = parse_event_types(ptr);
        } else if (strcmp(key, "objectTypes") == 0) {
            ptr = parse_object_types(ptr);
        } else if (strcmp(key, "events") == 0) {
            ptr = parse_events(ptr);
        } else if (strcmp(key, "objects") == 0) {
            ptr = parse_objects(ptr);
        } else {
            fprintf(stderr, "Unknown key: %s\n", key);
            free(json);
            exit(1);
        }

        ptr = skip_whitespace(ptr);
        if (*ptr == ',') {
            ptr++;
            continue;
        } else if (*ptr == '}') {
            break;
        } else {
            fprintf(stderr, "Expected ',' or '}' in JSON\n");
            free(json);
            exit(1);
        }
    }

    free(json);
}

void write_ocel(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        exit(1);
    }

    fprintf(file, "{\n");

    /* Write objectTypes */
    fprintf(file, "  \"objectTypes\": [\n");
    for (int i = 0; i < objectType_count; i++) {
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", objectTypes[i].name);
        fprintf(file, "      \"attributes\": [\n");
        for (int j = 0; j < objectTypes[i].attribute_count; j++) {
            fprintf(file, "        {\n");
            fprintf(file, "          \"name\": \"%s\",\n", objectTypes[i].attributes[j].name);
            fprintf(file, "          \"type\": \"%s\"\n", objectTypes[i].attributes[j].type);
            fprintf(file, "        }");
            if (j < objectTypes[i].attribute_count - 1) {
                fprintf(file, ",");
            }
            fprintf(file, "\n");
        }
        fprintf(file, "      ]\n");
        fprintf(file, "    }");
        if (i < objectType_count - 1) {
            fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fprintf(file, "  ],\n");

    /* Write eventTypes */
    fprintf(file, "  \"eventTypes\": [\n");
    for (int i = 0; i < eventType_count; i++) {
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", eventTypes[i].name);
        fprintf(file, "      \"attributes\": [\n");
        for (int j = 0; j < eventTypes[i].attribute_count; j++) {
            fprintf(file, "        {\n");
            fprintf(file, "          \"name\": \"%s\",\n", eventTypes[i].attributes[j].name);
            fprintf(file, "          \"type\": \"%s\"\n", eventTypes[i].attributes[j].type);
            fprintf(file, "        }");
            if (j < eventTypes[i].attribute_count - 1) {
                fprintf(file, ",");
            }
            fprintf(file, "\n");
        }
        fprintf(file, "      ]\n");
        fprintf(file, "    }");
        if (i < eventType_count - 1) {
            fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fprintf(file, "  ],\n");

    /* Write objects */
    fprintf(file, "  \"objects\": [\n");
    for (int i = 0; i < object_count; i++) {
        fprintf(file, "    {\n");
        fprintf(file, "      \"id\": \"%s\",\n", objects[i].id);
        fprintf(file, "      \"type\": \"%s\"", objects[i].type);
        if (objects[i].attribute_count > 0) {
            fprintf(file, ",\n      \"attributes\": [\n");
            for (int j = 0; j < objects[i].attribute_count; j++) {
                fprintf(file, "        {\n");
                fprintf(file, "          \"name\": \"%s\",\n", objects[i].attributes[j].name);
                fprintf(file, "          \"time\": \"%s\",\n", objects[i].attributes[j].time);
                fprintf(file, "          \"value\": \"%s\"\n", objects[i].attributes[j].value);
                fprintf(file, "        }");
                if (j < objects[i].attribute_count - 1) {
                    fprintf(file, ",");
                }
                fprintf(file, "\n");
            }
            fprintf(file, "      ]");
        }
        if (objects[i].relationship_count > 0) {
            if (objects[i].attribute_count > 0) {
                fprintf(file, ",");
            }
            fprintf(file, "\n      \"relationships\": [\n");
            for (int j = 0; j < objects[i].relationship_count; j++) {
                fprintf(file, "        {\n");
                fprintf(file, "          \"objectId\": \"%s\",\n", objects[i].relationships[j].objectId);
                fprintf(file, "          \"qualifier\": \"%s\"\n", objects[i].relationships[j].qualifier);
                fprintf(file, "        }");
                if (j < objects[i].relationship_count - 1) {
                    fprintf(file, ",");
                }
                fprintf(file, "\n");
            }
            fprintf(file, "      ]");
        }
        fprintf(file, "\n    }");
        if (i < object_count - 1) {
            fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fprintf(file, "  ],\n");

    /* Write events */
    fprintf(file, "  \"events\": [\n");
    for (int i = 0; i < event_count; i++) {
        fprintf(file, "    {\n");
        fprintf(file, "      \"id\": \"%s\",\n", events[i].id);
        fprintf(file, "      \"type\": \"%s\",\n", events[i].type);
        fprintf(file, "      \"time\": \"%s\"", events[i].time);
        if (events[i].attribute_count > 0) {
            fprintf(file, ",\n      \"attributes\": [\n");
            for (int j = 0; j < events[i].attribute_count; j++) {
                fprintf(file, "        {\n");
                fprintf(file, "          \"name\": \"%s\",\n", events[i].attributes[j].name);
                fprintf(file, "          \"value\": \"%s\"\n", events[i].attributes[j].value);
                fprintf(file, "        }");
                if (j < events[i].attribute_count - 1) {
                    fprintf(file, ",");
                }
                fprintf(file, "\n");
            }
            fprintf(file, "      ]");
        }
        if (events[i].relationship_count > 0) {
            if (events[i].attribute_count > 0) {
                fprintf(file, ",");
            }
            fprintf(file, "\n      \"relationships\": [\n");
            for (int j = 0; j < events[i].relationship_count; j++) {
                fprintf(file, "        {\n");
                fprintf(file, "          \"objectId\": \"%s\",\n", events[i].relationships[j].objectId);
                fprintf(file, "          \"qualifier\": \"%s\"\n", events[i].relationships[j].qualifier);
                fprintf(file, "        }");
                if (j < events[i].relationship_count - 1) {
                    fprintf(file, ",");
                }
                fprintf(file, "\n");
            }
            fprintf(file, "      ]");
        }
        fprintf(file, "\n    }");
        if (i < event_count - 1) {
            fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fprintf(file, "  ]\n");

    fprintf(file, "}\n");

    fclose(file);
}

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    char* buffer;
    long length;

    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, length, file) != (size_t)length) {
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';  /* Null-terminate the string */
    fclose(file);
    return buffer;
}

char* skip_whitespace(char* ptr) {
    while (*ptr && isspace((unsigned char)*ptr)) {
        ptr++;
    }
    return ptr;
}

char* parse_string(char* ptr, char* buffer) {
    ptr = skip_whitespace(ptr);
    if (*ptr != '\"') {
        fprintf(stderr, "Expected '\"' at beginning of string\n");
        exit(1);
    }
    ptr++;
    int i = 0;
    while (*ptr && *ptr != '\"') {
        if (*ptr == '\\') {
            ptr++;
            if (*ptr == '\"' || *ptr == '\\' || *ptr == '/') {
                buffer[i++] = *ptr++;
            } else if (*ptr == 'b') {
                buffer[i++] = '\b';
                ptr++;
            } else if (*ptr == 'f') {
                buffer[i++] = '\f';
                ptr++;
            } else if (*ptr == 'n') {
                buffer[i++] = '\n';
                ptr++;
            } else if (*ptr == 'r') {
                buffer[i++] = '\r';
                ptr++;
            } else if (*ptr == 't') {
                buffer[i++] = '\t';
                ptr++;
            } else {
                fprintf(stderr, "Unknown escape sequence\n");
                exit(1);
            }
        } else {
            buffer[i++] = *ptr++;
        }
    }
    buffer[i] = '\0';
    if (*ptr != '\"') {
        fprintf(stderr, "Expected '\"' at end of string\n");
        exit(1);
    }
    ptr++;
    return ptr;
}

char* parse_value(char* ptr, char* buffer) {
    ptr = skip_whitespace(ptr);
    if (*ptr == '\"') {
        return parse_string(ptr, buffer);
    } else {
        int i = 0;
        while (*ptr && *ptr != ',' && *ptr != '}' && *ptr != ']' && !isspace((unsigned char)*ptr)) {
            buffer[i++] = *ptr++;
        }
        buffer[i] = '\0';
        return ptr;
    }
}

char* parse_array(char* ptr, char** endptr) {
    int depth = 1;
    ptr = skip_whitespace(ptr);
    if (*ptr != '[') {
        fprintf(stderr, "Expected '[' at beginning of array\n");
        exit(1);
    }
    ptr++;
    while (*ptr) {
        if (*ptr == '[') {
            depth++;
        } else if (*ptr == ']') {
            depth--;
            if (depth == 0) {
                ptr++;
                break;
            }
        }
        ptr++;
    }
    *endptr = ptr;
    return ptr;
}

char* parse_object(char* ptr, char** endptr) {
    int depth = 1;
    ptr = skip_whitespace(ptr);
    if (*ptr != '{') {
        fprintf(stderr, "Expected '{' at beginning of object\n");
        exit(1);
    }
    ptr++;
    while (*ptr) {
        if (*ptr == '{') {
            depth++;
        } else if (*ptr == '}') {
            depth--;
            if (depth == 0) {
                ptr++;
                break;
            }
        }
        ptr++;
    }
    *endptr = ptr;
    return ptr;
}

/* Parsing eventTypes */
char* parse_event_types(char* ptr) {
    ptr = skip_whitespace(ptr);
    if (*ptr != '[') {
        fprintf(stderr, "Expected '[' after \"eventTypes\"\n");
        exit(1);
    }
    ptr++;
    while (*ptr) {
        ptr = skip_whitespace(ptr);
        if (*ptr == ']') {
            ptr++;
            break;
        }
        if (*ptr != '{') {
            fprintf(stderr, "Expected '{' at beginning of eventType object\n");
            exit(1);
        }
        ptr++;
        EventType et;
        et.attribute_count = 0;
        while (*ptr) {
            ptr = skip_whitespace(ptr);
            if (*ptr == '}') {
                ptr++;
                break;
            }
            char key[256];
            ptr = parse_string(ptr, key);
            ptr = skip_whitespace(ptr);
            if (*ptr != ':') {
                fprintf(stderr, "Expected ':' after key %s in eventType\n", key);
                exit(1);
            }
            ptr++;
            ptr = skip_whitespace(ptr);
            if (strcmp(key, "name") == 0) {
                char value[256];
                ptr = parse_string(ptr, value);
                strcpy(et.name, value);
            } else if (strcmp(key, "attributes") == 0) {
                if (*ptr != '[') {
                    fprintf(stderr, "Expected '[' after \"attributes\" in eventType\n");
                    exit(1);
                }
                ptr++;
                while (*ptr) {
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ']') {
                        ptr++;
                        break;
                    }
                    if (*ptr != '{') {
                        fprintf(stderr, "Expected '{' at beginning of attribute object in eventType\n");
                        exit(1);
                    }
                    ptr++;
                    TypeAttribute ta;
                    while (*ptr) {
                        ptr = skip_whitespace(ptr);
                        if (*ptr == '}') {
                            ptr++;
                            break;
                        }
                        char akey[256];
                        ptr = parse_string(ptr, akey);
                        ptr = skip_whitespace(ptr);
                        if (*ptr != ':') {
                            fprintf(stderr, "Expected ':' after key %s in attribute of eventType\n", akey);
                            exit(1);
                        }
                        ptr++;
                        ptr = skip_whitespace(ptr);
                        char avalue[256];
                        ptr = parse_string(ptr, avalue);
                        if (strcmp(akey, "name") == 0) {
                            strcpy(ta.name, avalue);
                        } else if (strcmp(akey, "type") == 0) {
                            strcpy(ta.type, avalue);
                        }
                        ptr = skip_whitespace(ptr);
                        if (*ptr == ',') {
                            ptr++;
                            continue;
                        }
                    }
                    et.attributes[et.attribute_count++] = ta;
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ',') {
                        ptr++;
                        continue;
                    }
                }
            }
            ptr = skip_whitespace(ptr);
            if (*ptr == ',') {
                ptr++;
                continue;
            }
        }
        eventTypes[eventType_count++] = et;
        ptr = skip_whitespace(ptr);
        if (*ptr == ',') {
            ptr++;
            continue;
        }
    }
    return ptr;
}

/* Parsing objectTypes */
char* parse_object_types(char* ptr) {
    ptr = skip_whitespace(ptr);
    if (*ptr != '[') {
        fprintf(stderr, "Expected '[' after \"objectTypes\"\n");
        exit(1);
    }
    ptr++;
    while (*ptr) {
        ptr = skip_whitespace(ptr);
        if (*ptr == ']') {
            ptr++;
            break;
        }
        if (*ptr != '{') {
            fprintf(stderr, "Expected '{' at beginning of objectType object\n");
            exit(1);
        }
        ptr++;
        ObjectType ot;
        ot.attribute_count = 0;
        while (*ptr) {
            ptr = skip_whitespace(ptr);
            if (*ptr == '}') {
                ptr++;
                break;
            }
            char key[256];
            ptr = parse_string(ptr, key);
            ptr = skip_whitespace(ptr);
            if (*ptr != ':') {
                fprintf(stderr, "Expected ':' after key %s in objectType\n", key);
                exit(1);
            }
            ptr++;
            ptr = skip_whitespace(ptr);
            if (strcmp(key, "name") == 0) {
                char value[256];
                ptr = parse_string(ptr, value);
                strcpy(ot.name, value);
            } else if (strcmp(key, "attributes") == 0) {
                if (*ptr != '[') {
                    fprintf(stderr, "Expected '[' after \"attributes\" in objectType\n");
                    exit(1);
                }
                ptr++;
                while (*ptr) {
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ']') {
                        ptr++;
                        break;
                    }
                    if (*ptr != '{') {
                        fprintf(stderr, "Expected '{' at beginning of attribute object in objectType\n");
                        exit(1);
                    }
                    ptr++;
                    TypeAttribute ta;
                    while (*ptr) {
                        ptr = skip_whitespace(ptr);
                        if (*ptr == '}') {
                            ptr++;
                            break;
                        }
                        char akey[256];
                        ptr = parse_string(ptr, akey);
                        ptr = skip_whitespace(ptr);
                        if (*ptr != ':') {
                            fprintf(stderr, "Expected ':' after key %s in attribute of objectType\n", akey);
                            exit(1);
                        }
                        ptr++;
                        ptr = skip_whitespace(ptr);
                        char avalue[256];
                        ptr = parse_string(ptr, avalue);
                        if (strcmp(akey, "name") == 0) {
                            strcpy(ta.name, avalue);
                        } else if (strcmp(akey, "type") == 0) {
                            strcpy(ta.type, avalue);
                        }
                        ptr = skip_whitespace(ptr);
                        if (*ptr == ',') {
                            ptr++;
                            continue;
                        }
                    }
                    ot.attributes[ot.attribute_count++] = ta;
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ',') {
                        ptr++;
                        continue;
                    }
                }
            }
            ptr = skip_whitespace(ptr);
            if (*ptr == ',') {
                ptr++;
                continue;
            }
        }
        objectTypes[objectType_count++] = ot;
        ptr = skip_whitespace(ptr);
        if (*ptr == ',') {
            ptr++;
            continue;
        }
    }
    return ptr;
}

/* Parsing events */
char* parse_events(char* ptr) {
    ptr = skip_whitespace(ptr);
    if (*ptr != '[') {
        fprintf(stderr, "Expected '[' after \"events\"\n");
        exit(1);
    }
    ptr++;
    while (*ptr) {
        ptr = skip_whitespace(ptr);
        if (*ptr == ']') {
            ptr++;
            break;
        }
        if (*ptr != '{') {
            fprintf(stderr, "Expected '{' at beginning of event object\n");
            exit(1);
        }
        ptr++;
        Event e;
        e.attribute_count = 0;
        e.relationship_count = 0;
        while (*ptr) {
            ptr = skip_whitespace(ptr);
            if (*ptr == '}') {
                ptr++;
                break;
            }
            char key[256];
            ptr = parse_string(ptr, key);
            ptr = skip_whitespace(ptr);
            if (*ptr != ':') {
                fprintf(stderr, "Expected ':' after key %s in event\n", key);
                exit(1);
            }
            ptr++;
            ptr = skip_whitespace(ptr);
            if (strcmp(key, "id") == 0) {
                char value[256];
                ptr = parse_string(ptr, value);
                strcpy(e.id, value);
            } else if (strcmp(key, "type") == 0) {
                char value[256];
                ptr = parse_string(ptr, value);
                strcpy(e.type, value);
            } else if (strcmp(key, "time") == 0) {
                char value[64];
                ptr = parse_string(ptr, value);
                strcpy(e.time, value);
            } else if (strcmp(key, "attributes") == 0) {
                if (*ptr != '[') {
                    fprintf(stderr, "Expected '[' after \"attributes\" in event\n");
                    exit(1);
                }
                ptr++;
                while (*ptr) {
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ']') {
                        ptr++;
                        break;
                    }
                    if (*ptr != '{') {
                        fprintf(stderr, "Expected '{' at beginning of attribute object in event\n");
                        exit(1);
                    }
                    ptr++;
                    Attribute a;
                    while (*ptr) {
                        ptr = skip_whitespace(ptr);
                        if (*ptr == '}') {
                            ptr++;
                            break;
                        }
                        char akey[256];
                        ptr = parse_string(ptr, akey);
                        ptr = skip_whitespace(ptr);
                        if (*ptr != ':') {
                            fprintf(stderr, "Expected ':' after key %s in attribute of event\n", akey);
                            exit(1);
                        }
                        ptr++;
                        ptr = skip_whitespace(ptr);
                        char avalue[256];
                        ptr = parse_string(ptr, avalue);
                        if (strcmp(akey, "name") == 0) {
                            strcpy(a.name, avalue);
                        } else if (strcmp(akey, "value") == 0) {
                            strcpy(a.value, avalue);
                        }
                        ptr = skip_whitespace(ptr);
                        if (*ptr == ',') {
                            ptr++;
                            continue;
                        }
                    }
                    e.attributes[e.attribute_count++] = a;
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ',') {
                        ptr++;
                        continue;
                    }
                }
            } else if (strcmp(key, "relationships") == 0) {
                if (*ptr != '[') {
                    fprintf(stderr, "Expected '[' after \"relationships\" in event\n");
                    exit(1);
                }
                ptr++;
                while (*ptr) {
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ']') {
                        ptr++;
                        break;
                    }
                    if (*ptr != '{') {
                        fprintf(stderr, "Expected '{' at beginning of relationship object in event\n");
                        exit(1);
                    }
                    ptr++;
                    Relationship r;
                    while (*ptr) {
                        ptr = skip_whitespace(ptr);
                        if (*ptr == '}') {
                            ptr++;
                            break;
                        }
                        char rkey[256];
                        ptr = parse_string(ptr, rkey);
                        ptr = skip_whitespace(ptr);
                        if (*ptr != ':') {
                            fprintf(stderr, "Expected ':' after key %s in relationship of event\n", rkey);
                            exit(1);
                        }
                        ptr++;
                        ptr = skip_whitespace(ptr);
                        char rvalue[256];
                        ptr = parse_string(ptr, rvalue);
                        if (strcmp(rkey, "objectId") == 0) {
                            strcpy(r.objectId, rvalue);
                        } else if (strcmp(rkey, "qualifier") == 0) {
                            strcpy(r.qualifier, rvalue);
                        }
                        ptr = skip_whitespace(ptr);
                        if (*ptr == ',') {
                            ptr++;
                            continue;
                        }
                    }
                    e.relationships[e.relationship_count++] = r;
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ',') {
                        ptr++;
                        continue;
                    }
                }
            }

            ptr = skip_whitespace(ptr);
            if (*ptr == ',') {
                ptr++;
                continue;
            }
        }
        events[event_count++] = e;
        ptr = skip_whitespace(ptr);
        if (*ptr == ',') {
            ptr++;
            continue;
        }
    }
    return ptr;
}

/* Parsing objects */
char* parse_objects(char* ptr) {
    ptr = skip_whitespace(ptr);
    if (*ptr != '[') {
        fprintf(stderr, "Expected '[' after \"objects\"\n");
        exit(1);
    }
    ptr++;
    while (*ptr) {
        ptr = skip_whitespace(ptr);
        if (*ptr == ']') {
            ptr++;
            break;
        }
        if (*ptr != '{') {
            fprintf(stderr, "Expected '{' at beginning of object\n");
            exit(1);
        }
        ptr++;
        Object o;
        o.attribute_count = 0;
        o.relationship_count = 0;
        while (*ptr) {
            ptr = skip_whitespace(ptr);
            if (*ptr == '}') {
                ptr++;
                break;
            }
            char key[256];
            ptr = parse_string(ptr, key);
            ptr = skip_whitespace(ptr);
            if (*ptr != ':') {
                fprintf(stderr, "Expected ':' after key %s in object\n", key);
                exit(1);
            }
            ptr++;
            ptr = skip_whitespace(ptr);
            if (strcmp(key, "id") == 0) {
                char value[256];
                ptr = parse_string(ptr, value);
                strcpy(o.id, value);
            } else if (strcmp(key, "type") == 0) {
                char value[256];
                ptr = parse_string(ptr, value);
                strcpy(o.type, value);
            } else if (strcmp(key, "attributes") == 0) {
                if (*ptr != '[') {
                    fprintf(stderr, "Expected '[' after \"attributes\" in object\n");
                    exit(1);
                }
                ptr++;
                while (*ptr) {
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ']') {
                        ptr++;
                        break;
                    }
                    if (*ptr != '{') {
                        fprintf(stderr, "Expected '{' at beginning of attribute object in object\n");
                        exit(1);
                    }
                    ptr++;
                    Attribute a;
                    while (*ptr) {
                        ptr = skip_whitespace(ptr);
                        if (*ptr == '}') {
                            ptr++;
                            break;
                        }
                        char akey[256];
                        ptr = parse_string(ptr, akey);
                        ptr = skip_whitespace(ptr);
                        if (*ptr != ':') {
                            fprintf(stderr, "Expected ':' after key %s in attribute of object\n", akey);
                            exit(1);
                        }
                        ptr++;
                        ptr = skip_whitespace(ptr);
                        char avalue[256];
                        ptr = parse_string(ptr, avalue);
                        if (strcmp(akey, "name") == 0) {
                            strcpy(a.name, avalue);
                        } else if (strcmp(akey, "time") == 0) {
                            strcpy(a.time, avalue);
                        } else if (strcmp(akey, "value") == 0) {
                            strcpy(a.value, avalue);
                        }
                        ptr = skip_whitespace(ptr);
                        if (*ptr == ',') {
                            ptr++;
                            continue;
                        }
                    }
                    o.attributes[o.attribute_count++] = a;
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ',') {
                        ptr++;
                        continue;
                    }
                }
            } else if (strcmp(key, "relationships") == 0) {
                if (*ptr != '[') {
                    fprintf(stderr, "Expected '[' after \"relationships\" in object\n");
                    exit(1);
                }
                ptr++;
                while (*ptr) {
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ']') {
                        ptr++;
                        break;
                    }
                    if (*ptr != '{') {
                        fprintf(stderr, "Expected '{' at beginning of relationship object in object\n");
                        exit(1);
                    }
                    ptr++;
                    Relationship r;
                    while (*ptr) {
                        ptr = skip_whitespace(ptr);
                        if (*ptr == '}') {
                            ptr++;
                            break;
                        }
                        char rkey[256];
                        ptr = parse_string(ptr, rkey);
                        ptr = skip_whitespace(ptr);
                        if (*ptr != ':') {
                            fprintf(stderr, "Expected ':' after key %s in relationship of object\n", rkey);
                            exit(1);
                        }
                        ptr++;
                        ptr = skip_whitespace(ptr);
                        char rvalue[256];
                        ptr = parse_string(ptr, rvalue);
                        if (strcmp(rkey, "objectId") == 0) {
                            strcpy(r.objectId, rvalue);
                        } else if (strcmp(rkey, "qualifier") == 0) {
                            strcpy(r.qualifier, rvalue);
                        }
                        ptr = skip_whitespace(ptr);
                        if (*ptr == ',') {
                            ptr++;
                            continue;
                        }
                    }
                    o.relationships[o.relationship_count++] = r;
                    ptr = skip_whitespace(ptr);
                    if (*ptr == ',') {
                        ptr++;
                        continue;
                    }
                }
            }

            ptr = skip_whitespace(ptr);
            if (*ptr == ',') {
                ptr++;
                continue;
            }
        }
        objects[object_count++] = o;
        ptr = skip_whitespace(ptr);
        if (*ptr == ',') {
            ptr++;
            continue;
        }
    }
    return ptr;
}