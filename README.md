# Patient Vitals Monitoring System

# and Donor Management System

## Overview

##### A distributed, multi-process hospital management system featuring real-time patient

##### monitoring, remote donor database management via TCP sockets, and automated critical care

##### response with intelligent donor matching.

## Key Features

- **Distributed Architecture** : Donor server runs on separate device
- **Real-time Monitoring** : Simultaneous tracking of up to 5 patients
- **Multi-threading** : Parallel vital sign generation (HR, BP, O2)
- **Socket Communication** : TCP-based inter-device communication
- **Automatic Critical Response** : Auto-donor search and patient discharge
- **Web Dashboards** : Real-time web interfaces for monitoring
- **IPC Integration** : Shared memory, message queues, and semaphores


## System Components

### 1. donor.c - Remote Donor Server 🩸

##### Location : Runs on separate device

##### Responsibilities :

- TCP socket server listening on port 8080
- Manages donor database using message queue + donor.bin file
- Handles two socket operations:

##### o Add Donor : Stores donor info in message queue and persistent binary file

##### o Search Donor : Finds matching blood group donor from message queue

##### o Remove Donor : Automatically removes matched donor from queue after

##### successful match

##### Key Functions :

- Socket server setup on port 8080
- Donor addition → Message queue + donor.bin
- Donor search → Query message queue by blood group
- Donor removal → Delete from queue after match

### 2. main.c - Registration Interface 📋

##### Location : Main system device

##### Features :

- ncurses-based terminal UI with emoji support 🚑🛏️🤒
- Real-time bed availability tracking (shows X/5 beds)
- Two registration modes:

##### o Donor Registration : Connects to remote donor server via TCP socket

##### o Patient Registration : Adds to shared memory (max 5 concurrent)

- Dynamic bed management with "BEDS ARE FULL" alert

##### Socket Communication :

connect_to_donor_server() → Establishes TCP connection

###### add_donor_to_server() → Sends donor data via socket


### 3. patient.c - Patient Vital Monitor

##### Location : Main system device

##### Responsibilities :

- Monitors up to 5 patients simultaneously in separate slots
- Multi-threaded vital generation (3 threads per patient):

##### o Heart Rate: 60-140 bpm

##### o Blood Pressure: 90-150 / 60-100 mmHg

##### o Oxygen Level: 88-100%

- Updates every 3 seconds
- Web dashboard on port 8081

##### Threading Model :

pthread_create() → heart_rate()
pthread_create() → bloodPressure()

###### pthread_create() → oxygen_level()

### 4. lab.c - Laboratory Analysis

##### Location : Main system device

##### Critical Condition Detection :

##### Analyzes patient vitals and hemoglobin levels to detect:

- High HR (>120) + Low HB (<8)
- Low BP (<90/65) + Low HB (<8)
- Low O2 (<90%) + Low HB (<8)

##### Actions :

- Sends status messages via message queue
- Marks patients as CRITICAL or STABLE
- Web dashboard on port 8082


### 5. ui.c - Main Display Interface 🖥️

##### Location : Main system device

##### Three-Panel ncurses UI :

┌─────────────────────────────────────┐
│ PATIENT VITALS (Live Updates) │
│ Bed 0: John (HR:85 BP:120/80 O2:98)│
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│ PATIENT STATUS (Alerts) │
│ 🚨 CRITICAL - Slot 1: Mary (AB+) │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│ DONOR STATUS (Socket Response) │
│ ✅ DONOR FOUND! Blood transfusion │
└─────────────────────────────────────┘

##### Automated Workflow :

##### 1. Receives critical alert from lab

##### 2. Connects to donor server via TCP socket

##### 3. Searches for matching blood group donor

##### 4. If found: Discharges patient + removes donor from queue

##### 5. If not found: Shows "Contacting external blood banks"


## System Architecture (refer pdf document in github)

┌─────────────────────────────────────────────────────────┐
│ DEVICE 1 (Main System) │
│ │
│ main.c (Registration) │
│ ├─> TCP Socket ──────────────┐ │
│ └─> Shared Memory │ │
│ ↓ │ │
│ patient.c (Monitor) │ │
│ ↓ │ │
│ [3 Threads × 5 Patients] │ │
│ ↓ │ │
│ Shared Memory (Vitals) │ │
│ ↓ │ │
│ lab.c (Analysis) │ │
│ ↓ │ │
│ Message Queue (Status) │ │
│ ↓ │ │
│ ui.c (Display) │ │
│ ├─> Display UI │ │
│ └─> TCP Socket─────┼──────────────┐ │
└──────────────────────────────────┼──────────────┼────────┘
│ │
┌──────────┴──────────┐ │
│ NETWORK (TCP) │ │
└──────────┬──────────┘ │
│ │
┌──────────────────────────────────┼──────────────┼────────┐
│ DEVICE 2 (Donor Server) │ │
│ │ │
│ donor.c │ │
│ (TCP Server - Port 8080) │ │
│ ↓ ←┘ │
│ ┌──────────┴──────────┐ │
│ │ Message Queue │ │
│ │ (Donor Database) │ │
│ └──────────┬──────────┘ │
│ ↓ │
│ donor.bin │
│ (Persistent Storage) │
└───────────────────────────────────────────────────────────┘

## Installation & Setup

#### Prerequisites

# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential libncurses5-dev gcc

#### Compilation

##### Create Makefile (or compile manually):

CC = gcc
CFLAGS = -pthread
LIBS = -lncurses -lrt


all: donor patient lab ui main
donor: donor.c
$(CC) donor.c - o donor $(CFLAGS)
patient: patient.c
$(CC) patient.c -o patient $(CFLAGS) $(LIBS)
lab: lab.c
$(CC) lab.c -o lab $(CFLAGS)
ui: ui.c
$(CC) ui.c -o ui $(LIBS)
main: main.c
$(CC) main.c -o main $(LIBS)
clean:
rm -f donor patient lab ui main donor.bin
ipcrm -a 2>/dev/null || true

###### .PHONY: all clean

#### Compile :

###### make all

## Running the System

### Option A: Same Device (Development/Testing)

#### Method 1: Manual Terminal Launch

# Terminal 1: Donor Server
./donor
# Terminal 2: Patient Monitor
./patient
# Terminal 3: Lab Analysis
./lab
# Terminal 4: UI Display
./ui
# Terminal 5: Registration

###### ./main


#### Method 2: Automated Script

##### Use the provided main.sh:

###### ./main.sh

##### The script automatically opens separate terminals for each component.

### Option B: Different Devices (Production)

##### Device 1: Donor Server Setup

gcc donor.c -o donor -pthread

###### ./donor

##### You should see: Donor server started on port 8080...

##### Device 2: Main System Setup

##### 1. Edit source files - Update IP address in both files:

##### main.c :

###### #define SERVER_IP "192.168.1.XXX" // Replace with donor server's IP

##### ui.c :

###### #define SERVER_IP "192.168.1.XXX" // Same IP as above

##### 2. Compile and run :


make all
./patient &
./lab &
./ui &

###### ./main

## Web Dashboards

##### The system provides three real-time web interfaces:

#### Patient Monitoring Dashboard

- **URL** : [http://localhost:](http://localhost:)
- **Features** : Live vital signs for all active patients
- **Auto-refresh** : Every 3 seconds

#### Lab Analysis Dashboard

- **URL** : [http://localhost:](http://localhost:)
- **Features** : Hemoglobin levels and critical status
- **Color-coded** : Red (critical) / Green (stable)

#### Donor Dashboard(In another device)

- **URL** : [http://localhost:](http://localhost:)
- **Features** : Details of donor, status of request whether donor found or not
- **Data: Added everytime data entered in main or everytime searched for data**


## Usage Guide

### 1. Register Donors (Option 1 in main.c)

Enter name: Harsha
Enter age: 22

###### Enter blood group: O+

- Data sent to remote donor server via TCP socket
- Stored in message queue + donor.bin file
- Confirmation: "Donor registered successfully!"

### 2. Register Patients (Option 2 in main.c)

Enter name: Madhuri
Enter age: 22

###### Enter blood group: AB+

- Assigned to monitoring slot (0-4)
- Shows: "Assigned to Bed X (monitoring...)"
- Vital signs generation starts automatically
- If full: "BEDS ARE FULL!" message


### 3. Monitor Patients (Automatic - ui.c)

##### Top Panel : Real-time vitals

[Slot 0: Madhuri (Age: 22, BG: AB+)]

###### HR: 85 bpm | BP: 120/80 | O2: 98%

##### Middle Panel : Status updates

[STABLE - Slot 0] Madhuri (BG: AB+) - All vitals normal

### 4. Critical Care Response (Automatic)

##### When lab detects critical condition:

##### 1. Alert displayed :

🚨 [CRITICAL - Slot 0] Madhuri (AB+)

###### CRITICAL: Low HB=7.0, HR=125, BP=85/60, O2=88%

##### 2. Donor search initiated :

[DONOR STATUS]

###### Searching for donor...

##### 3. Socket communication to donor server :

- Connects to SERVER_IP:
- Sends search request with blood group
- Receives donor match result

##### 4. Outcome A - Donor Found ✅:

DONOR FOUND!
Name: Harsha
Age: 22
Blood Group: AB+

###### Patient is SAFE!

- Patient discharged automatically
- Donor removed from message queue on donor server
- Bed freed for new patient

##### 5. Outcome B - Donor Not Found ❌:

DONOR NOT FOUND 🚨
No matching donor available in database.

###### Contacting external blood banks...


## Testing Scenarios

#### Test 1: Normal Flow

##### Scenario: Complete workflow with successful donor match

##### 1. Start donor server on Device 1

##### 2. Start all components on Device 2

##### 3. Register donor: Name=Madhuri, Age=22, BG=O+

##### 4. Register patient: Name=Adithya, Age=22 BG=O+

##### 5. Wait for vitals to generate (3 sec intervals)

##### 6. Observe patient status in UI

##### 7. If critical alert triggers:

##### o Verify donor search message

##### o Confirm "DONOR FOUND" result

##### o Check patient discharge

##### o Verify bed freed (Available Beds: 5/5)


#### Test 2: No Donor Available

##### Scenario: Critical patient with no matching donor

##### 1. Register patient with rare blood group (AB-)

##### 2. Do NOT register any AB- donors

##### 3. Wait for critical condition

##### 4. Expected: "DONOR NOT FOUND 🚨" message

##### 5. Patient remains in system (not discharged)

#### Test 3: Full Capacity

##### Scenario: Bed management under load

##### 1. Register 5 patients (fill all beds)

##### 2. Try to register 6th patient

##### 3. Expected: "BEDS ARE FULL!" message

##### 4. Wait for one patient to become critical

##### 5. When donor found and patient discharged

##### 6. Available beds increases to 1/

##### 7. Now register 6th patient successfully

#### Test 4: Multiple Critical Patients

##### Scenario: Sequential donor matching

##### 1. Register 5 donors (various blood groups)

##### 2. Register 5 patients (matching blood groups)

##### 3. Multiple patients may become critical

##### 4. Observe sequential donor searches

##### 5. Verify donors removed after each match

##### 6. Check remaining donor count in donor.bin

#### Test 5: Network Resilience

##### Scenario: Connection failure handling

##### 1. Stop donor server on Device 1

##### 2. Register patient on Device 2

##### 3. When critical alert triggers

##### 4. Expected: "Cannot connect to donor server"

##### 5. Restart donor server


##### 6. Next critical patient should connect successfully

## Files Generated

##### File Purpose Location

##### donor.bin Persistent donor database Donor server device

##### Shared memory

##### segments

##### Patient vitals & registration

##### Main system (auto-

##### cleaned)

##### Message queues

##### Status messages & donor

##### queue

##### Both devices (auto-

##### cleaned)

##### Semaphores Process synchronization

##### Main system (auto-

##### cleaned)


## Cleanup

#### Graceful Shutdown

##### Press q in UI to exit gracefully

#### Manual Cleanup

# Stop all processes
make stop
# Clean IPC resources
ipcrm -a
# Remove binaries and files
make clean
rm -f donor.bin
# Verify cleanup

###### ipcs # Should show no resources

## Security Notes

- Donor server accepts connections from any IP (use firewall rules)
- No authentication implemented (add for production)
- Socket communication unencrypted (consider TLS)
- File permissions on donor.bin (set to 600 for security)
- IPC resources visible to all users (use proper permissions)


## System Limits

##### Resource Limit Configurable

##### Max concurrent

##### patients

##### 5 Yes (MAX_PATIENTS in headers.h)

##### Donor database size Unlimited Limited by disk space

##### Vital update frequency 3 seconds Yes (change sleep(3) in patient.c)

##### Message queue size

##### System

##### dependent

##### OS limit

##### Socket timeout 30 seconds

##### Yes (add SO_RCVTIMEO in socket

##### code)


## Code Structure

##### hospital-management/

##### ├── headers.h # Common structures and definitions

##### ├── main.c # Registration interface (socket client)

##### ├── patient.c # Vital signs monitor (multi-threaded)

##### ├── lab.c # Critical condition analyzer

##### ├── ui.c # Main display (socket client)

##### ├── donor.c # Donor server (socket server)

##### ├── main.sh # Automated launcher script

##### ├── Makefile # Build configuration

##### └── README.md # This file

## Key Technical Details

#### IPC Mechanisms Used

##### Mechanism Purpose Key Value

##### Shared Memory 1 Patient registration data 123456

##### Shared Memory 2 Patient vitals 12345678

##### Message Queue 1 Lab status messages 789 (MSG_KEY)

##### Message Queue 2 Donor database (donor.c) 456

##### Semaphore 1 Patient registration sync /semos

##### Semaphore 2 Vitals update sync /semos


## Port Configuration

##### Port Service Protocol

##### 8080 Donor Server (TCP Socket) TCP

##### 8081 Patient Monitoring Dashboard HTTP

##### 8082 Lab Analysis Dashboard HTTP

#### Critical Conditions Logic

##### A patient is marked as CRITICAL when:

###### IF (Heart_Rate > 120 AND Hemoglobin < 8)

###### OR (BP_Systolic < 90 AND BP_Diastolic < 65 AND Hemoglobin < 8)

###### OR (Oxygen_Level < 90 AND Hemoglobin < 8)

###### THEN

###### Status = CRITICAL

###### Trigger_Donor_Search()

###### ELSE

##### Status = STABLE


## Data Flow Diagram

DONOR REGISTRATION FLOW:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
User Input (main.c)
↓
TCP Socket Connection (port 8080)
↓
donor.c receives DonorRequest
↓
Store in Message Queue
↓
Append to donor.bin file
↓
Send acknowledgment to main.c
↓
Display "Donor registered successfully!"
PATIENT REGISTRATION FLOW:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
User Input (main.c)
↓
Check available beds (shared memory)
↓
If beds available:
Add to Shared Memory 1 (key: 123456)
Post semaphore (/semos)
↓
patient.c detects new patient
↓
Create 3 threads (HR, BP, O2)
↓
Store vitals in Shared Memory 2 (key: 12345678)
↓
Post semaphore (/semos5)
CRITICAL CARE FLOW:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
lab.c monitors vitals (Shared Memory 2)


↓
Detect critical condition
↓
Send StatusMessage to Message Queue (key: 789)
↓
ui.c receives critical alert
↓
TCP Socket to donor.c (port 8080)
↓
Search donor by blood group
↓
If found:
├─> Remove donor from message queue
├─> Display "DONOR FOUND"
├─> Set patient.active = 0
└─> Free bed
Else:
└─> Display "DONOR NOT FOUND"

## Learning Outcomes

##### This project demonstrates:

- **Socket Programming** : Client-server architecture using TCP sockets
- **Multi-threading** : Parallel execution using pthreads
- **IPC Mechanisms** : Shared memory, message queues, semaphores
- **Process Synchronization** : Coordinating multiple processes
- **ncurses UI** : Terminal-based user interfaces
- **Web Servers** : Simple HTTP servers for real-time dashboards
- **File I/O** : Binary file operations for persistent storage
- **Distributed Systems** : Communication between separate devices
- **Real-time Systems** : Time-critical patient monitoring


