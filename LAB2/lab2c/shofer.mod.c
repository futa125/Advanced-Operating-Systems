#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x152fb424, "module_layout" },
	{ 0x965bbc56, "param_ops_int" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xc245d526, "cdev_add" },
	{ 0xa755e624, "cdev_init" },
	{ 0xdcb764ad, "memset" },
	{ 0x292ee3ca, "kmem_cache_alloc_trace" },
	{ 0x13c47250, "kmalloc_caches" },
	{ 0xbd462b55, "__kfifo_init" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xa648e561, "__ubsan_handle_shift_out_of_bounds" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0xad900ce9, "cdev_del" },
	{ 0x4578f528, "__kfifo_to_user" },
	{ 0x1000e51, "schedule" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x30a80826, "__kfifo_from_user" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x800473f, "__cond_resched" },
	{ 0x89940875, "mutex_lock_interruptible" },
	{ 0x8da6585d, "__stack_chk_fail" },
	{ 0x281823c5, "__kfifo_out_peek" },
	{ 0x92997ed8, "_printk" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "EE5C965F8EEE917B94E4AED");
