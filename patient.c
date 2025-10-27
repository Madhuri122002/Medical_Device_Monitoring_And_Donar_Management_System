#include "headers.h"

sem_t* sem;
sem_t* sem1;

#define WEB_PORT 8081

patient* s_global; // For web server access

void* heart_rate(void* arg) {
    *((float*)arg) = 60 + rand() % 80; // 60-140 bpm
    pthread_exit(NULL);
}

void* bloodPressure(void* arg) {
    blood_pressure* bp = ((blood_pressure*)arg);
    bp->systollic = 90 + rand() % 60;  // 90-150
    bp->diastollic = 60 + rand() % 40; // 60-100
    pthread_exit(NULL);
}

void* oxygen_level(void* arg) {
    *((float*)arg) = 88 + rand() % 13; // 88-100%
    pthread_exit(NULL);
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
    
    printf("Patient web dashboard: http://localhost:%d\n", WEB_PORT);
    
    while(1) {
        client_sock = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if(client_sock >= 0) {
            char html[4096] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3'>"
                "<style>body{font-family:Arial;margin:20px;background:#667eea;}"
                ".card{background:white;padding:20px;margin:10px;border-radius:10px;}"
                "h1{color:white;text-align:center;}</style></head><body>"
                "<h1>Patient Monitoring</h1>";
            
            for(int i = 0; i < MAX_PATIENTS; i++) {
                if(s_global[i].active == 1) {
                    char temp[512];
                    sprintf(temp, "<div class='card'><b>Bed %d: %s</b> (Age:%d, BG:%s)<br>"
                           "HR: %.0f bpm | BP: %d/%d | O2: %.0f%%</div>",
                           i, s_global[i].name, s_global[i].age, s_global[i].blood_group,
                           s_global[i].hr, s_global[i].bp.systollic, 
                           s_global[i].bp.diastollic, s_global[i].o2);
                    strcat(html, temp);
                }
            }
            strcat(html, "</body></html>");
            write(client_sock, html, strlen(html));
            close(client_sock);
        }
    }
    return NULL;
}
int main() {
    pthread_t t1, t2, t3;
    srand(time(NULL));
    
    key_t key = 123456;
    
    sem = sem_open("/semos", 0);
    if(sem == SEM_FAILED) {
        perror("Semaphore open failed");
        exit(1);
    }
    
    int shmid = shmget(key, sizeof(person) * MAX_PATIENTS, IPC_CREAT | 0666);
    if(shmid == -1) {
        perror("Failed shmget");
        exit(1);
    }
    
    person* shared_memory1 = (person*)shmat(shmid, NULL, 0);
    if(shared_memory1 == (person*)-1) {
        perror("Failed shmat");
        exit(1);
    }
    
    // Create shared memory for patient vitals
    int shmid1 = shmget(12345678, sizeof(patient) * MAX_PATIENTS, IPC_CREAT | 0666);
    if(shmid1 == -1) {
        perror("Failed shmget for vitals");
        exit(1);
    }
    
    patient* s = (patient*)shmat(shmid1, NULL, 0);
    if(s == (patient*)-1) {
        perror("Failed shmat for vitals");
        exit(1);
    }
    memset(s, 0, sizeof(patient) * MAX_PATIENTS);
    
    s_global = s; // Set global pointer for web server
    
    pthread_t web_thread;
    pthread_create(&web_thread, NULL, web_server, NULL);
    
    sem1 = sem_open("/semos5", O_CREAT, 0777, 0);
    if(sem1 == NULL) {
        perror("sem1 failed");
        exit(1);
    }
    
    printf("Patient monitoring started...\n");
    
    int initialized[MAX_PATIENTS] = {0};
    
    while(1) {
        for(int i = 0; i < MAX_PATIENTS; i++) {
        
        if(shared_memory1[i].active == 0 && initialized[i] == 1) {
            // Patient discharged - clean up
            s[i].active = 0;
            initialized[i] = 0;
            printf("Patient in slot %d has been discharged\n", i);
        }
            if(shared_memory1[i].active == 1 && initialized[i] == 0) {
                // New patient detected
                strcpy(s[i].name, shared_memory1[i].name);
                s[i].age = shared_memory1[i].age;
                strcpy(s[i].blood_group, shared_memory1[i].bg);
                s[i].active = 1;
                s[i].slot_id = i;
                initialized[i] = 1;
                printf("Patient %s assigned to slot %d\n", s[i].name, i);
            }
            
            if(s[i].active == 1) {
                // Generate vitals
                pthread_create(&t1, NULL, heart_rate, (void*)&s[i].hr);
                pthread_create(&t2, NULL, bloodPressure, (void*)&s[i].bp);
                pthread_create(&t3, NULL, oxygen_level, (void*)&s[i].o2);
                
                pthread_join(t1, NULL);
                pthread_join(t2, NULL);
                pthread_join(t3, NULL);
                
                printf("Slot %d - %s: HR=%.0f BP=%d/%d O2=%.0f%%\n",
                       i, s[i].name, s[i].hr, s[i].bp.systollic, 
                       s[i].bp.diastollic, s[i].o2);
                
                sem_post(sem1);
            }
        }
        
        sleep(3);
    }
    
    sem_close(sem);
    sem_close(sem1);
    shmdt(s);
    shmdt(shared_memory1);
    
    return 0;
}
