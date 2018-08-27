#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_STATES_NO 512    // Numero iniziale degli stati nella tabella degli stati
#define BASE_TRANS_NO  32     // Numero iniziale di transizioni allocate per ogni coppia (stato, carattere)
#define BASE_TAPE_LEN  128    // Lunghezza base del nastro
#define INPUT_BUF_SIZE 128
#define MAX(x, y)      (((x) > (y)) ? (x) : (y))


typedef struct {
    char write;
    signed char move;
    int next_state;
} transition;  // Oggetto "transizione"

typedef struct {
    transition *transitions;
    int items;
    int len;
} tr_list;  // Oggetto "lista di transizioni"

typedef struct {
    tr_list *states;
    int len;
} states_list; // Oggetto "lista di stati"

typedef struct {
    char *left_part;
    char *right_part;
    int left_len;
    int right_len;
    int left_max;
    int right_max;
    int pos;
    int mov_buf;
} tape_t;  // Il nastro

typedef struct cf {
    int state;
    int mv_count;
    tape_t *tape;
    struct cf *next;
} config;  // oggetto "configurazione della macchina"

typedef struct qu {
    config *head;
    config *tail;
} queue_t;  // oggetto "coda"


#ifdef DEBUG
    void print(tape_t *tape) {
        // Stampa un nastro
        printf("❮ ");
        int r_print_from = 0;
        for (int i = 0; i < tape->left_len; i++) {
            if (tape->left_part[i] == '\0') {
                r_print_from = i;
                break;
            }
        }
        for (int i = tape->left_len - 1; i >= 0; i--) {
            if (i > r_print_from) {
                printf("□ ");
            } else if (i == r_print_from) {
                printf("■ ");
            } else {
                printf("%c ", tape->left_part[i]);
            }
        }
        printf("| ");
        char buf;
        int actually_print = 1;
        for (int i = 0; i < tape->right_len; i++) {
            buf = tape->right_part[i];
            if (actually_print && buf == '\0') {
                printf("■ ");
                actually_print = 0;
            } else if (actually_print) {
                printf("%c ", buf);
            } else {
                printf("□ ");
            }
        }
        printf("❯\n     ");
        if (tape->pos < 0) {
            for (int i = 0; i < tape->left_len+tape->pos; i++) {
                printf("  ");
            }
            printf("^\n");
        } else {
            for (int i = 0; i <= tape->pos + tape->left_len; i++) {
                printf("  ");
            }
            printf("^\n");
        }
    }


    void print_tape_data(tape_t *tape) {
        printf("  left_len:    %d\n", tape->left_len);
        printf("  right_len:   %d\n", tape->right_len);
        printf("  pos:         %d\n", tape->pos);
        printf("  left_max:    %d\n", tape->left_max);
        printf("  right_max:   %d\n", tape->right_max);
        printf("  mov_buf:     %d\n", tape->mov_buf);
    }


    void print_queue(queue_t *q) {
        config *cursor = q->head;
        printf("== CONTENUTO CODA ==\n");
        while (cursor != NULL) {
            printf("%d  ", cursor->state);
            print(cursor->tape);
            cursor = cursor->next;
        }
    }
#endif


void move_tape(int mov, tape_t *tape) {
    /*
    Sposta la testina. Se la testina va spostata oltre l'ultimo
    carattere su ogni lato del nastro, rialloca la dimensione.
    */
    tape->pos += mov;
    if (tape->pos >= tape->right_len - 1) {
        tape->right_len *= 2;
        tape->right_part = realloc(tape->right_part, tape->right_len * sizeof(char));
    } else if (tape->pos <= -tape->left_len) {
        tape->left_len *= 2;
        tape->left_part = realloc(tape->left_part, tape->left_len * sizeof(char));
    }
}


char read(tape_t *tape) {
    /*
    Legge il contenuto del nastro. Se la testina si trova oltre
    l'ultimo carattere su ogni lato del nastro, ritorna '_' a
    prescindere.
    */
    if (tape->pos > tape->right_max || tape->pos < tape->left_max) {
        return '_';
    }
    if (tape->pos >= 0) {
        return tape->right_part[tape->pos];
    } else {
        return tape->left_part[-tape->pos-1];
    }
}


void write(char wc, tape_t *tape) {
    /*
    Scrive un carattere sul nastro.
    */
    if (tape->pos >= 0) {
        tape->right_part[tape->pos] = wc;
        if (tape->pos > tape->right_max) {
            tape->right_max = tape->pos;
            tape->right_part[tape->pos+1] = '\0';
        }
    } else {
        tape->left_part[-tape->pos-1] = wc;
        if (tape->pos < tape->left_max) {
            tape->left_max = tape->pos;
            tape->left_part[-tape->pos] = '\0';
        }
    }
}


void duplicate(tape_t *newtape, tape_t *tape) {
    /*
    Crea una copia esatta di un nastro.
    */
    newtape->left_len = tape->left_len;
    newtape->right_len = tape->right_len;
    newtape->left_part = malloc(newtape->left_len * sizeof(char));
    newtape->right_part = malloc(newtape->right_len * sizeof(char));
    newtape->pos = tape->pos;
    newtape->right_max = tape->right_max;
    newtape->left_max = tape->left_max;
    newtape->mov_buf = tape->mov_buf;
    strcpy(newtape->left_part, tape->left_part);
    strcpy(newtape->right_part, tape->right_part);
}


void readline(char *buffer) {
    /*
    Legge una linea del file di input.
    */
    int i = 0;
    char c;
    while ((c = getchar()) != EOF) {
        if (c == '\r') continue;
        if (c == '\n') break;
        buffer[i] = c;
        i++;
    }
    buffer[i] = '\0';
}


config *new_config(int state, int mv_count, tape_t *tape) {
    /*
    Funzione che genera una nuova configurazione
    */
    config *new_conf = malloc(sizeof(config));
    new_conf->state = state;
    new_conf->mv_count = mv_count;
    new_conf->tape = tape;
    new_conf->next = NULL;
    return new_conf;
}


void enqueue_config(queue_t *q, config *new_conf) {
    /*
    Funzione che aggiunge una configurazione in fondo alla coda
    */
    if (q->tail == NULL) {
        q->tail = q->head = new_conf;
        return;
    }
    q->tail->next = new_conf;
    q->tail = new_conf;
}


config *dequeue_config(queue_t *q) {
    /*
    Funzione che rimuove la configurazione in cima alla coda
    e la ritorna
    */
    if (q->head == NULL) {
        return NULL;
    }
    config *temp = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    return temp;
}


void cleanup(queue_t *q) {
    // Svuota la coda
    config *tmp;
    while (q->head != NULL) {
        tmp = dequeue_config(q);
        free(tmp->tape->left_part);
        free(tmp->tape->right_part);
        free(tmp->tape);
        free(tmp);
    }
}


int main() {
    unsigned int fromstate;
    char fromchar;
    char tochar;
    signed char move;
    unsigned int tostate;
    unsigned int newtmlen;
    unsigned int max_states_no = 0;
    unsigned int str2int;
    char input_buf[INPUT_BUF_SIZE];
    int cmp_ok;
    unsigned int max_moves;
    int keep_reading = 1;
    int did_read_smth = 0;
    int mv_count;
    config *cur_conf;
    char result = '0';
    char c, d;

    // Inizializzo la tabella di caratteri, ovvero l'array più esterno
    // È un array di liste di stati
    states_list trans_map[75];
    for (int i = 0; i < 75; i++) {
        trans_map[i].len = 0;
    }

    // Leggo il file fino a raggiungere l'inizio "tr"
    do {
        readline(input_buf);
        cmp_ok = strcmp(input_buf, "tr");
    } while (cmp_ok != 0);

    // Leggo l'elenco degli stati
    do {
        readline(input_buf);
        cmp_ok = strcmp(input_buf, "acc");
        if (cmp_ok != 0) {
            sscanf(input_buf, "%u %c %c %c %u", &fromstate, &fromchar, &tochar, &move, &tostate);

            max_states_no = MAX(max_states_no, MAX(fromstate, tostate));

            // Converto il carattere della mossa in qualcosa di più utile
            if (move == 'S') {
                move = 0;
            } else if (move == 'R') {
                move = 1;
            } else if (move == 'L') {
                move = -1;
            }

            fromchar -= 48;

            // Se non ho ancora visto questo carattere, devo allocare
            // per esso una lista di stati
            if (trans_map[fromchar].len == 0) {
                trans_map[fromchar].len = MAX(BASE_STATES_NO, fromstate + 1);
                trans_map[fromchar].states = malloc(trans_map[fromchar].len * sizeof(tr_list));
                for (int i = 0; i < trans_map[fromchar].len; i++) {
                    trans_map[fromchar].states[i].len = 0;
                    trans_map[fromchar].states[i].items = 0;
                }
            }

            // Se per questo carattere ho già allocato la lista di
            // stati ma non è sufficientemente lunga
            else if (trans_map[fromchar].len <= fromstate) {
                newtmlen = MAX(2 * trans_map[fromchar].len, fromstate + 1);
                trans_map[fromchar].states = realloc(trans_map[fromchar].states, newtmlen * sizeof(tr_list));
                for (int i = trans_map[fromchar].len; i < newtmlen; i++) {
                    trans_map[fromchar].states[i].len = 0;
                    trans_map[fromchar].states[i].items = 0;
                }
                trans_map[fromchar].len = newtmlen;
            }

            // Se non ho ancora visto questa combinazione stato-car,
            // devo allocare per esso una lista di transizioni
            if (trans_map[fromchar].states[fromstate].len == 0) {
                trans_map[fromchar].states[fromstate].len = BASE_TRANS_NO;
                trans_map[fromchar].states[fromstate].transitions = malloc(trans_map[fromchar].states[fromstate].len * sizeof(transition));
            }

            // Se per questa combinazione stato-carattere ho già
            // allocato una lista di transizioni ma non è
            // sufficientemente lunga
            else if (trans_map[fromchar].states[fromstate].len == trans_map[fromchar].states[fromstate].items) {
                trans_map[fromchar].states[fromstate].len = 2 * trans_map[fromchar].states[fromstate].len;
                trans_map[fromchar].states[fromstate].transitions = realloc(trans_map[fromchar].states[fromstate].transitions, trans_map[fromchar].states[fromstate].len * sizeof(transition));
            }

            // Finalmente, salvo la transizione
            trans_map[fromchar].states[fromstate].transitions[trans_map[fromchar].states[fromstate].items].write = tochar;
            trans_map[fromchar].states[fromstate].transitions[trans_map[fromchar].states[fromstate].items].move = move;
            trans_map[fromchar].states[fromstate].transitions[trans_map[fromchar].states[fromstate].items].next_state = tostate;
            trans_map[fromchar].states[fromstate].items++;
        }
    } while (cmp_ok != 0);

    // Inizializzo la lista di stati di accettazione e li salvo
    char accepting_states[max_states_no + 1];
    memset(accepting_states, 0, max_states_no + 1);
    do {
        readline(input_buf);
        cmp_ok = strcmp(input_buf, "max");
        if (cmp_ok != 0) {
            sscanf(input_buf, "%u", &str2int);
            accepting_states[str2int] = 1;
        }
    } while (cmp_ok != 0);

    // Leggo il massimo numero di mosse
    readline(input_buf);
    sscanf(input_buf, "%u", &max_moves);

    // Leggo il file fino a raggiungere "run"
    do {
        readline(input_buf);
        cmp_ok = strcmp(input_buf, "run");
    } while (cmp_ok != 0);

    // Preparo una coda per il BFS
    queue_t *queue;
    queue = malloc(sizeof(queue_t));
    queue->head = queue->tail = NULL;

    while (keep_reading) {
        // Per ogni riga di input, fino alla fine del file:
        // Creo e preparo il nastro
        tape_t *tape = malloc(sizeof(tape_t));
        tape->left_len = BASE_TAPE_LEN;
        tape->right_len = BASE_TAPE_LEN;
        tape->left_part = malloc(tape->left_len * sizeof(char));
        tape->right_part = malloc(tape->right_len * sizeof(char));
        strcpy(tape->left_part, "_");
        strcpy(tape->right_part, "_");
        tape->pos = 0;
        tape->right_max = 0;
        tape->left_max = -1;
        tape->mov_buf = 0;
        did_read_smth = 0; // Controllo di non considerare linee vuote
        // Scrivo i caratteri sul nastro
        while ((c = getchar())) {
            if (c == '\r') continue;
            if (c == '\n') break;
            if (c == EOF) {
                keep_reading = 0;
                break;
            }
            did_read_smth = 1;
            write(c, tape);
            move_tape(1, tape);
        }
        tape->pos = 0; // Riavvolgo il nastro
        if (! did_read_smth) {
            // Se ho letto una linea vuota, cancello il nastro che
            // ho inutilmente allocato.
            free(tape->left_part);
            free(tape->right_part);
            free(tape);
            break;
        }

        // Aggiungo la configurazione iniziale della macchina alla coda
        // del BFS
        enqueue_config(queue, new_config(0, 0, tape));
        result = '0';

        // Qui incomincia il BFS
        while (queue->head != NULL) {
            // Prelevo il primo elemento della coda
            cur_conf = dequeue_config(queue);
            if (accepting_states[cur_conf->state]) {
                // Se mi trovo in uno stato di accettazione ho finito
                result = '1';
                free(cur_conf->tape->left_part);
                free(cur_conf->tape->right_part);
                free(cur_conf->tape);
                free(cur_conf);
                break;
            }
            if (cur_conf->mv_count >= max_moves) {
                // Se ho superato il numero massimo di mosse ho finito
                // con questo ramo di esecuzione, ma continuo a
                // esaminare gli altri elementi in coda
                if (result != 'U') result = 'U';
                free(cur_conf->tape->left_part);
                free(cur_conf->tape->right_part);
                free(cur_conf->tape);
                free(cur_conf);
                continue;
            }
            mv_count = cur_conf->mv_count + 1;
            c = read(cur_conf->tape);
            d = c - 48;
            if (trans_map[d].len > cur_conf->state && trans_map[d].states[cur_conf->state].items > 0) {
                for (int i = 0; i < trans_map[d].states[cur_conf->state].items - 1; i++) {
                    // Per ogni transizione compatibile con la
                    // configurazione attuale TRANNE L'ULTIMA procedo
                    // duplicando il nastro e aggiungendo una nuova
                    // configurazione alla coda
                    tape_t *tape_copy = malloc(sizeof(tape_t));
                    duplicate(tape_copy, cur_conf->tape);
                    if (trans_map[d].states[cur_conf->state].transitions[i].write != c) {
                        write(trans_map[d].states[cur_conf->state].transitions[i].write, tape_copy);
                    }
                    if (trans_map[d].states[cur_conf->state].transitions[i].move != 0) {
                        move_tape(trans_map[d].states[cur_conf->state].transitions[i].move, tape_copy);
                    }
                    enqueue_config(queue, new_config(trans_map[d].states[cur_conf->state].transitions[i].next_state, mv_count, tape_copy));
                }
                // Ora mi occupo dell'ultima transizione, oppure
                // dell'unica transizione che ho se mi trovo in un
                // caso deterministico. Anzi che duplicare il nastro
                // modifico direttamente quello corrente.
                if (trans_map[d].states[cur_conf->state].transitions[trans_map[d].states[cur_conf->state].items-1].write != c) {
                    write(trans_map[d].states[cur_conf->state].transitions[trans_map[d].states[cur_conf->state].items-1].write, cur_conf->tape);
                }
                if (trans_map[d].states[cur_conf->state].transitions[trans_map[d].states[cur_conf->state].items-1].move != 0) {
                    move_tape(trans_map[d].states[cur_conf->state].transitions[trans_map[d].states[cur_conf->state].items-1].move, cur_conf->tape);
                }
                cur_conf->state = trans_map[d].states[cur_conf->state].transitions[trans_map[d].states[cur_conf->state].items-1].next_state;
                cur_conf->mv_count = mv_count;
                cur_conf->next = NULL;
                enqueue_config(queue, cur_conf);
            } else {
                free(cur_conf->tape->left_part);
                free(cur_conf->tape->right_part);
                free(cur_conf->tape);
                free(cur_conf);
            }
        }
        printf("%c\n", result);
        cleanup(queue);
    }
    return 0;
}
