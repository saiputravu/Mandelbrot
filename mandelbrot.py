import numpy as np
from PIL import Image

def f(z, c):
	return pow(z,2) + c

def mandelbrot(c, max_iter):
	n = 0
	z = 0
	while abs(z) <= 2 and n < max_iter:
		z = f(z, c)
		n+=1
	return n

def map_range(c, range_min, range_max, mapped_min, mapped_max):
	return mapped_min + (((c - range_min) * (mapped_max - mapped_min)) / (range_max - range_min))


width, height = 1024,1024
max_iter = 500

# Min Max
real = (-2, 2)
imaginary = (-1, 1)

pixels = np.zeros((height, width, 3))

for y in range(0, height):
	for x in range(0, width):
		c = complex(map_range(x, 0, width, real[0], real[1]),
                    map_range(y, 0, height, imaginary[0], imaginary[1]))
		n = mandelbrot(c, max_iter)
		colour = [int(255 * n / max_iter), 255, 255 if n < max_iter else 0]
		pixels[y][x] = colour

img = Image.fromarray(pixels.astype(np.uint8), mode='HSV')
img.convert('RGB')
img.show()
