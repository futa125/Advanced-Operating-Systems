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
	{ 0x24d273d1, "add_timer" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xa648e561, "__ubsan_handle_shift_out_of_bounds" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x30a80826, "__kfifo_from_user" },
	{ 0x4578f528, "__kfifo_to_user" },
	{ 0x9c1e5bf5, "queued_spin_lock_slowpath" },
	{ 0x5e2d7875, "cpu_hwcap_keys" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0x2b68bd2f, "del_timer" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xad900ce9, "cdev_del" },
	{ 0xc245d526, "cdev_add" },
	{ 0xa755e624, "cdev_init" },
	{ 0x292ee3ca, "kmem_cache_alloc_trace" },
	{ 0x13c47250, "kmalloc_caches" },
	{ 0x37a0cba, "kfree" },
	{ 0xbd462b55, "__kfifo_init" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x8da6585d, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0x281823c5, "__kfifo_out_peek" },
	{ 0xdcb764ad, "memset" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "16BBDAB20D8E6F7D12700FD");
