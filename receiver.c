//#include <cstdio>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include<sys/msg.h>
#include <sys/shm.h>

#define SHM_SIZE 1024
#define Filled 0
#define Ready 1
#define NotReady -1
#define DOT 3
#define DASH 5
#define CHAR_END 7
#define WORD_END 9

sigset_t block_mask, old_mask;

struct memory {
    char buff[100];
    int status, pid1, pid2, signal_type;
};

struct morse_table {
    char* character;
    char* code;
} morse_table;

struct morse_table table[]= {
    {"A", ".-"},
    {"B", "-..."},
    {"C", "-.-."},
    {"D", "-.."},
    {"E", "."},
    {"F", "..-."},
    {"G", "--."},
    {"H", "...."},
    {"I", ".."},
    {"J", ".---"},
    {"K", "-.-"},
    {"L", ".-.."},
    {"M", "--"},
    {"N", "-."},
    {"O", "---"},
    {"P", ".--."},
    {"Q", "--.-"},
    {"R", ".-."},
    {"S", "..."},
    {"T", "-"},
    {"U", "..-"},
    {"V", "...-"},
    {"W", ".--"},
    {"X", "-..-"},
    {"Y", "-.--"},
    {"Z", "--.."},
    {"1", ".----"},
    {"2", "..---"},
    {"3", "...--"},
    {"4", "....-"},
    {"5", "....."},
    {"6", "-...."},
    {"7", "--..."},
    {"8", "---.."},
    {"9", "----."},
    {"0", "-----"}
};


struct memory* shmptr;

//function to convert text to Morse code
void text_to_morse(const char* text, char* morse) {
    morse[0]='\0'; //morse init
    for(int i=0; text[i]!='\0'; i++) {
        if(text[i]==' ') {
            strcat(morse, " / ");
        } else {
            for(int j=0; j<sizeof(table)/sizeof(table[0]); j++) {
                if(toupper(text[i])==table[j].character[0]) {
                    strcat(morse, table[j].code);
                }
            }
        }
        if(text[i+1]!=' ' && text[i+1]!='\0') {
            strcat(morse, " ");
        }
    }
}

//function to convert Morse code to text
char morse_to_text(const char* morse) {
    for(int i=0; i<sizeof(table)/sizeof(table[0]); i++) {
        if(strcmp(table[i].code, morse)==0) {
            return table[i].character[0];
        }
    }
    return '\0'; //unknown character
}

void signal_handler(int signum) {
    //is signum is SIGUSR2, user2 receives a message from user1
    static char current_morse[10]="";
    if(signum==SIGUSR2) {
        if(shmptr->signal_type==DOT) {
            strcat(current_morse, ".");
        } else if(shmptr->signal_type==DASH) {
            strcat(current_morse, "-");
        } else if(shmptr->signal_type==CHAR_END) {
            char decoded_char=morse_to_text(current_morse);
            printf("%c", decoded_char);
            fflush(stdout); //immediate printing
            current_morse[0]='\0'; //reset
        } else if(shmptr->signal_type==WORD_END) {
            printf(" ");
            fflush(stdout); //immediate printing
            current_morse[0]='\0'; //reset
        }
    }
}

int main() {
    int pid=getpid(); //user2 process id
    int key=1234; //key for shared memory

    int shmid=shmget(key, sizeof(struct memory), IPC_CREAT | 0666);
    if(shmid==-1) {
        perror("shmget failed");
        return 1;
    }

    shmptr=shmat(shmid, NULL, 0);
    if(shmptr==(void*)-1) {
        perror("shmat failed");
        return 1;
    }

    //store pid2 in shared memory
    shmptr->pid2=pid;
    shmptr->status=NotReady;
    signal(SIGUSR2, signal_handler);

    while(1) {
        //wait for user1 to send message
        printf("Waiting for User1 to send a message...\n");
        printf("Received message: ");
        fflush(stdout);
        //waits until status is Filled
        while(shmptr->status!=Filled) {
            usleep(100000);
        }
        printf("\n");    
        //Block signals while reading input
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGUSR2);
        sigprocmask(SIG_BLOCK, &block_mask, &old_mask);
        //take message from user2
        printf("User2, enter message to send: ");
        fgets(shmptr->buff, 100, stdin); //might need to change the size
        shmptr->buff[strcspn(shmptr->buff, "\n")]='\0';
        // Unblock signals
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        shmptr->status=Filled;
        char morse_message[200];
        //send message to user1
        text_to_morse(shmptr->buff, morse_message);
        for(int i=0; morse_message[i]!='\0'; i++) {
            if(morse_message[i]=='.') {
                shmptr->signal_type=DOT;
                kill(shmptr->pid1, SIGUSR1);
                sleep(1); //dot duration
            } else if(morse_message[i]=='-') {
                shmptr->signal_type=DASH;
                kill(shmptr->pid1, SIGUSR1);
                sleep(1); //dash duration
            } else if(morse_message[i]==' ') {
                shmptr->signal_type=CHAR_END;
                kill(shmptr->pid1, SIGUSR1);
                sleep(2); //space
            } else if (morse_message[i]=='/' && morse_message[i+1]==' ') {
                shmptr->signal_type=WORD_END;
                kill(shmptr->pid1, SIGUSR1);
                sleep(4); //word space
                i++; //skip next space
            }
        }
        //sends final signal to indicate last character
        shmptr->signal_type=CHAR_END;
        kill(shmptr->pid1, SIGUSR1);
        sleep(2);
        printf("\nMessage sent\n");
        //user1's turn
        shmptr->status=Ready;
    }

    shmdt(shmptr);
    return 0;
}
