package main

import (
    "fmt"
    "os"
    "time"
    "image"
    "exp/draw/x11"
)

func main() {
    win, err := x11.NewWindow()
    if err != nil {
	    fmt.Fprintf(os.Stderr, "Error: %v\n", err)
	    os.Exit(1)
    }
    color := image.RGBAColor{255, 255, 255, 255}
    _ = color
    img := win.Screen()
    for i, j := 0, 0; i < 100 && j < 100; i, j = i + 1, j + 1 {
        img.Set(i, j, color)
    }
    
    win.FlushImage()
    time.Sleep(2 * 1e9)
    win.Close()
    os.Exit(0)
}


/*
package main

import (
    "exp/draw/x11"
    "image"
    "os"
)

func main() {
    const width, height = 400, 400

    screen, err := x11.NewWindow(image.Rect(0, 0, width, height))
    if err != nil {
        println("cannot open X11 window:", err.String())
        os.Exit(1)
    }

    white := image.RGBAColor{255, 255, 255, 255}
    blue := image.RGBAColor{0, 0, 255, 255}

    for {
        mouse := x11.Mouse()
        pos := image.Pt(mouse.X, mouse.Y)

        clear(screen, white)
        drawCircle(screen, pos, 20, blue)

        // x11.NewWindow auto-flushes (or had implicit redraw)
    }
}

func clear(dst image.Image, col image.Color) {
    bounds := dst.Bounds()
    for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
        for x := bounds.Min.X; x < bounds.Max.X; x++ {
            dst.(draw.Image).Set(x, y, col)
        }
    }
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
*/
