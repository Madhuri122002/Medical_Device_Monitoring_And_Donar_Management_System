#include "headers.h"
#define WEB_PORT 8082
LabResult lab_results[MAX_PATIENTS];
pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;


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
    
    printf("Lab web dashboard: http://localhost:%d\n", WEB_PORT);
    
    while(1) {
        client_sock = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if(client_sock >= 0) {
            pthread_mutex_lock(&results_mutex);
            
            char html[4096] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3'>"
                "<style>body{font-family:Arial;margin:20px;background:#f093fb;}"
                ".card{background:white;padding:20px;margin:10px;border-radius:10px;}"
                ".critical{background:#ffcccc;} .stable{background:#ccffcc;}"
                "h1{color:white;text-align:center;}</style></head><body>"
                "<h1>Lab Analysis</h1>";
            
            for(int i = 0; i < MAX_PATIENTS; i++) {
                if(lab_results[i].slot_id >= 0) {
                    char temp[512];
                    sprintf(temp, "<div class='card %s'><b>Bed %d: %s</b> (BG:%s)<br>"
                           "Hemoglobin: %d g/dL<br>Status: %s</div>",
                           lab_results[i].is_critical ? "critical" : "stable",
                           i, lab_results[i].name, lab_results[i].blood_group,
                           lab_results[i].hb_level,
                           lab_results[i].is_critical ? "CRITICAL" : "STABLE");
                    strcat(html, temp);
                }
            }
            
            pthread_mutex_unlock(&results_mutex);
            strcat(html, "</body></html>");
            write(client_sock, html, strlen(html));
            close(client_sock);
        }
    }
    return NULL;
}

int main() {
    srand(time(0));
    int key = 12345678;
    
    // Initialize lab results
    for(int i = 0; i < MAX_PATIENTS; i++) {
        lab_results[i].slot_id = -1;
    }
    
    sem_t* sem1 = sem_open("/semos5", 0);
    if(sem1 == SEM_FAILED) {
        perror("Semaphore open failed");
        exit(1);
    }
    
    int shmid = shmget((key_t)key, sizeof(patient) * MAX_PATIENTS, IPC_CREAT | 0666);
    if(shmid == -1) {
        perror("Shared memory failed");
        exit(1);
    }
    
    patient* shared = (patient*)shmat(shmid, NULL, 0);
    if(shared == (void*)-1) {
        perror("Virtual memory failed");
        exit(1);
    }
    
    int msgid = msgget((key_t)MSG_KEY, IPC_CREAT | 0777);
    if(msgid == -1) {
        perror("Message queue creation failed");
        exit(1);
    }
    
    printf("Lab analysis started...\n");
    
    // Start web server
    pthread_t web_thread;
    pthread_create(&web_thread, NULL, web_server, NULL);
    
    while(1) {
        sem_wait(sem1);
        
        for(int i = 0; i < MAX_PATIENTS; i++) {
            if(shared[i].active == 1) {
                int hb = rand() % 7 + 5;
                
                // Update lab results for web
                pthread_mutex_lock(&results_mutex);
                lab_results[i].slot_id = i;
                strcpy(lab_results[i].name, shared[i].name);
                strcpy(lab_results[i].blood_group, shared[i].blood_group);
                lab_results[i].hb_level = hb;
                
                StatusMessage msg;
                msg.mtype = 1;
                msg.slot_id = i;
                strcpy(msg.name, shared[i].name);
                strcpy(msg.blood_group, shared[i].blood_group);
                
                if((shared[i].hr > 120 && hb < 8) || 
                   (hb < 8 && shared[i].bp.systollic < 90 && shared[i].bp.diastollic < 65) || 
                   (hb < 8 && shared[i].o2 < 90)) {
                    
                    msg.is_critical = 1;
                    lab_results[i].is_critical = 1;
                    sprintf(msg.message, "CRITICAL: Low HB=%.1f, HR=%.0f, BP=%d/%d, O2=%.0f%%", 
                            (float)hb, shared[i].hr, shared[i].bp.systollic, 
                            shared[i].bp.diastollic, shared[i].o2);
                    sprintf(lab_results[i].status, "CRITICAL");
                    
                    printf("ALERT: Patient %s (Slot %d) is CRITICAL!\n", 
                           shared[i].name, i);
                } else {
                    msg.is_critical = 0;
                    lab_results[i].is_critical = 0;
                    sprintf(msg.message, "STABLE: HB=%.1f, All vitals normal", (float)hb);
                    sprintf(lab_results[i].status, "STABLE");
                }
                
                pthread_mutex_unlock(&results_mutex);
                
                if(msgsnd(msgid, &msg, sizeof(StatusMessage) - sizeof(long), IPC_NOWAIT) == -1) {
                    perror("Message send failed");
                }
            } else {
                pthread_mutex_lock(&results_mutex);
                if(lab_results[i].slot_id == i) {
                    lab_results[i].slot_id = -1;
                }
                pthread_mutex_unlock(&results_mutex);
            }
        }
        
        usleep(100000);
    }
    
    shmdt(shared);
    sem_close(sem1);
    
    return 0;
}
