#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_IN_BUF_SIZE 20  // Massima lunghezza (iniziale) di ogni linea del file di input
#define BASE_TRANS_NO 50  // Numero iniziale di transizioni
#define BASE_ACCEPT_NO 5 // Numero iniziale di stati di accettazione
#define LEFT 'L'
#define RIGHT 'R'
#define STOP 'S'
#define BLANK '_' // Simboli del nastro
#define UNDEFINED 'U'
#define ACCEPTED '1'
#define REJECTED '0'


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


typedef struct TC {
    char content;
    struct TC *right;
    struct TC *left;
} tape_cell;  // oggetto "cella del nastro di memoria"


typedef tape_cell *cell_ptr;  // puntatore a una cella del nastro


typedef struct CF {
    int state;
    int mv_count;
    cell_ptr tape;
    struct CF *next;
} config;  // oggetto "configurazione della macchina"


typedef config *conf_ptr;


typedef struct QU {
    config *head;
    config *tail;
} queue;  // oggetto "coda"


typedef queue *queue_ptr;


#ifdef DEBUG
    void _print_transitions(trans_list t) {
        printf("== TRANSIZIONI ==\n");
        for (int i = 0; i < t.items; i++) {
            printf(" %d | %c | %c | %c | %d\n", t.data[i].cur_state, t.data[i].read, t.data[i].write, t.data[i].move, t.data[i].next_state);
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


    void _print_tape(cell_ptr tape_head) {
        cell_ptr cursor = tape_head;
        printf("== CONTENUTO NASTRO ==\n [");
        while (cursor != NULL) {
            printf("%c, ", cursor->content);
            cursor = cursor->right;
        }
        printf("]\n");
    }


    void _print_queue(queue_ptr q) {
        conf_ptr cursor = q->head;
        printf("== CONTENUTO CODA ==\n");
        while (cursor != NULL) {
            printf("  %d %c\n", cursor->state, cursor->tape->content);
            cursor = cursor->next;
        }
    }
#endif


cell_ptr create_cell() {
    // Funzione che genera una nuova cella

    cell_ptr new_cell = malloc(sizeof(tape_cell));
    new_cell->content = BLANK;
    new_cell->left = NULL;
    new_cell->right = NULL;
    return new_cell;
}


conf_ptr new_config(int state, int mv_count, cell_ptr tape) {
    // Funzione che genera una nuova configurazione

    conf_ptr new_conf = malloc(sizeof(config));
    new_conf->state = state;
    new_conf->mv_count = mv_count;
    new_conf->tape = tape;
    new_conf->next = NULL;
    return new_conf;
}


queue_ptr new_queue() {
    queue_ptr q = malloc(sizeof(queue));
    q->head = q->tail = NULL;
    return q;
}


void grow_tape_lft(cell_ptr cur_cell) {
    // Funzione che inserisce una nuova cella a sinistra del nastro

    cell_ptr new_cell = create_cell();
    new_cell->right = cur_cell;
    cur_cell->left = new_cell;
}


void grow_tape_rgt(cell_ptr cur_cell) {
    // Funzione che inserisce una nuova cella a destra del nastro

    cell_ptr new_cell = create_cell();
    new_cell->left = cur_cell;
    cur_cell->right = new_cell;
}


void enqueue_config(queue_ptr q, conf_ptr new_conf) {
    if (q->tail == NULL) {
        q->tail = q->head = new_conf;
        return;
    }
    q->tail->next = new_conf;
    q->tail = new_conf;
}


conf_ptr dequeue_config(queue_ptr q) {
    if (q->head == NULL) {
        return NULL;
    }
    conf_ptr temp = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    return temp;
}


void move_head(char move, cell_ptr *cur_cell) {
    // Funzione che sposta la testina sul nastro

    if (move == STOP) return;
    if (move == LEFT) {
        if ((*cur_cell)->left == NULL) {
            grow_tape_lft(*cur_cell);
        }
        *cur_cell = (*cur_cell)->left;
    } else if (move == RIGHT) {
        if ((*cur_cell)->right == NULL) {
            grow_tape_rgt(*cur_cell);
        }
        *cur_cell = (*cur_cell)->right;

    }
}


void rewind_(cell_ptr *cur_cell) {
    // Funzione che riavvolge il nastro spostando la testina nella
    // cella più a sinistra.

    while ((*cur_cell)->left != NULL) {
        *cur_cell = (*cur_cell)->left;
    }
}


void obliterate(cell_ptr tape_head) {
    // Distrugge un natro

    cell_ptr tmp;
    while (tape_head != NULL) {
        tmp = tape_head;
        tape_head = tape_head->right;
        free(tmp);
    }
}


void cleanup(queue_ptr q) {
    conf_ptr tmp;
    while (q->head != NULL) {
        tmp = dequeue_config(q);
        rewind_(&(tmp->tape));
        obliterate(tmp->tape);
        free(tmp);
    }
}


cell_ptr duplicate(cell_ptr source) {
    // Duplica un nastro

    cell_ptr new_cell;
    cell_ptr prev_new_cell = NULL;
    cell_ptr old_cursor;
    cell_ptr new_head = NULL;
    int moves = 0;
    old_cursor = source;

    // Riavvolgo il nastro tenendo conto di quanto
    while (old_cursor->left != NULL) {
        old_cursor = old_cursor->left;
        moves++;
    }

    // Duplico il nastro
    while (old_cursor != NULL) {
        new_cell = create_cell();
        new_cell->content = old_cursor->content;
        if (new_head != NULL) {
            new_cell->left = prev_new_cell;
            prev_new_cell->right = new_cell;
        } else {
            new_head = new_cell;
        }
        prev_new_cell = new_cell;
        old_cursor = old_cursor->right;
    }

    // Mando avanti il nuovo nastro fino alla posizione corretta
    while (moves > 0) {
        new_head = new_head->right;
        moves--;
    }

    return new_head;
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


int is_accepting(int status, accept_list acclist) {
    // Determina se uno stato è di accettazione

    for (int i = 0; i < acclist.items; i++) {
        if (acclist.data[i] == status) {
            return 1;
        }
    }
    return 0;
 }


cell_ptr write_inputstr(char *string) {
    // Scrive la stringa di input sul nastro, lo riavvolge e lo restituisce

    cell_ptr cell = create_cell();
    unsigned long i;
    for (i = 0; i < strlen(string) - 1; i++) {
        cell->content = string[i];
        move_head(RIGHT, &cell);
    }
    cell->content = string[i];
    rewind_(&cell);
    return cell;
}


char* readline(int buf_size) {
    // Funzione che legge una linea di testo e la salva in una stringa.
    // Se necessario, rialloca la lunghezza della stringa.

    int i = 0;
    char c;
    char *line;

    line = malloc(buf_size * sizeof(char));

    while ((c = getchar()) != EOF) {
        if (buf_size - 2 < i) {
            buf_size *= 2;
            line = realloc(line, buf_size * sizeof(char));
        }
        if (c == '\n') break;
        line[i] = c;
        i++;
    }

    line[i] = '\0';
    return line;
}


void parse_input(trans_list *transitions, accept_list *accepts, int *max_moves) {
    int cmp_ok;
    int str2int;
    char *line;

    // Leggo il file fino a raggiungere l'inizio "tr"
    do {
        line = readline(BASE_IN_BUF_SIZE);
        cmp_ok = strcmp(line, "tr");
        free(line);
    } while (cmp_ok);

    // Leggo l'elenco degli stati
    do {
        line = readline(BASE_IN_BUF_SIZE);
        cmp_ok = strcmp(line, "acc");
        if (cmp_ok) {
            trans_append(transitions, str2trans(line));
        }
        free(line);
    } while (cmp_ok);

    // Leggo l'elenco degli stati di accettazione
    do {
        line = readline(BASE_IN_BUF_SIZE);
        cmp_ok = strcmp(line, "max");
        if (cmp_ok) {
            sscanf(line, "%d", &str2int);
            accept_append(accepts, str2int);
        }
        free(line);
    } while (cmp_ok);

    // Leggo il massimo numero di mosse
    line = readline(BASE_IN_BUF_SIZE);
    sscanf(line, "%d", max_moves);
    free(line);

    // Leggo il file fino a raggiungere "run"
    do {
        line = readline(BASE_IN_BUF_SIZE);
        cmp_ok = strcmp(line, "run");
        free(line);
    } while (cmp_ok);
}


char run_mt(cell_ptr tape, int max_moves, trans_list transitions, accept_list accepts) {
    queue_ptr queue = new_queue();
    conf_ptr init_conf = new_config(0, 0, tape);
    enqueue_config(queue, init_conf);
    int mv_count;
    conf_ptr cur_conf, next_conf;
    cell_ptr tape_cpy;
    char result = REJECTED;

    while (queue->head != NULL) {
        // Estraggo la prima configurazione dalla coda
        cur_conf = dequeue_config(queue);
        if (is_accepting(cur_conf->state, accepts)) {
            result = ACCEPTED;
            rewind_(&(cur_conf->tape));
            obliterate(cur_conf->tape);
            free(cur_conf);
            cleanup(queue);
            break;
        }
        if (cur_conf->mv_count >= max_moves) {
            result = UNDEFINED;
            rewind_(&(cur_conf->tape));
            obliterate(cur_conf->tape);
            free(cur_conf);
            cleanup(queue);
            break;
        }
        mv_count = cur_conf->mv_count + 1;
        for (int i = 0; i < transitions.items; i++) {
            if ((cur_conf->state == transitions.data[i].cur_state) && (cur_conf->tape->content == transitions.data[i].read)) {
                // Per ogni transizione che posso effettuare da qui,
                // duplico il nastro e aggiungo la nuova configurazione
                // alla coda.
                tape_cpy = duplicate(cur_conf->tape);
                tape_cpy->content = transitions.data[i].write;
                move_head(transitions.data[i].move, &tape_cpy);
                next_conf = new_config(transitions.data[i].next_state, mv_count, tape_cpy);
                enqueue_config(queue, next_conf);
            }
        }
        rewind_(&(cur_conf->tape));
        obliterate(cur_conf->tape);
        free(cur_conf);
    }
    free(queue);
    return result;
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

    int max_moves;


    // Popolo tutto leggendo il file di input
    parse_input(&transitions, &accepts, &max_moves);


    char read_char; // Buffer per la lettura di un carattere dallo stdin
    int keep_reading = 1;
    cell_ptr tape = NULL;


    // Per ogni stringa di input
    while (keep_reading) {

        // Preparo un nastro contenente la stringa
        tape = create_cell();
        while ((read_char = getchar())) {
            if (read_char == '\n') break;
            if (read_char == EOF) {
                keep_reading = 0;
                break;
            }
            tape->content = read_char;
            move_head(RIGHT, &tape);
        }
        rewind_(&tape);
        if (! keep_reading) {
            // Se non dovevo continuare a leggere altre stringhe,
            // cancello il nastro che ho inutilmente creato.
            obliterate(tape);
            break;
        }

        // Esecuzione della macchina di Turing
        printf("%c\n", run_mt(tape, max_moves, transitions, accepts));
    }

    free(transitions.data);
    free(accepts.data);

    return 0;
}
