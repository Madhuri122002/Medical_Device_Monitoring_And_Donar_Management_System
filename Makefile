CC = gcc
CFLAGS = -pthread -lrt -lncursesw
TARGETS = main patient lab ui headers

all: $(TARGETS)


main: main.c
	$(CC) main.c -o main $(CFLAGS)

patient: patient.c
	$(CC) patient.c -o patient -pthread -lrt

lab: lab.c
	$(CC) lab.c -o lab -lrt

ui: ui.c
	$(CC) ui.c -o ui $(CFLAGS)
	
headers: headers.h
	$(CC) headers.h -o headers
clean:
	rm -f $(TARGETS)
	ipcrm -a


stop:
	pkill -f "./patient"
	pkill -f "./lab"
	pkill -f "./ui"
	ipcrm -a

.PHONY: all clean run_main run_patient run_lab run_ui run_all stop
