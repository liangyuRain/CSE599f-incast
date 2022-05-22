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
	"time"
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

	maxSize := 1 << 20
	data := make([]byte, maxSize)
	for i := 0; i < maxSize; i++ {
		data[i] = '0'
	}

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println(err)
			continue
		}
		go handleConnection(conn, data)
	}
}

func handleConnection(conn net.Conn, data []byte) {
	defer conn.Close()
	conn.SetDeadline(time.Time{})
	clientReader := bufio.NewReader(conn)
	for {
		request, err := clientReader.ReadString('\n')
		switch err {
		case nil:
			args := strings.Fields(request)
			delay, size := 0, 0
			if len(args) > 0 {
				delay, _ = strconv.Atoi(args[0])
			}
			if len(args) > 1 {
				size, _ = strconv.Atoi(args[1])
			}
			fmt.Printf("[Client request] delay=%d size=%d\n", delay, size)
			var reply []byte
			reply = append(reply, data[:size]...)
			time.Sleep(time.Duration(delay) * time.Microsecond)
			reply = append(reply, '\n')
			conn.Write(reply)
		case io.EOF:
			fmt.Println("Client closed the connection")
			return
		default:
			fmt.Println(err)
			return
		}
	}
}
