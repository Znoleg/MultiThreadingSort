all:
	g++ Server.cpp -o Server
	g++ Client.cpp -o Client
	./Server 1000
	./Client "znoleg-VirtualBox" 1000
