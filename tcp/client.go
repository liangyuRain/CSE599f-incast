package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
)

func main() {
	var host = flag.String("host", "localhost", "Host IP")
	var port = flag.String("port", "9000", "Port")
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
		request, err := clientReader.ReadString('\n')
		switch err {
		case nil:
			conn.Write([]byte(request))
		case io.EOF:
			fmt.Println("Client closed the connection")
			return
		default:
			fmt.Println(err)
			return
		}

		reply, err := serverReader.ReadString('\n')
		switch err {
		case nil:
			fmt.Printf("[Server reply] len=%d reply=%s", len(reply), reply)
		case io.EOF:
			fmt.Println("Server closed the connection")
			return
		default:
			fmt.Println(err)
			return
		}
	}
}
