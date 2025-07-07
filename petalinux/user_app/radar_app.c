#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#define RADAR_IOC_MAGIC 'R'
#define RADAR_IOC_START         _IO(RADAR_IOC_MAGIC, 0)
#define RADAR_IOC_STOP          _IO(RADAR_IOC_MAGIC, 1)
#define RADAR_IOC_SET_PRF       _IOW(RADAR_IOC_MAGIC, 2, uint32_t)
#define RADAR_IOC_SET_PULSE_WIDTH _IOW(RADAR_IOC_MAGIC, 3, uint32_t)
#define RADAR_IOC_SET_THRESHOLD _IOW(RADAR_IOC_MAGIC, 4, uint32_t)
#define RADAR_IOC_GET_STATUS    _IOR(RADAR_IOC_MAGIC, 5, uint32_t)
#define RADAR_IOC_GET_TARGET    _IOR(RADAR_IOC_MAGIC, 6, struct radar_target)

struct radar_target {
    uint16_t range;      // Range in meters
    uint16_t velocity;   // Velocity in m/s (signed)
    uint16_t amplitude;  // Target amplitude
    uint16_t doppler_bin; // Doppler bin number
};

void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -s           Start radar\n");
    printf("  -p <prf>     Set PRF (1000-10000 Hz)\n");
    printf("  -w <width>   Set pulse width (1-100 us)\n");
    printf("  -t <thresh>  Set CFAR threshold\n");
    printf("  -m           Monitor targets (continuous)\n");
    printf("  -h           Show this help\n");
}

int main(int argc, char *argv[]) {
    int fd;
    int ret;
    int opt;
    uint32_t prf = 2000;
    uint32_t pulse_width = 10;
    uint32_t threshold = 100;
    uint32_t status;
    bool start_radar = false;
    bool monitor_mode = false;
    struct radar_target target;
    struct pollfd pfd;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "sp:w:t:mh")) != -1) {
        switch (opt) {
        case 's':
            start_radar = true;
            break;
        case 'p':
            prf = atoi(optarg);
            if (prf < 1000 || prf > 10000) {
                fprintf(stderr, "PRF must be between 1000-10000 Hz\n");
                return 1;
            }
            break;
        case 'w':
            pulse_width = atoi(optarg);
            if (pulse_width < 1 || pulse_width > 100) {
                fprintf(stderr, "Pulse width must be between 1-100 us\n");
                return 1;
            }
            break;
        case 't':
            threshold = atoi(optarg);
            break;
        case 'm':
            monitor_mode = true;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Open radar device
    fd = open("/dev/pulse_radar_ip", O_RDWR);
    if (fd < 0) {
        perror("Failed to open radar device");
        return 1;
    }
    
    printf("Pulse Radar Control Application\n");
    printf("================================\n");
    
    // Configure radar parameters
    ret = ioctl(fd, RADAR_IOC_SET_PRF, &prf);
    if (ret < 0) {
        perror("Failed to set PRF");
        goto cleanup;
    }
    printf("PRF set to %d Hz\n", prf);
    
    ret = ioctl(fd, RADAR_IOC_SET_PULSE_WIDTH, &pulse_width);
    if (ret < 0) {
        perror("Failed to set pulse width");
        goto cleanup;
    }
    printf("Pulse width set to %d us\n", pulse_width);
    
    ret = ioctl(fd, RADAR_IOC_SET_THRESHOLD, &threshold);
    if (ret < 0) {
        perror("Failed to set threshold");
        goto cleanup;
    }
    printf("CFAR threshold set to %d\n", threshold);
    
    // Start radar if requested
    if (start_radar) {
        ret = ioctl(fd, RADAR_IOC_START);
        if (ret < 0) {
            perror("Failed to start radar");
            goto cleanup;
        }
        printf("Radar started\n");
        
        // Check status
        ret = ioctl(fd, RADAR_IOC_GET_STATUS, &status);
        if (ret < 0) {
            perror("Failed to get status");
            goto cleanup;
        }
        printf("Radar status: 0x%08x\n", status);
    }
    
    // Monitor mode
    if (monitor_mode) {
        printf("\nMonitoring for targets... (Press Ctrl+C to stop)\n");
        printf("Range (m) | Velocity (m/s) | Amplitude | Doppler Bin\n");
        printf("----------|----------------|-----------|------------\n");
        
        pfd.fd = fd;
        pfd.events = POLLIN;
        
        while (1) {
            ret = poll(&pfd, 1, 1000); // 1 second timeout
            if (ret < 0) {
                perror("poll");
                break;
            } else if (ret == 0) {
                printf("No targets detected...\n");
                continue;
            }
            
            if (pfd.revents & POLLIN) {
                ret = read(fd, &target, sizeof(target));
                if (ret == sizeof(target)) {
                    printf("%8d  | %12d   | %8d  | %10d\n",
                           target.range, (int16_t)target.velocity,
                           target.amplitude, target.doppler_bin);
                    
                    // Calculate actual velocity from Doppler
                    float velocity_ms = (float)((int16_t)target.velocity);
                    printf("Target detected at %.1f m, velocity %.1f m/s\n",
                           (float)target.range, velocity_ms);
                }
            }
        }
    }
    
cleanup:
    // Stop radar before closing
    ioctl(fd, RADAR_IOC_STOP);
    close(fd);
    return 0;
}
