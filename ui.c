#include "headers.h"

DonorResponse search_donor(char* blood_group) {
    DonorResponse response;
    response.found = 0;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        return response;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return response;
    }
    
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return response;
    }
    
    DonorRequest req;
    req.type = 2;
    strcpy(req.blood_group, blood_group);
    
    write(sock, &req, sizeof(DonorRequest));
    read(sock, &response, sizeof(DonorResponse));
    
    close(sock);
    return response;
}

void draw_vitals_box(WINDOW *win, patient* patients) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, "[ PATIENT VITALS ]");
    
    int row = 1;
    for(int i = 0; i < MAX_PATIENTS; i++) {
        if(patients[i].active == 1) {
            mvwprintw(win, row++, 1, "[Slot %d: %s (Age: %d, BG: %s)]  HR: %.0f bpm | BP: %d/%d | O2: %.0f%%", 
                     i, patients[i].name, patients[i].age, patients[i].blood_group,patients[i].hr, patients[i].bp.systollic, 
                     patients[i].bp.diastollic, patients[i].o2);
        }
    }
    
    if(row == 1) {
        mvwprintw(win, 2, 2, "No patients currently monitored");
    }
    
    wrefresh(win);
}

void draw_status_box(WINDOW *win, patient* patients, PatientStatus* statuses) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, "[ PATIENT STATUS ]");
	const wchar_t w_char[]=L"🚨";
    int row = 1;
    int any_patient = 0;
    
    // Display status for each active patient in order
    for(int i = 0; i < MAX_PATIENTS; i++) {
        if(patients[i].active == 1) {
            any_patient = 1;
            
            if(statuses[i].has_status) {
                if(statuses[i].is_critical) {
                    wattron(win, A_BOLD | COLOR_PAIR(1));
                    mvwprintw(win, row++, 2, "[CRITICAL - Slot %d] %s (BG: %s) - %s  %ls", 
                             i, patients[i].name, patients[i].blood_group, statuses[i].message,w_char);
                    wattroff(win, A_BOLD | COLOR_PAIR(1));
                } else {
                    wattron(win, COLOR_PAIR(2));
                    mvwprintw(win, row++, 2, "[STABLE - Slot %d] %s (BG: %s) - %s", 
                             i, patients[i].name, patients[i].blood_group, statuses[i].message);
                    wattroff(win, COLOR_PAIR(2));
                }
            } else {
                mvwprintw(win, row++, 2, "[Slot %d] %s (BG: %s) - Analyzing...", 
                         i, patients[i].name, patients[i].blood_group);
            }
        }
    }
    
    if(!any_patient) {
        mvwprintw(win, 1, 2, "No patients currently monitored");
    }
    
    wrefresh(win);
}

void draw_donor_box(WINDOW *win, DonorResponse* donor, int has_donor, int searching) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, "[ DONOR STATUS ]");
    const wchar_t w_char[]=L"🚨";
    if(searching) {
        wattron(win, A_BOLD);
        mvwprintw(win, 2, 2, "Searching for donor...");
        wattroff(win, A_BOLD);
    } else if(has_donor) {
        if(donor->found) {
            wattron(win, A_BOLD | COLOR_PAIR(2));
            mvwprintw(win, 2, 2, "DONOR FOUND!");
            wattroff(win, A_BOLD | COLOR_PAIR(2));
            mvwprintw(win, 3, 2, "Name: %s", donor->name);
            mvwprintw(win, 4, 2, "Age: %d", donor->age);
            mvwprintw(win, 5, 2, "Blood Group: %s", donor->blood_group);
            mvwprintw(win, 6, 2, "Patient is SAFE! Blood transfusion arranged.");
        } else {
            wattron(win, A_BOLD | COLOR_PAIR(1));
            mvwprintw(win, 2, 2, "DONOR NOT FOUND %ls",w_char);
            wattroff(win, A_BOLD | COLOR_PAIR(1));
            mvwprintw(win, 4, 2, "No matching donor available in database.");
            mvwprintw(win, 5, 2, "Contacting external blood banks...");
        }
    } else {
        mvwprintw(win, 2, 2, "No donor search initiated");
    }
    
    wrefresh(win);
}

int main() {

	setlocale(LC_ALL,"");
    int shmid, msgid;
    patient* patients;
    PatientStatus patient_statuses[MAX_PATIENTS] = {0};
    int has_donor = 0, searching = 0;
    DonorResponse donor_response;
    
    // Initialize shared memory for patient vitals
    shmid = shmget(12345678, sizeof(patient) * MAX_PATIENTS, IPC_CREAT | 0666);//reading from shared memory
    if(shmid == -1) {
        perror("Shared memory failed");
        exit(1);
    }
    
    patients = (patient*)shmat(shmid, NULL, 0);
    if(patients == (void*)-1) {
        perror("Shared memory attach failed");
        exit(1);
    }
    
    // Initialize message queue
    msgid = msgget((key_t)MSG_KEY, IPC_CREAT | 0777);
    if(msgid == -1) {
        perror("Message queue failed");
        exit(1);
    }
    
    // Shared memory for patient registration
    int shmid_reg = shmget(123456, sizeof(person) * MAX_PATIENTS, IPC_CREAT | 0666);//reading from shared memory
    person* patient_reg = (person*)shmat(shmid_reg, NULL, 0);
    
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    
    // Define color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    
    // Create windows
    int height = LINES;
    int width = COLS;
    
    int vitals_height = height * 0.35;
    int status_height = height * 0.35;
    int donor_height = height * 0.35;
    
    WINDOW *vitals_win = newwin(vitals_height, width, 0, 0);
    WINDOW *status_win = newwin(status_height, width, vitals_height, 0);
    WINDOW *donor_win = newwin(donor_height, width, vitals_height + status_height, 0);
    
    int critical_patient_slot = -1;
    
    while(1) {
        // Read patient vitals
        draw_vitals_box(vitals_win, patients);
        
        // Check for status messages
        StatusMessage msg;
        if(msgrcv(msgid, &msg, sizeof(StatusMessage) - sizeof(long), 0, IPC_NOWAIT) != -1) {
            // Update status for this specific patient slot
            if(msg.slot_id >= 0 && msg.slot_id < MAX_PATIENTS) {
                patient_statuses[msg.slot_id].has_status = 1;
                patient_statuses[msg.slot_id].is_critical = msg.is_critical;
                strcpy(patient_statuses[msg.slot_id].message, msg.message);
            }
            draw_status_box(status_win, patients, patient_statuses);
            // If critical, search for donor
            if(msg.is_critical && critical_patient_slot != msg.slot_id) {
                critical_patient_slot = msg.slot_id;
                searching = 1;
                has_donor = 0;
                draw_donor_box(donor_win, &donor_response, has_donor, searching);
                
                sleep(1); // Simulate search time
                
                donor_response = search_donor(msg.blood_group);
                searching = 0;
                has_donor = 1;
                draw_donor_box(donor_win, &donor_response, has_donor, searching);
                /*if(donor_response.found) {
                    // Discharge patient - mark as inactive
                    patients[msg.slot_id].active = 0;
                    patient_reg[msg.slot_id].active = 0;
                    patient_statuses[msg.slot_id].has_status = 0;
                    critical_patient_slot = -1;
                }*/
                
                if (donor_response.found) {
		    // Ask user whether to discharge patient
		    echo(); // Enable user input
		    curs_set(1); // Show cursor for input
		    mvwprintw(donor_win, 7, 2, "Remove patient? (y/n): ");
		    wrefresh(donor_win);

		    char choice = wgetch(donor_win);
		    noecho();
		    curs_set(0);

		    if (choice == 'y' || choice == 'Y') {
			// Discharge patient - mark as inactive
			patients[msg.slot_id].active = 0;
			patient_reg[msg.slot_id].active = 0;
			patient_statuses[msg.slot_id].has_status = 0;
			critical_patient_slot = -1;
			mvwprintw(donor_win, 7, 2, "Patient removed successfully.");
		    } else {
			mvwprintw(donor_win, 7, 2, "Patient retained. Monitoring continues.");
			//patient_reg[msg.slot_id].active = 0;
			patient_statuses[msg.slot_id].has_status = 0;
			critical_patient_slot = -1;
		    }
		    wrefresh(donor_win);
		}
            }
        }
        
        draw_status_box(status_win, patients, patient_statuses);
        draw_donor_box(donor_win, &donor_response, has_donor, searching);
        
        // Check for 'q' to quit
        int ch = getch();
        if(ch == 'q' || ch == 'Q') {
            break;
        }
        
        usleep(500000); // 500ms refresh
    }
    
    // Cleanup
    delwin(vitals_win);
    delwin(status_win);
    delwin(donor_win);
    endwin();
    
    shmdt(patients);
    shmdt(patient_reg);
    
    return 0;
}
