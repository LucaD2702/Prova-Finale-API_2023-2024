<br />
<p align="center">
  <a>
    <img width="250px" src="https://github.com/LucaD2702/Algoritmi-e-Strutture-Dati_Prova-Finale_2023-2024/blob/main/assets/logo-light.svg" alt="Logo PoliMi">
  </a>

  <h3 align="center">Prova finale di Algoritmi e Strutture Dati</h3>

  <p align="center">
    Politecnico di Milano - A.A. 2023-2024
    <br />
    Gestione Pasticceria Industriale
  </p>
</p>

## Introduzione
Il progetto consiste nello sviluppo di un software in C per la gestione automatizzata di una pasticceria industriale, basato su una **simulazione a tempo discreto**. Ogni operazione o comando ricevuto comporta l'avanzamento del tempo di un istante. 

L'obiettivo è orchestrare la produzione di dolci, la gestione delle scorte e la logistica delle spedizioni in modo efficiente, rispettando i vincoli temporali e di inventario.

## Logica del Sistema

### 1. Gestione Ingredienti e Scadenze
* Il magazzino stocca ogni ingrediente in lotti, ognuno caratterizzato da una quantità e una data di scadenza.
* Per minimizzare gli sprechi, la preparazione dei dolci preleva sempre gli ingredienti dai lotti con la scadenza più prossima.
* Se un lotto raggiunge la data di scadenza, non può più essere utilizzato per alcuna ricetta.

### 2. Ciclo di Produzione e Ordini
* Le ricette definiscono le quantità necessarie di ogni ingrediente.
* Se le scorte sono sufficienti, l'ordine viene evaso istantaneamente.
* In caso di scorte insufficienti, l'ordine entra in una **coda di attesa FIFO**.
* Ogni nuovo rifornimento sblocca il controllo automatico della coda per evadere gli ordini in sospeso.

### 3. Logistica e Spedizione
* Un corriere arriva periodicamente (ogni *n* istanti) con una capacità di carico massima espressa in grammi.
* Gli ordini vengono scelti per il carico seguendo l'ordine cronologico di arrivo.
* Una volta scelti, gli ordini vengono caricati sul camioncino in **ordine di peso decrescente** per ottimizzare lo spazio; a parità di peso, si segue l'ordine cronologico.

## Strutture Dati e Scelte Implementative

Per rispettare i vincoli di tempo e memoria richiesti dalla simulazione, il progetto fa uso di diverse strutture dati ottimizzate per specifici compiti:

### Catalogo Ricette (Hash Table)
Le ricette sono archiviate in una **Hash Table** con risoluzione delle collisioni tramite *probing quadratico*.
Permette una ricerca in tempo medio $O(1)$ partendo dal nome della ricetta (fino a 255 caratteri).


### Magazzino Ingredienti (BST + Min-Heap)
La gestione del magazzino è la parte più complessa, poiché richiede di trovare rapidamente un ingrediente e, contemporaneamente, estrarre sempre il lotto che scade prima.
* **Albero Binario di Ricerca (BST):** Ogni nodo dell'albero rappresenta un tipo di ingrediente (es. "farina"). La chiave del nodo è un intero a 32-bit generato dal nome dell'ingrediente. Questo garantisce una ricerca efficiente, $O(\log N)$ in media.
* **Min-Heap (Lotti):** All'interno di ogni nodo del BST, i lotti di quell'ingrediente sono memorizzati in un *Min-Heap* basato sulla data di scadenza. Questo garantisce che il lotto con la scadenza più prossima sia sempre in radice, estraibile in $O(\log M)$ (dove $M$ è il numero di lotti).

### Gestione Ordini (Code e Min-Heap)
Il ciclo di vita di un ordine attraversa diverse strutture:
* **Ordini Sospesi (Linked List):** Gli ordini che non possono essere preparati subito vengono inseriti in coda a una lista concatenata, garantendo la logica FIFO $O(1)$ per l'inserimento. Ad ogni rifornimento viene verificato se è possibile preparare uno (o più) ordini inseriti in lista d'attesa.
* **Ordini in Pronta Consegna (Min-Heap):** I dolci preparati vengono inseriti in un Min-Heap ordinato per **tempo di arrivo dell'ordine**. Questo assicura che il corriere carichi sempre gli ordini meno recenti per primi.
* **Caricamento Furgone (QuickSort):** Prima della spedizione, gli ordini prelevati dal Min-Heap vengono ordinati tramite algoritmo *QuickSort* in base al loro **peso decrescente**, come richiesto dalle specifiche, per ottimizzare lo spazio sul camioncino.

## Comandi supportati
Il programma supporta i seguenti comandi via standard input:
* `aggiungi_ricetta` / `rimuovi_ricetta`
* `rifornimento`
* `ordine`
