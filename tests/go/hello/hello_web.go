// Hello world 2.0 (The Web era).

package main

import (
	"fmt"
	"http"
)

func logRequest(r *http.Request) {
	fmt.Printf("\nNew request\n")
	//fmt.Printf("RemoteAddr: %s\n", r.RemoteAddr)
	fmt.Printf("Method: %s\n", r.Method)
	fmt.Printf("URL: %s\n", r.URL.String())
	fmt.Printf("Proto: %s\n", r.Proto)
	fmt.Printf("Header: %+v\n", r.Header)
	fmt.Printf("Host: %s\n", r.Host)
	//fmt.Printf("RequestURI: %s\n", r.RequestURI)
	fmt.Printf("ContentLength: %d\n", r.ContentLength)
}
func handler(w http.ResponseWriter, r *http.Request) {
	logRequest(r)
	fmt.Fprintf(w, "Hi there, I love %s!", r.URL.Path[1:])
}


func main() {
	fmt.Printf("To test go to http://localhost:8080/sushi\n")
	http.HandleFunc("/", handler)
	http.ListenAndServe(":8080", nil)
}

