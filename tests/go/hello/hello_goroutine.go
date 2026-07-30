package main

import (
	"fmt"
	"time"
)

// from doc/GoCourseDay3.pdf

func IsReady(what string, seconds int64) {
       time.Sleep(seconds * 1e9);
       fmt.Println(what, "is ready")
}

func main() {
	
	go IsReady("tea", 6);
	go IsReady("coffee", 2);
	fmt.Println("I'm waiting....");
	IsReady("done", 7);
}
