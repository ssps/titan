/*******************************************************************************

  Intel(R) 10GbE PCI Express Linux Network Driver
  Copyright(c) 1999 - 2016 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Linux NICS <linux.nics@intel.com>
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

#include <linux/types.h>
#include <linux/module.h>

#include "ixgbe.h"

/* This is the only thing that needs to be changed to adjust the
 * maximum number of ports that the driver can manage.
 */

#define IXGBE_MAX_NIC	32

#define OPTION_UNSET	-1
#define OPTION_DISABLED	0
#define OPTION_ENABLED	1

#define STRINGIFY(foo)	#foo /* magic for getting defines into strings */
#define XSTRINGIFY(bar)	STRINGIFY(bar)

/* All parameters are treated the same, as an integer array of values.
 * This macro just reduces the need to repeat the same declaration code
 * over and over (plus this helps to avoid typo bugs).
 */

#define IXGBE_PARAM_INIT { [0 ... IXGBE_MAX_NIC] = OPTION_UNSET }
#ifndef module_param_array
/* Module Parameters are always initialized to -1, so that the driver
 * can tell the difference between no user specified value or the
 * user asking for the default value.
 * The true default values are loaded in when ixgbe_check_options is called.
 *
 * This is a GCC extension to ANSI C.
 * See the item "Labelled Elements in Initializers" in the section
 * "Extensions to the C Language Family" of the GCC documentation.
 */

#define IXGBE_PARAM(X, desc) \
	static const int __devinitdata X[IXGBE_MAX_NIC+1] = IXGBE_PARAM_INIT; \
	MODULE_PARM(X, "1-" __MODULE_STRING(IXGBE_MAX_NIC) "i"); \
	MODULE_PARM_DESC(X, desc);
#else
#define IXGBE_PARAM(X, desc) \
	static int __devinitdata X[IXGBE_MAX_NIC+1] = IXGBE_PARAM_INIT; \
	static unsigned int num_##X; \
	module_param_array_named(X, X, int, &num_##X, 0); \
	MODULE_PARM_DESC(X, desc);
#endif

IXGBE_PARAM(InterruptType, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), "
	    "default IntMode (deprecated)");
IXGBE_PARAM(IntMode, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), "
	    "default 2");
#define IXGBE_INT_LEGACY		0
#define IXGBE_INT_MSI			1
#define IXGBE_INT_MSIX			2
#define IXGBE_DEFAULT_INT		IXGBE_INT_MSIX

/* MQ - Multiple Queue enable/disable
 *
 * Valid Range: 0, 1
 *  - 0 - disables MQ
 *  - 1 - enables MQ
 *
 * Default Value: 1
 */

IXGBE_PARAM(MQ, "Disable or enable Multiple Queues, default 1");

#if IS_ENABLED(CONFIG_DCA)
/* DCA - Direct Cache Access (DCA) Control
 *
 * This option allows the device to hint to DCA enabled processors
 * which CPU should have its cache warmed with the data being
 * transferred over PCIe.  This can increase performance by reducing
 * cache misses.  ixgbe hardware supports DCA for:
 * tx descriptor writeback
 * rx descriptor writeback
 * rx data
 * rx data header only (in packet split mode)
 *
 * enabling option 2 can cause cache thrash in some tests, particularly
 * if the CPU is completely utilized
 *
 * Valid Range: 0 - 2
 *  - 0 - disables DCA
 *  - 1 - enables DCA
 *  - 2 - enables DCA with rx data included
 *
 * Default Value: 2
 */

#define IXGBE_MAX_DCA 2

IXGBE_PARAM(DCA, "Disable or enable Direct Cache Access, 0=disabled, "
	    "1=descriptor only, 2=descriptor and data");
#endif /* CONFIG_DCA */

/* RSS - Receive-Side Scaling (RSS) Descriptor Queues
 *
 * Valid Range: 0-16
 *  - 0 - enables RSS and sets the Desc. Q's to min(16, num_online_cpus()).
 *  - 1-16 - enables RSS and sets the Desc. Q's to the specified value.
 *
 * Default Value: 0
 */

IXGBE_PARAM(RSS, "Number of Receive-Side Scaling Descriptor Queues, "
	    "default 0=number of cpus");

IXGBE_PARAM(TSO, "Disable or enable TSO, 0=disabled, 1=enabled");

IXGBE_PARAM(WRR, "Use WRR scheduling between Tx queues. 0=disabled. 1=enabled");

IXGBE_PARAM(UsePoolQueues,  "Transparently use all of the queues in a pool. 0=disabled. 1=enabled");

IXGBE_PARAM(XmitBatch, "Use xmit batching, 0=disabled, 1=enabled");

IXGBE_PARAM(UseSgseg, "Use scatter/gather segmentation, 0=disabled, 1=enabled");

IXGBE_PARAM(UsePktr, "Use the packet ring, 0=disabled, 1=enabled");

IXGBE_PARAM(GSOSize, "GSO Size, range 0-262144"
	    "default 65536 bytes"); 

IXGBE_PARAM(DrvGSOSize, "Driver internal GSO Size, range 0-262144"
	    "default 65536 bytes"); 

/* VMDQ - Virtual Machine Device Queues (VMDQ)
 *
 * Valid Range: 1-16
 *  - 0/1 Disables VMDQ by allocating only a single queue.
 *  - 2-16 - enables VMDQ and sets the Desc. Q's to the specified value.
 *
 * Default Value: 8
 */

#define IXGBE_DEFAULT_NUM_VMDQ 8

IXGBE_PARAM(VMDQ, "Number of Virtual Machine Device Queues: 0/1 = disable (1 queue) "
	    "2-16 enable (default=" XSTRINGIFY(IXGBE_DEFAULT_NUM_VMDQ) ")");

#ifdef CONFIG_PCI_IOV
/* max_vfs - SR I/O Virtualization
 *
 * Valid Range: 0-63
 *  - 0 Disables SR-IOV
 *  - 1-63 - enables SR-IOV and sets the number of VFs enabled
 *
 * Default Value: 0
 */

#define MAX_SRIOV_VFS 63

IXGBE_PARAM(max_vfs, "Number of Virtual Functions: 0 = disable (default), "
	    "1-" XSTRINGIFY(MAX_SRIOV_VFS) " = enable "
	    "this many VFs");

/* VEPA - Set internal bridge to VEPA mode
 *
 * Valid Range: 0-1
 *  - 0 Set bridge to VEB mode
 *  - 1 Set bridge to VEPA mode
 *
 * Default Value: 0
 */
/*
 *Note:
 *=====
 * This provides ability to ensure VEPA mode on the internal bridge even if
 * the kernel does not support the netdev bridge setting operations.
*/
IXGBE_PARAM(VEPA, "VEPA Bridge Mode: 0 = VEB (default), 1 = VEPA");
#endif

/* Interrupt Throttle Rate (interrupts/sec)
 *
 * Valid Range: 956-488281 (0=off, 1=dynamic)
 *
 * Default Value: 1
 */
#define DEFAULT_ITR		1
IXGBE_PARAM(InterruptThrottleRate, "Maximum interrupts per second, per vector, "
	    "(0,1,956-488281), default 1");
#define MAX_ITR		IXGBE_MAX_INT_RATE
#define MIN_ITR		IXGBE_MIN_INT_RATE

#ifndef IXGBE_NO_LLI

/* LLIPort (Low Latency Interrupt TCP Port)
 *
 * Valid Range: 0 - 65535
 *
 * Default Value: 0 (disabled)
 */
IXGBE_PARAM(LLIPort, "Low Latency Interrupt TCP Port (0-65535)");

#define DEFAULT_LLIPORT		0
#define MAX_LLIPORT		0xFFFF
#define MIN_LLIPORT		0

/* LLIPush (Low Latency Interrupt on TCP Push flag)
 *
 * Valid Range: 0,1
 *
 * Default Value: 0 (disabled)
 */
IXGBE_PARAM(LLIPush, "Low Latency Interrupt on TCP Push flag (0,1)");

#define DEFAULT_LLIPUSH		0
#define MAX_LLIPUSH		1
#define MIN_LLIPUSH		0

/* LLISize (Low Latency Interrupt on Packet Size)
 *
 * Valid Range: 0 - 1500
 *
 * Default Value: 0 (disabled)
 */
IXGBE_PARAM(LLISize, "Low Latency Interrupt on Packet Size (0-1500)");

#define DEFAULT_LLISIZE		0
#define MAX_LLISIZE		1500
#define MIN_LLISIZE		0

/* LLIEType (Low Latency Interrupt Ethernet Type)
 *
 * Valid Range: 0 - 0x8fff
 *
 * Default Value: 0 (disabled)
 */
IXGBE_PARAM(LLIEType, "Low Latency Interrupt Ethernet Protocol Type");

#define DEFAULT_LLIETYPE	0
#define MAX_LLIETYPE		0x8fff
#define MIN_LLIETYPE		0

/* LLIVLANP (Low Latency Interrupt on VLAN priority threshold)
 *
 * Valid Range: 0 - 7
 *
 * Default Value: 0 (disabled)
 */
IXGBE_PARAM(LLIVLANP, "Low Latency Interrupt on VLAN priority threshold");

#define DEFAULT_LLIVLANP	0
#define MAX_LLIVLANP		7
#define MIN_LLIVLANP		0

#endif /* IXGBE_NO_LLI */
#ifdef HAVE_TX_MQ
/* Flow Director packet buffer allocation level
 *
 * Valid Range: 1-3
 *   1 = 8k hash/2k perfect,
 *   2 = 16k hash/4k perfect,
 *   3 = 32k hash/8k perfect
 *
 * Default Value: 0
 */
IXGBE_PARAM(FdirPballoc, "Flow Director packet buffer allocation level:\n"
	    "\t\t\t1 = 8k hash filters or 2k perfect filters\n"
	    "\t\t\t2 = 16k hash filters or 4k perfect filters\n"
	    "\t\t\t3 = 32k hash filters or 8k perfect filters");

#define IXGBE_DEFAULT_FDIR_PBALLOC IXGBE_FDIR_PBALLOC_64K

/* Software ATR packet sample rate
 *
 * Valid Range: 0-255  0 = off, 1-255 = rate of Tx packet inspection
 *
 * Default Value: 20
 */
IXGBE_PARAM(AtrSampleRate, "Software ATR Tx packet sample rate");

#define IXGBE_MAX_ATR_SAMPLE_RATE	255
#define IXGBE_MIN_ATR_SAMPLE_RATE	1
#define IXGBE_ATR_SAMPLE_RATE_OFF	0
#define IXGBE_DEFAULT_ATR_SAMPLE_RATE	20
#endif /* HAVE_TX_MQ */

#if IS_ENABLED(CONFIG_FCOE)
/* FCoE - Fibre Channel over Ethernet Offload  Enable/Disable
 *
 * Valid Range: 0, 1
 *  - 0 - disables FCoE Offload
 *  - 1 - enables FCoE Offload
 *
 * Default Value: 1
 */
IXGBE_PARAM(FCoE, "Disable or enable FCoE Offload, default 1");
#endif /* CONFIG_FCOE */

/* Enable/disable Malicious Driver Detection
 *
 * Valid Values: 0(off), 1(on)
 *
 * Default Value: 1
 */
IXGBE_PARAM(MDD, "Malicious Driver Detection: (0,1), default 1 = on");

/* Enable/disable Large Receive Offload
 *
 * Valid Values: 0(off), 1(on)
 *
 * Default Value: 1
 */
IXGBE_PARAM(LRO, "Large Receive Offload (0,1), default 0 = off");

/* Enable/disable support for untested SFP+ modules on 82599-based adapters
 *
 * Valid Values: 0(Disable), 1(Enable)
 *
 * Default Value: 0
 */
IXGBE_PARAM(allow_unsupported_sfp, "Allow unsupported and untested "
	    "SFP+ modules on 82599 based adapters, default 0 = Disable");

/* Enable/disable support for DMA coalescing
 *
 * Valid Values: 0(off), 41 - 10000(on)
 *
 * Default Value: 0
 */
IXGBE_PARAM(dmac_watchdog,
	    "DMA coalescing watchdog in microseconds (0,41-10000), default 0 = off");

/* Enable/disable support for VXLAN rx checksum offload
 *
 * Valid Values: 0(Disable), 1(Enable)
 *
 * Default Value: 1 on hardware that supports it
 */
IXGBE_PARAM(vxlan_rx,
	    "VXLAN receive checksum offload (0,1), default 1 = Enable");

struct ixgbe_option {
	enum { enable_option, range_option, list_option } type;
	const char *name;
	const char *err;
	const char *msg;
	int def;
	union {
		struct { /* range_option info */
			int min;
			int max;
		} r;
		struct { /* list_option info */
			int nr;
			const struct ixgbe_opt_list {
				int i;
				char *str;
			} *p;
		} l;
	} arg;
};

#ifndef IXGBE_NO_LLI
#ifdef module_param_array
/**
 *  helper function to determine LLI support
 *
 *  LLI is only supported for 82599 and X540
 *  LLIPush is not supported on 82599
 **/
static bool __devinit ixgbe_lli_supported(struct ixgbe_adapter *adapter,
					  struct ixgbe_option *opt)
{
	struct ixgbe_hw *hw = &adapter->hw;

	if (hw->mac.type == ixgbe_mac_82599EB) {

		if (LLIPush[adapter->bd_number] > 0)
			goto not_supp;

		return true;
	}

	if (hw->mac.type == ixgbe_mac_X540)
		return true;

not_supp:
	DPRINTK(PROBE, INFO, "%s not supported on this HW\n", opt->name);
	return false;
}
#endif /* module_param_array */
#endif /* IXGBE_NO_LLI */

static int __devinit ixgbe_validate_option(unsigned int *value,
					   struct ixgbe_option *opt)
{
	if (*value == OPTION_UNSET) {
		printk(KERN_INFO "ixgbe: Invalid %s specified (%d),  %s\n",
			opt->name, *value, opt->err);
		*value = opt->def;
		return 0;
	}

	switch (opt->type) {
	case enable_option:
		switch (*value) {
		case OPTION_ENABLED:
			printk(KERN_INFO "ixgbe: %s Enabled\n", opt->name);
			return 0;
		case OPTION_DISABLED:
			printk(KERN_INFO "ixgbe: %s Disabled\n", opt->name);
			return 0;
		}
		break;
	case range_option:
		if ((*value >= opt->arg.r.min && *value <= opt->arg.r.max) ||
		    *value == opt->def) {
			if (opt->msg)
				printk(KERN_INFO "ixgbe: %s set to %d, %s\n",
				       opt->name, *value, opt->msg);
			else
				printk(KERN_INFO "ixgbe: %s set to %d\n",
				       opt->name, *value);
			return 0;
		}
		break;
	case list_option: {
		int i;

		for (i = 0; i < opt->arg.l.nr; i++) {
			const struct ixgbe_opt_list *ent = &opt->arg.l.p[i];
			if (*value == ent->i) {
				if (ent->str[0] != '\0')
					printk(KERN_INFO "%s\n", ent->str);
				return 0;
			}
		}
	}
		break;
	default:
		BUG();
	}

	printk(KERN_INFO "ixgbe: Invalid %s specified (%d),  %s\n",
	       opt->name, *value, opt->err);
	*value = opt->def;
	return -1;
}

#define LIST_LEN(l) (sizeof(l) / sizeof(l[0]))

/**
 * ixgbe_check_options - Range Checking for Command Line Parameters
 * @adapter: board private structure
 *
 * This routine checks all command line parameters for valid user
 * input.  If an invalid value is given, or if no user specified
 * value exists, a default value is used.  The final value is stored
 * in a variable in the adapter structure.
 **/
void __devinit ixgbe_check_options(struct ixgbe_adapter *adapter)
{
	unsigned int mdd;
	int bd = adapter->bd_number;
	u32 *aflags = &adapter->flags;
	struct ixgbe_ring_feature *feature = adapter->ring_feature;
	unsigned int vmdq;

	if (bd >= IXGBE_MAX_NIC) {
		printk(KERN_NOTICE
		       "Warning: no configuration for board #%d\n", bd);
		printk(KERN_NOTICE "Using defaults for all values\n");
#ifndef module_param_array
		bd = IXGBE_MAX_NIC;
#endif
	}

	{ /* Interrupt Mode */
		unsigned int int_mode;
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Interrupt Mode",
			.err =
			  "using default of "__MODULE_STRING(IXGBE_DEFAULT_INT),
			.def = IXGBE_DEFAULT_INT,
			.arg = { .r = { .min = IXGBE_INT_LEGACY,
					.max = IXGBE_INT_MSIX} }
		};

#ifdef module_param_array
		if (num_IntMode > bd || num_InterruptType > bd) {
#endif
			int_mode = IntMode[bd];
			if (int_mode == OPTION_UNSET)
				int_mode = InterruptType[bd];
			ixgbe_validate_option(&int_mode, &opt);
			switch (int_mode) {
			case IXGBE_INT_MSIX:
				if (!(*aflags & IXGBE_FLAG_MSIX_CAPABLE))
					printk(KERN_INFO
					       "Ignoring MSI-X setting; "
					       "support unavailable\n");
				break;
			case IXGBE_INT_MSI:
				if (!(*aflags & IXGBE_FLAG_MSI_CAPABLE)) {
					printk(KERN_INFO
					       "Ignoring MSI setting; "
					       "support unavailable\n");
				} else {
					*aflags &= ~IXGBE_FLAG_MSIX_CAPABLE;
				}
				break;
			case IXGBE_INT_LEGACY:
			default:
				*aflags &= ~IXGBE_FLAG_MSIX_CAPABLE;
				*aflags &= ~IXGBE_FLAG_MSI_CAPABLE;
				break;
			}
#ifdef module_param_array
		} else {
			/* default settings */
			if (opt.def == IXGBE_INT_MSIX &&
			    *aflags & IXGBE_FLAG_MSIX_CAPABLE) {
				*aflags |= IXGBE_FLAG_MSIX_CAPABLE;
				*aflags |= IXGBE_FLAG_MSI_CAPABLE;
			} else if (opt.def == IXGBE_INT_MSI &&
			    *aflags & IXGBE_FLAG_MSI_CAPABLE) {
				*aflags &= ~IXGBE_FLAG_MSIX_CAPABLE;
				*aflags |= IXGBE_FLAG_MSI_CAPABLE;
			} else {
				*aflags &= ~IXGBE_FLAG_MSIX_CAPABLE;
				*aflags &= ~IXGBE_FLAG_MSI_CAPABLE;
			}
		}
#endif
	}
	{ /* Multiple Queue Support */
		static struct ixgbe_option opt = {
			.type = enable_option,
			.name = "Multiple Queue Support",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED
		};

#ifdef module_param_array
		if (num_MQ > bd) {
#endif
			unsigned int mq = MQ[bd];
			ixgbe_validate_option(&mq, &opt);
			if (mq)
				*aflags |= IXGBE_FLAG_MQ_CAPABLE;
			else
				*aflags &= ~IXGBE_FLAG_MQ_CAPABLE;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_ENABLED)
				*aflags |= IXGBE_FLAG_MQ_CAPABLE;
			else
				*aflags &= ~IXGBE_FLAG_MQ_CAPABLE;
		}
#endif
		/* Check Interoperability */
		if ((*aflags & IXGBE_FLAG_MQ_CAPABLE) &&
		    !(*aflags & IXGBE_FLAG_MSIX_CAPABLE)) {
			DPRINTK(PROBE, INFO,
				"Multiple queues are not supported while MSI-X "
				"is disabled.  Disabling Multiple Queues.\n");
			*aflags &= ~IXGBE_FLAG_MQ_CAPABLE;
		}
	}
#if IS_ENABLED(CONFIG_DCA)
	{ /* Direct Cache Access (DCA) */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Direct Cache Access (DCA)",
			.err  = "defaulting to Enabled",
			.def  = IXGBE_MAX_DCA,
			.arg  = { .r = { .min = OPTION_DISABLED,
					 .max = IXGBE_MAX_DCA} }
		};
		unsigned int dca = opt.def;

#ifdef module_param_array
		if (num_DCA > bd) {
#endif
			dca = DCA[bd];
			ixgbe_validate_option(&dca, &opt);
			if (!dca)
				*aflags &= ~IXGBE_FLAG_DCA_CAPABLE;

			/* Check Interoperability */
			if (!(*aflags & IXGBE_FLAG_DCA_CAPABLE)) {
				DPRINTK(PROBE, INFO, "DCA is disabled\n");
				*aflags &= ~IXGBE_FLAG_DCA_ENABLED;
			}

			if (dca == IXGBE_MAX_DCA) {
				DPRINTK(PROBE, INFO,
					"DCA enabled for rx data\n");
				adapter->flags |= IXGBE_FLAG_DCA_ENABLED_DATA;
			}
#ifdef module_param_array
		} else {
			/* make sure to clear the capability flag if the
			 * option is disabled by default above */
			if (opt.def == OPTION_DISABLED)
				*aflags &= ~IXGBE_FLAG_DCA_CAPABLE;
		}
#endif
		if (dca == IXGBE_MAX_DCA)
			adapter->flags |= IXGBE_FLAG_DCA_ENABLED_DATA;
	}
#endif /* CONFIG_DCA */
	{ /* TSO Support */
                static struct ixgbe_option opt = {
                        .type = enable_option,
                        .name = "TSO Support",
                        .err  = "defaulting to Enabled",
                        .def  = OPTION_ENABLED
                };
		struct net_device *netdev = adapter->netdev;
#ifdef module_param_array
                if (num_TSO > bd) {
#endif
                        unsigned int tso = TSO[bd];
			//printk("TSO value :::: %d",tso);
                        ixgbe_validate_option(&tso, &opt);
			if(tso)
			{
				printk("Enabling TSO\n");
				netdev->features |= NETIF_F_TSO;
				//printk("TSO FLAG %u\n",(netdev->features & NETIF_F_TSO));
			}
			else
			{
				printk("Disabling TSO\n");
				netdev->features &= ~NETIF_F_TSO;
			}

#ifdef module_param_array
                } else {
			//printk("Deafault TSO value");
			//printk("%d",opt.def);
                        if (opt.def == OPTION_ENABLED)
                               netdev->features |= NETIF_F_TSO;
                        else
                            	netdev->features &= ~NETIF_F_TSO;
                }
#endif
        }
	{ /* WRR Support */
                static struct ixgbe_option opt = {
                        .type = enable_option,
                        .name = "WRR Support",
                        .err  = "defaulting to Disabled",
                        .def  = OPTION_DISABLED
                };
#ifdef module_param_array
                if (num_WRR > bd) {
#endif
                        unsigned int wrr = WRR[bd];
			//printk("WRR value :::: %d",tso);
                        ixgbe_validate_option(&wrr, &opt);
			if(wrr)
			{
				printk("Enabling WRR");
                                adapter->wrr = true;
			}
			else
			{
				printk("Disabling WRR");
				adapter->wrr = false;
			}

#ifdef module_param_array
                } else {
			//printk("Deafault WRR value");
			//printk("%d",opt.def);
                        if (opt.def == OPTION_ENABLED) {
				printk("Enabling WRR");
                                adapter->wrr = true;
                        } else {
				printk("Disabling WRR");
				adapter->wrr = false;
                        }
                }
#endif
        }
        { /* GSOSize */
                static struct ixgbe_option opt = {
                        .type = range_option,
                        .name = "GSO Size initialization",
                        .err  = "using default.",
                        .def  = 0,
                        .arg  = { .r = { .min = 0,
                                         .max = 262144} }
                };
		struct net_device *netdev = adapter->netdev;
                unsigned int gsosize = GSOSize[bd];
#ifdef module_param_array
                if (num_GSOSize > bd) {
#endif
                        ixgbe_validate_option(&gsosize, &opt);
                        if (gsosize)
			{
				//pr_info("GSOSize Non-default value setup: %u\n",
                                //        gsosize);
                                netif_set_gso_max_size(netdev, gsosize);
                                adapter->kern_gso_size = gsosize;
			}
#ifdef module_param_array
                } else if (opt.def == 0) {
			//pr_info("GSOSize Default Value\n");
                        adapter->kern_gso_size = 65536;
                        netif_set_gso_max_size(netdev, 65536);
                }
#endif
        }
        { /* DrvGSOSize */
                static struct ixgbe_option opt = {
                        .type = range_option,
                        .name = "Driver GSO Size initialization",
                        .err  = "using default.",
                        .def  = 0,
                        .arg  = { .r = { .min = 100,
                                         .max = 262144} }
                };
                unsigned int drvgsosize = DrvGSOSize[bd];
#ifdef module_param_array
                if (num_DrvGSOSize > bd) {
#endif
                        ixgbe_validate_option(&drvgsosize, &opt);
                        if (drvgsosize)
			{
				//pr_info("DrvGSOSize Non-default value setup\n");
                                //TODO: Does anything break if drvgsosize is large?
                                //BUG_ON (drvgsosize > IXGBE_MAX_DATA_PER_TXD);
                                adapter->drv_gso_size = drvgsosize;
			}
#ifdef module_param_array
                } else if (opt.def == 0) {
			//pr_info("DrvGSOSize Default Value: %u\n", 65536);
                        adapter->drv_gso_size = 65536;
                }
#endif
        }
	{ /* Use all of the tx queues in a pool */
                static struct ixgbe_option opt = {
                        .type = enable_option,
                        .name = "Use all pool queues",
                        .err  = "defaulting to Disabled",
                        .def  = OPTION_DISABLED
                };
#ifdef module_param_array
                if (num_UsePoolQueues > bd) {
#endif
                        unsigned int use_pool_queues = UsePoolQueues[bd];
			//printk("UsePoolQueues value :::: %d",tso);
                        ixgbe_validate_option(&use_pool_queues, &opt);
			if(use_pool_queues)
			{
				pr_info ("Enabling UsePoolQueues\n");
                                adapter->use_pool_queues = true;
			}
			else
			{
				pr_info ("Disabling UsePoolQueues\n");
				adapter->use_pool_queues = false;
			}

#ifdef module_param_array
                } else {
                        if (opt.def == OPTION_ENABLED)
                                adapter->use_pool_queues = true;
                        else
				adapter->use_pool_queues = false;
			pr_info ("Default UsePoolQueues value: %d\n",
				 adapter->use_pool_queues);
                }
#endif
        }
	{ /* Xmit Batching */
                static struct ixgbe_option opt = {
                        .type = enable_option,
                        .name = "Xmit Batching",
                        .err  = "defaulting to Disabled",
                        .def  = OPTION_DISABLED
                };
#ifdef module_param_array
                if (num_XmitBatch > bd) {
#endif
                        unsigned int xmit_batch = XmitBatch[bd];
			//printk("XmitBatch value :::: %d",tso);
                        ixgbe_validate_option(&xmit_batch, &opt);
			if(xmit_batch)
			{
				//pr_info ("Enabling XmitBatch\n");
                                adapter->xmit_batch = true;
			}
			else
			{
				//pr_info ("Disabling XmitBatch\n");
				adapter->xmit_batch = false;
			}

#ifdef module_param_array
                } else {
			//pr_info ("Default XmitBatch value\n");
                        if (opt.def == OPTION_ENABLED)
                                adapter->xmit_batch = true;
                        else
				adapter->xmit_batch = false;
                }
#endif
        }
	{ /* Use Sgseg */
                static struct ixgbe_option opt = {
                        .type = enable_option,
                        .name = "Use Sgseg",
                        .err  = "defaulting to Disabled",
                        .def  = OPTION_DISABLED
                };
#ifdef module_param_array
                if (num_UseSgseg > bd) {
#endif
                        unsigned int use_sgseg = UseSgseg[bd];
			//printk("XmitBatch value :::: %d",tso);
                        ixgbe_validate_option(&use_sgseg, &opt);
			if(use_sgseg)
			{
				//pr_info ("Enabling UseSgseg\n");
                                adapter->use_sgseg = true;
			}
			else
			{
				//pr_info ("Disabling UseSgseg\n");
				adapter->use_sgseg = false;
			}

#ifdef module_param_array
                } else {
			//pr_info ("Default UseSgseg value\n");
                        if (opt.def == OPTION_ENABLED)
                                adapter->use_sgseg = true;
                        else
				adapter->use_sgseg = false;
                }
#endif
        }
	{ /* Use Pktr */
                static struct ixgbe_option opt = {
                        .type = enable_option,
                        .name = "Use Pktr",
                        .err  = "defaulting to Disabled",
                        .def  = OPTION_DISABLED
                };
#ifdef module_param_array
                if (num_UsePktr > bd) {
#endif
                        unsigned int use_pkt_ring = UsePktr[bd];
			//printk("XmitBatch value :::: %d",tso);
                        ixgbe_validate_option(&use_pkt_ring, &opt);
			if(use_pkt_ring)
			{
				//pr_info ("Enabling UsePktr\n");
                                adapter->use_pkt_ring = true;
			}
			else
			{
				//pr_info ("Disabling UsePktr\n");
				adapter->use_pkt_ring = false;
			}

#ifdef module_param_array
                } else {
			//pr_info ("Default UsePktr value\n");
                        if (opt.def == OPTION_ENABLED)
                                adapter->use_pkt_ring = true;
                        else
				adapter->use_pkt_ring = false;
                }
#endif
        }


	{ /* Receive-Side Scaling (RSS) */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Receive-Side Scaling (RSS)",
			.err  = "using default.",
			.def  = 0,
			.arg  = { .r = { .min = 0,
					 .max = 64} }
		};
		unsigned int rss = RSS[bd];

		/* adjust Max allowed RSS queues based on MAC type */
                /* TODO: I am not sure that the 82599 supports more than 16
                 * RSS queues.  If this is the case, this code could be
                 * broken.  In order to fix this, the driver needs to be
                 * modified to have a different number of tx queues than rx
                 * queues. */
		opt.arg.r.max = 64; //ixgbe_max_rss_indices(adapter);

#ifdef module_param_array
		if (num_RSS > bd) {
#endif
			ixgbe_validate_option(&rss, &opt);
			/* base it off num_online_cpus() with hardware limit */
			if (!rss)
				rss = min_t(int, opt.arg.r.max,
					    num_online_cpus());
			else
				feature[RING_F_FDIR].limit = rss;

			feature[RING_F_RSS].limit = rss;
#ifdef module_param_array
		} else if (opt.def == 0) {
			/* We allow for more queues than cores for testing. */
			//rss = min_t(int, ixgbe_max_rss_indices(adapter),
			//	    num_online_cpus());
			rss = ixgbe_max_rss_indices(adapter);
			feature[RING_F_RSS].limit = rss;
		}
#endif
		/* Check Interoperability */
		if (rss > 1) {
			if (!(*aflags & IXGBE_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"Multiqueue is disabled.  "
					"Limiting RSS.\n");
				feature[RING_F_RSS].limit = 1;
			}
		}
	}
	{ /* Virtual Machine Device Queues (VMDQ) */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Virtual Machine Device Queues (VMDQ)",
			.err  = "defaulting to Disabled",
			.def  = 16,
			.arg  = { .r = { .min = OPTION_DISABLED,
					 .max = IXGBE_MAX_VMDQ_INDICES
				} }
		};

		switch (adapter->hw.mac.type) {
		case ixgbe_mac_82598EB:
			/* 82598 only supports up to 16 pools */
				opt.arg.r.max = 16;
			break;
		default:
			break;
		}

#ifdef module_param_array
		if (num_VMDQ > bd) {
#endif
			vmdq = VMDQ[bd];

			ixgbe_validate_option(&vmdq, &opt);

			/* zero or one both mean disabled from our driver's
			 * perspective */
			//if (vmdq > 1) {
			//	*aflags |= IXGBE_FLAG_VMDQ_ENABLED;
			//} else
			//	*aflags &= ~IXGBE_FLAG_VMDQ_ENABLED;

			/* In the qweight version, 1 still means VMDq so we can
			 * get queue weights? */
			if (vmdq > 0) {
				*aflags |= IXGBE_FLAG_VMDQ_ENABLED;
			} else {
				*aflags &= ~IXGBE_FLAG_VMDQ_ENABLED;
			}

			feature[RING_F_VMDQ].limit = vmdq;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_DISABLED)
				*aflags &= ~IXGBE_FLAG_VMDQ_ENABLED;
			else
				*aflags |= IXGBE_FLAG_VMDQ_ENABLED;

			feature[RING_F_VMDQ].limit = opt.def;
		}
#endif
		/* Check Interoperability */
		if (*aflags & IXGBE_FLAG_VMDQ_ENABLED) {
			if (!(*aflags & IXGBE_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"VMDQ is not supported while multiple "
					"queues are disabled.  "
					"Disabling VMDQ.\n");
				*aflags &= ~IXGBE_FLAG_VMDQ_ENABLED;
				feature[RING_F_VMDQ].limit = 0;
			}
		}

                /* XXX: HACK to ensure that VMDq is always enabled */
		if (!(*aflags & IXGBE_FLAG_VMDQ_ENABLED)) {
			DPRINTK(PROBE, INFO,
				"The QWeight version of ixgbe requires that "
				"VMDq is enabled. Enabling VMDq.\n");

			*aflags |= IXGBE_FLAG_VMDQ_ENABLED;
			feature[RING_F_VMDQ].limit = opt.def;
                }
	}
#ifdef CONFIG_PCI_IOV
	{ /* Single Root I/O Virtualization (SR-IOV) */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "I/O Virtualization (IOV)",
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED,
			.arg  = { .r = { .min = OPTION_DISABLED,
					 .max = MAX_SRIOV_VFS} }
		};

#ifdef module_param_array
		if (num_max_vfs > bd) {
#endif
			unsigned int vfs = max_vfs[bd];
			if (ixgbe_validate_option(&vfs, &opt)) {
				vfs = 0;
				DPRINTK(PROBE, INFO,
					"max_vfs out of range "
					"Disabling SR-IOV.\n");
			}

			adapter->num_vfs = vfs;

			if (vfs)
				*aflags |= IXGBE_FLAG_SRIOV_ENABLED;
			else
				*aflags &= ~IXGBE_FLAG_SRIOV_ENABLED;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_DISABLED) {
				adapter->num_vfs = 0;
				*aflags &= ~IXGBE_FLAG_SRIOV_ENABLED;
			} else {
				adapter->num_vfs = opt.def;
				*aflags |= IXGBE_FLAG_SRIOV_ENABLED;
			}
		}
#endif

		/* Check Interoperability */
		if (*aflags & IXGBE_FLAG_SRIOV_ENABLED) {
			if (!(*aflags & IXGBE_FLAG_SRIOV_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"IOV is not supported on this "
					"hardware.  Disabling IOV.\n");
				*aflags &= ~IXGBE_FLAG_SRIOV_ENABLED;
				adapter->num_vfs = 0;
			} else if (!(*aflags & IXGBE_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"IOV is not supported while multiple "
					"queues are disabled.  "
					"Disabling IOV.\n");
				*aflags &= ~IXGBE_FLAG_SRIOV_ENABLED;
				adapter->num_vfs = 0;
			} 
		}
	}
	{ /* VEPA Bridge Mode enable for SR-IOV mode */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "VEPA Bridge Mode Enable",
			.err  = "defaulting to disabled",
			.def  = OPTION_DISABLED,
			.arg  = { .r = { .min = OPTION_DISABLED,
					 .max = OPTION_ENABLED} }
		};

#ifdef module_param_array
		if (num_VEPA > bd) {
#endif
			unsigned int vepa = VEPA[bd];
			ixgbe_validate_option(&vepa, &opt);
			if (vepa)
				adapter->flags |=
					IXGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_ENABLED)
				adapter->flags |=
					IXGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		}
#endif
	}
#endif /* CONFIG_PCI_IOV */
	{ /* Interrupt Throttling Rate */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Interrupt Throttling Rate (ints/sec)",
			.err  = "using default of "__MODULE_STRING(DEFAULT_ITR),
			.def  = DEFAULT_ITR,
			.arg  = { .r = { .min = MIN_ITR,
					 .max = MAX_ITR } }
		};

#ifdef module_param_array
		if (num_InterruptThrottleRate > bd) {
#endif
			u32 itr = InterruptThrottleRate[bd];
			switch (itr) {
			case 0:
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
				adapter->rx_itr_setting = 0;
				break;
			case 1:
				DPRINTK(PROBE, INFO, "dynamic interrupt "
					"throttling enabled\n");
				adapter->rx_itr_setting = 1;
				break;
			default:
				ixgbe_validate_option(&itr, &opt);
				DPRINTK(PROBE, INFO, "static interrupt "
					"throttling enabled: %d ITRPS\n",
					itr);
				/* the first bit is used as control */
				adapter->rx_itr_setting = (1000000/itr) << 2;
				break;
			}
			adapter->tx_itr_setting = adapter->rx_itr_setting;
#ifdef module_param_array
		} else {
			adapter->rx_itr_setting = opt.def;
			adapter->tx_itr_setting = opt.def;
		}
#endif
	}
#ifndef IXGBE_NO_LLI
	{ /* Low Latency Interrupt TCP Port*/
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt TCP Port",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLIPORT),
			.def  = DEFAULT_LLIPORT,
			.arg  = { .r = { .min = MIN_LLIPORT,
					 .max = MAX_LLIPORT } }
		};

#ifdef module_param_array
		if (num_LLIPort > bd && ixgbe_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_port = LLIPort[bd];
			if (adapter->lli_port) {
				ixgbe_validate_option(&adapter->lli_port, &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_port = opt.def;
		}
#endif
	}
	{ /* Low Latency Interrupt on Packet Size */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Packet Size",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLISIZE),
			.def  = DEFAULT_LLISIZE,
			.arg  = { .r = { .min = MIN_LLISIZE,
					 .max = MAX_LLISIZE } }
		};

#ifdef module_param_array
		if (num_LLISize > bd && ixgbe_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_size = LLISize[bd];
			if (adapter->lli_size) {
				ixgbe_validate_option(&adapter->lli_size, &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_size = opt.def;
		}
#endif
	}
	{ /*Low Latency Interrupt on TCP Push flag*/
		static struct ixgbe_option opt = {
			.type = enable_option,
			.name = "Low Latency Interrupt on TCP Push flag",
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED
		};

#ifdef module_param_array
		if (num_LLIPush > bd && ixgbe_lli_supported(adapter, &opt)) {
#endif
			unsigned int lli_push = LLIPush[bd];

			ixgbe_validate_option(&lli_push, &opt);
			if (lli_push)
				*aflags |= IXGBE_FLAG_LLI_PUSH;
			else
				*aflags &= ~IXGBE_FLAG_LLI_PUSH;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_ENABLED)
				*aflags |= IXGBE_FLAG_LLI_PUSH;
			else
				*aflags &= ~IXGBE_FLAG_LLI_PUSH;
		}
#endif
	}
	{ /* Low Latency Interrupt EtherType*/
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Ethernet Protocol "
				"Type",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLIETYPE),
			.def  = DEFAULT_LLIETYPE,
			.arg  = { .r = { .min = MIN_LLIETYPE,
					 .max = MAX_LLIETYPE } }
		};

#ifdef module_param_array
		if (num_LLIEType > bd && ixgbe_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_etype = LLIEType[bd];
			if (adapter->lli_etype) {
				ixgbe_validate_option(&adapter->lli_etype,
						      &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_etype = opt.def;
		}
#endif
	}
	{ /* LLI VLAN Priority */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on VLAN priority "
				"threshold",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLIVLANP),
			.def  = DEFAULT_LLIVLANP,
			.arg  = { .r = { .min = MIN_LLIVLANP,
					 .max = MAX_LLIVLANP } }
		};

#ifdef module_param_array
		if (num_LLIVLANP > bd && ixgbe_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_vlan_pri = LLIVLANP[bd];
			if (adapter->lli_vlan_pri) {
				ixgbe_validate_option(&adapter->lli_vlan_pri,
						      &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_vlan_pri = opt.def;
		}
#endif
	}
#endif /* IXGBE_NO_LLI */
#ifdef HAVE_TX_MQ
	{ /* Flow Director packet buffer allocation */
		unsigned int fdir_pballoc_mode;
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Flow Director packet buffer allocation",
			.err = "using default of "
				__MODULE_STRING(IXGBE_DEFAULT_FDIR_PBALLOC),
			.def = IXGBE_DEFAULT_FDIR_PBALLOC,
			.arg = {.r = {.min = IXGBE_FDIR_PBALLOC_64K,
				      .max = IXGBE_FDIR_PBALLOC_256K} }
		};
		char pstring[10];

		if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
			adapter->fdir_pballoc = IXGBE_FDIR_PBALLOC_NONE;
		} else if (num_FdirPballoc > bd) {
			fdir_pballoc_mode = FdirPballoc[bd];
			ixgbe_validate_option(&fdir_pballoc_mode, &opt);
			switch (fdir_pballoc_mode) {
			case IXGBE_FDIR_PBALLOC_256K:
				adapter->fdir_pballoc = IXGBE_FDIR_PBALLOC_256K;
				sprintf(pstring, "256kB");
				break;
			case IXGBE_FDIR_PBALLOC_128K:
				adapter->fdir_pballoc = IXGBE_FDIR_PBALLOC_128K;
				sprintf(pstring, "128kB");
				break;
			case IXGBE_FDIR_PBALLOC_64K:
			default:
				adapter->fdir_pballoc = IXGBE_FDIR_PBALLOC_64K;
				sprintf(pstring, "64kB");
				break;
			}
			DPRINTK(PROBE, INFO, "Flow Director will be allocated "
				"%s of packet buffer\n", pstring);
		} else {
			adapter->fdir_pballoc = opt.def;
		}
		
	}
	{ /* Flow Director ATR Tx sample packet rate */
		static struct ixgbe_option opt = {
			.type = range_option,
			.name = "Software ATR Tx packet sample rate",
			.err = "using default of "
				__MODULE_STRING(IXGBE_DEFAULT_ATR_SAMPLE_RATE),
			.def = IXGBE_DEFAULT_ATR_SAMPLE_RATE,
			.arg = {.r = {.min = IXGBE_ATR_SAMPLE_RATE_OFF,
				      .max = IXGBE_MAX_ATR_SAMPLE_RATE} }
		};
		static const char atr_string[] =
					    "ATR Tx Packet sample rate set to";

		if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
			adapter->atr_sample_rate = IXGBE_ATR_SAMPLE_RATE_OFF;
		} else if (num_AtrSampleRate > bd) {
			adapter->atr_sample_rate = AtrSampleRate[bd];

			if (adapter->atr_sample_rate) {
				ixgbe_validate_option(&adapter->atr_sample_rate,
						      &opt);
				DPRINTK(PROBE, INFO, "%s %d\n", atr_string,
					adapter->atr_sample_rate);
			}
		} else {
			adapter->atr_sample_rate = opt.def;
		}
	}
#endif /* HAVE_TX_MQ */
#if IS_ENABLED(CONFIG_FCOE)
	{
		*aflags &= ~IXGBE_FLAG_FCOE_CAPABLE;

		switch (adapter->hw.mac.type) {
		case ixgbe_mac_X540:
		case ixgbe_mac_X550:
		case ixgbe_mac_82599EB: {
			struct ixgbe_option opt = {
				.type = enable_option,
				.name = "Enabled/Disable FCoE offload",
				.err = "defaulting to Enabled",
				.def = OPTION_ENABLED
			};
#ifdef module_param_array
			if (num_FCoE > bd) {
#endif
				unsigned int fcoe = FCoE[bd];

				ixgbe_validate_option(&fcoe, &opt);
				if (fcoe)
					*aflags |= IXGBE_FLAG_FCOE_CAPABLE;
#ifdef module_param_array
			} else {
				if (opt.def == OPTION_ENABLED)
					*aflags |= IXGBE_FLAG_FCOE_CAPABLE;
			}
#endif
			DPRINTK(PROBE, INFO, "FCoE Offload feature %sabled\n",
				(*aflags & IXGBE_FLAG_FCOE_CAPABLE) ?
				"en" : "dis");
		}
			break;
		default:
			break;
		}
	}
#endif /* CONFIG_FCOE */
	{ /* LRO - Set Large Receive Offload */
		struct ixgbe_option opt = {
			.type = enable_option,
			.name = "LRO - Large Receive Offload",
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED
		};
		struct net_device *netdev = adapter->netdev;

		if (!(adapter->flags2 & IXGBE_FLAG2_RSC_CAPABLE))
			opt.def = OPTION_DISABLED;

#ifdef module_param_array
		if (num_LRO > bd) {
#endif
			unsigned int lro = LRO[bd];
			ixgbe_validate_option(&lro, &opt);
			if (lro)
				netdev->features |= NETIF_F_LRO;
			else
				netdev->features &= ~NETIF_F_LRO;
#ifdef module_param_array
		} else if (opt.def == OPTION_ENABLED) {
			netdev->features |= NETIF_F_LRO;
		} else {
			netdev->features &= ~NETIF_F_LRO;
		}
#endif
		if ((netdev->features & NETIF_F_LRO) &&
		    !(adapter->flags2 & IXGBE_FLAG2_RSC_CAPABLE)) {
			DPRINTK(PROBE, INFO,
				"RSC is not supported on this "
				"hardware.  Disabling RSC.\n");
			netdev->features &= ~NETIF_F_LRO;
		}
	}
	{ /*
	   * allow_unsupported_sfp - Enable/Disable support for unsupported
	   * and untested SFP+ modules.
	   */
	struct ixgbe_option opt = {
			.type = enable_option,
			.name = "allow_unsupported_sfp",
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED
		};
#ifdef module_param_array
		if (num_allow_unsupported_sfp > bd) {
#endif
			unsigned int enable_unsupported_sfp =
						      allow_unsupported_sfp[bd];
			ixgbe_validate_option(&enable_unsupported_sfp, &opt);
			if (enable_unsupported_sfp) {
				adapter->hw.allow_unsupported_sfp = true;
			} else {
				adapter->hw.allow_unsupported_sfp = false;
			}
#ifdef module_param_array
		} else if (opt.def == OPTION_ENABLED) {
				adapter->hw.allow_unsupported_sfp = true;
		} else {
				adapter->hw.allow_unsupported_sfp = false;
		}
#endif
	}
	{ /* DMA Coalescing */
		struct ixgbe_option opt = {
			.type = range_option,
			.name = "dmac_watchdog",
			.err  = "defaulting to 0 (disabled)",
			.def  = 0,
			.arg  = { .r = { .min = 41, .max = 10000 } },
		};
		const char *cmsg = "DMA coalescing not supported on this hardware";

		switch (adapter->hw.mac.type) {
		case ixgbe_mac_X550:
		case ixgbe_mac_X550EM_x:
			if (adapter->rx_itr_setting || adapter->tx_itr_setting)
				break;
			opt.err = "interrupt throttling disabled also disables DMA coalescing";
			opt.arg.r.min = 0;
			opt.arg.r.max = 0;
			break;
		default:
			opt.err = cmsg;
			opt.msg = cmsg;
			opt.arg.r.min = 0;
			opt.arg.r.max = 0;
		}
#ifdef module_param_array
		if (num_dmac_watchdog > bd) {
#endif
			unsigned int dmac_wd = dmac_watchdog[bd];

			ixgbe_validate_option(&dmac_wd, &opt);
			adapter->hw.mac.dmac_config.watchdog_timer = dmac_wd;
#ifdef module_param_array
		} else {
			adapter->hw.mac.dmac_config.watchdog_timer = opt.def;
		}
#endif
	}
	{ /* VXLAN rx offload */
		struct ixgbe_option opt = {
			.type = range_option,
			.name = "vxlan_rx",
			.err  = "defaulting to 1 (enabled)",
			.def  = 1,
			.arg  = { .r = { .min = 0, .max = 1 } },
		};
		const char *cmsg = "VXLAN rx offload not supported on this hardware";
		const u32 flag = IXGBE_FLAG_VXLAN_OFFLOAD_ENABLE;

		if (!(adapter->flags & IXGBE_FLAG_VXLAN_OFFLOAD_CAPABLE)) {
			opt.err = cmsg;
			opt.msg = cmsg;
			opt.def = 0;
			opt.arg.r.max = 0;
		}
#ifdef module_param_array
		if (num_vxlan_rx > bd) {
#endif
			unsigned int enable_vxlan_rx = vxlan_rx[bd];

			ixgbe_validate_option(&enable_vxlan_rx, &opt);
			if (enable_vxlan_rx)
				adapter->flags |= flag;
			else
				adapter->flags &= ~flag;
#ifdef module_param_array
		} else if (opt.def) {
			adapter->flags |= flag;
		} else {
			adapter->flags &= ~flag;
		}
#endif
	}

	{ /* MDD support */
		struct ixgbe_option opt = {
			.type = enable_option,
			.name = "Malicious Driver Detection",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED,
		};

		switch (adapter->hw.mac.type) {
		case ixgbe_mac_X550:
		case ixgbe_mac_X550EM_x:
#ifdef module_param_array
			if (num_MDD > bd) {
#endif
				mdd = MDD[bd];
				ixgbe_validate_option(&mdd, &opt);

				if (mdd){
					*aflags |= IXGBE_FLAG_MDD_ENABLED;

				} else{
					*aflags &= ~IXGBE_FLAG_MDD_ENABLED;
				}
#ifdef module_param_array
			} else {
					if (opt.def == OPTION_ENABLED){
						*aflags |= IXGBE_FLAG_MDD_ENABLED;

					} else {
						*aflags &= ~IXGBE_FLAG_MDD_ENABLED;
				}
			}
#endif
			break;
		default:
			*aflags &= ~IXGBE_FLAG_MDD_ENABLED;
			break;
		}
	}

}