#include "headers.h"

sem_t* sem;
int patient_count = 0;

int connect_to_donor_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        return -1;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }
    
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

void add_donor_to_server(char* name, int age, char* blood_group) {
    int sock = connect_to_donor_server();
    if(sock < 0) {
        printw("Error: Cannot connect to donor server\n");
        refresh();
        return;
    }
    
    DonorRequest req;
    req.type = 1;
    strcpy(req.name, name);
    req.age = age;
    strcpy(req.blood_group, blood_group);
    
    write(sock, &req, sizeof(DonorRequest));
    
    int success;
    read(sock, &success, sizeof(int));
    
    close(sock);
}

// Function to count available beds by checking shared memory
int count_available_beds(person* shared) {
    int available = 0;
    for(int i = 0; i < MAX_PATIENTS; i++) {
        if(shared[i].active == 0) {
            available++;
        }
    }
    return available;
}

void draw_ui(WINDOW *win, int beds_available) {
    werase(win);
    box(win, 0, 0);
    echo();
    const wchar_t amb[]=L"🚑";
    const wchar_t bed[]=L"🛏";
    const wchar_t pat[]=L"🤒";
    const wchar_t don[]=L"🩸";
    const wchar_t bye[]=L"🛑";
    mvwprintw(win, 1, 7, "%ls %ls %ls HOSPITAL MANAGEMENT SYSTEM %ls %ls %ls",amb,amb,amb,amb,amb,amb);
    mvwprintw(win, 3, 2, "Available Beds : %d/%d %ls", beds_available, MAX_PATIENTS,bed);
    mvwprintw(win, 5, 2, "1. Register Donor %ls",don);
    mvwprintw(win, 6, 2, "2. Register Patient %ls",pat);
    mvwprintw(win, 7, 2, "3. Exit %ls",bye);
    mvwprintw(win, 9, 2, "Enter your choice: ");
    
    wrefresh(win);
}

int main() {
setlocale(LC_ALL,"");
	
    person p;
    int shmid;
    person* shared;
    
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    
    int height = 20, width = 60;
    int starty = (LINES - height) / 2;
    int startx = (COLS - width) / 2;
    
    WINDOW *main_win = newwin(height, width, starty, startx);
    keypad(main_win, TRUE);
    
    // Initialize semaphore
    sem = sem_open("/semos", O_CREAT, 0666, 0);
    
    // Create shared memory for patients
    shmid = shmget((key_t)123456, sizeof(person) * MAX_PATIENTS, IPC_CREAT | 0777);
    if(shmid == -1) {
        endwin();
        perror("Shared memory failed");
        exit(1);
    }
    
    shared = (person*)shmat(shmid, NULL, 0);
    if(shared == (void*)-1) {
        endwin();
        perror("Shared memory attach failed");
        exit(1);
    }
    
    // Initialize all slots as inactive
    memset(shared, 0, sizeof(person) * MAX_PATIENTS);
    
    while(1) {
        // Dynamically calculate available beds by checking shared memory
        int beds_available = count_available_beds(shared);
        
        draw_ui(main_win, beds_available);
        
        int choice;
        wscanw(main_win, "%d", &choice);
        
        if(choice == 3) {
            break;
        }
        
        werase(main_win);
        box(main_win, 0, 0);
        
        if(choice == 1) {
            // Donor Registration
            mvwprintw(main_win, 2, 17, "=== DONOR REGISTRATION ===");
            mvwprintw(main_win, 4, 2, "Enter name: ");
            wrefresh(main_win);
            echo();
            wscanw(main_win, "%s", p.name);
            
            mvwprintw(main_win, 5, 2, "Enter age: ");
            wrefresh(main_win);
            wscanw(main_win, "%d", &p.age);
            
            mvwprintw(main_win, 6, 2, "Enter blood group: ");
            wrefresh(main_win);
            wscanw(main_win, "%s", p.bg);
            noecho();
            
            // Send to donor server via socket
            add_donor_to_server(p.name, p.age, p.bg);
            
            mvwprintw(main_win, 8, 2, "Donor registered successfully!");
            mvwprintw(main_win, 10, 2, "Press any key to continue...");
            wrefresh(main_win);
            wgetch(main_win);
            
        } else if(choice == 2) {
            // Recalculate beds before registering
            beds_available = count_available_beds(shared);
            
            // Patient Registration
            if(beds_available == 0) {
                mvwprintw(main_win, 4, 2, "BEDS ARE FULL!");
                mvwprintw(main_win, 6, 2, "Press any key to continue...");
                wrefresh(main_win);
                wgetch(main_win);
                continue;
            }
            const wchar_t pat[]=L"🤒";
            mvwprintw(main_win, 2, 17, "=== PATIENT REGISTRATION %ls ===",pat);
            mvwprintw(main_win, 4, 2, "Enter name: ");
            wrefresh(main_win);
            echo();
            wscanw(main_win, "%s", p.name);
            
            mvwprintw(main_win, 5, 2, "Enter age: ");
            wrefresh(main_win);
            wscanw(main_win, "%d", &p.age);
            
            mvwprintw(main_win, 6, 2, "Enter blood group: ");
            wrefresh(main_win);
            wscanw(main_win, "%s", p.bg);
            noecho();
            
            p.active = 1;
            
            // Find first empty slot and add patient
            int assigned_slot = -1;
            for(int i = 0; i < MAX_PATIENTS; i++) {
                if(shared[i].active == 0) {
                    memcpy(&shared[i], &p, sizeof(person));
                    patient_count++;
                    assigned_slot = i;
                    sem_post(sem);
                    break;
                }
            }
            
            if(assigned_slot != -1) {
                mvwprintw(main_win, 8, 2, "Patient registered successfully!");
                mvwprintw(main_win, 9, 2, "Assigned to Bed %d (monitoring...)", assigned_slot);
            } else {
                mvwprintw(main_win, 8, 2, "Error: Could not assign bed!");
            }
            
            mvwprintw(main_win, 11, 2, "Press any key to continue...");
            wrefresh(main_win);
            wgetch(main_win);
            
        } else {
            mvwprintw(main_win, 4, 2, "Invalid choice!");
            mvwprintw(main_win, 6, 2, "Press any key to continue...");
            wrefresh(main_win);
            wgetch(main_win);
        }
    }
    
    // Cleanup
    delwin(main_win);
    endwin();
    
    shmdt(shared);
    sem_close(sem);
    
    return 0;
}
