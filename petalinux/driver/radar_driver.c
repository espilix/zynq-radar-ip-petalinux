#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>

#define DRIVER_NAME "pulse_radar_ip"
#define RADAR_REG_SIZE 0x10000

// Register offsets
#define RADAR_CONTROL_REG     0x00
#define RADAR_STATUS_REG      0x04
#define RADAR_PRF_REG         0x08
#define RADAR_PULSE_WIDTH_REG 0x0C
#define RADAR_RANGE_GATE_REG  0x10
#define RADAR_DOPPLER_BINS_REG 0x14
#define RADAR_DETECTED_RANGE_REG 0x18
#define RADAR_DETECTED_VELOCITY_REG 0x1C
#define RADAR_THRESHOLD_REG   0x20

// Control register bits
#define RADAR_ENABLE_BIT      0x01
#define RADAR_RANGE_PROC_BIT  0x02
#define RADAR_MTI_ENABLE_BIT  0x04
#define RADAR_DOPPLER_PROC_BIT 0x08
#define RADAR_CFAR_ENABLE_BIT 0x10

// Status register bits
#define RADAR_READY_BIT       0x01
#define RADAR_PROCESSING_BIT  0x02
#define RADAR_TARGET_DETECTED_BIT 0x04
#define RADAR_ERROR_BIT       0x08

// IOCTL commands
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

struct radar_device {
    void __iomem *base;
    struct cdev cdev;
    struct device *dev;
    int major;
    int target_detected_irq;
    int processing_complete_irq;
    wait_queue_head_t target_wait;
    wait_queue_head_t processing_wait;
    bool target_available;
    bool processing_complete;
    struct radar_target current_target;
    struct mutex mutex;
};

static struct radar_device *radar_dev;

// Interrupt handlers
static irqreturn_t radar_target_detected_irq(int irq, void *dev_id)
{
    struct radar_device *rdev = dev_id;
    uint32_t status;
    
    status = ioread32(rdev->base + RADAR_STATUS_REG);
    if (status & RADAR_TARGET_DETECTED_BIT) {
        // Read target data
        rdev->current_target.range = ioread32(rdev->base + RADAR_DETECTED_RANGE_REG);
        rdev->current_target.velocity = ioread32(rdev->base + RADAR_DETECTED_VELOCITY_REG);
        rdev->current_target.amplitude = status >> 16; // Upper 16 bits
        
        rdev->target_available = true;
        wake_up_interruptible(&rdev->target_wait);
        
        dev_info(rdev->dev, "Target detected: Range=%d m, Velocity=%d m/s\n",
                 rdev->current_target.range, rdev->current_target.velocity);
    }
    
    return IRQ_HANDLED;
}

static irqreturn_t radar_processing_complete_irq(int irq, void *dev_id)
{
    struct radar_device *rdev = dev_id;
    
    rdev->processing_complete = true;
    wake_up_interruptible(&rdev->processing_wait);
    
    return IRQ_HANDLED;
}

// File operations
static int radar_open(struct inode *inode, struct file *file)
{
    struct radar_device *rdev = container_of(inode->i_cdev, struct radar_device, cdev);
    file->private_data = rdev;
    return 0;
}

static ssize_t radar_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct radar_device *rdev = file->private_data;
    struct radar_target target;
    
    if (count < sizeof(struct radar_target))
        return -EINVAL;
    
    // Wait for target detection
    if (wait_event_interruptible(rdev->target_wait, rdev->target_available))
        return -ERESTARTSYS;
    
    mutex_lock(&rdev->mutex);
    target = rdev->current_target;
    rdev->target_available = false;
    mutex_unlock(&rdev->mutex);
    
    if (copy_to_user(buf, &target, sizeof(target)))
        return -EFAULT;
    
    return sizeof(target);
}

static ssize_t radar_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct radar_device *rdev = file->private_data;
    uint32_t command;
    
    if (count != sizeof(uint32_t))
        return -EINVAL;
    
    if (copy_from_user(&command, buf, sizeof(command)))
        return -EFAULT;
    
    mutex_lock(&rdev->mutex);
    iowrite32(command, rdev->base + RADAR_CONTROL_REG);
    mutex_unlock(&rdev->mutex);
    
    return sizeof(command);
}

static long radar_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct radar_device *rdev = file->private_data;
    uint32_t value;
    int ret = 0;
    
    switch (cmd) {
    case RADAR_IOC_START:
        mutex_lock(&rdev->mutex);
        value = ioread32(rdev->base + RADAR_CONTROL_REG);
        value |= RADAR_ENABLE_BIT | RADAR_RANGE_PROC_BIT | RADAR_MTI_ENABLE_BIT | 
                 RADAR_DOPPLER_PROC_BIT | RADAR_CFAR_ENABLE_BIT;
        iowrite32(value, rdev->base + RADAR_CONTROL_REG);
        mutex_unlock(&rdev->mutex);
        dev_info(rdev->dev, "Radar started\n");
        break;
        
    case RADAR_IOC_STOP:
        mutex_lock(&rdev->mutex);
        iowrite32(0, rdev->base + RADAR_CONTROL_REG);
        mutex_unlock(&rdev->mutex);
        dev_info(rdev->dev, "Radar stopped\n");
        break;
        
    case RADAR_IOC_SET_PRF:
        if (copy_from_user(&value, (void __user *)arg, sizeof(value)))
            return -EFAULT;
        if (value < 1000 || value > 10000) {
            dev_err(rdev->dev, "PRF out of range (1000-10000 Hz)\n");
            return -EINVAL;
        }
        mutex_lock(&rdev->mutex);
        iowrite32(value, rdev->base + RADAR_PRF_REG);
        mutex_unlock(&rdev->mutex);
        dev_info(rdev->dev, "PRF set to %d Hz\n", value);
        break;
        
    case RADAR_IOC_SET_PULSE_WIDTH:
        if (copy_from_user(&value, (void __user *)arg, sizeof(value)))
            return -EFAULT;
        if (value < 1 || value > 100) {
            dev_err(rdev->dev, "Pulse width out of range (1-100 us)\n");
            return -EINVAL;
        }
        mutex_lock(&rdev->mutex);
        iowrite32(value, rdev->base + RADAR_PULSE_WIDTH_REG);
        mutex_unlock(&rdev->mutex);
        dev_info(rdev->dev, "Pulse width set to %d us\n", value);
        break;
        
    case RADAR_IOC_SET_THRESHOLD:
        if (copy_from_user(&value, (void __user *)arg, sizeof(value)))
            return -EFAULT;
        mutex_lock(&rdev->mutex);
        iowrite32(value, rdev->base + RADAR_THRESHOLD_REG);
        mutex_unlock(&rdev->mutex);
        dev_info(rdev->dev, "CFAR threshold set to %d\n", value);
        break;
        
    case RADAR_IOC_GET_STATUS:
        mutex_lock(&rdev->mutex);
        value = ioread32(rdev->base + RADAR_STATUS_REG);
        mutex_unlock(&rdev->mutex);
        if (copy_to_user((void __user *)arg, &value, sizeof(value)))
            return -EFAULT;
        break;
        
    case RADAR_IOC_GET_TARGET:
        if (!rdev->target_available)
            return -ENODATA;
        mutex_lock(&rdev->mutex);
        if (copy_to_user((void __user *)arg, &rdev->current_target, sizeof(rdev->current_target)))
            ret = -EFAULT;
        mutex_unlock(&rdev->mutex);
        break;
        
    default:
        return -ENOTTY;
    }
    
    return ret;
}

static unsigned int radar_poll(struct file *file, poll_table *wait)
{
    struct radar_device *rdev = file->private_data;
    unsigned int mask = 0;
    
    poll_wait(file, &rdev->target_wait, wait);
    
    if (rdev->target_available)
        mask |= POLLIN | POLLRDNORM;
    
    return mask;
}

static struct file_operations radar_fops = {
    .owner = THIS_MODULE,
    .open = radar_open,
    .read = radar_read,
    .write = radar_write,
    .unlocked_ioctl = radar_ioctl,
    .poll = radar_poll,
};

// Sysfs attributes
static ssize_t prf_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct radar_device *rdev = dev_get_drvdata(dev);
    uint32_t prf;
    
    mutex_lock(&rdev->mutex);
    prf = ioread32(rdev->base + RADAR_PRF_REG);
    mutex_unlock(&rdev->mutex);
    
    return sprintf(buf, "%d\n", prf);
}

static ssize_t prf_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
    struct radar_device *rdev = dev_get_drvdata(dev);
    uint32_t prf;
    
    if (kstrtou32(buf, 10, &prf))
        return -EINVAL;
    
    if (prf < 1000 || prf > 10000)
        return -EINVAL;
    
    mutex_lock(&rdev->mutex);
    iowrite32(prf, rdev->base + RADAR_PRF_REG);
    mutex_unlock(&rdev->mutex);
    
    return count;
}

static ssize_t pulse_width_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct radar_device *rdev = dev_get_drvdata(dev);
    uint32_t pulse_width;
    
    mutex_lock(&rdev->mutex);
    pulse_width = ioread32(rdev->base + RADAR_PULSE_WIDTH_REG);
    mutex_unlock(&rdev->mutex);
    
    return sprintf(buf, "%d\n", pulse_width);
}

static ssize_t pulse_width_store(struct device *dev, struct device_attribute *attr,
                                const char *buf, size_t count)
{
    struct radar_device *rdev = dev_get_drvdata(dev);
    uint32_t pulse_width;
    
    if (kstrtou32(buf, 10, &pulse_width))
        return -EINVAL;
    
    if (pulse_width < 1 || pulse_width > 100)
        return -EINVAL;
    
    mutex_lock(&rdev->mutex);
    iowrite32(pulse_width, rdev->base + RADAR_PULSE_WIDTH_REG);
    mutex_unlock(&rdev->mutex);
    
    return count;
}

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct radar_device *rdev = dev_get_drvdata(dev);
    uint32_t status;
    
    mutex_lock(&rdev->mutex);
    status = ioread32(rdev->base + RADAR_STATUS_REG);
    mutex_unlock(&rdev->mutex);
    
    return sprintf(buf, "0x%08x\n", status);
}

static DEVICE_ATTR_RW(prf);
static DEVICE_ATTR_RW(pulse_width);
static DEVICE_ATTR_RO(status);

static struct attribute *radar_attrs[] = {
    &dev_attr_prf.attr,
    &dev_attr_pulse_width.attr,
    &dev_attr_status.attr,
    NULL,
};

static struct attribute_group radar_attr_group = {
    .attrs = radar_attrs,
};

// Platform driver functions
static int radar_probe(struct platform_device *pdev)
{
    struct resource *res;
    int ret;
    
    radar_dev = devm_kzalloc(&pdev->dev, sizeof(*radar_dev), GFP_KERNEL);
    if (!radar_dev)
        return -ENOMEM;
    
    radar_dev->dev = &pdev->dev;
    
    // Map memory resources
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    radar_dev->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(radar_dev->base))
        return PTR_ERR(radar_dev->base);
    
    // Get IRQ resources
    radar_dev->target_detected_irq = platform_get_irq(pdev, 0);
    if (radar_dev->target_detected_irq < 0)
        return radar_dev->target_detected_irq;
    
    radar_dev->processing_complete_irq = platform_get_irq(pdev, 1);
    if (radar_dev->processing_complete_irq < 0)
        return radar_dev->processing_complete_irq;
    
    // Initialize wait queues and mutex
    init_waitqueue_head(&radar_dev->target_wait);
    init_waitqueue_head(&radar_dev->processing_wait);
    mutex_init(&radar_dev->mutex);
    
    // Request IRQs
    ret = devm_request_irq(&pdev->dev, radar_dev->target_detected_irq,
                          radar_target_detected_irq, IRQF_TRIGGER_RISING,
                          "radar_target_detected", radar_dev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request target detected IRQ\n");
        return ret;
    }
    
    ret = devm_request_irq(&pdev->dev, radar_dev->processing_complete_irq,
                          radar_processing_complete_irq, IRQF_TRIGGER_RISING,
                          "radar_processing_complete", radar_dev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request processing complete IRQ\n");
        return ret;
    }
    
    // Register character device
    radar_dev->major = register_chrdev(0, DRIVER_NAME, &radar_fops);
    if (radar_dev->major < 0) {
        dev_err(&pdev->dev, "Failed to register character device\n");
        return radar_dev->major;
    }
    
    cdev_init(&radar_dev->cdev, &radar_fops);
    radar_dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&radar_dev->cdev, MKDEV(radar_dev->major, 0), 1);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add character device\n");
        unregister_chrdev(radar_dev->major, DRIVER_NAME);
        return ret;
    }
    
    // Create sysfs attributes
    ret = sysfs_create_group(&pdev->dev.kobj, &radar_attr_group);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create sysfs attributes\n");
        cdev_del(&radar_dev->cdev);
        unregister_chrdev(radar_dev->major, DRIVER_NAME);
        return ret;
    }
    
    // Initialize radar IP with default values
    iowrite32(0, radar_dev->base + RADAR_CONTROL_REG);     // Stop radar
    iowrite32(2000, radar_dev->base + RADAR_PRF_REG);      // 2 kHz PRF
    iowrite32(10, radar_dev->base + RADAR_PULSE_WIDTH_REG); // 10 us pulse
    iowrite32(1024, radar_dev->base + RADAR_RANGE_GATE_REG); // 1024 range gates
    iowrite32(64, radar_dev->base + RADAR_DOPPLER_BINS_REG); // 64 Doppler bins
    iowrite32(100, radar_dev->base + RADAR_THRESHOLD_REG);   // Default threshold
    
    platform_set_drvdata(pdev, radar_dev);
    
    dev_info(&pdev->dev, "Pulse radar IP driver probed successfully\n");
    dev_info(&pdev->dev, "Base address: %p, Target IRQ: %d, Processing IRQ: %d\n",
             radar_dev->base, radar_dev->target_detected_irq, radar_dev->processing_complete_irq);
    
    return 0;
}

static int radar_remove(struct platform_device *pdev)
{
    struct radar_device *rdev = platform_get_drvdata(pdev);
    
    // Stop radar
    iowrite32(0, rdev->base + RADAR_CONTROL_REG);
    
    // Remove sysfs attributes
    sysfs_remove_group(&pdev->dev.kobj, &radar_attr_group);
    
    // Unregister character device
    cdev_del(&rdev->cdev);
    unregister_chrdev(rdev->major, DRIVER_NAME);
    
    dev_info(&pdev->dev, "Pulse radar IP driver removed\n");
    
    return 0;
}

static const struct of_device_id radar_of_match[] = {
    { .compatible = "mycompany,pulse-radar-ip", },
    { /* end of list */ },
};
MODULE_DEVICE_TABLE(of, radar_of_match);

static struct platform_driver radar_driver = {
    .probe = radar_probe,
    .remove = radar_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = radar_of_match,
    },
};

module_platform_driver(radar_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ESPILINX");
MODULE_DESCRIPTION("Pulse Radar IP Driver with Doppler Processing");
MODULE_VERSION("1.0");
