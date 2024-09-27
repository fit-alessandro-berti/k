#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure for a Place
typedef struct Place {
    char id[50];
    int initialMarking;
    struct Place *next;
} Place;

// Structure for a Final Marking
typedef struct FinalMarking {
    char place_id[50];
    int finalMarking;
    struct FinalMarking *next;
} FinalMarking;

// Structure for a Transition
typedef struct Transition {
    char id[50];
    char name[100]; // Transition label (name)
    int visible; // 1 if visible, 0 if invisible
    struct Transition *next;
} Transition;

// Structure for an Arc
typedef struct Arc {
    char id[50];
    char source[50];
    char target[50];
    struct Arc *next;
} Arc;

// Structure for a Petri Net
typedef struct PetriNet {
    Place *places;
    Transition *transitions;
    Arc *arcs;
    FinalMarking *finalMarkings;
} PetriNet;

// Function to create a new PetriNet
PetriNet* createPetriNet() {
    PetriNet* net = (PetriNet*) malloc(sizeof(PetriNet));
    net->places = NULL;
    net->transitions = NULL;
    net->arcs = NULL;
    net->finalMarkings = NULL;
    return net;
}

// Function to add a place
void addPlace(PetriNet* net, char* id, int initialMarking) {
    Place* place = (Place*) malloc(sizeof(Place));
    strcpy(place->id, id);
    place->initialMarking = initialMarking;
    place->next = net->places;
    net->places = place;
}

// Function to add a transition
void addTransition(PetriNet* net, char* id, char* name, int visible) {
    Transition* transition = (Transition*) malloc(sizeof(Transition));
    strcpy(transition->id, id);
    strcpy(transition->name, name);
    transition->visible = visible;
    transition->next = net->transitions;
    net->transitions = transition;
}

// Function to add an arc
void addArc(PetriNet* net, char* id, char* source, char* target) {
    Arc* arc = (Arc*) malloc(sizeof(Arc));
    strcpy(arc->id, id);
    strcpy(arc->source, source);
    strcpy(arc->target, target);
    arc->next = net->arcs;
    net->arcs = arc;
}

// Function to add a final marking
void addFinalMarking(PetriNet* net, char* place_id, int finalMarking) {
    FinalMarking* marking = (FinalMarking*) malloc(sizeof(FinalMarking));
    strcpy(marking->place_id, place_id);
    marking->finalMarking = finalMarking;
    marking->next = net->finalMarkings;
    net->finalMarkings = marking;
}

// Helper function to extract the transition name
void extractText(char *line, char *output) {
    char *start = strstr(line, "<text>");
    char *end = strstr(line, "</text>");
    if (start != NULL && end != NULL) {
        start += 6; // Move past <text> tag
        strncpy(output, start, end - start);
        output[end - start] = '\0';
    }
}

// Function to import from PNML
void importPNML(PetriNet* net, const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file: %s\n", filename);
        return;
    }

    char line[256], id[50], name[100], source[50], target[50];
    int initialMarking = 0, finalMarking = 0, visible = 1;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "<place")) {
            sscanf(line, " <place id=\"%49[^\"]", id);
            fgets(line, sizeof(line), file); // Read the next line
            initialMarking = 0;
            if (strstr(line, "<initialMarking>")) {
                sscanf(line, " <initialMarking><text>%d</text></initialMarking>", &initialMarking);
            }
            addPlace(net, id, initialMarking);
        } else if (strstr(line, "<transition")) {
            sscanf(line, " <transition id=\"%49[^\"]", id);
            fgets(line, sizeof(line), file); // Read the name or next line
            name[0] = '\0'; // Initialize name
            if (strstr(line, "<name>")) {
                fgets(line, sizeof(line), file); // Read the actual <text> line
                extractText(line, name);
            }
            visible = 1; // Assume visible unless specified
            if (fgets(line, sizeof(line), file) && strstr(line, "activity=\"$invisible$\"")) {
                visible = 0;
            }
            addTransition(net, id, name, visible);
        } else if (strstr(line, "<arc")) {
            sscanf(line, " <arc id=\"%49[^\"]\" source=\"%49[^\"]\" target=\"%49[^\"]\"", id, source, target);
            addArc(net, id, source, target);
        } else if (strstr(line, "<finalmarkings>")) {
            fgets(line, sizeof(line), file); // Read the <marking> line
            sscanf(line, " <place idref=\"%49[^\"]", id);
            fgets(line, sizeof(line), file); // Read the final marking text line
            sscanf(line, " <text>%d</text>", &finalMarking);
            addFinalMarking(net, id, finalMarking);
        }
    }
    fclose(file);
}

// Function to export to PNML
void exportPNML(PetriNet* net, const char* filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error opening file: %s\n", filename);
        return;
    }

    fprintf(file, "<?xml version='1.0' encoding='UTF-8'?>\n<pnml>\n  <net id=\"generated_net\" type=\"http://www.pnml.org/version-2009/grammar/pnmlcoremodel\">\n    <page id=\"n0\">\n");

    // Export places
    Place* p = net->places;
    while (p) {
        fprintf(file, "      <place id=\"%s\">\n        <name>\n          <text>%s</text>\n        </name>\n", p->id, p->id);
        if (p->initialMarking) {
            fprintf(file, "        <initialMarking>\n          <text>%d</text>\n        </initialMarking>\n", p->initialMarking);
        }
        fprintf(file, "      </place>\n");
        p = p->next;
    }

    // Export transitions
    Transition* t = net->transitions;
    while (t) {
        fprintf(file, "      <transition id=\"%s\">\n        <name>\n          <text>%s</text>\n        </name>\n", t->id, t->name);
        if (!t->visible) {
            fprintf(file, "        <toolspecific tool=\"ProM\" version=\"6.4\" activity=\"$invisible$\"/>\n");
        }
        fprintf(file, "      </transition>\n");
        t = t->next;
    }

    // Export arcs
    Arc* a = net->arcs;
    while (a) {
        fprintf(file, "      <arc id=\"%s\" source=\"%s\" target=\"%s\"/>\n", a->id, a->source, a->target);
        a = a->next;
    }

    fprintf(file, "    </page>\n");

    // Final markings section
    if (net->finalMarkings) {
        fprintf(file, "    <finalmarkings>\n");
        FinalMarking* f = net->finalMarkings;
        while (f) {
            fprintf(file, "      <marking>\n        <place idref=\"%s\">\n          <text>%d</text>\n        </place>\n      </marking>\n", f->place_id, f->finalMarking);
            f = f->next;
        }
        fprintf(file, "    </finalmarkings>\n");
    }

    fprintf(file, "  </net>\n</pnml>\n");
    fclose(file);
}

// Function to free the memory
void freePetriNet(PetriNet* net) {
    Place* p = net->places;
    while (p) {
        Place* tmp = p;
        p = p->next;
        free(tmp);
    }

    Transition* t = net->transitions;
    while (t) {
        Transition* tmp = t;
        t = t->next;
        free(tmp);
    }

    Arc* a = net->arcs;
    while (a) {
        Arc* tmp = a;
        a = a->next;
        free(tmp);
    }

    FinalMarking* f = net->finalMarkings;
    while (f) {
        FinalMarking* tmp = f;
        f = f->next;
        free(tmp);
    }

    free(net);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_pnml> <output_pnml>\n", argv[0]);
        return 1;
    }

    PetriNet* net = createPetriNet();
    importPNML(net, argv[1]);
    exportPNML(net, argv[2]);
    freePetriNet(net);

    return 0;
}
