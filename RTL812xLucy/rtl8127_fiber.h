/* SPDX-License-Identifier: GPL-2.0-only */
/*
################################################################################
#
# RTL8127 fiber (SFP+) support for the RTL812xLucy macOS driver.
#
# Ported from r8127_fiber.c/h of the r8127 Linux device driver released for
# Realtek 10 Gigabit Ethernet controllers with PCI-Express interface.
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

#ifndef _RTL8127_FIBER_H
#define _RTL8127_FIBER_H

enum {
        FIBER_MODE_NIC_ONLY = 0,
        FIBER_MODE_RTL8127ATF,
        FIBER_MODE_MAX
};

#define HW_FIBER_MODE_ENABLED(_M)        ((_M)->HwFiberModeVer > 0)

struct rtl8125_private;

u16 rtl8127_sds_phy_read_8127(struct rtl8125_private *tp, u16 index, u16 page,
                              u16 reg);
void rtl8127_sds_phy_write_8127(struct rtl8125_private *tp, u16 index, u16 page,
                                u16 reg, u16 val);
void rtl8127_hw_fiber_phy_config(struct rtl8125_private *tp);
bool rtl8127_check_fiber_mode_support(struct rtl8125_private *tp);

#endif /* _RTL8127_FIBER_H */
