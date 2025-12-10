#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>

#define MSGSZ 256

struct msgbuf {
    long mtype;       /* message type, must be > 0 */
    char mtext[MSGSZ];    /* message data */
};

//if mtext=0 -> message with no text -> permitted
//mtype field must have a strictly positive integer value
//queue capacity is defined by the msg_qbytes field in the associated data structure for the message queue
//The msgrcv() system call removes a message from the queue specified by msqid and places it in the buffer pointed to by msgp

//in the end the goal is to communicate using Morse code -> sender gets input, converts it to Morse code(1s dot, 2s dash), sends it to the receiver
//which decodes it on the fly and displays the original message

int main() {
    key_t key;
    int msgflg = IPC_CREAT | 0666;
    struct msgbuf sender_message_buffer, receiver_message_buffer;
    key=1234; //can use ftok to generate key
    int msgid=msgget(key, msgflg); //create message queue

    if(msgid==-1) {
        perror("msgget failed");
        return 1;
    }

    // Sending a message
    sender_message_buffer.mtype=1;
    printf("Enter message to send: ");
    fgets(sender_message_buffer.mtext, MSGSZ, stdin);
    if(msgsnd(msgid, &sender_message_buffer, sizeof(sender_message_buffer.mtext), IPC_NOWAIT)==-1) {
        perror("msgsend failed"); //invalid argument
            return 1;
    } else {
        printf("Message sent: %s", sender_message_buffer.mtext);
    }

    // Receeiving a message
    receiver_message_buffer.mtype=1;
    msgrcv(msgid, &receiver_message_buffer, sizeof(sender_message_buffer.mtext), 1, 0);
    printf("Message received: %s", receiver_message_buffer.mtext);
    return 0;
}

