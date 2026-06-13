#!/usr/bin/env python3
"""Generate KernelSPI overlay headers for building against the standard macOS SDK.

The RTL812xLucy driver uses the IONetworkingFamily polled-mode SPI
(outputStart / pollInputPackets / output pull-model), which Apple strips from
the public Kernel.framework headers. The kernel itself is built WITH the SPI:
vtable slots 2-5 of IONetworkController and 5-10 of IONetworkInterface hold the
real methods (verified: the kernel collection exports the SPI symbols and only
exports _RESERVED pads for the slots that are genuinely unused, 8-31 / 11-15).

This script copies IONetworkController.h and IONetworkInterface.h from the
current SDK and re-inserts the SPI declarations at the exact reserved slots,
guarded by #ifdef __PRIVATE_SPI__ exactly like Apple's own source tree
(declarations taken from acidanthera MacKernelSDK, which mirrors Apple's
IONetworkingFamily source). The result matches the kernel ABI: real SPI
methods at slots that the public header pads out, current public methods
(allocatePacketNoWait, setHardwareAssists, ...) untouched.

Usage: python3 gen_spi_headers.py [path-to-MacKernelSDK]
Writes IOKit/network/IONetworkController.h and IONetworkInterface.h next to
this script. Re-run after a major SDK update.
"""

import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
MKSDK = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE.parent / "MacKernelSDK"
SDK = Path(subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip())
NETHDRS = SDK / "System/Library/Frameworks/Kernel.framework/Headers/IOKit/network"
OUT = HERE / "IOKit/network"
OUT.mkdir(parents=True, exist_ok=True)


def extract(path, start_pred, end_pred):
    """Return the lines of `path` from the first line matching start_pred
    through the first subsequent line matching end_pred (inclusive)."""
    lines = path.read_text().splitlines(keepends=True)
    out, active = [], False
    for line in lines:
        if not active and start_pred(line):
            active = True
        if active:
            out.append(line)
            if end_pred(line):
                return "".join(out)
    raise SystemExit(f"block not found in {path}")


def replace_once(text, pattern, repl, label):
    new, n = re.subn(pattern, repl, text, count=1, flags=re.M)
    if n != 1:
        raise SystemExit(f"anchor not found: {label}")
    return new


# --- IONetworkController.h -------------------------------------------------
# Replace pad slots 2-5 with the SPI virtuals (same order as the kernel:
# 2=outputStart 3=setInputPacketPollingEnable 4=pollInputPackets
# 5=networkInterfaceNotification).
CONTROLLER_SPI = """\
#ifdef __PRIVATE_SPI__
public:
    virtual IOReturn outputStart(
                        IONetworkInterface *    interface,
                        IOOptionBits            options );

    OSMetaClassDeclareReservedUsed( IONetworkController,  2);

    virtual IOReturn setInputPacketPollingEnable(
                        IONetworkInterface *    interface,
                        bool                    enabled );

    OSMetaClassDeclareReservedUsed( IONetworkController,  3);

    virtual void     pollInputPackets(
                        IONetworkInterface *    interface,
                        uint32_t                maxCount,
                        IOMbufQueue *           pollQueue,
                        void *                  context );

    OSMetaClassDeclareReservedUsed( IONetworkController,  4);

    virtual IOReturn networkInterfaceNotification(
                        IONetworkInterface *    interface,
                        uint32_t                type,
                        void *                  argument );

    OSMetaClassDeclareReservedUsed( IONetworkController,  5);

#else   /* !__PRIVATE_SPI__ */
    OSMetaClassDeclareReservedUnused( IONetworkController,  2);
    OSMetaClassDeclareReservedUnused( IONetworkController,  3);
    OSMetaClassDeclareReservedUnused( IONetworkController,  4);
    OSMetaClassDeclareReservedUnused( IONetworkController,  5);
#endif  /* !__PRIVATE_SPI__ */
"""

ctrl = (NETHDRS / "IONetworkController.h").read_text()
ctrl = replace_once(
    ctrl,
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkController,  2\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkController,  3\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkController,  4\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkController,  5\);\n",
    CONTROLLER_SPI,
    "IONetworkController pad slots 2-5",
)
(OUT / "IONetworkController.h").write_text(ctrl)

# --- IONetworkInterface.h ----------------------------------------------------
mk_iface = MKSDK / "Headers/IOKit/network/IONetworkInterface.h"

# Block 1: SPI enums + IONetworkPacketPollingParameters (file scope, before the
# class). Self-guarded by #ifdef __PRIVATE_SPI__ in the extracted text.
enums_block = extract(
    mk_iface,
    lambda l: l.startswith("#ifdef __PRIVATE_SPI__"),
    lambda l: l.startswith("#endif /* __PRIVATE_SPI__ */"),
)
assert "IONetworkPacketPollingParameters" in enums_block

# Block 2: OutputPreEnqueueHandler typedef + errnoToIOReturn (inside class,
# right after the IONetworkStack friend declaration).
text = mk_iface.read_text()
m = re.search(
    r"#ifdef __PRIVATE_SPI__\npublic:\n.*?OutputPreEnqueueHandler.*?"
    r"#endif /\* __PRIVATE_SPI__ \*/\n",
    text,
    re.S,
)
if not m:
    raise SystemExit("OutputPreEnqueueHandler block not found")
peq_block = m.group(0)

# Block 3: the big SPI method block replacing interface pad slots 5-10
# (5=configureOutputPullModel 6=configureInputPacketPolling
#  7=reportDataTransferRates 8=dequeueOutputPackets
#  9=dequeueOutputPacketsWithServiceClass 10=enqueueInputPacket, plus the
# non-virtual thread/queue helpers). Ends with its own #else pads, so the
# extracted text stays valid without __PRIVATE_SPI__ too.
lines = text.splitlines(keepends=True)
start = next(
    i for i, l in enumerate(lines)
    if l.startswith("#ifdef __PRIVATE_SPI__") and i > 1000
)
end = next(
    i for i, l in enumerate(lines)
    if i > start and l.startswith("#endif  /* !__PRIVATE_SPI__ */")
)
methods_block = "".join(lines[start:end + 1])
assert "configureOutputPullModel" in methods_block
assert "OSMetaClassDeclareReservedUnused( IONetworkInterface, 10);" in methods_block

iface = (NETHDRS / "IONetworkInterface.h").read_text()

# Insert block 1 before the IONetworkInterface class doc comment.
iface = replace_once(
    iface,
    r"(?=/\*! @class IONetworkInterface\n)",
    enums_block + "\n",
    "class doc comment",
)

# Insert block 2 after the IONetworkStack friend declaration.
iface = replace_once(
    iface,
    r"([ \t]*friend class IONetworkStack;\n)",
    r"\1\n" + peq_block.replace("\\", "\\\\"),
    "friend class IONetworkStack",
)

# Replace pad slots 5-10 with block 3.
iface = replace_once(
    iface,
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkInterface,  5\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkInterface,  6\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkInterface,  7\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkInterface,  8\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkInterface,  9\);\n"
    r"[ \t]*OSMetaClassDeclareReservedUnused\( IONetworkInterface, 10\);\n",
    methods_block.replace("\\", "\\\\"),
    "IONetworkInterface pad slots 5-10",
)
(OUT / "IONetworkInterface.h").write_text(iface)

print(f"wrote {OUT}/IONetworkController.h ({len(ctrl)} bytes)")
print(f"wrote {OUT}/IONetworkInterface.h ({len(iface)} bytes)")
