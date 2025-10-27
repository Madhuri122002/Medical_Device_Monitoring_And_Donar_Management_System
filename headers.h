#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<ncurses.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<locale.h>
#include<wchar.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>
#define MAX_PATIENTS 5
#define MSG_KEY 789
#define PORT 8080
#define SERVER_IP "192.168.1.140"

//taking input from main for patient
typedef struct persons {
    char name[20];
    int age;
    char bg[5];
    int active; // 1: active, 0: discharged
} person;

//taking input from main for donor
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

//to check BP
typedef struct BloodPressure {
    int systollic;
    int diastollic;
} blood_pressure;

//to check all vitals of patient
typedef struct PatientDetails {
    int age;
    float hr;
    float o2;
    char blood_group[5];
    blood_pressure bp;
    char name[20];
    int active;
    int slot_id;
} patient;

//To check lab result whether the result is critical or not
typedef struct {
    int slot_id;
    char name[20];
    char blood_group[5];
    int hb_level;
    int is_critical;
    char status[100];
} LabResult;

//structure to generate status message
typedef struct {
    long mtype;
    int slot_id;
    char name[20];
    char blood_group[5];
    int is_critical;
    char message[100];
} StatusMessage; 

// Structure to hold current status for each patient slot
typedef struct {
    int has_status;
    int is_critical;
    char message[100];
} PatientStatus;

#endif
