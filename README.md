# RTL8720DN patches for Wii U gamepad (DRC) connection

The RTL8720DN is an Arduino-like microcontroller that also has support for dual-band Wi-Fi (2.4GHz and 5GHz) and Bluetooth. 5GHz Wi-Fi support is rare in both microcontrollers and single-board computers, making the RTL8720DN a uniquely attractive as far as embedded devices go for the Wii U gamepad.

That being said, the Wii U gamepad requires slight tweaks to the 802.11 protocol, and the RTL8720DN's lower level MLME commands are done in closed source binary blobs, with the source only available by acquiring a commercial license and signing an NDA with Realtek. This repo provides patches to this closed source firmware to allow connecting to the Wii U.

**NOTE: Using these patches will prevent the RTL8720DN from connecting to non-Wii U access points.**

## Setup (Automatic)

These instructions are for if you simply want to use the patched firmware:

1. Set up Arduino IDE for the RTL8720DN using the [official instructions](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started/).
1. Download `lib_wps.a` and `lib_wlan.a` from the [Releases](/vanilla-wiiu/rtl8720dn/releases) tab.
1. Locate the existing `lib_wps.a` and `lib_wlan.a` files in the Arduino/Realtek data folders. This will differ by platform.
  - On Linux, they are typically located in: `~/.arduino15/packages/realtek/hardware/AmebaD/3.1.7/variants/common_libs`
1. (Optional) Back up the two files in case you want to restore normal Wi-Fi functionality later. It is not necessary to back these up, they can always be re-acquired from [Realtek's official GitHub repository](https://github.com/ambiot/ambd_arduino/tree/dev/Arduino_package/hardware/variants/common_libs).
1. Replace the existing `lib_wps.a` and `lib_wlan.a` with the files you downloaded. 
1. Done! Any programs compiled with Arduino IDE should now include the patched firmware.

## Setup (Manual)

These instructions are for if you want to patch the firmware yourself before installing and using it:

1. Locate the existing `lib_wps.a` and `lib_wlan.a` files in the Arduino/Realtek data folders. This will differ by platform.
  - On Linux, they are typically located in: `~/.arduino15/packages/realtek/hardware/AmebaD/3.1.7/variants/common_libs`
1. (Optional) Back up the two files in case you want to restore normal Wi-Fi functionality later. It is not necessary to back these up, they can always be re-acquired from [Realtek's official GitHub repository](https://github.com/ambiot/ambd_arduino/tree/dev/Arduino_package/hardware/variants/common_libs).
1. Install GCC for ARM 32-bit. This will differ by platform.
  - On Arch Linux: `pacman -S arm-none-eabi-gcc arm-none-eabi-newlib`
1. Clone the Wii U patched wpa_supplicant:
  ```
  git clone https://github.com/rolandoislas/drc-hostap.git
  ```
1. Apply `rtl8720dn_wps_common.patch` by running the following command in the directory you just cloned:
  ```
  git apply rtl8720dn_wps_common.patch
  ```
  This patch adds the necessary byte rotation required for the WPS PIN authentication.
1. From the `wpa_supplicant` repository, compile **only** `src/wps/wps_common.c` using the following command:
  ```
  arm-none-eabi-gcc -march=armv8-m.main+fp -mfloat-abi=hard -c -O2 -DRTL8720DN -I<drc-hostap-directory>/src/utils -I<drc-hostap-directory>/src -D__BYTE_ORDER=__LITTLE_ENDIAN -DCONFIG_TENDONIN <drc-hostap-directory>/src/wps/wps_common.c
  ```
  Replace `<drc-hostap-directory>` with the directory you cloned `wpa_supplicant` to earlier.
1. You should now have a compiled object file `wps_common.o`. Inject it into the existing `lib_wps.a` file using AR:
  ```
  ar r lib_wps.a wps_common.o
  ```
1. Compile `rom_rtw_psk_wiiu.c` into an object file using the following command:
  ```
  arm-none-eabi-gcc -march=armv8-m.main+fp -mfloat-abi=hard -c -O2 -D__BYTE_ORDER=__LITTLE_ENDIAN rom_rtw_psk_wiiu.c
  ```
  This is a decompilation/reimplementation of Realtek's PTK calculation function that adds the byte rotation required for connection to the Wii U.
1. You should now have a compiled object file `rom_rtw_psk_wiiu.o`. Inject it into the existing `lib_wlan.a` file using AR:
  ```
  ar r lib_wlan.a rom_rtw_psk_wiiu.o
  ```
1. Open `lib_wlan.a` with an archiving tool like 7-Zip or Ark. Extract the following files from it:
  ```
  rom_rtw_psk.o
  rom_rtw_ieee80211.o
  ```
1. Using a hex editor, do the following:
  - In `rom_rtw_psk.o`, replace all instances of `rom_psk_CalcPTK` with `rom_psk_CalcNVM` (can be anything as long as it's the same length and not the original string). This will ensure the Arduino compiler will use our PTK calculation function instead of Realtek's.
  - In `rom_rtw_ieee80211.o`, go to offsets `0xB34` and `0xB3C` and replace the three bytes `00 0F AC` at each offset with `A4 C0 E1`. This will spoof our OUI (organizationally unique identifier) to Nintendo's so the console believes we are a gamepad.
1. Inject these modified objects back into `lib_wlan.a` using AR:
  ```
  ar r lib_wlan.a rom_rtw_psk.o
  ar r lib_wlan.a rom_rtw_ieee80211.o
  ```
1. Ensure your patched files have replaced the original files located earlier on.
1. Done! Any programs compiled with Arduino IDE should now include the patched firmware.