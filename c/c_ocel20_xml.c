#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 4096
#define MAX_NAME_LENGTH 256
#define MAX_ATTRIBUTES 100
#define MAX_OBJECTS 1000
#define MAX_EVENTS 1000
#define MAX_RELATIONSHIPS 1000

typedef struct {
    char name[MAX_NAME_LENGTH];
    char type[MAX_NAME_LENGTH];
} AttributeDef;

typedef struct {
    char name[MAX_NAME_LENGTH];
    AttributeDef attributes[MAX_ATTRIBUTES];
    int attribute_count;
} ObjectType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    AttributeDef attributes[MAX_ATTRIBUTES];
    int attribute_count;
} EventType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    char type[MAX_NAME_LENGTH];
    char value[MAX_NAME_LENGTH];
    char time[MAX_NAME_LENGTH];
} ObjectAttribute;

typedef struct {
    char object_id[MAX_NAME_LENGTH];
    char qualifier[MAX_NAME_LENGTH];
} Relationship;

typedef struct {
    char id[MAX_NAME_LENGTH];
    char type[MAX_NAME_LENGTH];
    ObjectAttribute attributes[MAX_ATTRIBUTES];
    int attribute_count;
    Relationship relationships[MAX_RELATIONSHIPS];
    int relationship_count;
} Object;

typedef struct {
    char id[MAX_NAME_LENGTH];
    char type[MAX_NAME_LENGTH];
    char time[MAX_NAME_LENGTH];
    ObjectAttribute attributes[MAX_ATTRIBUTES];
    int attribute_count;
    Relationship relationships[MAX_RELATIONSHIPS];
    int relationship_count;
} Event;

/* Global arrays to store parsed data */
ObjectType object_types[MAX_OBJECTS];
int object_type_count = 0;

EventType event_types[MAX_EVENTS];
int event_type_count = 0;

Object objects[MAX_OBJECTS];
int object_count = 0;

Event events[MAX_EVENTS];
int event_count = 0;

/* Function prototypes */
void parse_file(const char *filename);
void write_file(const char *filename);
void parse_log(FILE *file);
void parse_object_types(FILE *file);
void parse_event_types(FILE *file);
void parse_objects(FILE *file);
void parse_events(FILE *file);
void parse_attributes(FILE *file, AttributeDef *attributes, int *attribute_count);
void parse_object_attributes(FILE *file, Object *obj);
void parse_event_attributes(FILE *file, Event *event);
void parse_relationships(FILE *file, Relationship *relationships, int *relationship_count, const char *end_tag);

/* Main function */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input_file.xml output_file.xml\n", argv[0]);
        return 1;
    }

    parse_file(argv[1]);
    write_file(argv[2]);

    return 0;
}

/* Function implementations */

void parse_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening input file.\n");
        exit(1);
    }

    parse_log(file);

    fclose(file);
}

void write_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error opening output file.\n");
        exit(1);
    }

    fprintf(file, "<?xml version='1.0' encoding='UTF-8'?>\n");
    fprintf(file, "<log>\n");

    /* Write object-types */
    fprintf(file, "  <object-types>\n");
    for (int i = 0; i < object_type_count; i++) {
        fprintf(file, "    <object-type name=\"%s\">\n", object_types[i].name);
        fprintf(file, "      <attributes>\n");
        for (int j = 0; j < object_types[i].attribute_count; j++) {
            fprintf(file, "        <attribute name=\"%s\" type=\"%s\"/>\n",
                    object_types[i].attributes[j].name,
                    object_types[i].attributes[j].type);
        }
        fprintf(file, "      </attributes>\n");
        fprintf(file, "    </object-type>\n");
    }
    fprintf(file, "  </object-types>\n");

    /* Write event-types */
    fprintf(file, "  <event-types>\n");
    for (int i = 0; i < event_type_count; i++) {
        fprintf(file, "    <event-type name=\"%s\">\n", event_types[i].name);
        fprintf(file, "      <attributes>\n");
        for (int j = 0; j < event_types[i].attribute_count; j++) {
            fprintf(file, "        <attribute name=\"%s\" type=\"%s\"/>\n",
                    event_types[i].attributes[j].name,
                    event_types[i].attributes[j].type);
        }
        fprintf(file, "      </attributes>\n");
        fprintf(file, "    </event-type>\n");
    }
    fprintf(file, "  </event-types>\n");

    /* Write objects */
    fprintf(file, "  <objects>\n");
    for (int i = 0; i < object_count; i++) {
        fprintf(file, "    <object id=\"%s\" type=\"%s\">\n", objects[i].id, objects[i].type);
        fprintf(file, "      <attributes>\n");
        for (int j = 0; j < objects[i].attribute_count; j++) {
            fprintf(file, "        <attribute name=\"%s\" time=\"%s\">%s</attribute>\n",
                    objects[i].attributes[j].name,
                    objects[i].attributes[j].time,
                    objects[i].attributes[j].value);
        }
        fprintf(file, "      </attributes>\n");
        if (objects[i].relationship_count > 0) {
            fprintf(file, "      <objects>\n");
            for (int j = 0; j < objects[i].relationship_count; j++) {
                fprintf(file, "        <relationship object-id=\"%s\" qualifier=\"%s\"/>\n",
                        objects[i].relationships[j].object_id,
                        objects[i].relationships[j].qualifier);
            }
            fprintf(file, "      </objects>\n");
        }
        fprintf(file, "    </object>\n");
    }
    fprintf(file, "  </objects>\n");

    /* Write events */
    fprintf(file, "  <events>\n");
    for (int i = 0; i < event_count; i++) {
        fprintf(file, "    <event id=\"%s\" type=\"%s\" time=\"%s\">\n",
                events[i].id, events[i].type, events[i].time);
        fprintf(file, "      <attributes>\n");
        for (int j = 0; j < events[i].attribute_count; j++) {
            fprintf(file, "        <attribute name=\"%s\">%s</attribute>\n",
                    events[i].attributes[j].name,
                    events[i].attributes[j].value);
        }
        fprintf(file, "      </attributes>\n");
        if (events[i].relationship_count > 0) {
            fprintf(file, "      <objects>\n");
            for (int j = 0; j < events[i].relationship_count; j++) {
                fprintf(file, "        <relationship object-id=\"%s\" qualifier=\"%s\"/>\n",
                        events[i].relationships[j].object_id,
                        events[i].relationships[j].qualifier);
            }
            fprintf(file, "      </objects>\n");
        }
        fprintf(file, "    </event>\n");
    }
    fprintf(file, "  </events>\n");

    fprintf(file, "</log>\n");

    fclose(file);
}

void parse_log(FILE *file) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "<object-types>")) {
            parse_object_types(file);
        } else if (strstr(line, "<event-types>")) {
            parse_event_types(file);
        } else if (strstr(line, "<objects>")) {
            parse_objects(file);
        } else if (strstr(line, "<events>")) {
            parse_events(file);
        }
    }
}

void parse_object_types(FILE *file) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "</object-types>")) {
            break;
        } else if (strstr(line, "<object-type")) {
            ObjectType ot;
            memset(&ot, 0, sizeof(ObjectType));
            char *ptr = strstr(line, "name=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", ot.name);
            }

            /* Read attributes */
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "</object-type>")) {
                    break;
                } else if (strstr(line, "<attributes>")) {
                    parse_attributes(file, ot.attributes, &ot.attribute_count);
                }
            }
            object_types[object_type_count++] = ot;
        }
    }
}

void parse_event_types(FILE *file) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "</event-types>")) {
            break;
        } else if (strstr(line, "<event-type")) {
            EventType et;
            memset(&et, 0, sizeof(EventType));
            char *ptr = strstr(line, "name=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", et.name);
            }

            /* Read attributes */
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "</event-type>")) {
                    break;
                } else if (strstr(line, "<attributes>")) {
                    parse_attributes(file, et.attributes, &et.attribute_count);
                }
            }
            event_types[event_type_count++] = et;
        }
    }
}

void parse_attributes(FILE *file, AttributeDef *attributes, int *attribute_count) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "</attributes>")) {
            break;
        } else if (strstr(line, "<attribute")) {
            AttributeDef attr;
            memset(&attr, 0, sizeof(AttributeDef));

            char *ptr = strstr(line, "name=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", attr.name);
            }

            ptr = strstr(line, "type=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", attr.type);
            }

            attributes[(*attribute_count)++] = attr;
        }
    }
}

void parse_objects(FILE *file) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "</objects>") && !strstr(line, "<objects>")) {
            break;
        } else if (strstr(line, "<object")) {
            Object obj;
            memset(&obj, 0, sizeof(Object));

            char *ptr = strstr(line, "id=\"");
            if (ptr) {
                sscanf(ptr + 4, "%[^\"]", obj.id);
            }

            ptr = strstr(line, "type=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", obj.type);
            }

            /* Read object contents */
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "</object>")) {
                    break;
                } else if (strstr(line, "<attributes>")) {
                    parse_object_attributes(file, &obj);
                } else if (strstr(line, "<objects>")) {
                    parse_relationships(file, obj.relationships, &obj.relationship_count, "</objects>");
                }
            }
            objects[object_count++] = obj;
        }
    }
}

void parse_object_attributes(FILE *file, Object *obj) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "</attributes>")) {
            break;
        } else if (strstr(line, "<attribute")) {
            ObjectAttribute attr;
            memset(&attr, 0, sizeof(ObjectAttribute));

            char *ptr = strstr(line, "name=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", attr.name);
            }

            ptr = strstr(line, "time=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", attr.time);
            }

            ptr = strstr(line, ">");
            if (ptr) {
                ptr++;
                char *end = strstr(ptr, "</attribute>");
                if (end) {
                    *end = '\0';
                }
                strcpy(attr.value, ptr);
            }

            obj->attributes[obj->attribute_count++] = attr;
        }
    }
}

void parse_relationships(FILE *file, Relationship *relationships, int *relationship_count, const char *end_tag) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, end_tag)) {
            break;
        } else if (strstr(line, "<relationship")) {
            Relationship rel;
            memset(&rel, 0, sizeof(Relationship));

            char *ptr = strstr(line, "object-id=\"");
            if (ptr) {
                sscanf(ptr + 11, "%[^\"]", rel.object_id);
            }

            ptr = strstr(line, "qualifier=\"");
            if (ptr) {
                sscanf(ptr + 11, "%[^\"]", rel.qualifier);
            }

            relationships[(*relationship_count)++] = rel;
        }
    }
}

void parse_events(FILE *file) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "</events>")) {
            break;
        } else if (strstr(line, "<event")) {
            Event event;
            memset(&event, 0, sizeof(Event));

            char *ptr = strstr(line, "id=\"");
            if (ptr) {
                sscanf(ptr + 4, "%[^\"]", event.id);
            }

            ptr = strstr(line, "type=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", event.type);
            }

            ptr = strstr(line, "time=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", event.time);
            }

            /* Read event contents */
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "</event>")) {
                    break;
                } else if (strstr(line, "<attributes>")) {
                    parse_event_attributes(file, &event);
                } else if (strstr(line, "<objects>")) {
                    parse_relationships(file, event.relationships, &event.relationship_count, "</objects>");
                }
            }
            events[event_count++] = event;
        }
    }
}

void parse_event_attributes(FILE *file, Event *event) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "</attributes>")) {
            break;
        } else if (strstr(line, "<attribute")) {
            ObjectAttribute attr;
            memset(&attr, 0, sizeof(ObjectAttribute));

            char *ptr = strstr(line, "name=\"");
            if (ptr) {
                sscanf(ptr + 6, "%[^\"]", attr.name);
            }

            ptr = strstr(line, ">");
            if (ptr) {
                ptr++;
                char *end = strstr(ptr, "</attribute>");
                if (end) {
                    *end = '\0';
                }
                strcpy(attr.value, ptr);
            }

            event->attributes[event->attribute_count++] = attr;
        }
    }
}