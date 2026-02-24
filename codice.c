/*  PROGETTO API 2023/2024  */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/* DEFINES */
#define MAXCHAR 256
#define HASH_SIZE (1 << 15)
#define HEAP_SIZE 3000
#define ALPHA ((sqrt(5) - 1) / 2)
#define DELETED ((ricetta *)(-1))  //Per indicare una cella eliminata della hash table
#define PRONTA_CONSEGNA_SIZE 3500



/* STRUCTURES */
typedef struct ingrediente{
    //char nome[MAXCHAR];
    uint32_t key;
    int qnt;
    struct ingrediente *next;
}ingrediente;

typedef struct ricetta{
    char nome[MAXCHAR];
    ingrediente *lista_ingredienti;
    int peso;                           // peso = somma delle qnt di ogni ingrediente
}ricetta;

typedef struct lotto{
    int qnt;
    int scadenza;
}lotto;

typedef struct lotti {
    //char nome[MAXCHAR]; // <-- forse si può rimuovere
    lotto MinHeap[HEAP_SIZE];
    int qntTot;                         //qntTot = somma delle qnt di ogni lotto
    int heapSize;
}lotti;

typedef struct ordine{
    uint32_t index;
    int t;
    char nome[MAXCHAR];
    int num;
    int pesoOrdine;                     //pesoOrdine = peso ricetta * num
}ordine;

typedef struct nodo {
    ordine ordine_sospeso;
    struct nodo *next;
} nodo;

typedef struct Lista {                  //gestione degli ordini in attesa
    nodo *testa;
    nodo *coda;
}lista;

typedef struct heapOrdini{              //Heap ordini pronti e heap ordini da spedire
    ordine ordini[4000];
    int size;
}heapOrdini;





// Nodo del magazzino di ingredienti
typedef struct TreeNode {
    uint32_t key;
    lotti ingrediente;
    struct TreeNode *left;
    struct TreeNode *right;
}TreeNode;




void init_globali();
void free_globali();

int lettura_comando(char *scan);
void lettura_nome(char *scan);
void pulizia_buffer();

uint32_t key_generator(char *scan);
uint32_t hash_function(uint32_t key);

uint32_t search_ricetta(char *scan, int operation);
void aggiungi_ricetta(char *scan);
ingrediente* aggiungi_ingrediente(char *scan, uint32_t index);
void rimuovi_ricetta(char *scan);
void cancella_ingrediente(uint32_t index);

void freeTree(TreeNode* root);
TreeNode* create_node(uint32_t key);
TreeNode* TreeInsert(TreeNode* root, uint32_t key);
TreeNode* search_tipo_lotto(TreeNode* root, uint32_t key);
void Min_Heapify(TreeNode *nodo_ing, int size, int i);
void MinHeap_delete(TreeNode *nodo_ing);
void MinHeap_insert(TreeNode *pnt);
void rifornisci_magazzino(char *scan);

int prontaConsegna_search(uint32_t index);
void prontaConsegna_Heapify(int i);
void prontaConsegna_delete();
void prontaConsegna_insert(char *nome, int num, int peso, int inListaDiAttesa, int tArrivo);
int verificaRisorse(ingrediente *pnt, int num);
int preparaOrdine(uint32_t index_ricetta, int num, int inListaDiAttesa, int tArrivo);
void confermaOrdine(char *scan);

int sospesi_search(uint32_t index);
void sospesi_insert(uint32_t index, int num);
void sospesi_delete();

void swap(ordine *a, ordine *b);
int partition(int p, int r);
void quickSort(int p,int r);
void spedisci(int size);
void caricaFurgone();

int strcomp2(char *str1, char *str2);




/* VARIABILI GLOBALI */
int TEMPO, PERIODO, CAPIENZA;
ricetta *hash_table_ricette[HASH_SIZE];             /* Hash Table contenente tutte le ricette */

heapOrdini *prontaConsegna;                         /* minHeap, ordini completati e pronti alla stampa*/
lista *ordini_sospesi;                              /* lista con inserimento in coda per la gestione degli ordini in sospeso */
ordine spedizioneOrdini[PRONTA_CONSEGNA_SIZE];      /* array, ordini presenti nel furgone in ordine decrescente di peso */

TreeNode *ROOT;







/* MAIN */
int main(){

    char *scan;

    scan = NULL;
    scan = malloc(sizeof(char) * MAXCHAR);
    init_globali();


    //CONFIGURAZIONE CORRIERE
    if (scanf("%d %d ", &PERIODO, &CAPIENZA) != 2) {
        printf("Errore nell'input. Impossibile leggere i due valori richiesti.\n");
        return 1;
    }

    while(1){

        TEMPO++;

        if (!lettura_comando(scan)) {
            if(!(TEMPO % PERIODO) && TEMPO != 0)
                caricaFurgone();
            break;
        }

        if(!(TEMPO % PERIODO) && TEMPO != 0)
            caricaFurgone();

        if(scan[2] == 'g'){

            lettura_nome(scan);
            aggiungi_ricetta(scan);

        }

        else if(scan[2] == 'm') {

            lettura_nome(scan);
            rimuovi_ricetta(scan);

        }

        else if(scan[2] == 'f'){

            rifornisci_magazzino(scan);
            sospesi_delete();

        }

        else{

            lettura_nome(scan);
            confermaOrdine(scan);

        }

    }

    free(scan);
    free_globali();
    return 0;
}














/* FUNZIONI */
uint32_t key_generator(char *scan){

    uint32_t key=0;

    for (size_t i = 0; i < 256; i++) {
        if(scan[i] == '\0')
            break;
        key ^= (scan[i] * 17 + i * i * 101);
    }

    return key;
}





uint32_t hash_function(uint32_t key) {
    /*
     * alternativa metodo moltiplicazione:
     * float key = k * ALPHA;
     * key = key - (int)key;
     * uint32_t index = (uint32_t)(HASH_SIZE * key);
     */

    uint64_t index_ = (uint64_t)( key * (double)ALPHA * ((uint64_t)1 << 32));
    uint32_t index = (uint32_t)(index_ & (HASH_SIZE-1));

    return index;
}

// Funzione di probing quadratico
uint32_t quadratic_probing(uint32_t index, uint32_t i) {
    return (index + ((i / 2) + ((i * i) / 2))) & (HASH_SIZE - 1);
}




uint32_t search_ricetta(char *scan, int operation){
    uint32_t index = hash_function(key_generator(scan));
    uint32_t i = 0, firstDelete = 1;
    uint32_t deleted_index = 0;

    switch(operation){

        case(0):    //RIMUOVI RICETTA

            if(hash_table_ricette[index] == NULL)
                return 99999;   //ricetta non presente

            while (hash_table_ricette[index] != NULL && i < HASH_SIZE) {                                                //verifico l'effettiva presenza in tabella

                if (hash_table_ricette[index] != DELETED && !strcomp2(hash_table_ricette[index]->nome, scan))
                    return index; // ricetta trovata
                i++;
                index = quadratic_probing(index, i);
            }

            return 99999; //ricetta non presente


        case(1):    //AGGIUNGI RICETTA

            if(hash_table_ricette[index] == NULL)
                return index;

            while (hash_table_ricette[index] != NULL && i < HASH_SIZE) {                                                //verifico l'effettiva assenza in tabella

                if(firstDelete && hash_table_ricette[index] == DELETED) {                                               //salvo la prima cella deleted (disponibile)
                    firstDelete--;
                    deleted_index = index;
                }

                if (hash_table_ricette[index] != DELETED && !strcomp2(hash_table_ricette[index]->nome, scan))
                    return 99999; // è già presente
                i++;
                index = quadratic_probing(index, i);
            }

            if(firstDelete && hash_table_ricette[index] == NULL)
                return index;
            else if(!firstDelete)
                return deleted_index;
            else
                return 100000; //errore o tabella piena


        default:
            puts("errore");
            return 0;
    }
}





void aggiungi_ricetta(char *scan){

    if(scan == NULL){
        puts("ERRRORE PUNTATORE SCAN IN AGGIUNGI RICETTA");
        return;
    }

    uint32_t index = search_ricetta(scan, 1);

    if(index == 99999){
        pulizia_buffer();
        puts("ignorato");
        return;
    }

    if(index == 100000) {
        pulizia_buffer();
        puts("errore in aggiungi-ricetta");
        return;
    }

    //allocazione memoria della singola cella in hash_table
    hash_table_ricette[index] = malloc(sizeof(ricetta));
    hash_table_ricette[index]->lista_ingredienti = NULL;
    strcpy(hash_table_ricette[index]->nome, scan);
    hash_table_ricette[index] -> peso = 0;
    hash_table_ricette[index]->lista_ingredienti = aggiungi_ingrediente(scan, index);    //lega la LISTA degli ingredienti associati al dolce
    puts("aggiunta");

}




ingrediente* aggiungi_ingrediente(char *scan, uint32_t index){

    if(scan == NULL){
        puts("ERRRORE PUNTATORE SCAN IN AGGIUNGI INGREDIENTE");
        return NULL;
    }

    int quant;
    ingrediente *head = NULL;

    while(*scan != '\n') {
        lettura_nome(scan);
        if(scanf("%d", &quant) != 1)
            return NULL;

        ingrediente *new = malloc(sizeof(ingrediente));

        //strcpy(new->nome, scan);
        new -> key = key_generator(scan);
        new->qnt = quant;
        hash_table_ricette[index] -> peso += new -> qnt;
        new->next = head;
        head = new;

        *scan = getchar_unlocked();
    }

    return head;
}




void rimuovi_ricetta(char *scan){

    uint32_t index = search_ricetta(scan, 0);

    if(index == 99999){
        puts("non presente");
        return;
    }

    if( (prontaConsegna_search(index) || sospesi_search(index)) ){
        puts("ordini in sospeso");
        return;
    }

    cancella_ingrediente(index);
    free(hash_table_ricette[index]);
    hash_table_ricette[index] = DELETED;
    puts("rimossa");

}




void cancella_ingrediente(uint32_t index){

    ingrediente *curr = hash_table_ricette[index]->lista_ingredienti;
    ingrediente *tmp = NULL;

    if (curr == NULL)
        return;

    while (curr != NULL) {
        tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    hash_table_ricette[index]->lista_ingredienti = NULL;
}






//minHeapify
void prontaConsegna_Heapify(int i){

    if (prontaConsegna -> size <= 1)
        return;

    int left = 2*i + 1;
    int right = 2*i + 2;

    int min = i;

    if (left < prontaConsegna -> size && prontaConsegna -> ordini[left].t < prontaConsegna -> ordini[i].t)
        min = left;

    if (right < prontaConsegna -> size && prontaConsegna -> ordini[right].t < prontaConsegna -> ordini[min].t)
        min = right;

    if (min != i)
    {
        int tempT = prontaConsegna -> ordini[i].t;
        int tempNum = prontaConsegna -> ordini[i].num;
        int tempPeso = prontaConsegna -> ordini[i].pesoOrdine;
        char tempNome[256]; strcpy(tempNome, prontaConsegna -> ordini[i].nome);

        prontaConsegna -> ordini[i].t = prontaConsegna -> ordini[min].t;
        prontaConsegna -> ordini[i].num = prontaConsegna -> ordini[min].num;
        prontaConsegna -> ordini[i].pesoOrdine = prontaConsegna -> ordini[min].pesoOrdine;
        strcpy(prontaConsegna -> ordini[i].nome, prontaConsegna -> ordini[min].nome);

        prontaConsegna -> ordini[min].t = tempT;
        prontaConsegna -> ordini[min].num = tempNum;
        prontaConsegna -> ordini[min].pesoOrdine = tempPeso;
        strcpy(prontaConsegna -> ordini[min].nome, tempNome);

        prontaConsegna_Heapify(min);
    }
}





void prontaConsegna_delete(){
    if (!prontaConsegna || prontaConsegna -> size == 0)
        return;

    prontaConsegna -> ordini[0].t = prontaConsegna -> ordini[prontaConsegna -> size-1].t;;
    prontaConsegna -> ordini[0].num = prontaConsegna -> ordini[prontaConsegna -> size-1].num;
    prontaConsegna -> ordini[0].pesoOrdine = prontaConsegna -> ordini[prontaConsegna -> size-1].pesoOrdine;
    if(prontaConsegna -> size > 1)
        strcpy(prontaConsegna -> ordini[0].nome, prontaConsegna -> ordini[prontaConsegna -> size-1].nome);


    prontaConsegna -> size--;

    prontaConsegna_Heapify(0);
}





//minHeap insert
void prontaConsegna_insert(char *nome, int num, int peso, int inListaDiAttesa, int tArrivo){

    if (prontaConsegna -> size == 11000) {
        puts("ERRORE INSERIMENTO HEAP PRONTA COSEGNA");
        return;
    }

    prontaConsegna -> size++;

    if(!inListaDiAttesa)
        prontaConsegna -> ordini[prontaConsegna -> size -1].t = TEMPO;
    else
        prontaConsegna -> ordini[prontaConsegna -> size -1].t = tArrivo;
    strcpy(prontaConsegna -> ordini[prontaConsegna -> size -1].nome, nome);
    prontaConsegna -> ordini[prontaConsegna -> size -1].num = num;
    prontaConsegna -> ordini[prontaConsegna -> size -1].pesoOrdine = peso;

    int i = prontaConsegna -> size - 1;

    while (i > 0 && prontaConsegna -> ordini[(i-1)/2].t > prontaConsegna -> ordini[i].t) {
        // Swap
        int tempT = prontaConsegna -> ordini[(i-1)/2].t;
        int tempNum = prontaConsegna -> ordini[(i-1)/2].num;
        int tempPeso = prontaConsegna -> ordini[(i-1)/2].pesoOrdine;
        char tempNome[256]; strcpy( tempNome, prontaConsegna -> ordini[(i-1)/2].nome );

        prontaConsegna -> ordini[(i-1)/2].t = prontaConsegna -> ordini[i].t;
        prontaConsegna -> ordini[(i-1)/2].num = prontaConsegna -> ordini[i].num;
        prontaConsegna -> ordini[(i-1)/2].pesoOrdine = prontaConsegna -> ordini[i].pesoOrdine;
        strcpy( prontaConsegna -> ordini[(i-1)/2].nome, prontaConsegna -> ordini[i].nome );


        prontaConsegna -> ordini[i].t = tempT;
        prontaConsegna -> ordini[i].num = tempNum;
        prontaConsegna -> ordini[i].pesoOrdine = tempPeso;
        strcpy( prontaConsegna -> ordini[i].nome, tempNome );

        i = (i-1)/2;
    }
}







void confermaOrdine(char *scan){

    if(scan == NULL){
        puts("ERRORE PUNTATORE SCAN IN CONFERMAORDINE");
        return;
    }

    uint32_t index; //indice ricetta
    int num;        //quantità di dolci richiesta
    if(scanf("%d", &num) != 1)
        return;

    index = search_ricetta(scan, 0);

    if(index == 99999){
        *scan = getchar_unlocked();
        puts("rifiutato");
        return;
    }

    preparaOrdine(index, num, 0, 0);

    *scan = getchar_unlocked();
    puts("accettato");
}





int prontaConsegna_search(uint32_t index){

    int i = 0;
    if(prontaConsegna -> size == 0)
        return 0;

    while(i < prontaConsegna -> size){
        if(!strcomp2(hash_table_ricette[index] -> nome, prontaConsegna -> ordini[i].nome))
            return 1;
        i++;
    }

    return 0;
}






int sospesi_search(uint32_t index){

    nodo *curr = ordini_sospesi->testa;

    if(curr == NULL)
        return 0;

    while (curr != NULL) {
        if(!strcomp2(hash_table_ricette[index] -> nome, curr -> ordine_sospeso.nome))
            return 1;
        curr = curr->next;
    }

    return 0;
}





void sospesi_insert(uint32_t index, int num){

    nodo *nuovo = malloc(sizeof(nodo));
    if (nuovo == NULL)
        return;

    nuovo -> ordine_sospeso.index = index;
    nuovo -> ordine_sospeso.t = TEMPO;
    nuovo -> ordine_sospeso.num = num;
    nuovo -> ordine_sospeso.pesoOrdine = hash_table_ricette[index] -> peso * num;
    strcpy(nuovo -> ordine_sospeso.nome, hash_table_ricette[index] -> nome);
    nuovo -> next = NULL;

    if (ordini_sospesi -> coda != NULL)             //se esista già una coda, nuovo è il next della coda
        ordini_sospesi -> coda -> next = nuovo;

    ordini_sospesi -> coda = nuovo;                 //aggiorno la coda

    if (ordini_sospesi -> testa == NULL)            //se vuota, testa punta a nuovo
        ordini_sospesi -> testa = nuovo;

}




void sospesi_delete(){

    nodo *curr = ordini_sospesi -> testa;
    nodo *precedente = NULL;

    while (curr != NULL) {
        uint32_t index_ricetta = curr -> ordine_sospeso.index;

        if(preparaOrdine(index_ricetta, curr -> ordine_sospeso.num, 1, curr -> ordine_sospeso.t)){
            nodo *daRimuovere = curr;

            if(precedente == NULL){
                // Il nodo da rimuovere è la testa
                ordini_sospesi->testa = curr->next;
                if (ordini_sospesi->testa == NULL) {
                    ordini_sospesi->coda = NULL;
                }
                curr = curr->next;
            }
            else{
                precedente->next = curr->next;

                if(curr->next == NULL)
                    ordini_sospesi->coda = precedente;

                curr = curr->next;
            }

            free(daRimuovere);
        }
        else{
            precedente = curr;
            curr = curr->next;
        }
    }
}



void swap(ordine *a, ordine *b) {
    ordine t = *a;
    *a = *b;
    *b = t;
}


int partition(int p, int r) {
    ordine x = spedizioneOrdini[r];
    int i = p - 1;
    for (int j = p; j <= r - 1; j++) {
        if (spedizioneOrdini[j].pesoOrdine > x.pesoOrdine) {
            i++;
            swap(&spedizioneOrdini[i], &spedizioneOrdini[j]);
        }
        else if(spedizioneOrdini[j].pesoOrdine == x.pesoOrdine && spedizioneOrdini[j].t < x.t) {
            i++;
            swap(&spedizioneOrdini[i], &spedizioneOrdini[j]);
        }
    }
    swap(&spedizioneOrdini[i + 1], &spedizioneOrdini[r]);
    return i + 1;
}




void quickSort(int p, int r){
    if (p < r) {
        int q = partition(p, r);
        quickSort(p, q - 1);
        quickSort(q + 1, r);
    }
}





void caricaFurgone(){

    int ammontare = 0;
    int iSpedizione = 0;

    if(prontaConsegna -> size > 0) {

        while (ammontare <= CAPIENZA && prontaConsegna -> size > 0) {
            ammontare += prontaConsegna -> ordini[0].pesoOrdine;

            if(ammontare > CAPIENZA)
                break;

            spedizioneOrdini[iSpedizione].t = prontaConsegna -> ordini[0].t;
            spedizioneOrdini[iSpedizione].num = prontaConsegna -> ordini[0].num;
            spedizioneOrdini[iSpedizione].pesoOrdine = prontaConsegna -> ordini[0].pesoOrdine;
            strcpy(spedizioneOrdini[iSpedizione].nome, prontaConsegna -> ordini[0].nome);

            prontaConsegna_delete();
            iSpedizione++;
        }
    }
    else{
        puts("camioncino vuoto");
        return;
    }

    quickSort(0, iSpedizione - 1);
    spedisci(iSpedizione);
}





void spedisci(int size){
    for (int i = 0; i < size; i++) {
        printf("%d %s %d\n", spedizioneOrdini[i].t, spedizioneOrdini[i].nome, spedizioneOrdini[i].num);
    }
}





int lettura_comando(char *scan){

    if(scan == NULL){
        puts("ERRRORE PUNTATORE SCAN IN LETTURA COMANDO");
        return 0;
    }

    int i;
    char c;

    i = 0;
    c = getchar_unlocked();
    while (c != ' ' && c != EOF) {
        *(scan+i) = c;
        i++;
        c = getchar_unlocked();
    }

    if (c == EOF) return 0;

    *(scan+i) = '\0';
    return 1;
}




void lettura_nome(char *scan){

    if(scan == NULL){
        puts("ERRRORE PUNTATORE SCAN IN LETTURA NOME");
        return;
    }

    int i;
    char c;

    i = 0;
    c = getchar_unlocked();
    while (c != ' ' && c != '\n') {     //se non funziona aggiungere un getchar in rimuovi ricetta e togliere la condizione '\n' qui
        *(scan+i) = c;
        i++;
        c = getchar_unlocked();
    }
    *(scan+i) = '\0';
}





void pulizia_buffer(){
    char c;
    c = getchar_unlocked();
    while (c != '\n' && c != EOF)
        c = getchar_unlocked();
}



void init_globali(){

    ROOT = NULL;

    prontaConsegna = malloc(sizeof(heapOrdini));
    if (prontaConsegna)
        prontaConsegna->size = 0;

    ordini_sospesi = malloc(sizeof(lista));
    if (ordini_sospesi)
        ordini_sospesi->testa = ordini_sospesi->coda = NULL;

    TEMPO = -1;
}

void free_globali(){

    for (int i = 0; i < HASH_SIZE; i++) {
        if (hash_table_ricette[i] != NULL && hash_table_ricette[i] != DELETED) {
            ingrediente *curr = hash_table_ricette[i]->lista_ingredienti;
            while (curr != NULL) {
                ingrediente *temp = curr;
                curr = curr->next;
                free(temp);
            }
            free(hash_table_ricette[i]);
        }
    }

    freeTree(ROOT);

    free(prontaConsegna);
    while (ordini_sospesi->testa != NULL) {
        nodo *temp = ordini_sospesi->testa;
        ordini_sospesi->testa = ordini_sospesi->testa->next;
        free(temp);
    }
    free(ordini_sospesi);
}



int strcomp2(char *str1, char *str2) {

    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return 1; // Le stringhe sono diverse
        }
        str1++;
        str2++;
    }
    // Se uno dei due è terminato e l'altro no, sono diverse
    return (*str1 != *str2);
}




void freeTree(TreeNode* root) {
    if (root == NULL)
        return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}



TreeNode* create_node(uint32_t key) {
    TreeNode* new = (TreeNode*)malloc(sizeof(TreeNode));

    new -> key = key;
    new -> ingrediente.heapSize = 1;
    if(scanf("%d %d", &new->ingrediente.MinHeap[0].qnt, &new->ingrediente.MinHeap[0].scadenza) != 2) return NULL;
    new -> ingrediente.qntTot = new->ingrediente.MinHeap[0].qnt;
    new -> left = NULL;
    new -> right = NULL;

    return new;
}



//Inserimento nell'albero (magazzino) del nuovo tipo di ingrediente
TreeNode* TreeInsert(TreeNode* root, uint32_t key) {
    if (root == NULL)
        return create_node(key);

    if (key < root->key)
        root->left = TreeInsert(root->left, key);
    else if (key > root->key)
        root->right = TreeInsert(root->right, key);

    return root;
}











void Min_Heapify(TreeNode *nodo_ing, int size, int i) {

    if (size <= 1)
        return;

    int left = 2*i + 1;
    int right = 2*i + 2;

    int minore = i;

    if (left < size && nodo_ing -> ingrediente.MinHeap[left].scadenza < nodo_ing -> ingrediente.MinHeap[i].scadenza)
        minore = left;

    if (right < size && nodo_ing -> ingrediente.MinHeap[right].scadenza < nodo_ing -> ingrediente.MinHeap[minore].scadenza)
        minore = right;

    if (minore != i)
    {
        int tmpSca = nodo_ing -> ingrediente.MinHeap[i].scadenza;
        int tmpQnt = nodo_ing -> ingrediente.MinHeap[i].qnt;

        nodo_ing -> ingrediente.MinHeap[i].scadenza = nodo_ing -> ingrediente.MinHeap[minore].scadenza;
        nodo_ing -> ingrediente.MinHeap[i].qnt = nodo_ing -> ingrediente.MinHeap[minore].qnt;

        nodo_ing -> ingrediente.MinHeap[minore].scadenza = tmpSca;
        nodo_ing -> ingrediente.MinHeap[minore].qnt = tmpQnt;

        Min_Heapify(nodo_ing, size, minore);
    }
}




// passa il nodo dell'albero in cui è situato l'ingrediente
// e cancella il primo dall'heap con i lotti

void MinHeap_delete(TreeNode *nodo_ing){
    // Deletes the minimum element, at the root
    if (nodo_ing -> ingrediente.heapSize == 0)
        return;

    //sottraggo la qnt del lotto scaduto alla qnt totale
    nodo_ing -> ingrediente.qntTot -= nodo_ing -> ingrediente.MinHeap[0].qnt;

    //inserisco i dati dell'ultima cella nella prima
    nodo_ing -> ingrediente.MinHeap[0].scadenza = nodo_ing -> ingrediente.MinHeap[nodo_ing -> ingrediente.heapSize -1].scadenza;
    nodo_ing -> ingrediente.MinHeap[0].qnt = nodo_ing -> ingrediente.MinHeap[nodo_ing -> ingrediente.heapSize -1].qnt;

    nodo_ing -> ingrediente.heapSize--;

    Min_Heapify(nodo_ing, nodo_ing -> ingrediente.heapSize, 0);
}





void MinHeap_insert(TreeNode *pnt){
    if(pnt -> ingrediente.heapSize == HASH_SIZE){
        puts("::::::::::::ERRORE HEAP ingredienti PIENO::::::::::::::::::");
        return;
    }

    pnt -> ingrediente.heapSize += 1;
    int i = pnt -> ingrediente.heapSize - 1;

    if(scanf("%d %d", &pnt -> ingrediente.MinHeap[i].qnt,   &pnt -> ingrediente.MinHeap[i].scadenza) != 2) return;


    //Aggiungo qnt del nuovo lotto alla qnt totale dei lotti
    pnt -> ingrediente.qntTot += pnt -> ingrediente.MinHeap[i].qnt;

    while( i>0 && (pnt -> ingrediente.MinHeap[(i-1)/2].scadenza > pnt -> ingrediente.MinHeap[i].scadenza) ){

        // Swap di qnt e scadenza tra padre e figlio
        int tmpSca = pnt -> ingrediente.MinHeap[(i-1)/2].scadenza;
        int tmpQnt = pnt -> ingrediente.MinHeap[(i-1)/2].qnt;

        pnt -> ingrediente.MinHeap[(i-1)/2].scadenza = pnt -> ingrediente.MinHeap[i].scadenza;
        pnt -> ingrediente.MinHeap[(i-1)/2].qnt = pnt -> ingrediente.MinHeap[i].qnt;

        pnt -> ingrediente.MinHeap[i].scadenza = tmpSca;
        pnt -> ingrediente.MinHeap[i].qnt = tmpQnt;

        // Update i
        i = (i-1)/2;
    }
}





//ricerca nell'albero
TreeNode* search_tipo_lotto(TreeNode * root, uint32_t key){

    // Se l'albero è vuoto o il valore è trovato
    if (root == NULL || root->key == key) {
        return root;
    }

    // Il valore è minore della radice, cerca nel sottoalbero sinistro
    if (key < root->key) {
        return search_tipo_lotto(root->left, key);
    }

    // Altrimenti, cerca nel sottoalbero destro
    return search_tipo_lotto(root->right, key);

}




void rifornisci_magazzino(char *scan){

    if(scan == NULL){
        puts("ERRRORE PUNTATORE SCAN IN RIFORNISCI MAGAZZINO");
        return;
    }

    while(*scan != '\n') {

        lettura_nome(scan);
        //non ho ancora letto i valori di scadenza e quantità

        uint32_t key = key_generator(scan);
        TreeNode *pnt = search_tipo_lotto(ROOT, key);

        if(pnt == NULL){
            ROOT = TreeInsert(ROOT, key);  //tipo non presente in magazzino
        }
        else{
            MinHeap_insert(pnt);       //Tipo presente in magazzino
        }

        *scan = getchar_unlocked(); //legge '\n' nel caso dell'ultimo int e consuma lo spazio nel caso di un generico secondo int
    }

    puts("rifornito");
}











int verificaRisorse(ingrediente *pnt, int num){
    int flag = 1;

    //rimozione lotti scaduti + verifica quantità sufficiente per procedere alla preparazione
    while (pnt != NULL) {
        //nodo dell'albero in cui è presente l'ingrediente
        TreeNode *nodo_ing = search_tipo_lotto(ROOT, pnt ->key);

        if(nodo_ing == NULL || nodo_ing -> ingrediente.heapSize == 0){
            flag = 0;
            break;
        }
        else{
            while(nodo_ing -> ingrediente.MinHeap[0].scadenza <= TEMPO && nodo_ing -> ingrediente.heapSize > 0)
                MinHeap_delete(nodo_ing);

            //controllo che la somma delle quantità dei lotti sia sufficiente a coprire la preparazione
            //pnt -> qnt = quantità dell'ingrediente per un singolo dolce
            if(nodo_ing -> ingrediente.qntTot < pnt -> qnt * num) {
                flag = 0;
                break;
            }
        }

        // Passa all'ingrediente successivo
        pnt = pnt->next;
    }

    //risorse disponibili se flag rimane 1
    return flag;
}










int preparaOrdine(uint32_t index_ricetta, int num, int inListadiAttesa, int tArrivo){

    int prelievoQnt, pesoOrdine;
    ingrediente *pnt = hash_table_ricette[index_ricetta] -> lista_ingredienti;

    if(!verificaRisorse(pnt, num)){
        if(!inListadiAttesa)
            sospesi_insert(index_ricetta, num); //aggiungo in lista ordini sospesi
        return 0;
    }

    //procedo alla preparazione, utilizzo delle quantità richieste dalla ricetta e diminuzione delle scorte
    pnt = hash_table_ricette[index_ricetta] -> lista_ingredienti;
    while (pnt != NULL) {

        prelievoQnt = 0;
        TreeNode *nodo_ing = search_tipo_lotto(ROOT, pnt -> key);

        if (nodo_ing -> ingrediente.MinHeap[0].qnt > pnt -> qnt * num){

            nodo_ing -> ingrediente.MinHeap[0].qnt -= pnt -> qnt * num;
            nodo_ing -> ingrediente.qntTot -= pnt -> qnt * num;
        }
        else if (nodo_ing -> ingrediente.MinHeap[0].qnt == pnt -> qnt * num){
            MinHeap_delete(nodo_ing);
        }
        else if (nodo_ing -> ingrediente.MinHeap[0].qnt < pnt -> qnt * num){

            prelievoQnt += nodo_ing -> ingrediente.MinHeap[0].qnt;
            MinHeap_delete(nodo_ing);
            int differenza = pnt -> qnt * num - prelievoQnt;

            while(differenza > 0){

                if( nodo_ing -> ingrediente.MinHeap[0].qnt > differenza){
                    nodo_ing -> ingrediente.MinHeap[0].qnt -= differenza;
                    nodo_ing -> ingrediente.qntTot -= differenza;
                    differenza = 0;
                }
                else if(nodo_ing -> ingrediente.MinHeap[0].qnt == differenza){
                    MinHeap_delete(nodo_ing);
                    differenza = 0;
                }
                else if(nodo_ing -> ingrediente.MinHeap[0].qnt < differenza){
                    differenza -= nodo_ing -> ingrediente.MinHeap[0].qnt;
                    MinHeap_delete(nodo_ing);
                }
            }
        }
        pnt = pnt->next;
    }

    pesoOrdine = hash_table_ricette[index_ricetta] -> peso * num;

    if(!inListadiAttesa)
        prontaConsegna_insert(hash_table_ricette[index_ricetta] -> nome, num, pesoOrdine, 0, 0);
    else
        prontaConsegna_insert(hash_table_ricette[index_ricetta] -> nome, num, pesoOrdine, 1, tArrivo);

    return 1;   //ordine preparatoe inserito in pronta consegna

}
