#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/msg.h>
#include <fcntl.h>

#define PORT 8080
#define KEY_MSG 256

typedef struct donor {
    long mtype;
    char name[40];
    int age;
    char blood_group[5];
} donor;

typedef struct {
    int type; // 1: Add donor, 2: Search donor
    char name[40];
    int age;
    char blood_group[5];
} DonorRequest;

typedef struct {
    int found;
    char name[40];
    int age;
    char blood_group[5];
} DonorResponse;

int msgid;
int fd;

void add_donor(DonorRequest *req) {
    donor d;
    strcpy(d.name, req->name);
    d.age = req->age;
    strcpy(d.blood_group, req->blood_group);
    
    // Determine message type based on blood group
    char* arr[8] = {"O+", "O-", "A+", "A-", "B+", "B-", "AB+", "AB-"};
    for(int i = 0; i < 8; i++) {
        if(strcmp(arr[i], d.blood_group) == 0) {
            d.mtype = i + 1;
            break;
        }
    }
    
    // Add to message queue
    if(msgsnd(msgid, &d, sizeof(donor) - sizeof(long), 0) == -1) {
        perror("Message sending failed");
        return;
    }
    
    // Write to file
    lseek(fd, 0, SEEK_END);
    write(fd, &d, sizeof(donor));
    
    printf("[DONOR ADDED] Name: %s, Age: %d, Blood Group: %s\n", 
           d.name, d.age, d.blood_group);
}

DonorResponse search_donor(char* blood_group) {
    DonorResponse response;
    response.found = 0;
    
    donor h;
    int type = 0;
    
    char* arr[8] = {"O+", "O-", "A+", "A-", "B+", "B-", "AB+", "AB-"};
    for(int i = 0; i < 8; i++) {
        if(strcmp(arr[i], blood_group) == 0) {
            type = i + 1;
            break;
        }
    }
    
    if(msgrcv(msgid, &h, sizeof(donor) - sizeof(long), type, IPC_NOWAIT) != -1) {
        response.found = 1;
        strcpy(response.name, h.name);
        response.age = h.age;
        strcpy(response.blood_group, h.blood_group);
        
        printf("[DONOR FOUND] Name: %s, Age: %d, Blood Group: %s\n",
               h.name, h.age, h.blood_group);
    } else {
        printf("[DONOR NOT FOUND] Blood Group: %s\n", blood_group);
    }
    
    return response;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Initialize message queue
    msgid = msgget((key_t)KEY_MSG, IPC_CREAT | 0777);
    if(msgid == -1) {
        perror("Message queue creation failed");
        exit(1);
    }
    
    // Open donor file
    fd = open("donor.bin", O_CREAT | O_RDWR | O_APPEND, 0777);
    if(fd == -1) {
        perror("File open failed");
        exit(1);
    }
    
    // Create socket
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Set socket options
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(1);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket
    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen for connections
    if(listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("Donor server running on port %d...\n", PORT);
    
    while(1) {
        if((client_socket = accept(server_fd, (struct sockaddr *)&address, 
                                   (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        DonorRequest req;
        int bytes_read = read(client_socket, &req, sizeof(DonorRequest));
        
        if(bytes_read > 0) {
            if(req.type == 1) {
                // Add donor
                add_donor(&req);
                int success = 1;
                write(client_socket, &success, sizeof(int));
            } else if(req.type == 2) {
                // Search donor
                DonorResponse response = search_donor(req.blood_group);
                write(client_socket, &response, sizeof(DonorResponse));
            }
        }
        
        close(client_socket);
    }
    
    close(fd);
    close(server_fd);
    return 0;
}
