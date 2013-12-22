//
//  main.c
//  stah
//
//  Created by Matteo Bertozzi on 12/17/13.
//  Copyright (c) 2013 Matteo Bertozzi. All rights reserved.
//

#include <CoreFoundation/CoreFoundation.h>

#include <CoreServices/CoreServices.h>
#include <ImageIO/CGImageDestination.h>
#include <CoreGraphics/CGDataProvider.h>
#include <CoreGraphics/CGImage.h>

static void __cgimage_save_png (const CGImageRef image, const char *filename) {
  CFStringRef path;
  CFURLRef url;
  
  path = CFStringCreateWithCString(NULL, filename, kCFStringEncodingUTF8);
  url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, 0);
  CFRelease(path);
  
  CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, NULL);
  CGImageDestinationAddImage(destination, image, nil);
  bool success = CGImageDestinationFinalize(destination);
  if (!success) {
    fprintf(stderr, "Failed to write image to %s", filename);
  }

  CFRelease(url);
}

int strendswith(const char *str, const char *suffix) {
  if (str == NULL || suffix == NULL)
    return(0);
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix >  lenstr)
    return(0);
  return(!strncmp(str + lenstr - lensuffix, suffix, lensuffix));
}

struct img_data {
  size_t width;
  size_t height;
  uint8_t *rgba;
};

static int __image_load (struct img_data *img_data, const char *path) {
  CGDataProviderRef dataProvider = CGDataProviderCreateWithFilename(path);
  CGImageRef image;
  
  if (strendswith(path, ".png")) {
    image = CGImageCreateWithPNGDataProvider(dataProvider, NULL, false, kCGRenderingIntentDefault);
  } else {
    image = CGImageCreateWithJPEGDataProvider(dataProvider, NULL, false, kCGRenderingIntentDefault);
  }
  
  img_data->rgba = NULL;
  img_data->width = CGImageGetWidth(image);
  img_data->height = CGImageGetHeight(image);
  
  size_t bpc = CGImageGetBitsPerComponent(image);
  if (bpc != 8) return(1);
  
  size_t bpp = CGImageGetBitsPerPixel(image);
  size_t bytes_per_pixel = bpp / bpc;
  
  size_t raw_data_length = img_data->width * img_data->height * bytes_per_pixel;
  uint8_t *raw_data = (uint8_t *) malloc(raw_data_length);
  if (raw_data == NULL) return(1);
  
  // Fill the raw_data
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(raw_data, img_data->width, img_data->height,
                                               bpc, img_data->width * (bpp / 8),
                                               colorSpace,
                                               kCGImageAlphaPremultipliedLast);
  if (context == NULL) return(1);
  
  CGContextDrawImage(context, CGRectMake(0, 0, img_data->width, img_data->height), image);
  CGContextRelease(context);
  CGImageRelease(image);
  CFRelease(colorSpace);
  CGDataProviderRelease(dataProvider);
  
  // Convert to RGBA-8888
  img_data->rgba = (uint8_t *) malloc(img_data->width * img_data->height * 4);
  uint8_t *rgba = img_data->rgba;
  
  while (raw_data_length > 0) {
    rgba[0] = raw_data[0];
    rgba[1] = raw_data[1];
    rgba[2] = raw_data[2];
    rgba[3] = (bpc < 32) ? 0xFF : raw_data[3];
    
    rgba += 4;
    raw_data += bytes_per_pixel;
    raw_data_length -= bytes_per_pixel;
  }
  
  raw_data_length = img_data->width * img_data->height * bytes_per_pixel;
  free(raw_data - raw_data_length);
  return(0);
}

static void __image_free (struct img_data *img_data) {
  if (img_data->rgba != NULL) {
    free(img_data->rgba);
  }
}

static void __image_save (struct img_data *img_data, const char *path) {
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef bitmapContext = CGBitmapContextCreate(img_data->rgba, img_data->width, img_data->height,
                                                     8, 4 * img_data->width, colorSpace,
                                                     kCGImageAlphaPremultipliedLast);
  CGImageRef image = CGBitmapContextCreateImage(bitmapContext);
  __cgimage_save_png(image, path);
  
  CGImageRelease(image);
  CFRelease(colorSpace);
}

static void __image_encode (struct img_data *img_data) {
  uint8_t buffer[512];
  size_t img_avail;
  size_t bufsize;
  uint8_t *rgba;
  uint8_t *pbuf;
  size_t total;
  ssize_t rd;
  
  img_avail = img_data->width * img_data->height * 4;
  rgba = img_data->rgba + (4 * 4); // Reserve the first 4 pixel (6 bytes)
  
  total = 0;
  pbuf = buffer;
  bufsize = sizeof(buffer);
  while ((rd = read(0, pbuf, bufsize)) > 0) {
    total += rd;
    
    while (rd >= 3) {
      // 0x[0f0f][0f 0x0][f0f0f] 3 char in 2 pixel
      rgba[0] = (rgba[0] & 0xf0) | ((pbuf[0] >> 4) & 0x0f);
      rgba[1] = (rgba[1] & 0xf0) | ((pbuf[0] >> 0) & 0x0f);
      rgba[2] = (rgba[2] & 0xf0) | ((pbuf[1] >> 4) & 0x0f);
      
      rgba[4] = (rgba[4] & 0xf0) | ((pbuf[1] >> 0) & 0x0f);
      rgba[5] = (rgba[5] & 0xf0) | ((pbuf[2] >> 4) & 0x0f);
      rgba[6] = (rgba[6] & 0xf0) | ((pbuf[2] >> 0) & 0x0f);

      img_avail -= 4;
      rgba += 8;
      pbuf += 3;
      rd -= 3;
    }
    
    switch (rd) {
      case 2:
        buffer[0] = *pbuf++;
        buffer[1] = *pbuf;
        break;
      case 1:
        buffer[0] = *pbuf++;
        break;
    }
    
    bufsize = sizeof(buffer) - rd;
    pbuf = buffer + rd;
  }
  
  switch (sizeof(buffer) - bufsize) {
    case 2:
      rgba[2] = (rgba[2] & 0xf0) | (buffer[1] & 0xf0);
      rgba[4] = (rgba[4] & 0xf0) | (buffer[1] & 0x0f);
    case 1:
      rgba[0] = (rgba[0] & 0xf0) | (buffer[0] & 0xf0);
      rgba[1] = (rgba[1] & 0xf0) | (buffer[0] & 0x0f);
      break;
  }
  
  /* Write the length */
  rgba = img_data->rgba;
  rgba[0] = (rgba[0] & 0xf0) | ((total >>  0) & 0xf);
  rgba[1] = (rgba[1] & 0xf0) | ((total >>  4) & 0xf);
  rgba[2] = (rgba[2] & 0xf0) | ((total >>  8) & 0xf);
  
  rgba[4] = (rgba[4] & 0xf0) | ((total >> 12) & 0xf);
  rgba[5] = (rgba[5] & 0xf0) | ((total >> 16) & 0xf);
  rgba[6] = (rgba[6] & 0xf0) | ((total >> 20) & 0xf);

  rgba[8] = (rgba[8] & 0xf0) | ((total >> 24) & 0xf);
  rgba[9] = (rgba[9] & 0xf0) | ((total >> 28) & 0xf);
  fprintf(stderr, "Write Length %zu\n", total);
}

static void __image_decode (struct img_data *img_data) {
  const uint8_t *rgba;
  size_t total;
  
  /* Read length */
  rgba = img_data->rgba;
  total  = ((rgba[0] & 0x0f) <<  0);
  total |= ((rgba[1] & 0x0f) <<  4);
  total |= ((rgba[2] & 0x0f) <<  8);
  
  total |= ((rgba[4] & 0x0f) << 12);
  total |= ((rgba[5] & 0x0f) << 16);
  total |= ((rgba[6] & 0x0f) << 20);
  
  total |= ((rgba[8] & 0x0f) << 24);
  total |= ((rgba[9] & 0x0f) << 28);
  
  rgba += (4 * 4);  // Reserve the first 4 pixel (6 bytes)
  while (total >= 3) {
    // 0x[0f0f][0f 0x0][f0f0f] 3 char in 2 pixel
    fputc(((rgba[0] & 0x0f) << 4) | (rgba[1] & 0x0f), stdout);
    fputc(((rgba[2] & 0x0f) << 4) | (rgba[4] & 0x0f), stdout);
    fputc(((rgba[5] & 0x0f) << 4) | (rgba[6] & 0x0f), stdout);
    
    total -= 3;
    rgba += 8;
  }
  
  switch (total) {
    case 2:
      fputc(((rgba[0] & 0x0f) << 4) | (rgba[1] & 0x0f), stdout);
      fputc(((rgba[2] & 0x0f) << 4) | (rgba[4] & 0x0f), stdout);
      break;
    case 1:
      fputc(((rgba[0] & 0x0f) << 4) | (rgba[1] & 0x0f), stdout);
      break;
  }
}

int main(int argc, const char *argv[]) {
  struct img_data image;
  const char *source;
  int decode;

  if (argc < 2) {
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  stah <in-image> <out-image>\n");
    fprintf(stderr, "  stah -d <in-image>\n");
    return(1);
  }
  
  if ((decode = !strcmp(argv[1], "-d"))) {
    source = argv[2];
  } else {
    source = argv[1];
  }

  if (__image_load(&image, source)) {
    fprintf(stderr, "Failed to open image %s\n", source);
    return(1);
  }
  
  if (decode) {
    __image_decode(&image);
  } else {
    __image_encode(&image);
    __image_save(&image, argv[2]);
  }
  
  __image_free(&image);

  return(0);
}

