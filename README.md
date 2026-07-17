# Fractals
This is just a basic fractal renderer written in C using SDL3. It's not the most performant or feature rich but I just made it as a way to learn SDL3 better and have fun.

Currently it has just a Mandelbrot function and a basic Koch Snowflake, but I plan to also add the Sierpinski Triangle

The Mandelbrot function can be shifted with the arrow keys and can be zoomed in and out of with the plus and minus keys, although the plus key is technically the equals key. After pressing any keys, it is not very resposive as it takes a while to loop over every pixel in a single thread, and unfortunately I'm too stupid to do multithreading in C.
After rendering when shifted, it does not render a brand new frame until more input is recieved, which should help performance.

The Koch snowflake is, unlike the Mandelbrot set, is rendered with lines directly with SDL. This means it performs way better, but it's not as accurate as it only goes on for a limited amount of iterations.
