#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#define WEB_PORT 8083
#define MAX_LOG 20

typedef struct {
    char action[20];
    char name[40];
    char blood_group[5];
    time_t timestamp;
} LogEntry;

LogEntry activity_log[MAX_LOG];
int log_count = 0;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void add_log(const char* action, const char* name, const char* bg) {
    pthread_mutex_lock(&log_mutex);
    if(log_count >= MAX_LOG) {
        for(int i = 0; i < MAX_LOG - 1; i++) {
            activity_log[i] = activity_log[i + 1];
        }
        log_count = MAX_LOG - 1;
    }
    strcpy(activity_log[log_count].action, action);
    strcpy(activity_log[log_count].name, name);
    strcpy(activity_log[log_count].blood_group, bg);
    activity_log[log_count].timestamp = time(NULL);
    log_count++;
    pthread_mutex_unlock(&log_mutex);
}

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
           
    add_log("REGISTERED", d.name, d.blood_group);
}

DonorResponse search_donor(char* blood_group) {
    DonorResponse response;
    response.found = 0;
   
    
    donor h;
    int type = 0;
    
     // After "response.found = 1;" line, add:
    //add_log("MATCHED", h.name, h.blood_group);
    
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
        
        add_log("MATCHED", h.name, h.blood_group);
        
        printf("[DONOR FOUND] Name: %s, Age: %d, Blood Group: %s\n",
               h.name, h.age, h.blood_group);
               
       
    } else {
    // In the else block, add:
    	add_log("SEARCH FAILED", "N/A", blood_group);
        printf("[DONOR NOT FOUND] Blood Group: %s\n", blood_group);
        
    }
    
    return response;
}


void* web_server(void* arg) {
    int server_fd, client_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(WEB_PORT);
    
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);
    
    printf("Donor web dashboard: http://localhost:%d\n", WEB_PORT);
    
    while(1) {
        client_sock = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if(client_sock >= 0) {
            pthread_mutex_lock(&log_mutex);
            
            char html[4096] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='5'>"
                "<style>body{font-family:Arial;margin:20px;background:#4facfe;}"
                ".card{background:white;padding:15px;margin:10px;border-radius:8px;}"
                ".registered{border-left:4px solid green;}"
                ".matched{border-left:4px solid orange;}"
                ".failed{border-left:4px solid red;}"
                "h1{color:white;text-align:center;}</style></head><body>"
                "<h1>Donor Management</h1>";
            
            for(int i = log_count - 1; i >= 0 && i >= log_count - 15; i--) {
                char temp[256];
                char time_str[32];
                struct tm* tm_info = localtime(&activity_log[i].timestamp);
                strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
                
                if(strcmp(activity_log[i].action, "REGISTERED") == 0)
                {
                	
                	sprintf(temp, "<div class='card %s'><b>%s</b> [%s]<br>%s - %s</div>",
                		"registered",
		               activity_log[i].action, time_str,
		               activity_log[i].name, activity_log[i].blood_group);
                	strcat(html, temp);
                }
                else if(strcmp(activity_log[i].action, "MATCHED") == 0)
                {
                	
                	sprintf(temp, "<div class='card %s'><b>%s</b> [%s]<br>%s - %s</div>",
                		"matched",
		               activity_log[i].action, time_str,
		               activity_log[i].name, activity_log[i].blood_group);
                	strcat(html, temp);
                }
                else
                {
                	sprintf(temp, "<div class='card %s'><b>%s</b> [%s]<br>%s - %s</div>",
                		"failed",
		               activity_log[i].action, time_str,
		               activity_log[i].name, activity_log[i].blood_group);
                	strcat(html, temp);
                }
                	
            }
            
            pthread_mutex_unlock(&log_mutex);
            strcat(html, "</body></html>");
            write(client_sock, html, strlen(html));
            close(client_sock);
        }
    }
    return NULL;
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
    // Start web server
    pthread_t web_thread;
    pthread_create(&web_thread, NULL, web_server, NULL);
    
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
