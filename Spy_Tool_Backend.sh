#!/bin/bash
OUT="/storage/roms/ports/Spy_Data"
LOG="$OUT/spy_master.log"
mkdir -p "$OUT"
rm -f "$OUT"/*.log "$OUT"/*.bin "$OUT"/*.cfg "$OUT"/*.conf "$OUT"/*.txt "$OUT"/*.tar.gz 2>/dev/null

exec > >(tee -a "$LOG") 2>&1

delay=0.3
echo "[SESSION ACTIVE :: LINK-UP ESTABLISHED]"
sleep $delay
echo "> INITIALIZING DECRYPTION PROTOCOL [DP-X7]..."
sleep $delay
echo "> KERNEL, SYSTEM & ENVIRONMENT..."
dmesg > "$OUT/spy_dmesg.log" 2>/dev/null
sleep $delay
cat /proc/cmdline > "$OUT/spy_cmdline.log" 2>/dev/null
sleep $delay
echo "> MOUNT POINTS EXTRACTED."
cat /proc/mounts | sort > "$OUT/spy_mounts.log" 2>/dev/null
sleep $delay
echo "> EXECUTING SHELLCODE (hardware_probe.bin)"
sleep $delay
cat /proc/cpuinfo > "$OUT/spy_hardware.log" 2>/dev/null
sleep $delay
echo "> UPLINK SECURED. Accessing data node 4b/c9."
lsusb > "$OUT/spy_usb.log" 2>&1
sleep $delay
echo "> SCANNING USB SUBSYSTEM... [SUCCESS]"
sleep $delay
echo "> NETWORK & RFKILL ANALYTICS..."
ip a > "$OUT/spy_network.log" 2>&1
sleep $delay
echo "> SYSTEMD & STORAGE CORRUPTION CHECK..."
systemctl status > "$OUT/spy_systemd.log" 2>&1
sleep $delay
echo "> BYPASSING FIREWALL... [SUCCESS]"
cp /storage/.config/retroarch/retroarch.cfg "$OUT/spy_retroarch.cfg" 2>/dev/null
sleep $delay
echo "> DOWNLOADING RETROARCH CONFIGS... [SUCCESS]"
sleep $delay
echo "> INITIATING FULL FILESYSTEM TREE SCAN (Please wait...)"
timeout 15 find / -maxdepth 5 -type d -print > "$OUT/tree_dirs.txt" 2>/dev/null
sleep $delay
echo "> TREE SCAN COMPLETE."
sleep $delay
echo "> PACKING DATA STREAMS..."
cd /storage/roms/ports
tar -czf Spy_Data/Ultimate_Spy_Data.tar.gz Spy_Data/ --exclude="Spy_Data/Ultimate_Spy_Data.tar.gz" 2>/dev/null
sleep 0.5
echo "> ALL DATA COMPRESSED TO Ultimate_Spy_Data.tar.gz"
echo "> SYSTEM BREACH SUCCESSFUL. READY FOR SHUTDOWN."
