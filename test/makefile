supervisor:supervisor.o accessories.o dispatcher
	gcc -o ./bin/supervisor  supervisor.o accessories.o

supervisor.o:supervisor.c accessories.h
	gcc -c supervisor.c

compileAll:sendfdUsingSocket recvfdUsingSocket

sendfdUsingSocket.o:sendfdUsingSocket.c accessories.h
	gcc -c sendfdUsingSocket.c

sendfdUsingSocket:sendfdUsingSocket.o accessories.o
	gcc -o ./bin/sendfdUsingSocket sendfdUsingSocket.o accessories.o

recvfdUsingSocket.o:recvfdUsingSocket.c accessories.h
	gcc -c recvfdUsingSocket.c

recvfdUsingSocket:recvfdUsingSocket.o accessories.o
	gcc -o ./bin/recvfdUsingSocket recvfdUsingSocket.o accessories.o

dispatcher:dispatcher.o access.o psql.o
	g++ dispatcher.o access.o psql.o -pthread -I /usr/include/postgresql -lpq -o ./bin/dispatcher

dispatcher.o:dispatcher.cpp
	g++ -I /usr/include/postgresql -lpq -c dispatcher.cpp

accessories.o: accessories.c accessories.h
	gcc -c accessories.c

access.o:access.cpp access.h
	g++ -c access.cpp

psql.o:psql.cpp psql.h
	g++ -c psql.cpp -I /usr/include/postgresql -lpq


cleanAll:cleanO clean~
	
cleanO:
	rm *.o 

clean~:
	rm *~
# sendfdUsingSocket:sendfdUsingSocket.o accessories.o
# 	gcc -o sendfdUsingSocket sendfdUsingSocket.o accessories.o

# sendfdUsingSocket.o:sendfdUsingSocket.c
# 	gcc -c sendfdUsingSocket.c
