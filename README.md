File description
	1. server.cpp: this runs the server program
	2. client.cpp: this files runs the client program
        
Compiling
      1. "make server": this command creates an executable for server.cpp file named server.x
      2. "make client": this command create an executable for client.cpp file named client.x
      
Executing
	1. for executing server: server.x configserver.txt
	2. for executing client: client.x configclient.txt
	where configserver.txt and configclient.txt are configuration files
	
About pthread and select():

1. Here I have used pthread in server program to create a new thread to handle each client's connection.
The function process_connection() is called in a new thread for each client connection. The arg parameter of the process_connection() is a pointer to an int that contains the client socket file descriptor. The function then reads and processes messages from the client using the socket file descriptor. To create anew thread for each client connection, the program calls pthread_create and passes a pointer to the process_connection() and the client socket file descriptor as arguments.

2. Here I have used select()and pthread_detach() in client which is used to allow the program to wait for input from the server socket and standard input simultaneously. When input is detected on the server socket, the program reads the input and displays it on the screen. When input is detected on standard input, the program reads the input and sends it to the server. Whereas pthread_detach is used to detach the thread that handles incoming messages from the server. 

