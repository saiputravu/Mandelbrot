#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

#define MAX_ITERATIONS 150
#define REAL_MIN -2
#define REAL_MAX  1
#define IMAGINARY_MIN -1
#define IMAGINARY_MAX  1

typedef struct {
	char red;
	char green;
	char blue;
} pixel_t;

typedef struct {
	pixel_t* pixel_buff;
	int width;
	int height;
} image_t;

typedef struct {
	float real;
	float imaginary;
} complex_t;

pixel_t* get_pixel (image_t* image, int x, int y) { 
	return image->pixel_buff + (image->width * y) + x;
}

void set_pixel (char r, char g, char b, image_t* image, int x, int y) {
	pixel_t * pixel = get_pixel(image, x, y);
	pixel->red = r;
	pixel->green = g;
	pixel->blue = b;
}

void spectral_color (double l, image_t* image, int x, int y) {
    double t;
	double r, g, b = 0.0;
         if ((l>=400.0)&&(l<410.0)) { t=(l-400.0)/(410.0-400.0); r=    +(0.33*t)-(0.20*t*t); }
    else if ((l>=410.0)&&(l<475.0)) { t=(l-410.0)/(475.0-410.0); r=0.14         -(0.13*t*t); }
    else if ((l>=545.0)&&(l<595.0)) { t=(l-545.0)/(595.0-545.0); r=    +(1.98*t)-(     t*t); }
    else if ((l>=595.0)&&(l<650.0)) { t=(l-595.0)/(650.0-595.0); r=0.98+(0.06*t)-(0.40*t*t); }
    else if ((l>=650.0)&&(l<700.0)) { t=(l-650.0)/(700.0-650.0); r=0.65-(0.84*t)+(0.20*t*t); }
         if ((l>=415.0)&&(l<475.0)) { t=(l-415.0)/(475.0-415.0); g=             +(0.80*t*t); }
    else if ((l>=475.0)&&(l<590.0)) { t=(l-475.0)/(590.0-475.0); g=0.8 +(0.76*t)-(0.80*t*t); }
    else if ((l>=585.0)&&(l<639.0)) { t=(l-585.0)/(639.0-585.0); g=0.84-(0.84*t)           ; }
         if ((l>=400.0)&&(l<475.0)) { t=(l-400.0)/(475.0-400.0); b=    +(2.20*t)-(1.50*t*t); }
    else if ((l>=475.0)&&(l<560.0)) { t=(l-475.0)/(560.0-475.0); b=0.7 -(     t)+(0.30*t*t); }

	set_pixel((char)(r*255), (char)(g*255), (char)(b*255), image, x, y);
}

int write_png (image_t* image, char* filepath) {
	FILE* pngfile;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_byte** row_pointers = NULL;
	
	int x,y = 0;
	int pixel_size = 3;
    int depth = 8;

	int status = -1;

	pngfile = fopen(filepath, "wb");
	if (!pngfile)
		goto finish;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		goto png_create_write_struct_failed;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto png_create_info_struct_failed;

	if (setjmp(png_jmpbuf(png_ptr)))
		goto png_failure;
	
	png_set_IHDR(png_ptr,
                  info_ptr,
                  image->width,
                  image->height,
                  depth,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
	
	row_pointers = png_malloc(png_ptr, image->height*sizeof(png_byte *));
	for (y = 0; y < image->height; y++) {
		png_byte *row = png_malloc(png_ptr, sizeof(char)*image->width*pixel_size);
		row_pointers[y] = row;
		for (x = 0; x < image->width; x++) {
			pixel_t * pixel = get_pixel(image, x, y);
			*row++ = pixel->red;
			*row++ = pixel->green;
			*row++ = pixel->blue;
		}
	}
    
    png_init_io(png_ptr, pngfile);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    status = 0;
    for (y = 0; y < image->height; y++) {
        png_free(png_ptr, row_pointers[y]);
    }
    png_free(png_ptr, row_pointers);

png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct(&png_ptr, &info_ptr);
png_create_write_struct_failed:
    fclose(pngfile);
finish:
	return status;
}

complex_t multiply (complex_t* a, complex_t* b) {
	complex_t result;
	result.real = a->real * b->real - a->imaginary * b->imaginary;
	result.imaginary = a->real * b->imaginary + a->imaginary * b->real;
	return result;
}

complex_t add (complex_t* a, complex_t* b) {
	complex_t result;
	result.real = a->real + b->real;
	result.imaginary = a->imaginary + b->imaginary;
	return result;
}

float abs_complex (complex_t* a) {
	return sqrt((a->real * a->real) + (a->imaginary * a->imaginary));
}

float map_range(float c, int range_min, int range_max, int mapped_min, int mapped_max) {
	return mapped_min + (((c - range_min) * (mapped_max - mapped_min)) / (range_max - range_min));
}

complex_t f (complex_t* z, complex_t* c) {
	complex_t z_2 = multiply(z, z);
	return add(&z_2, c);
}

int mandelbrot_iterations (complex_t* c) {
	int n = 0;
	complex_t z;
	z.real = 0;
	z.imaginary = 0;
	
	while (abs_complex(&z) <= 2 &&  n < MAX_ITERATIONS) {
		z = f(&z, c);
		n++;
	}
	return n;
}

void mandelbrot (image_t* image) {
	complex_t c;
	int n;
	char h,s,v = 0;
	float l;
	
	for (int y = 0; y < image->height; ++y) {
		for (int x = 0; x < image->width; ++x) {
			//c.real = REAL_MIN + ((float)x / image->width) * (REAL_MAX - REAL_MIN);
			//c.imaginary = IMAGINARY_MIN + ((float)y / image->height) * (IMAGINARY_MAX - IMAGINARY_MIN);
			c.real = map_range(x, 0, image->width, REAL_MIN, REAL_MAX);	
			c.imaginary = map_range(y, 0, image->height, IMAGINARY_MIN, IMAGINARY_MAX);	
			n = mandelbrot_iterations(&c);
			l = map_range(n, 0, MAX_ITERATIONS, 400, 700);
			spectral_color(l, image, x, y);
		}
	}
}

int main (){
	int status = -1;
	image_t image;	
	image.width = 3000;
	image.height = 3000;
	image.pixel_buff = calloc(image.width * image.height, sizeof(pixel_t));
	if (!image.pixel_buff) {
		fprintf(stderr, "Failed allocating the image buffer.\n");
		exit(-1);
	}

	printf("Generating mandelbrot image\n");
	mandelbrot(&image);
	
	if (write_png(&image, "output.png")) {
		fprintf (stderr, "Error writing file.\n");
		exit(-1);
	}

	free(image.pixel_buff);
	
	printf("Finished Running\n");
	status = 0;
	return status;
}
