package main

import (
    "fmt"
    "os"
    "time"
    "image"
    "exp/draw"
    "exp/draw/x11"
)

// See also https://go.dev/blog/image-draw

func main() {
    win, err := x11.NewWindow()
    if err != nil {
	    fmt.Fprintf(os.Stderr, "Error: %v\n", err)
	    os.Exit(1)
    }
    img := win.Screen()

    white := image.RGBAColor{255, 255, 255, 255}
    green := image.RGBAColor{0, 255, 0, 255}
    greenimg := image.NewColorImage(green)

    //src := image.NewRGBA(1, 1)
    //src.Set(0,0, green)

    for i, j := 0, 0; i < 100 && j < 100; i, j = i + 1, j + 1 {
        img.Set(i, j, white)
    }

    draw.Draw(img, image.Rect(300, 300, 200, 200), greenimg, image.Pt(0, 0))
    
    win.FlushImage()

    c := win.EventChan()
    for ev := range c {
      switch e := ev.(type) {
       case draw.KeyEvent:
        // handle key event
        fmt.Printf("Key pressed: %v\n", e.Key)
	if e.Key == 'q' {
            return
        }
       case draw.MouseEvent:
        // handle mouse event (if applicable)
        fmt.Printf("Mouse moved: %v, %v\n", e.Loc.X, e.Loc.Y)
	drawCircle(img, e.Loc, 20, green)
        win.FlushImage()
       default:
        fmt.Printf("Unknown event type: %T\n", e)
    }
}
    
    time.Sleep(2 * 1e9)
    win.Close()
    os.Exit(0)
}

func drawCircle(dst image.Image, center image.Point, radius int, col image.Color) {
    for dy := -radius; dy <= radius; dy++ {
        for dx := -radius; dx <= radius; dx++ {
            if dx*dx+dy*dy <= radius*radius {
                x := center.X + dx
                y := center.Y + dy
                dst.(draw.Image).Set(x, y, col)
            }
        }
    }
}
