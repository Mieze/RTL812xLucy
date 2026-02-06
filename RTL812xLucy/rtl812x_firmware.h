/* SPDX-License-Identifier: GPL-2.0-only */
/*
################################################################################
#
# r8125 is the Linux device driver released for Realtek 2.5 Gigabit Ethernet
# controllers with PCI-Express interface.
#
# Copyright(c) 2025 Realtek Semiconductor Corp. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>.
#
# Author:
# Realtek NIC software team <nicfae@realtek.com>
# No. 2, Innovation Road II, Hsinchu Science Park, Hsinchu 300, Taiwan
#
################################################################################
*/

/************************************************************************************
 *  This product is covered by one or more of the following patents:
 *  US6,570,884, US6,115,776, and US6,327,625.
 ***********************************************************************************/

#ifndef _LINUX_rtl8125_FIRMWARE_H
#define _LINUX_rtl8125_FIRMWARE_H

static inline
ssize_t strscpy(char *dest, const char *src, size_t count)
{
        long res = 0;

        if (count == 0)
                return -E2BIG;

        while (count) {
                char c;

                c = src[res];
                dest[res] = c;
                if (!c)
                        return res;
                res++;
                count--;
        }

        /* Hit buffer length without finding a NUL; force NUL-termination. */
        if (res)
                dest[res-1] = '\0';

        return -E2BIG;
}

struct firmware {
    size_t size;
    const u8 *data;

    /* firmware loader private fields */
    void *priv;
};

struct rtl8125_private;
typedef void (*rtl8125_fw_write_t)(struct rtl8125_private *tp, u16 reg, u16 val);
typedef u32 (*rtl8125_fw_read_t)(struct rtl8125_private *tp, u16 reg);

#define RTL8125_VER_SIZE		32

struct rtl8125_fw {
        rtl8125_fw_write_t phy_write;
        rtl8125_fw_read_t phy_read;
        rtl8125_fw_write_t mac_mcu_write;
        rtl8125_fw_read_t mac_mcu_read;
        const struct firmware *fw;
        const char *fw_name;
        struct device *dev;

        char version[RTL8125_VER_SIZE];

        struct rtl8125_fw_phy_action {
                __le32 *code;
                size_t size;
        } phy_action;
};

struct rtl812x_firmware {
    struct firmware fw;
    struct rtl8125_fw rtl_fw;
    u8 raw_data[0];
};

#ifdef __cplusplus
extern "C" {
    bool rtl8125_fw_format_ok(struct rtl8125_fw *rtl_fw);
    bool rtl8125_fw_data_ok(struct rtl8125_fw *rtl_fw);
    void rtl8125_fw_write_firmware(struct rtl8125_private *tp, struct rtl8125_fw *rtl_fw);
}
#else
bool rtl8125_fw_format_ok(struct rtl8125_fw *rtl_fw);
bool rtl8125_fw_data_ok(struct rtl8125_fw *rtl_fw);
void rtl8125_fw_write_firmware(struct rtl8125_private *tp, struct rtl8125_fw *rtl_fw);
#endif // __cplusplus

#endif /* _LINUX_rtl8125_FIRMWARE_H */
