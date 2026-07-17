# Fractals
This is just a basic fractal renderer written in C using SDL3. It's not the most performant or feature rich but I just made it as a way to learn SDL3 better and have fun.

Currently it has just a Mandelbrot function and a basic Koch Snowflake, but I plan to also add the Sierpinski Triangle

The Mandelbrot function is not very resposive as it takes a while to loop over every pixel in a single thread, and unfortunately I'm too stupid to do multithreading in C, so it lags after inputs while it calculates a new frame. After rendering when shifted, it does not render a brand new frame until more input is recieved, which should help performance.

The Koch snowflake is, unlike the Mandelbrot set, is rendered with lines directly with SDL. This means it performs way better, but it's not as accurate as it only goes on for a limited amount of iterations.

## Controls
- Up Arrow - Shift fractal view up
- Down Arrow - Shift fractal view down
- Left Arrow - Shift fractal view left
- Right Arrow - Shift fractal view right
- Minus key - Zoom out
- Plus / Equals key - Zoom in

## Development
- I did not use AI or an LLM while making this project.
- A lot of the algorithms or methods were found from much smarter people online. In particular, [the pseudocode examples for plotting a Mandelbrot set on wikipedia were very.](https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set)
- I used this project to attempt some new things. Firstly, this was my first big project using SDL, which meant I wasted at least a little time testing different implementations. I also used this to just learn some more about some fractal algorithms, although I don't have the maths knowledge to recreate them without help.
