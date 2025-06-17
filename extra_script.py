#  Copyright (c) Kuba Szczodrzyński 2023-5-4.

from os.path import realpath

Import("env")

platform = env["PIOPLATFORM"]
port = None
if platform == "espressif32":
    port = "esp32"
elif platform == "windows_x86":
    port = "windows-winusb"


defines = set()
for item in env.get("CPPDEFINES", []):
    if isinstance(item, tuple):
        defines.add(item[0])
    elif isinstance(item, str):
        defines.add(item)


env.Append(
    CPPPATH=[
        realpath("3rd-party/bluedroid/decoder/include"),
        realpath("3rd-party/bluedroid/encoder/include"),
        realpath("3rd-party/hxcmod-player"),
        realpath("3rd-party/hxcmod-player/mods"),
        realpath("3rd-party/lc3-google/include"),
        realpath("3rd-party/md5"),
        realpath("3rd-party/micro-ecc"),
        realpath("3rd-party/yxml"),
        realpath("include"),
        realpath("platform/embedded"),
        realpath("src"),
    ],
)
env.Replace(
    SRC_FILTER=[
        "+<3rd-party/bluedroid/decoder/srce/*.c>",
        "+<3rd-party/bluedroid/encoder/srce/*.c>",
        "+<3rd-party/hxcmod-player/mods/*.c>",
        "+<3rd-party/hxcmod-player/*.c>",
        "+<3rd-party/lc3-google/src/*.c>",
        "+<3rd-party/md5/md5.c>",
        "+<3rd-party/micro-ecc/uECC.c>",
        "+<3rd-party/yxml/yxml.c>",
        "+<src/*.c>",
    ],
)


if "ENABLE_CLASSIC" in defines:
    env.Append(
        SRC_FILTER=[
            "+<src/classic/*.c>",
        ],
    )
if "ENABLE_BLE" in defines:
    env.Append(
        SRC_FILTER=[
            "+<src/ble/*.c>",
            "+<src/ble/gatt-service/*.c>",
        ],
    )
if "ENABLE_MESH" in defines:
    env.Append(
        SRC_FILTER=[
            "+<src/mesh/*.c>",
        ],
    )
if "ENABLE_PRINTF_HEXDUMP" in defines:
    env.Append(
        SRC_FILTER=[
            "+<platform/embedded/hci_dump_embedded_stdout.c>",
        ],
    )


if port == "esp32":
    # from btstack/port/esp32/components/btstack/CMakeLists.txt
    env.Append(
        CPPPATH=[
            realpath("3rd-party/lwip/dhcp-server"),
            realpath("platform/freertos"),
            realpath("platform/lwip"),
        ],
        SRC_FILTER=[
            "+<3rd-party/lwip/dhcp-server>",
            "+<platform/freertos/*.c>",
            "+<platform/lwip>",
        ],
    )
elif port == "windows-winusb":
    # from btstack/port/windows-winusb/CMakeLists.txt
    env.Append(
        CPPPATH=[
            realpath("3rd-party/rijndael"),
            realpath("chipset/zephyr"),
            realpath("platform/posix"),
            realpath("platform/windows"),
        ],
        SRC_FILTER=[
            "+<3rd-party/rijndael/rijndael.c>",
            "+<chipset/zephyr/*.c>",
            "+<platform/posix/btstack_audio_portaudio.c>",
            "+<platform/posix/wav_util.c>",
            "+<platform/windows/*.c>",
        ],
        LIBS=[
            "setupapi",
            "winusb",
        ],
    )
else:
    raise ValueError(f"Incompatible platform '{platform}'")


env.Append(
    CPPPATH=[
        realpath(f"port/{port}"),
    ],
    SRC_FILTER=[
        f"+<port/{port}/*.c>",
    ],
)
