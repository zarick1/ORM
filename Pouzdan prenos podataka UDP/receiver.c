// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <ncurses.h>

#define DEBUGINFO1

#define PORT 8080

#define MAXCHAR 52
#define MAXLINE 17

#define TIMEOUT 10							// Max. number of seconds to wait for message
#define SLEEPTIME 1

#define OUTPUTFILE "receivedMessage.txt"

int sockfd;
struct sockaddr_in servaddr;
struct sockaddr_in cliaddr;
int len;

WINDOW* mainWindow;

void cleanUp() {

	close(sockfd);

	mvwprintw(mainWindow, MAXLINE + 3, 2, " PROGRAM ENDED. PRESS ANY KEY TO CONTINUE ");
	wrefresh(mainWindow);
	getch();
	endwin();

	printf("Socket closed.\n");
	printf("Program exited successfully.\n");
	exit(EXIT_SUCCESS);
}

void getCurrentTime(char* packageTime) {
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	snprintf(packageTime, strlen(asctime(timeinfo)), "%s", asctime(timeinfo));
}

void checkTerminalSize(int yMax, int xMax) {

	char timeStamp[30];
	getCurrentTime(timeStamp);
	int currentTimeStamp = strlen(timeStamp);

	int minTerminalHeight = MAXLINE + 2;
	int minTerminalWidth = currentTimeStamp + MAXCHAR + 2 + 4;

	if ((yMax < (minTerminalHeight + 4)) || (xMax < (minTerminalWidth + 4))) {	// +4 zbog box okvira

		mvwprintw(mainWindow, 2, 2, "RESIZE TERMINAL");
		mvwprintw(mainWindow, 3, 2, "MIN. HEIGHT: %d", minTerminalHeight + 4);
		mvwprintw(mainWindow, 4, 2, "MIN. WIDTH: %d", minTerminalWidth + 4);
		wrefresh(mainWindow);
		cleanUp();
	}
}

void initializeOutputFile() {

	FILE* filePointer;
	filePointer = fopen(OUTPUTFILE, "a");

	if (filePointer == NULL)
	{
		perror("file error");
		exit(EXIT_FAILURE);
	}

	fprintf(filePointer, "===== NEW RECEIVED MESSAGES =====\n");
	fclose(filePointer);
}

void writeMessageToFile(char* message, int id, WINDOW* win) {

	FILE* filePointer;
	filePointer = fopen(OUTPUTFILE, "a");

	if (filePointer == NULL)
	{
		perror("file error");
		exit(EXIT_FAILURE);
	}

	char packageTime[MAXCHAR];
	getCurrentTime(packageTime);

	mvwprintw(win, id % MAXLINE + 1, 2, "%s : %s", packageTime, message);
	box(win, 0, 0);

	refresh();
	wrefresh(win);

	fprintf(filePointer, "%s : %s", packageTime, message);
	fclose(filePointer);
}

int checkPackage(char* message, int* id) {

	char header[MAXCHAR] = "";

	char* token = strtok(message, ".");

	strcat(header, token);
	strcat(header, ".");

	if (atoi(token) != 0)
	{
		#ifdef DEBUGINFO
		mvwprintw(mainWindow, 1, 2, "[Wrong package type: %d, expected: 0]\n", atoi(token));
		wrefresh(mainWindow);
		#endif
		return 0;
	}

	token = strtok(NULL, ".");
	*id = atoi(token);

	strcat(header, token);
	strcat(header, ".");

	token = strtok(NULL, ".");

	strcat(header, token);

	token = strtok(NULL, ".");

	strncpy(message, token, MAXCHAR);

	#ifdef DEBUGINFO
	mvwprintw(mainWindow, 1, 2, "                                     ");
	mvwprintw(mainWindow, 1, 2, "[Received package header] : %s", header);
	wrefresh(mainWindow);
	#endif

	#ifndef DEBUGINFO
	mvwprintw(mainWindow, 1, 2, "[Newest message] : %s", message);
	wrefresh(mainWindow);
	#endif

	return 1;
}

void sendACK(int id) {

	char message[MAXCHAR];
	char ack[4] = "ACK";

	snprintf(message, MAXCHAR, "1.%d.%s", id, ack);

	sendto(sockfd, (const char *) message, strlen(message),
		0, (const struct sockaddr *) &cliaddr, len);

	#ifdef DEBUGINFO
	mvwprintw(mainWindow, 1, 40, "                      ");
	mvwprintw(mainWindow, 1, 40, "[ACK sent] : %s", message);
	wrefresh(mainWindow);
	#endif
}

void receivePackage(char* buffer) {

	memset(buffer, 0, MAXCHAR);

	int n = recvfrom(sockfd, (char *) buffer, MAXCHAR, 0,
				(struct sockaddr *) &cliaddr, &len);

	if (n < 0)
	{
		mvwprintw(mainWindow, 1, 2, "[There were no messages for %d seconds]\n", TIMEOUT);
		wrefresh(mainWindow);
		cleanUp();
	}

	buffer[n] = '\0';

}

// Driver code
int main() {

	char buffer[MAXCHAR];

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, cleanUp);	// CTRL + C

	struct timeval read_timeout;
	read_timeout.tv_sec = TIMEOUT;
	read_timeout.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof (read_timeout)) < 0)
	{
		perror("setsockopt() error");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);

	// Bind the socket with the server address
	if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	initializeOutputFile();

	int packageId = -1;

	initscr();

	WINDOW* messageWindow;

	int yMax, xMax;
	getmaxyx(stdscr, yMax, xMax);

	mainWindow = newwin(yMax, xMax - 2, 0, 1);
	box(mainWindow, 0, 0);
	mvwprintw(mainWindow, 0, 2, " RECEIVER ");
	refresh();

	checkTerminalSize(yMax, xMax);

	messageWindow = newwin(MAXLINE + 2, xMax - 4, 2, 2);
	box(messageWindow, 0, 0);
	refresh();

	mvwprintw(mainWindow, 1, 2, "Waiting for messages...");
	wrefresh(mainWindow);

	while (1)
	{
		receivePackage(buffer);

		sleep(SLEEPTIME);

		if (checkPackage(buffer, &packageId))
		{
			writeMessageToFile(buffer, packageId, messageWindow);
			sendACK(packageId);
		}

	}

	endwin();

	return 0;
}
