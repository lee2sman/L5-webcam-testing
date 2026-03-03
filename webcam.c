// webcam.c - Minimal webcam capture for LÖVE2D using V4L2 with MJPEG decoding
// Compile: gcc -O2 -shared -fPIC -o webcam.so webcam.c -ljpeg

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <jpeglib.h>

typedef struct {
    int fd;
    void *buffer;
    void *rgb_buffer;
    size_t buffer_size;
    int width;
    int height;
    int is_mjpeg;
} webcam_t;

// Custom error handler to suppress JPEG warnings
static void my_error_exit(j_common_ptr cinfo) {
    // Don't exit on error, just return
    return;
}

static void my_output_message(j_common_ptr cinfo) {
    // Suppress all JPEG error/warning messages
    return;
}

// Decode MJPEG to RGB
static int mjpeg_to_rgb(const unsigned char *jpeg_data, unsigned long jpeg_size, 
                        unsigned char *rgb_buffer, int width, int height) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    // Set up custom error handling to suppress warnings
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = my_error_exit;
    jerr.output_message = my_output_message;
    
    jpeg_create_decompress(&cinfo);
    
    jpeg_mem_src(&cinfo, jpeg_data, jpeg_size);
    
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }
    
    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);
    
    int row_stride = cinfo.output_width * cinfo.output_components;
    
    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char *row_ptr = rgb_buffer + (cinfo.output_scanline * row_stride);
        jpeg_read_scanlines(&cinfo, &row_ptr, 1);
    }
    
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    
    return 0;
}

// Initialize webcam
webcam_t* webcam_open(const char *device, int width, int height) {
    webcam_t *cam = malloc(sizeof(webcam_t));
    if (!cam) return NULL;
    
    cam->width = width;
    cam->height = height;
    cam->is_mjpeg = 1;  // Default to MJPEG since that's what most webcams use
    cam->rgb_buffer = NULL;
    
    // Open device
    cam->fd = open(device, O_RDWR);
    if (cam->fd < 0) {
        fprintf(stderr, "Failed to open device %s: %s\n", device, strerror(errno));
        free(cam);
        return NULL;
    }
    
    // Set format to MJPEG
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    if (ioctl(cam->fd, VIDIOC_S_FMT, &fmt) < 0) {
        fprintf(stderr, "Failed to set MJPEG format: %s\n", strerror(errno));
        close(cam->fd);
        free(cam);
        return NULL;
    }
    
    // Verify format
    cam->width = fmt.fmt.pix.width;
    cam->height = fmt.fmt.pix.height;
    cam->buffer_size = fmt.fmt.pix.sizeimage;
    
    printf("Using MJPEG format: %dx%d, buffer size: %zu\n", 
           cam->width, cam->height, cam->buffer_size);
    
    // Allocate RGB buffer
    cam->rgb_buffer = malloc(cam->width * cam->height * 3);
    if (!cam->rgb_buffer) {
        fprintf(stderr, "Failed to allocate RGB buffer\n");
        close(cam->fd);
        free(cam);
        return NULL;
    }
    
    // Request buffers
    struct v4l2_requestbuffers req = {0};
    req.count = 2;  // Use 2 buffers for better performance
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(cam->fd, VIDIOC_REQBUFS, &req) < 0) {
        fprintf(stderr, "Failed to request buffer: %s\n", strerror(errno));
        free(cam->rgb_buffer);
        close(cam->fd);
        free(cam);
        return NULL;
    }
    
    // Map buffer (just using the first one for simplicity)
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    
    if (ioctl(cam->fd, VIDIOC_QUERYBUF, &buf) < 0) {
        fprintf(stderr, "Failed to query buffer: %s\n", strerror(errno));
        free(cam->rgb_buffer);
        close(cam->fd);
        free(cam);
        return NULL;
    }
    
    cam->buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, buf.m.offset);
    cam->buffer_size = buf.length;
    
    if (cam->buffer == MAP_FAILED) {
        fprintf(stderr, "Failed to map buffer: %s\n", strerror(errno));
        free(cam->rgb_buffer);
        close(cam->fd);
        free(cam);
        return NULL;
    }
    
    // Queue all buffers
    for (int i = 0; i < req.count; i++) {
        struct v4l2_buffer qbuf = {0};
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = i;
        
        if (ioctl(cam->fd, VIDIOC_QBUF, &qbuf) < 0) {
            fprintf(stderr, "Failed to queue buffer: %s\n", strerror(errno));
            munmap(cam->buffer, cam->buffer_size);
            free(cam->rgb_buffer);
            close(cam->fd);
            free(cam);
            return NULL;
        }
    }
    
    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam->fd, VIDIOC_STREAMON, &type) < 0) {
        fprintf(stderr, "Failed to start streaming: %s\n", strerror(errno));
        munmap(cam->buffer, cam->buffer_size);
        free(cam->rgb_buffer);
        close(cam->fd);
        free(cam);
        return NULL;
    }
    
    return cam;
}

// Capture a frame
const void* webcam_capture(webcam_t *cam) {
    if (!cam) return NULL;
    
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    // Dequeue buffer
    if (ioctl(cam->fd, VIDIOC_DQBUF, &buf) < 0) {
        fprintf(stderr, "Failed to dequeue buffer: %s\n", strerror(errno));
        return NULL;
    }
    
    // Decode MJPEG to RGB
    if (cam->is_mjpeg && buf.bytesused > 0) {
        if (mjpeg_to_rgb(cam->buffer, buf.bytesused, cam->rgb_buffer, cam->width, cam->height) < 0) {
            // Frame decode failed, requeue and return NULL
            ioctl(cam->fd, VIDIOC_QBUF, &buf);
            return NULL;
        }
    }
    
    // Requeue buffer
    if (ioctl(cam->fd, VIDIOC_QBUF, &buf) < 0) {
        fprintf(stderr, "Failed to requeue buffer: %s\n", strerror(errno));
        return NULL;
    }
    
    return cam->rgb_buffer;
}

// Get width
int webcam_get_width(webcam_t *cam) {
    return cam ? cam->width : 0;
}

// Get height
int webcam_get_height(webcam_t *cam) {
    return cam ? cam->height : 0;
}

// Get buffer size (RGB24 size)
size_t webcam_get_buffer_size(webcam_t *cam) {
    return cam ? (cam->width * cam->height * 3) : 0;
}

// Close webcam
void webcam_close(webcam_t *cam) {
    if (!cam) return;
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(cam->fd, VIDIOC_STREAMOFF, &type);
    
    if (cam->buffer) {
        munmap(cam->buffer, cam->buffer_size);
    }
    
    if (cam->rgb_buffer) {
        free(cam->rgb_buffer);
    }
    
    if (cam->fd >= 0) {
        close(cam->fd);
    }
    
    free(cam);
}
