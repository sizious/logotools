/* 
 * pngtomr - convert pngs into files for use in a dreamcast ip.bin
 *
 * size should be 320x90 or less, colors must be less than 128
 * output file must be less than 8192 bytes to fit in a "normal" ip.bin
 *
 * adk / napalm 2001 
 *
 * andrewk@napalm-x.com
 *
 * http://www.napalm-x.com
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "png.h"

typedef struct {
    int width;
    int height;
    char *data;
} image_t;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color_t;

typedef struct {
    color_t color[128];
    int count;
} palette_t;

typedef struct {
    unsigned int size;
    unsigned int offset;
    unsigned int width;
    unsigned int height;
    unsigned int colors;
    char *data;
} mr_t;

int read_png(char *file_name, image_t *image)  /* We need to open the file */
{
   png_structp png_ptr;
   png_infop info_ptr;
   unsigned int sig_read = 0;
   png_uint_32 width, height, row;
   int bit_depth, color_type, interlace_type;
   FILE *fp;
   png_color_16 *image_background;

   if ((fp = fopen(file_name, "rb")) == NULL)
      return 0;

   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);

   if (png_ptr == NULL)
   {
      fclose(fp);
      return 0;
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      return 0;
   }

   if (setjmp(png_ptr->jmpbuf))
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      fclose(fp);
      return 0;
   }

   png_init_io(png_ptr, fp);

   png_set_sig_bytes(png_ptr, sig_read);

   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       &interlace_type, NULL, NULL);

   image->width = width;
   image->height = height;

   image->data = (char *)malloc(width*height*4);

   /* tell libpng to strip 16 bit/color files down to 8 bits/color */
   png_set_strip_16(png_ptr);

   /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
   png_set_packing(png_ptr);

   /* Expand paletted colors into true RGB triplets */
   if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_expand(png_ptr);

   /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
   if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand(png_ptr);

   /* Expand paletted or RGB images with transparency to full alpha channels
    * so the data will be available as RGBA quartets.
    */
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      png_set_expand(png_ptr);

   if (png_get_bKGD(png_ptr, info_ptr, &image_background))
      png_set_background(png_ptr, image_background,
                         PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);

   /* Add filler (or alpha) byte (before/after each RGB triplet) */
   png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

   png_read_update_info(png_ptr, info_ptr);

   {
       png_bytep row_pointers[height];

       for (row = 0; row < height; row++)
	   row_pointers[row] = image->data + image->width*4*row;
       
       png_read_image(png_ptr, row_pointers);
   }

   png_read_end(png_ptr, info_ptr);

   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

   fclose(fp);

   return 1;
}

int mrcompress(char *in, char *out, int size)
{
    int length = 0;
    int position = 0;
    int run;
    
    while (position <= size) {
	
	run = 1;

	while((in[position] == in[position+run]) && (run < 0x17f) && (position+run <= size)) {
	    run++;
	}
	
	if (run > 0xff) {
	    out[length++] = 0x82;
	    out[length++] = 0x80 | (run - 0x100);
	    out[length++] = in[position];
	} else if (run > 0x7f) {
	    out[length++] = 0x81;
	    out[length++] = run;
	    out[length++] = in[position];
	} else if (run > 1) {
	    out[length++] = 0x80 | run;
	    out[length++] = in[position];
	} else
	    out[length++] = in[position];
	
	position += run;

    }

    return length;

}


int main(int argc, char *argv[])
{
    image_t image;
    palette_t palette;
    mr_t mr;
    int i;
    int  *data;
    char *raw_output;
    char *compressed_output;
    FILE *output;
    int crap = 0;
    int compressed_size;

    printf("pngtomr 0.1 by adk/napalm\n");

    if (argc != 3) {
	printf("Usage: %s <input.png> <output.mr>\n",argv[0]);
	exit(0);
    }

    if (read_png(argv[1],&image)) {
	printf("Loaded %s, %d x %d\n",argv[1],image.width,image.height);
    } else {
	printf("Error with %s\n",argv[1]);
	exit(0);
    }
    
    palette.count = 0;

    data = (int *)image.data;

    raw_output = (char *)malloc(image.width * image.height);
    compressed_output = (char *)malloc(image.width * image.height);

    for(i=0; i<image.width*image.height; i++) {
	int found = 0;
	int c = 0;

	while ((!found) && (c < palette.count))
	    if (!memcmp(&data[i], &palette.color[c], 3))
		found = 1;
	    else
		c++;
	if ((!found) && (c == 128)) {
	    printf("Reduce the number of colors to <= 128 and try again.\n");
	    exit(0);
	}
	if (!found) {
	    memcpy(&palette.color[c], &data[i], 3);
	    palette.count++;
	}
	raw_output[i] = c;
    }

    printf("Found %d colors\n",palette.count);

    mr.width = image.width;
    mr.height = image.height;
    mr.colors = palette.count;

    printf("Compressing %d bytes to ",image.width*image.height); fflush(stdout);

    compressed_size = mrcompress(raw_output, compressed_output, image.width*image.height);

    printf("%d bytes.\n",compressed_size); fflush(stdout);

    if (compressed_size <= 8192)
	printf("This will fit in a normal ip.bin.\n");
    else
	printf("This will NOT fit in a normal ip.bin - it is %d bytes too big!\n",compressed_size - 8192);

    mr.offset = 2 + 7*4 + palette.count*4;
    mr.size = 2 + 7*4 + palette.count*4 + compressed_size;

    printf("Writing %s\n",argv[2]);

    output = fopen(argv[2], "wb");

    if (!output) {
        printf("Cannot open %s\n",argv[2]);
        exit(0);
    }

    fwrite("MR", 1, 2, output);
    fwrite(&mr.size, 1, 4, output);
    fwrite(&crap, 1, 4, output);
    fwrite(&mr.offset, 1, 4, output);
    fwrite(&mr.width, 1, 4, output);
    fwrite(&mr.height, 1, 4, output);
    fwrite(&crap, 1, 4, output);
    fwrite(&mr.colors, 1, 4, output);

    for(i=0; i<palette.count; i++) {
	fwrite(&palette.color[i].b, 1, 1, output);
	fwrite(&palette.color[i].g, 1, 1, output);
	fwrite(&palette.color[i].r, 1, 1, output);
	fwrite(&crap, 1, 1, output);
    }

    fwrite(compressed_output, compressed_size, 1, output);
    fclose(output);

    printf("Done!\n");

    exit(0);
}


