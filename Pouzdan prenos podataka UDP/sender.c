// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <ncurses.h>

#define PORT 8080
#define MAXCHAR 52
#define MAXLINE 17
#define MAXSEND 5					// Max. number of package re-transmisions
#define TIMEOUT 3					// Max. number of seconds to wait for ACK
#define INPUTFILE "message.txt"
#define SLEEPTIME 1

int sockfd;
struct sockaddr_in servaddr;

int sendCounter = 0;

WINDOW* mainWindow;
WINDOW* messageWindow;

typedef struct Header {
	int type;	// 0 - data, 1 - ack
	int id;
	int dataSize;
} Header;

typedef struct Package {
	struct Header header;
	char* data;
} Package;

void cleanUp() {

	close(sockfd);

	mvwprintw(mainWindow, 6, 2, " PROGRAM ENDED. PRESS ANY KEY TO CONTINUE ");
	wrefresh(mainWindow);
	getch();
	endwin();

	printf("Socket closed.\n");
	printf("Program exited successfully.\n");
	exit(EXIT_SUCCESS);
}

void freePackages(struct Package package[], int numberOfPackages) {

	for (int i = 0; i < numberOfPackages; i++)
	{
		free(package[i].data);
	}
}

void checkTerminalSize(int yMax, int xMax) {

	int minTerminalHeight = 9;	// 5 +4 zbog box okvira
	int minTerminalWidth = 65;	// 61 +4 zbog box okvira

	if ((yMax < minTerminalHeight) || (xMax < minTerminalWidth)) {

		mvwprintw(mainWindow, 2, 2, "RESIZE TERMINAL");
		mvwprintw(mainWindow, 3, 2, "MIN. HEIGHT: %d", minTerminalHeight);
		mvwprintw(mainWindow, 4, 2, "MIN. WIDTH: %d", minTerminalWidth);
		wrefresh(mainWindow);
		cleanUp();
	}
}

int checkACK(char* string, int id) {

	if (atoi(string) == 0)
	{
		mvwprintw(messageWindow, 2, 2, "[ACK timeout expired (%d)]", sendCounter);
		wrefresh(messageWindow);
		return 0;
	}

	char* token = strtok(string, ".");

	if (atoi(token) != 1)
	{
		printf("[Wrong ACK type: %d, expected: 1]\n", atoi(token));
		return 0;
	}

	token = strtok(NULL, ".");

	if (atoi(token) != id)
	{
		printf("[Wrong ACK id: %d, expected: %d]\n", atoi(token), id);
		return 0;
	}

	token = strtok(NULL, ".");

	if (strncmp(token, "ACK", strlen("ACK")) != 0)
	{
		printf("[Wrong ACK data: %s, expected: ACK]\n", token);
		return 0;
	}

	return 1;
}

void getNumberOfPackages(int* numberOfPackages)
{
	FILE* filePointer;
    char* line = NULL;
    size_t lineLength = 0;
    ssize_t read;

	filePointer = fopen(INPUTFILE, "r");

	if (filePointer == NULL)
	{
		perror("file error");
    	exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &lineLength, filePointer)) != EOF) {
        (*numberOfPackages)++;
    }

	//printf("[Number of packages: %d]\n", *numberOfPackages);

	fclose(filePointer);
}

void getAllPackages(struct Package package[])
{
	FILE* filePointer;
    char* line = NULL;
    size_t lineLength = 0;
    ssize_t read;

	filePointer = fopen(INPUTFILE, "r");

	if (filePointer == NULL)
	{
		perror("file error");
    	exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &lineLength, filePointer)) != EOF)
	{
		static int i = 0;

		package[i].data = (char* ) malloc(MAXCHAR * sizeof(char));
		strncpy(package[i].data, line, MAXCHAR);
		package[i].data[read] = '\0';

		i++;
    }

    fclose(filePointer);
    if (line)
	{
        free(line);
	}
}

void sendPackage(struct Package package[], int id)
{
	package[id].header.type = 0;
	package[id].header.id = id;
	package[id].header.dataSize = strlen(package[id].data) * sizeof(char);

	char buffer[MAXCHAR];
	snprintf(buffer, MAXCHAR, "%d.%d.%d.%s", package[id].header.type, package[id].header.id,
				package[id].header.dataSize, package[id].data);

	int n = sendto(sockfd, (const char *) buffer, strlen(buffer), 0,
				(const struct sockaddr *) &servaddr, sizeof(servaddr));

	if (n < 0)
	{
		mvwprintw(mainWindow, 1, 2, "sendto() error in function: %s", __func__);
		wrefresh(mainWindow);
		cleanUp();
	}

	mvwprintw(messageWindow, 1, 2, "[Package with id (%d) sent]", id);
	wrefresh(messageWindow);

	sendCounter++;
}

void receivePackage(struct Package package[], int* currentId) {

	int n, len;
	char buffer[MAXCHAR];

	mvwprintw(messageWindow, 1, 40, "Waiting for ACK...");
	wrefresh(messageWindow);

	n = recvfrom(sockfd, (char *) buffer, MAXCHAR, 0,
			(struct sockaddr *) &servaddr, &len);

	buffer[n] = '\0';

	if (checkACK(buffer, *currentId))
	{
		mvwprintw(messageWindow, 2, 2, "[ACK with id (%d) received]", *currentId);
		wrefresh(messageWindow);
		*currentId = *currentId + 1;
		sendCounter = 0;
	}
	else
	{
		mvwprintw(messageWindow, 2, 40, "Sending again...");
		wrefresh(messageWindow);
	}
}

// Driver code
int main() {

	int numberOfPackages = 0;
	int currentId = 0;

	getNumberOfPackages(&numberOfPackages);

	struct Package packages[numberOfPackages];

  	getAllPackages(packages);

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, cleanUp);

	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = INADDR_ANY;

	// Set waiting time for receiving ACK in recvfrom()
	struct timeval read_timeout;
	read_timeout.tv_sec = TIMEOUT;
	read_timeout.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)) < 0)
	{
		perror("setsockopt() error");
		exit(EXIT_FAILURE);
	}

	initscr();
	curs_set(0);

	int yMax, xMax;
	getmaxyx(stdscr, yMax, xMax);

	mainWindow = newwin(yMax, xMax - 2, 0, 1);
	box(mainWindow, 0, 0);
	mvwprintw(mainWindow, 0, 2, " SENDER ");
	mvwprintw(mainWindow, 1, 2, "[Number of packages: %d]", numberOfPackages);
	refresh();
	wrefresh(mainWindow);

	checkTerminalSize(yMax, xMax);

	messageWindow = newwin(5, xMax - 4, 2, 2);
	box(messageWindow, 0, 0);
	refresh();

	while (1)
	{

		if (sendCounter >= MAXSEND)
		{
			mvwprintw(messageWindow, 3, 2, "[Didn't receive ACK with id (%d) after %d tries]", currentId, sendCounter);
			wrefresh(messageWindow);
			freePackages(packages, numberOfPackages);
			cleanUp();
		}

		sendPackage(packages, currentId);

		receivePackage(packages, &currentId);

		if (currentId == numberOfPackages)
		{
			mvwprintw(messageWindow, 3, 2, "[All packages have been sent]");
			wrefresh(messageWindow);
			freePackages(packages, numberOfPackages);
			cleanUp();
		}

		sleep(SLEEPTIME);
	}

	return 0;
}
