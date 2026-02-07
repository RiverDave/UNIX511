//hardwareDevice.h - header file for the hardware device
//
// 29-Dec-20  M. Watler         Created
//

//IOCTL prototypes
//See https://01.org/linuxgraphics/gfx-docs/drm/ioctl/ioctl-number.html for macro syntax
// TODO: Still unsure what the letters as parameters represent, althought
// It's pretty stright forward that we're writing to the kernel and kernel reads
#define HARDWARE_DEVICE_HALT   _IOW('a','b',int*)
#define HARDWARE_DEVICE_RESUME _IOW('b','b',int*)
