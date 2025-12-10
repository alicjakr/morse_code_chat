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

struct memory {
    char buff[100];
    int status, pid1, pid2;
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

void signal_handler(int signum) {
    //is signum is SIGUSR2, user2 receives a message from user1
    if(signum==SIGUSR2) {
        printf("Recived message from user1: ");
        puts(shmptr->buff);
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
        wait(NULL);
        //take message from user2
        printf("User2, enter message to send: ");
        fgets(shmptr->buff, 100, stdin); //might need to change the size
        shmptr->status=Ready;
        //send message to user1
        kill(shmptr->pid1, SIGUSR1);
        while(shmptr->status==Ready) {
            continue;
        }
    }

    shmdt(shmptr);
    return 0;
}