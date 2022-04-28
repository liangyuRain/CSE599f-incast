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
	dest := *host + ":" + *port
	fmt.Printf("Connecting to %s\n", dest)
	conn, err := net.Dial("tcp", dest)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	defer conn.Close()
	clientReader := bufio.NewReader(os.Stdin)
	serverReader := bufio.NewReader(conn)

	for {
		clientRequest, err := clientReader.ReadString('\n')
		switch err {
		case nil:
			clientRequest = strings.TrimSuffix(clientRequest, "\n")
			conn.Write([]byte(clientRequest + "\n"))
		case io.EOF:
			fmt.Println("Client closed the connection")
			return
		default:
			fmt.Println(err)
			return
		}

		serverReply, err := serverReader.ReadString('~')
		switch err {
		case nil:
			fmt.Printf("Server reply: %s\n", serverReply)
		case io.EOF:
			fmt.Println("Server closed the connection")
			return
		default:
			fmt.Println(err)
			return
		}
	}
}
