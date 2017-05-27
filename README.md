# vistools
A basic signal-processing and visualization suite written in C and OpenGL, that I had to write as support for my [thesis](http://oa.upm.es/42882/). Please note that it's still **under development**.


## How it works

This project is comprised of a set of programs designed to run together, performing signal-processing and visualization of digital signals in real time.

Programs can be chained together through pipes to perform different tasks. Each program reads a digital signal from the standard input, performs some operations, and outputs a digital signal on the standard output. 

For example, say you have a device `/dev/mic0` from which we can read the input from a microphone connected to your machine. To visualize the raw signal in real time, you can run

```
cat /dev/mic0 | stream_plot 1 > /dev/null
```

`stream_plot` is a program that lets you visualize the raw signal. The signal is placed in the standard output without modification.

Say you also want to visualize the Fourier Transform of the signal. For this we can use `simple_fft` similarly to the previous example

```
cat /dev/mic0 | simple_fft | plot 1024 1 > /dev/null
```

If you want to visualize the FFT in a slightly more visually pleasing way, you can run,

```
cat /dev/mic0 | simple_fft | polar_show 1024 1 > /dev/null
```

Below you'll find a list of all the programs you have available.

## Visualization Programs

Visualization programs don't alter the input stream, and they let you visualize the data from the input stream in different ways. Data can be interlaced: several channels can exist in the same input stream. The visualization programs will deinterlace the data to visualize all channels separately and simultaneously.

These programs are

- **plot**: Plots N data points of interlaced data from the input stream
- **plot_stream**: Plots the interlaced data of the input stream continuously, ideal to monitor a raw signal
- **polar_plot**: Similar to **plot** but plots the data in a polar graph. Each channel is plotted in a different colour

## Signal Processing Programs

These programs alter the input stream, performing different operations on it and outputting it to the standard output.

- **gcc_phat**: Performs a gcc_phat transform on two signals. 
- **zero_pad**: Reads a signal from the input stream and performs a zero padding interpolation
- **simple_fft**: Reads a signal from the input stream and performs a Fast Fourier Transform


