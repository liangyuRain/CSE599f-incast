package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"strconv"
	"strings"
)

func main() {
	var host = flag.String("host", "localhost", "Host IP")
	var port = flag.String("port", "9000", "Port")
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
		request, err := clientReader.ReadString('\n')
		switch err {
		case nil:
			request = strings.TrimSuffix(request, "\n")
			fmt.Printf("Client request: %s\n", request)
			args := strings.Fields(request)
			delay, size := 0, 0
			if len(args) > 0 {
				delay, _ = strconv.Atoi(args[0])
			}
			if len(args) > 1 {
				size, _ = strconv.Atoi(args[1])
			}
			reply := fmt.Sprintf("Reply delay=%d size=%d\n", delay, size)
			conn.Write([]byte(reply))
		case io.EOF:
			fmt.Println("Client closed the connection")
			return
		default:
			fmt.Println(err)
			return
		}
	}
}
