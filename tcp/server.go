package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"strings"
)

func main() {
	var host = flag.String("host", "localhost", "Host IP")
	var port = flag.String("port", "9369", "Port")
	flag.Parse()
	addr := *host + ":" + *port
	fmt.Printf("Listening to %s\n", addr)
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	defer listener.Close()

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println(err)
			continue
		}
		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	defer conn.Close()
	clientReader := bufio.NewReader(conn)
	for {
		clientRequest, err := clientReader.ReadString('\n')
		switch err {
		case nil:
			clientRequest = strings.TrimSuffix(clientRequest, "\n")
			fmt.Printf("Client request: %s\n", clientRequest)
			conn.Write([]byte("Hello World~"))
		case io.EOF:
			fmt.Println("Client closed the connection")
			return
		default:
			fmt.Println(err)
			return
		}
	}
}
