#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BASE_IN_BUF_SIZE 20  // Massima lunghezza (iniziale) di ogni linea del file di input
#define BASE_TRANS_NO 50  // Numero iniziale di transizioni
#define BASE_ACCEPT_NO 5 // Numero iniziale di stati di accettazione
#define BASE_STR_LEN 100 // Massima lunghezza di ogni stringa di input per la MT
#define BASE_STR_NO 20 // Numero iniziale di stringhe date in input


typedef struct transition {
    int cur_state;
    char read;
    char write;
    char move;
    int next_state;
} transition;  // oggetto "transizione"


typedef struct trans_list {
    transition *data;
    int length;
    int items;
} trans_list;  // oggetto "lista di transizioni"


typedef struct accept_list {
    int *data;
    int length;
    int items;
} accept_list;  // oggetto "lista di stati di accettazione"


typedef struct inputstr_list {
    char **data;
    int length;
    int items;
} inputstr_list;  // oggetto "lista di stringhe di input"


void _print_transitions(trans_list t) {
    printf("== TRANSIZIONI ==\n");
    for (int i = 0; i < t.items; i++) {
        printf(" %d, %c, %c, %c, %d\n", t.data[i].cur_state, t.data[i].read, t.data[i].write, t.data[i].move, t.data[i].next_state);
    }
    printf("\n");
}


void _print_accepts(accept_list a) {
    printf("== STATI DI ACCETTAZIONE ==\n");
    for (int i = 0; i < a.items; i++) {
        printf(" %d\n", a.data[i]);
    }
    printf("\n");
}


void _print_inputstrings(inputstr_list s) {
    printf("== STRINGHE DI INPUT ==\n");
    for (int i = 0; i < s.items; i++) {
        printf(" %s\n", s.data[i]);
    }
    printf("\n");
}


transition str2trans(char *input) {
    // Funzione che converte una stringa in un oggetto "transizione".

    transition t;
    sscanf(input, "%d %c %c %c %d",
           &(t.cur_state),
           &(t.read),
           &(t.write),
           &(t.move),
           &(t.next_state));
    return t;
}


void trans_append(trans_list *trlist, transition item) {
    // Aggiungi una transizione a una lista di transizioni

    trlist->items++;
    if (trlist->items > trlist->length) {
        // Se non c'è più spazio nella lista, raddoppio la sua dimensione
        trlist->length *= 2;
        trlist->data = realloc(trlist->data, (trlist->length) * sizeof(transition));
    }
    trlist->data[trlist->items - 1] = item;
}


void accept_append(accept_list *acclist, int item) {
    // Aggiungi uno stato di accettazione a una lista di stati di accettazione

    acclist->items++;
    if (acclist->items > acclist->length) {
        acclist->length *= 2;
        // Se non c'è più spazio nella lista, raddoppio la sua dimensione
        acclist->data = realloc(acclist->data, (acclist->length) * sizeof(int));
    }
    acclist->data[acclist->items - 1] = item;
}


void inputstr_append(inputstr_list *instrlist, char *item) {
    // Aggiungi una stringa di input a una lista di stringhe di input

    instrlist->items++;
    if (instrlist->items > instrlist->length) {
        // Se non c'è più spazio nella lista, raddoppio la sua dimensione
        instrlist->length *= 2;
        instrlist->data = realloc(instrlist->data, (instrlist->length) * sizeof(char*));
        // Notare che in questo caso "data" è una lista di puntatori, quindi non dipende
        // dalla lunghezza di ogni stringa
    }
    instrlist->data[instrlist->items - 1] = malloc(strlen(item) * sizeof(char));
    // Qui teniamo finalmente conto della lunghezza della stringa
    strcpy(instrlist->data[instrlist->items - 1], item);
}


void readline(char *buf, int *buf_length) {
    // Funzione che legge una linea di testo e la salva in una stringa.
    // Se necessario, rialloca la lunghezza della stringa.

    int i = 0;
    char c;

    while ((c = getchar()) != EOF) {
        if (*buf_length - 2 < i) {
            *buf_length *= 2;
        }
        if (c == '\n') break;
        buf[i] = c;
        i++;
    }

    buf[i] = '\0';
}


void parse_input(trans_list *transitions, accept_list *accepts, inputstr_list *input_strings, int *max_moves) {
    int cmp_ok;
    int str2int;
    char *line;
    char *string;

    int line_len = BASE_IN_BUF_SIZE;
    int string_len = BASE_STR_LEN;

    line = malloc(line_len * sizeof(char));
    string = malloc(string_len * sizeof(char));

    // Leggo il file fino a raggiungere l'inizio "tr"
    do {
        readline(line, &line_len);
    } while (strcmp(line, "tr"));

    // Leggo l'elenco degli stati
    do {
        readline(line, &line_len);
        cmp_ok = strcmp(line, "acc");
        if (cmp_ok) {
            trans_append(transitions, str2trans(line));
        }
    } while (cmp_ok);

    // Leggo l'elenco degli stati di accettazione
    do {
        readline(line, &line_len);
        cmp_ok = strcmp(line, "max");
        if (cmp_ok) {
            sscanf(line, "%d", &str2int);
            accept_append(accepts, str2int);
        }
    } while (cmp_ok);

    // Leggo il massimo numero di mosse
    readline(line, &line_len);
    sscanf(line, "%d", max_moves);

    // Leggo il file fino a raggiungere l'inizio di "run"
    do {
        readline(line, &line_len);
    } while (strcmp(line, "run") != 0);

    // Leggo le stringhe di input per la MT
    do {
        readline(string, &string_len);
        cmp_ok = strcmp(string, "");
        if (cmp_ok) {
            inputstr_append(input_strings, string);
        }
    } while (cmp_ok);

}


int main() {

    // Inizializzo le variabili e le liste

    trans_list transitions;
    transitions.items = 0;
    transitions.length = BASE_TRANS_NO;
    transitions.data = malloc(transitions.length * sizeof(transition));

    accept_list accepts;
    accepts.items = 0;
    accepts.length = BASE_ACCEPT_NO;
    accepts.data = malloc(accepts.length * sizeof(int));

    inputstr_list input_strings;
    input_strings.items = 0;
    input_strings.length = BASE_STR_NO;
    input_strings.data = malloc(input_strings.length * sizeof(char*));

    int max_moves;


    // Popolo tutto leggendo il file di input
    parse_input(&transitions, &accepts, &input_strings, &max_moves);

    _print_transitions(transitions);
    _print_accepts(accepts);
    _print_inputstrings(input_strings);
    printf("== NUMERO MASSIMO MOSSE ==\n %d\n", max_moves);

    return 0;
}
