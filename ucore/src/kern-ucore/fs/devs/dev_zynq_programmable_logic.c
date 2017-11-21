#if defined(ARCH_ARM)

#include <types.h>
#include <stdio.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <inode.h>
#include <unistd.h>
#include <error.h>
#include <assert.h>
#include <pmm.h>
#include <zynq_programmable_logic.h>

//TODO: It would be better to use mmap based interface to implement this device.

struct zynq_programmable_logic_private_data {
  char* buffer_kernel_address;
  int buffer_used;
  uintptr_t buffer_physical_address;
};

static int zynq_programmable_logic_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int zynq_programmable_logic_close(struct device *dev)
{
	return 0;
}

static int zynq_programmable_logic_io(struct device *dev, struct iobuf *iob, bool write)
{
  struct zynq_programmable_logic_private_data *private_data = dev_get_private_data(dev);
	if (write) {
    char *data = iob->io_base;
    memcpy(private_data->buffer_kernel_address + private_data->buffer_used, data, iob->io_resid);
    private_data->buffer_used += iob->io_resid;
    iob->io_resid = 0;
    kprintf("zynqpl_buffer: %d\n", private_data->buffer_used);
		return 0;
	}
	return -E_INVAL;
}

static int zynq_programmable_logic_ctrl_io(struct device *dev, struct iobuf *iob, bool write)
{
  struct zynq_programmable_logic_private_data *private_data = dev_get_private_data(dev);
	if (write) {
    iob->io_resid = 0;
    kprintf("zynqpl write!\n");
    zynq_load(NULL, private_data->buffer_kernel_address, private_data->buffer_used, BIT_PARTIAL);
    private_data->buffer_used = 0;
		return 0;
	}
	return -E_INVAL;
}


static int zynq_programmable_logic_ioctl(struct device *dev, int op, void *data)
{
	return -E_INVAL;
}

static void zynq_programmable_logic_device_init(struct device *data_device, struct device *control_device)
{
	memset(data_device, 0, sizeof(*data_device));
  memset(control_device, 0, sizeof(*control_device));
	data_device->d_blocks = 0;
	data_device->d_blocksize = 1;
	data_device->d_open = zynq_programmable_logic_open;
	data_device->d_close = zynq_programmable_logic_close;
	data_device->d_io = zynq_programmable_logic_io;
	data_device->d_ioctl = zynq_programmable_logic_ioctl;
  control_device->d_blocks = 0;
  control_device->d_blocksize = 1;
  control_device->d_open = zynq_programmable_logic_open;
  control_device->d_close = zynq_programmable_logic_close;
  control_device->d_io = zynq_programmable_logic_ctrl_io;
  control_device->d_ioctl = zynq_programmable_logic_ioctl;
  struct zynq_programmable_logic_private_data *private_data = kmalloc(sizeof(struct zynq_programmable_logic_private_data));
  struct Page *page = alloc_pages(1024 * 1024 / PGSIZE);
  private_data->buffer_kernel_address = page2kva(page);
  private_data->buffer_used = 0;
  private_data->buffer_physical_address = page2pa(page);
  dev_set_private_data(data_device, private_data);
  dev_set_private_data(control_device, private_data);
}

void dev_init_zynq_programmable_logic(void)
{
	struct inode *data_node;
  struct inode *ctl_node;
	if ((data_node = dev_create_inode()) == NULL) {
		panic("zynq_programmable_logic: dev_create_node.\n");
	}
  if ((ctl_node = dev_create_inode()) == NULL) {
    panic("zynq_programmable_logic: dev_create_node.\n");
  }
	zynq_programmable_logic_device_init(vop_info(data_node, device), vop_info(ctl_node, device));

	int ret;
	if ((ret = vfs_add_dev("zynq_programmable_logic", data_node, 0)) != 0) {
		panic("stdout: vfs_add_dev: %e.\n", ret);
	}
  if ((ret = vfs_add_dev("zynq_programmable_logic_ctl", ctl_node, 0)) != 0) {
    panic("stdout: vfs_add_dev: %e.\n", ret);
  }
}

#endif
