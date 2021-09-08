//#include <vosk_api.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DIM_MAX_COMANDO 3200

#define HOT_WORD_A "okay" //prima parte della hotword
#define HOT_WORD_B "robot" //seconda parte della hotword
#define DIVISORE_PIU_COMANDI "and" //divisore per distinguere più comandi
#define NUMBER_REPLACE "X" //carattere utilizzato per definire che in quel punto del comando ci possono essere dei numeri

#define DIVISORE ";" //divisore del dizionario

FILE *dizionario;
FILE *lista_comandi;
FILE *numbers;
char riga_dizionario[DIM_MAX_COMANDO];

void comanda_robot(char *comando);
char *get_hotword_position(char *comando);
void decifra_e_invia_comando(char *token);
void invia_comando(char *comando);
void ricostruisci_comando(char* comando, char* token);
void invia_codice(int codice);
int estrai_codice(char *comando);
int get_numero_comandi_innestati(char* comando_sentito_senza_hotword);
void distributore_comandi_innestati(char *comando_singolo, char *originale, int numero_comando_da_generare, int numero_comandi_innestati);
bool is_X_nel_comando(char *comando);
int confronto_comando_X_dizionario(char *comando_sentito_senza_hotword,char *comando_permesso);
int turn_X_into_int(char *token_dopo_la_X, char *token_comando, char *rest_c);

long get_posizione_divisore_dizionario(char* stop);
int get_numero_elementi_divisore(long posizione_inizio_divisore, long posizione_fine_divisore);
void salva_comando_su_file(char *comando, FILE *ptr_file);
void crea_listacomandi();
int file_is_modified(const char *path, time_t oldMTime);

int main() {
    FILE *wavin; //puntatore a file
    char buf[DIM_MAX_COMANDO]; //inizio lettura file
    int nread, final;
    const char *temp; //temporaneo per il risultato resituito dal modello
    char comando[DIM_MAX_COMANDO] = "ok robot turn one hundred and fourty one degrees and face left twenty six degrees and go to the door";
    //struct json_object *parsed_json;
    //struct json_object *testo;

    struct stat file_stat;
    char path[] = "comando";
    stat(path, &file_stat);
    //int err = stat(path, &file_stat);
    time_t tempoModifica = file_stat.st_mtime;

    FILE *F_comando;


    crea_listacomandi();
    //comanda_robot(comando);

    while(1){
        if(file_is_modified(path, tempoModifica)){
            stat(path, &file_stat);
            tempoModifica = file_stat.st_mtime;
            printf("FILE MODIFICATO\n");
            F_comando = fopen(path, "r");
            fgets(comando, DIM_MAX_COMANDO, F_comando);
            printf("%s\n", comando);
            //comando[strlen(comando)-1] = '\0';
            comanda_robot(comando);
            fclose(F_comando);

           
        }

        //printf("controllo file\n");
        sleep(3);
    }
    
}

/* controlla se il file è stato modificato */
int file_is_modified(const char *path, time_t oldMTime) {
    struct stat file_stat;
    int err = stat(path, &file_stat);
    if (err != 0) {
        //perror(" [file_is_modified] stat");
        //exit(errno);
    }
    return file_stat.st_mtime > oldMTime;
}

/* funzione che a partire dal file 'dizionario' crea il file 'lista comandi' */
void crea_listacomandi(){
    char action[DIM_MAX_COMANDO];
    char parameter[DIM_MAX_COMANDO];
    char temp[DIM_MAX_COMANDO];
    
    int c_action; //contatore di elementi nel divisore 'inter'
    int c_parameters; //contatore di elementi nel divisore 'destinations'

    long posizione_action;
    long posizione_parameters;
    long posizione_inizio_famiglia;
    const long posizione_fine_famiglia = -1;

    char stop_action[] = "--action--";
    char stop_parameters[] = "--parameters--";
    char stop_famiglia[] = "<";
    char stop_END[] = "<END>";
 
    /* apro il dizionario */
    dizionario = fopen("dizionario", "r"); //assoccio il file al puntatore 
    if(dizionario == NULL){
        perror("Could not open dizionario \n");
        return;
    }
    fgets(riga_dizionario, DIM_MAX_COMANDO, dizionario); //leggo la prima linea del file
    lista_comandi = fopen("lista_comandi","w");
    

    while(strncmp(riga_dizionario, stop_END, sizeof(stop_END)-1) != 0){

        /* trova le posizioni dei divisori */
        fflush(dizionario);
        posizione_inizio_famiglia = ftell(dizionario);
        posizione_action = get_posizione_divisore_dizionario(stop_action);
        posizione_parameters = get_posizione_divisore_dizionario(stop_parameters);

        /* numero di elementi per divisore */
        c_action = get_numero_elementi_divisore(posizione_action, posizione_parameters) ; 
        c_parameters = get_numero_elementi_divisore(posizione_parameters, posizione_fine_famiglia);
        
        /* creo un array di stringhe e lo popolo con le combinazioni di comandi a partire dal dizionario */
        
        for(int i = 0; i < c_action; i++){

            /* leggo la action da associare con i parameters */
            fseek(dizionario, posizione_action, SEEK_SET); //posiziono il puntatore sulla sezione action
            for(int k = c_action - i ; k <= c_action; k++){
                fgets(action, DIM_MAX_COMANDO, dizionario);
            }
            
            /* associo alla action ogni parameters e li scrivo sul file */
            
            fseek(dizionario, posizione_parameters, SEEK_SET); 
            if(c_parameters != 0){
                for(int j = 0; j < c_parameters; j++){
                    strcpy(temp, action);
                    fgets(parameter, DIM_MAX_COMANDO, dizionario);
                    strncat(temp, parameter, sizeof(parameter));
                    for(int f = 0; f < DIM_MAX_COMANDO; f++){
                        if(temp[f] == '\n'){
                            temp[f] = ' ';
                            break;
                        }
                    }
                    salva_comando_su_file(temp, lista_comandi);
                }
            } else {
                salva_comando_su_file(action, lista_comandi);
            }
        }
    }
    fclose(lista_comandi);
    fclose(dizionario);
}

/* a partire da due posizioni in un file conta il numero di righe che le separano */
int get_numero_elementi_divisore(long posizione_inizio_divisore, long posizione_fine_divisore){
    
    int numero_elementi_divisore = 0;
    fflush(dizionario);
    
    if(fseek(dizionario, posizione_inizio_divisore, SEEK_SET)!=0){ //posiziono il puntatore all'inizio del divisore
        perror("Could not go to divisor \n");
    }

    if(posizione_fine_divisore == -1){
        fgets(riga_dizionario, DIM_MAX_COMANDO, dizionario); //mi posiziono alla riga successiva dell'intestazione del divisore
        while(riga_dizionario[0] != '<'){
            numero_elementi_divisore++;
            fgets(riga_dizionario, DIM_MAX_COMANDO, dizionario); //vado alla riga successiva
        }
    } else {
        fgets(riga_dizionario, DIM_MAX_COMANDO, dizionario); //mi posiziono alla riga successiva dell'intestazione del divisore
        while(ftell(dizionario) != posizione_fine_divisore){
            numero_elementi_divisore++;
            fgets(riga_dizionario, DIM_MAX_COMANDO, dizionario); //vado alla riga successiva
        }
    }

    return numero_elementi_divisore; 
}

/* a partire da una stringa restituisce la posizione nel file in cui è presente la stringa */
long get_posizione_divisore_dizionario(char* stop){

    while(strncmp(riga_dizionario, stop, strlen(stop)) != 0){
        fgets(riga_dizionario, DIM_MAX_COMANDO, dizionario); //leggo la linea della sezione verbs
    }
    fflush(dizionario);
    return ftell(dizionario);

}

/* a partire da un comando lo salva sul file passato dal puntatore */
void salva_comando_su_file(char *comando, FILE *ptr_file){

    fflush(ptr_file);
    fputs(comando, ptr_file);
    
}

/* Funzione che prende in ingresso il comando sentito e, se fa parte di quelli permessi, lo invia al robot */
void comanda_robot(char *comando){
   
    char *token = get_hotword_position(comando);

    if(token == NULL){
        printf("HOTWORD NON TROVATA \n");
        
    } else {
        printf("HOTWORD TROVATA \n");

        decifra_e_invia_comando(token);
    }

}

/* Funzione che cerca la HOTWORD e restituisce la posizione nella frase dove viene detta.
    restituisce:
    - token dove è stata trovata la hotword
    - NULL se non è stata trova la hotword
*/
char *get_hotword_position(char *comando){

    /* inizializzazione variabili */
    bool hot_word_detected = FALSE; //booleano per verificare se è stata individuata la hotword
    char hot_A[] = HOT_WORD_A;            
    char hot_B[] = HOT_WORD_B;            
    char *token = strtok(comando, " "); //inizializzazione del tokenizer

    
    /* ricerca della hotword */
    while(token != NULL){
        if(strncmp(token, hot_A, strlen(token)) == 0){
            hot_word_detected = TRUE;
        } else if(hot_word_detected){
            if(strncmp(token, hot_B, strlen(token)) == 0){
                token = strtok(NULL, " ");
                return token;
            } else {
                hot_word_detected = FALSE;
            }
        }
        token = strtok(NULL, " ");
    }
    return token;
}

/* Prende in ingresso la posizione della hotword (sotto forma di token) e confronta il comando sentito con tutti i comandi permessi dal file 'lista_comandi' */
void decifra_e_invia_comando(char *token){

    char comando_sentito_senza_hotword[DIM_MAX_COMANDO];
    char comando_permesso[DIM_MAX_COMANDO];
    int codice_comando;
    char originale[DIM_MAX_COMANDO];
    char temp[DIM_MAX_COMANDO];
    char temp2[DIM_MAX_COMANDO];
    bool flag_X_nel_comando = false;
    int X;
    

    char divisore[] = DIVISORE;
    FILE *lista_comandi = fopen("lista_comandi","r");
    comando_sentito_senza_hotword[0] = '\0';
    ricostruisci_comando(comando_sentito_senza_hotword, token);
    strcpy(originale, comando_sentito_senza_hotword);    

    int numero_comandi_innestati = get_numero_comandi_innestati(originale);
    //printf("%s\n",comando_sentito_senza_hotword);
    strcpy(originale, comando_sentito_senza_hotword);
    strcpy(temp, comando_sentito_senza_hotword);

    for(int i = 1; i <= numero_comandi_innestati; i++){
        
        strcpy(originale, temp);
        distributore_comandi_innestati(comando_sentito_senza_hotword, originale, i, numero_comandi_innestati);
        printf("COMANDO DISTRIBUITO : %s\n",comando_sentito_senza_hotword);
        fgets(comando_permesso, DIM_MAX_COMANDO, lista_comandi);
        while(!feof(lista_comandi)){
            codice_comando = estrai_codice(comando_permesso);

            // compara il comando sentito con quello caricato dal dizionario. Se sono uguali manda il codice corrispondente al robot.
            strcpy(temp2, comando_permesso);
            if(!is_X_nel_comando(temp2)){
                // solito controllo
                if(strncmp(comando_permesso, comando_sentito_senza_hotword, strlen(comando_sentito_senza_hotword)) == 0){
                    invia_codice(codice_comando*1000);
                    break;
                }
            } else {
                 //printf("X nel comando \n");
                 strcpy(temp2, comando_permesso);
                 //codice_comando = (codice_comando*1000 + confronto_comando_X_dizionario(comando_sentito_senza_hotword, comando_permesso));
                 X = confronto_comando_X_dizionario(comando_sentito_senza_hotword, comando_permesso);
                 if(X != -1){
                     invia_codice(codice_comando*1000+X);
                     break;
                 } 
                //controllo con X nel comando
            }

            
            //continua ciclo while
        fgets(comando_permesso, DIM_MAX_COMANDO, lista_comandi);
        }
        fseek(lista_comandi, 0, SEEK_SET);
    
    }

    fclose(lista_comandi);

}

int confronto_comando_X_dizionario(char *comando_sentito_senza_hotword,char *comando_permesso){
    int numero_corrispondente;
    char dizionario[DIM_MAX_COMANDO];
    char comando[DIM_MAX_COMANDO];
    char parola_dopo_la_X[DIM_MAX_COMANDO];
    char *token_dizionario, *token_comando;

    char *rest_d; //riferimenti alle stringhe per usare strtok_r
    char *rest_c; //riferimenti alle stringhe per usare strtok_r  
    char temp_cm[DIM_MAX_COMANDO];
    char *rest_cm;
    


    strcpy(dizionario, comando_permesso); //inizializzazione di variabili temporanee per tokenizzare quelle in ingresso
    strcpy(comando, comando_sentito_senza_hotword); //inizializzazione di variabili temporanee per tokenizzare quelle in ingresso
    strcpy(temp_cm, comando_sentito_senza_hotword);
    //rest_d = dizionario;
    //rest_c = comando;

    //printf("dizionario: %s\n",dizionario);
    //printf("comando: %s\n",comando);

    token_dizionario = strtok_r(dizionario, " ", &rest_d);
    token_comando = strtok_r(comando, " ", &rest_c);

    
    //controllo che le parti del comando e del dizionario prima della X equivalgano 
    while(true){
        //printf("Token  dizionario: %s\n",token_dizionario);
        //printf("Token  comando: %s\n",token_comando);
        if(strcmp(token_dizionario, NUMBER_REPLACE) == 0)
            break;
        if(strcmp(token_dizionario, token_comando) != 0)
            return -1;
        
        token_dizionario = strtok_r(NULL, " ", &rest_d);
        token_comando = strtok_r(NULL, " ", &rest_c);
    }

    //ricostruisco la stringa corrispondente alla X e restituisce il numero corrispondente (-1 se non corripsonde a nessun numero)
    token_dizionario = strtok_r(NULL, " ", &rest_d); //prendo la parola successiva a X
    
    
    numero_corrispondente = turn_X_into_int(token_dizionario, token_comando, rest_c); //restituisce -1 se il dizionario termina subito dopo la X ma il comando è più lungo e se il numero corrispondente alla X non è un numero 
    token_comando = strtok_r(temp_cm, " ", &rest_cm);
    //printf("Token  dizionario: %s\n",token_dizionario);
    //printf("Token  comando: %s\n",token_comando);
    //rest_c = &temp_cm;
    //token_comando = strtok_r(comando, " ", &rest_c); //resetto il tokenizer del comando
    if(numero_corrispondente == -1) {
        //le parole corrispondenti alla parte X non sono in realtà un numero
        return -1;

    //controllo che la seconda parte del comando equivalga con quella del dizionario
    } else {

        if(token_dizionario == NULL)
            return numero_corrispondente;
        else {
            //raggiungo la parola dopo il numero
            while(strcmp(token_comando, token_dizionario) != 0){
                //printf("Token comando: %s\n",token_comando);
                token_comando = strtok_r(NULL, " ", &rest_cm);
                if(token_comando == NULL)
                    return -1;
                
            }

            while(token_comando != NULL || token_dizionario != NULL){
                if(token_comando == NULL && token_dizionario != NULL || token_comando != NULL && token_dizionario == NULL)
                    return -1;
                if(strcmp(token_comando, token_dizionario) != 0)
                    return -1;
                token_comando = strtok_r(NULL, " ", &rest_cm);
                token_dizionario = strtok_r(NULL, " ", &rest_d);
            }

            return numero_corrispondente;
        }
        
    }
    
}

int turn_X_into_int(char *token_dopo_la_X, char *token_c, char *r_c){
    int numero_int; //numero convertito proveniente dal documento
    char numero_str[DIM_MAX_COMANDO]; //linea del documento
    char *token_numero_str; //token della linea del documento
    char *token_token_numero_str; //token per tokenizzare il numero scritto in stringa
    char *rest_ns = numero_str; //riferimenti alle stringhe per usare strtok_r  
    char *token_comando = token_c;
    char *rest_c = r_c;
    char stringa_ricostruita[DIM_MAX_COMANDO];

    numbers = fopen("numbers","r");

    //ricostruisco la X partendo dal token_comando fino a che non arrivo al token_dopo_la_X
    
    stringa_ricostruita[0] = '\0';

    while(token_comando != NULL && strcmp(token_dopo_la_X, token_comando) != 0){
        //printf("Token comando: %s\n",token_comando);
        if(stringa_ricostruita[0] != '\0')
            strcat(stringa_ricostruita, " ");
        strcat(stringa_ricostruita, token_comando);
        token_comando = strtok_r(NULL, " ", &rest_c);
       
    }

    //lo confronto con tutti i numeri del documento
    while(!feof(numbers)){
        fgets(numero_str, DIM_MAX_COMANDO, numbers); //prendo la linea del documento
        token_numero_str = strtok_r(numero_str, DIVISORE, &rest_ns); //la divido dove c'è il ';' e carico in token_numero_str il numero scritto in stringa prima del ';'
        
        if(strcmp(token_numero_str, stringa_ricostruita) == 0){
            //se sono uguali restituisco il numero corrispondente in alternativa -1
            token_numero_str = strtok_r(NULL, DIVISORE, &rest_ns); //carico il numero scritto dopo il ';' in token_numero_str
            numero_int = atoi(token_numero_str);
            return numero_int;
        }
            
        
    }

    fclose(numbers);
    return -1;
}

bool is_X_nel_comando(char *comando){
    if(strchr(comando, NUMBER_REPLACE[0]) == NULL)
        return false;
    return true;
}


/* prende in ingresso un comando e restituisce il numero di comandi innestati */
int get_numero_comandi_innestati(char *comando){
    char* token = strtok(comando, " ");
    int contatore = 0;
    while(token != NULL){
        if(strcmp(token, DIVISORE_PIU_COMANDI) == 0){
            contatore++;
        } 
        token = strtok(NULL, " ");
    }
    
    return contatore+1;
}

/* mette in 'comando singolo' il comando alla posizione 'numero_comando_da_generare' */
void distributore_comandi_innestati(char *comando_singolo, char *originale, int numero_comando_da_generare, int numero_comandi_innestati){
    
    char* token = strtok(originale, " ");
    //int temp = 1;
    //printf("%s\n", originale);
    if(numero_comandi_innestati != 1){
        comando_singolo[0] = '\0'; //elimino il contenuto del comando singolo
        for(int i = 1; i <= numero_comandi_innestati; i++){
            
            if(i < numero_comando_da_generare){
                /* posiziono il token all'inizio del comando 'numero_comando_da_generare */
                while(strcmp(token,DIVISORE_PIU_COMANDI) != 0 && token != NULL){
                    token = strtok(NULL, " ");
                }
                token = strtok(NULL, " ");

            } else if(i == numero_comando_da_generare){
                /* riscrivo il comando numero 'numero_comando_da_controllare' in comando_singolo */
                while(token != NULL && strcmp(token, DIVISORE_PIU_COMANDI) != 0){
                    if(comando_singolo[0] == '\0'){
                        strcpy(comando_singolo, token);
                    } else {
                        strcat(comando_singolo, " ");
                        strcat(comando_singolo, token);
                    }
                    token = strtok(NULL, " ");
                }
                //token = strtok(NULL, " ");
                


            } else if(i > numero_comando_da_generare){
                break;
            }

        }

    }

}

/* prende una stringa in ingresso e restituisce il numero all'interno */
int estrai_codice(char *comando){

    //MODIFICATO// FORSE NON LO MODIFICO
    //Aggiungi la ricerca della X
    //Modo per perstituire solo il codice per il  comando sentito e modo per sentire il codice e quello detto nella frase.

	int init_size = strlen(comando);
	char delim[] = DIVISORE;
	char *ptr = strtok(comando, delim);
    ptr = strtok(NULL, delim);

    return atoi(ptr);
}

/* Prende in ingresso un codice e lo invia al robot */
void invia_codice(int codice){
    printf("CODICE SENTITO: %d\n", codice);
    //Questo componente attualmente stampa solamente a video il codice sentito però il giorno che si vorrà effettuare la vera implementazione con il robot sarà qui che bisognerà creare il collegamento
}

/* Prende in ingresso la posizione della hotword (sotto forma di token) e restituisce il comando sentito dopo la hotword */ 
void ricostruisci_comando(char* comando, char* token){
    int contatore = 0;
    //char comando_ricostruito[DIM_MAX_COMANDO];
    while(token != NULL){
        //printf("Token: %s\n", token);
        if(contatore != 0) strcat(comando, " ");
        contatore++;
        strcat(comando, token);
        token = strtok(NULL, " ");
    }
}
