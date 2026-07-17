# Fractals
This is just a basic fractal renderer written in C using SDL3. It's not the most performant or feature rich but I just made it as a way to learn SDL3 better and have fun.

Currently it has a Mandelbrot set, a Sierpinski Triangle and a Koch Snowflake, but I plan to also add the Julia set;

The Mandelbrot function is multithreaded, with a default of 8 threads per frame, which helps to speed up the otherwise quite slow calculation. After rendering when shifted, it does not render a brand new frame until more input is recieved, which should help performance, but also causes stuttering when inputting.

The Koch snowflake and Sierpinski triangle are, unlike the Mandelbrot set, rendered with lines directly with SDL. This means it performs way better, but it's not as accurate as it only goes on for a limited amount of iterations, otherwise it does start to slow performance

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
- The hardest thing about this was probably figuring out how to use multithreading in SDL. I haven't used much multithreading before, but after seeing how the Mandelbrot set performed when single threaded, I was pretty sure it was necessary. To multithread, I split the window into 8 slices, and then each thread wrote pixels to of those slices to a surface, then each surface was aligned and rendered.
