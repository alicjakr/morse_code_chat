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
    //is signum is SIGUSR1, user1 receives a message from user2
    static char current_morse[10]="";
    if(signum==SIGUSR1) {
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
    int pid=getpid(); //user1 process id
    int key=1234; //key for shared memory
    sigset_t block_mask, old_mask;

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

    //store pid1 in shared memory
    shmptr->pid1=pid;
    shmptr->status=NotReady;
    signal(SIGUSR1, signal_handler);

    //wait for receiver to be ready
    printf("Waiting for receiver...\n");
    while(shmptr->pid2==0) {
        sleep(1);
    }
    printf("Receiver is connected\n");
    shmptr->status=Ready;
    while(1) {
        //waits until status is Ready
        while(shmptr->status!=Ready) {
            usleep(100000);
        }
        // Block signals while reading input
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGUSR1);
        sigprocmask(SIG_BLOCK, &block_mask, &old_mask);
        //take message from user1
        printf("User1, enter message to send: ");
        fgets(shmptr->buff, 100, stdin);
        shmptr->buff[strcspn(shmptr->buff, "\n")]='\0';
        // Unblock signals
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        shmptr->status=NotReady;
        char morse_message[200];
        //send message to user2
        text_to_morse(shmptr->buff, morse_message);
        for(int i=0; morse_message[i]!='\0'; i++) {
            if(morse_message[i]=='.') {
                shmptr->signal_type=DOT;
                kill(shmptr->pid2, SIGUSR2);
                sleep(1); //dot duration
            } else if(morse_message[i]=='-') {
                shmptr->signal_type=DASH;
                kill(shmptr->pid2, SIGUSR2);
                sleep(1); //dash duration
            } else if(morse_message[i]==' ') {
                shmptr->signal_type=CHAR_END;
                kill(shmptr->pid2, SIGUSR2);
                sleep(2); //space
            } else if (morse_message[i]=='/' && morse_message[i+1]==' ') {
                shmptr->signal_type=WORD_END;
                kill(shmptr->pid2, SIGUSR2);
                sleep(4); //word space
                i++; //skip next space
            }
        }
        //sends final signal to indicate last character
        shmptr->signal_type=CHAR_END;
        kill(shmptr->pid2, SIGUSR2);
        sleep(2);

        printf("\nMessage sent\n");
        //user2's turn
        shmptr->status=Filled;
        //wait for user2 to send message
        printf("Waiting for User2 to send a message...\n");
        printf("Received message: ");
        fflush(stdout);
        while(shmptr->status!=Ready) {
            usleep(100000);
        }
        printf("\n");
    }
    shmdt(shmptr);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
