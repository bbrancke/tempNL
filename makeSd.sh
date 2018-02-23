#!/bin/sh
# 3/15/2017 - derived from VM "TM_ShadowXPlatform"'s yocto4 folder.
# Setup SD card.
# Derived from ShadowX VM, ~/shadow/embedded/makeShadowXSdCard
#   There are many parts here still TODO...

# Makes ShadowX card for the latest kernel build which
# as of Jan 2017 handles both the Duovero Zephyr and Zephyr-Y
# (Zephyr Y has the new Wifi chipset).
# This gets kernel + modules + MLO etc from:
# "/home/osboxes/yocto/build/tmp/deploy/images/duovero/"
# (now yocto2 folder)
# NOTE: 6/20/2017: any references to yocto2 folder were for the old (now deceased) VM
# I think I have changed all of them but do NOT use yocto*2* it does not exist!
# *** yocto2 is NOW the 'yocto' folder (no '2')
#
# This does the work of ./copy_boot.sh, ./copy_rootfs.sh
# and also creates the partition table. (It is not really
# any faster to skip creating the partition table).
# Then copies config files, executables, and
# library files to make a brand-new custom ShadowX SD card.

# Use:
# BEFORE INSERTING the SD card into your computer:
# 1. ls /dev/sd*   -- I see "/dev/sda1" etc but they are part of sda
# 2. Insert SD card,
# 3. VM: Devices->USB Devices-> Select the USB / mass storage device
# 4. ls /dev/sd*  -- new device, mine now shows as "sdb" or sdc
# This will now auto un-mount anything that is mounted
# and it picks the device automatically. It will NOT
# ever write to /sda/ anything.
# You do not need to type the 'sdb' part (in #7) anymore.
# 5. Make sure Ubuntu does not auto-mount the new SD card device:
#    a. Wait a few seconds
#    b. sudo mount | grep sdb   -- or the new device name
#    c. If listed:
#       sudo umount /dev/sdb  -- do umount for ALL sdb (your new device name)  devices
# 6. cd [to this directory]
# 7. make_sd_card_shx.sh sdb    --- Include the new device name (omit the /dev/ part)
# 8. When complete, watch the SD card's USB adaptor and wait for the LED to stop blinking.
# 9. VM: Devices->USB Devices-> UNcheck the mass storage drive
#10  VM: Windows: Eject the Mass Storage Device.

# Make three partitions for the ShadowX SD card:
# TODO. For now, assume it is already partitioned...


function ver() {
        printf "%03d%03d%03d" $(echo "$1" | tr '.' ' ')
}

# Get Device name of SD card so I don't have to keep typing 'sdb' at the command
# To call, don't need to declare / define 'drive', just call:
#		getSdName drive
#		echo "Card: $drive"
# getSdName will exit if SD card not found (or if > 1)
# getSdName will NOT ALLOW /dev/sda to be used (the primary disk drive on this VM)
function getSdName() {

declare -n selected_drive=$1

ary=(/dev/sd*)
declare -a ary2
patt='([[:digit:]]+)'
for x in "${ary[@]}";
do
#	echo "2: $x"
	# Check and eliminate drives ending with a digit (e.g., /dev/sdb1)
	# "[[ $x =~ $patt ]]" crashes w/no explanation, don't know why...
	set +e
	[[ $x =~ $patt ]]
	test="${BASH_REMATCH[1]}"
	if [ ! "$test" ]; then
		# 'test' is empty; 'x' contains no digits, e.g. /dev/sda or /dev/sdb
		if [ "$x" != "/dev/sda" ]; then
			ary2+=($x)
		fi
	fi
	set -e
done

count=${#ary2[@]}
echo "count is $count"
if [ $count -lt 1 ]; then
	echo "No SD card device detected."
	exit 1
fi

if [ $count -lt 2 ]; then
	# A single device detected.
	selected_drive=${ary2[0]}
	echo "===================="
	echo "Confirm: Set up:"
	echo "       $selected_drive"
	echo "as ShadowX SD card?"
	echo "===================="
	read -p "Your choice (y/n)? " -n 1 -r
	echo
	if [[ ! $REPLY =~ ^[Yy]$ ]]
	then
		exit 1
	fi
	return
fi
# else... > 1 SD card detected. TODO: show select menu here...
echo "Detected $count disk devices:"
for y in "${ary2[@]}"
do
	echo $y
done

echo "FOR NOW, disconnect the other SD cards so I detect a single SD card to set up."
exit 1

}
# -------------- End of getSdName() -----------------
#
# ------------------- mkparts() ------------------
# Make three partitions for the ShadowX SD card.
# Caller has already guaranteed that it is not mounted.
mkparts () {
MYDRIVE=$1
echo "Creating partition table for ${MYDRIVE}."

# Here is sample fdisk output, listing the geometry of a disk, as well as its partitions.
# $ fdisk -l /dev/sda
# Disk /dev/sda: 19.3 GB, 19327352832 bytes
# 255 heads, 63 sectors/track, 2349 cylinders
# Units = cylinders of 16065 * 512 = 8225280 bytes
#
#    Device Boot      Start         End      Blocks   Id  System
# /dev/sda1   *           1          65      522081   83  Linux
# /dev/sda2              66        2349    18346230   8e  Linux LVM
#
# cut-use ' ' for delimiter, return field 5:
# Field 1       2       3  4       5
#     [Disk /dev/sda: 19.3 GB, 19327352832 bytes]
# Field # 5 is 19327352832. (Remember space bet. 19.2 and GB)
# Here is SD card result on the VM:
# [Disk /dev/sdb: 31.9 GB, 31914983424 bytes], and f5 is indeed 31914983424.
# (I originally did not see/count the space between "31.9" and "GB"
# NB: DRIVE MUST *NOT* have ending '/' - /dev/sdb=OK, /dev/sdb/=BAD!

SIZE=`sudo fdisk -l $MYDRIVE | grep "Disk $MYDRIVE" | cut -d' ' -f5`
# For 64GB SD card: SIZE is 63864569856 = 63,864,569,856

echo SD Card size is $SIZE bytes
if [ "$SIZE" -lt 1800000000 ]; then
	echo "Require an SD card of at least 2GB"
	exit 1
fi

# Sectors are 512 bytes, bc is BasicCalculator...
SECTORS=`echo $SIZE/512 | bc`
echo "Sectors = $SECTORS"


# new versions of sfdisk don't use rotating disk params
# THIS version (1/2017): sfdisk from util-linux 2.20.1
#    -uS  means units are sectors which new version assumes
sfdisk_ver=`sfdisk --version | awk '{ print $4 }'`

if [ $(ver $sfdisk_ver) -lt $(ver 2.26.2) ]; then
        CYLINDERS=`echo $SIZE/255/63/512 | bc`
        echo "Cylinders = $CYLINDERS"
        # CYLINDERS on 64GB SDcard: 7764
        SFDISK_CMD="sudo sfdisk --force -D -uS -H255 -S63 -C ${CYLINDERS}"
else
        SFDISK_CMD="sudo sfdisk"
fi

# Minimum required 2 partitions (we set up partition 3 as linux-swap partition)
# Sectors are 512 bytes
# 0-127: 64KB, no partition, MBR
# p1 : ~64 MB, FAT partition, bootloader, uEnv.txt 
# p2 : 2GB+, linux partition, root filesystem
# p3 : at the end, 512 MB swap partition
 
# man page:
# "sfdisk reads lines of the form
#    <start> <size> <id> <bootable> <c,h,s> <c,h,s>
# where each line fills one partition descriptor.
#
# Fields are separated by whitespace, or comma or semicolon possibly followed
# by whitespace; initial and trailing whitespace is ignored.
# Numbers can be octal, decimal or hexadecimal, decimal is default.
# When a field is absent or empty, a default value is used.
# The <c,h,s> parts can (and probably should) be omitted -
# sfdisk computes them from <start> and <size> and the disk geometry as given
# by the kernel or specified using the -H, -S, -C flags.
#
# Bootable is specified as [*|-], with as default not-bootable.
# (The value of this field is irrelevant for Linux - when Linux runs it has
# been booted already - but might play a role for certain boot loaders
# and for other operating systems.
# For example, when there are several primary DOS partitions, DOS assigns C:
# to the first among these that is bootable.)
#
# Id is given in hex, without the 0x prefix, or is [E|S|L|X],
# where L (LINUX_NATIVE (83)) is the default, S is LINUX_SWAP (82),
# E is EXTENDED_PARTITION (5), and X is LINUX_EXTENDED (85).
#
# The default value of start is the first nonassigned sector/cylinder/...
# The default value of size is as much as possible (until next partition
# or end-of-disk)."
#
#    <start> <size> <id> <bootable> <c,h,s> <c,h,s> (omit <c,h,s>'s)
# So Partition 1 starts at 128 through 131072
# (130,944 + 1) * 512 bytes/sector == 67,043,840 ("~64MB)
#     128,131072,0x0C,*. ID=0x0C, '*' means bootable:
#
# Partition 1:
# We want about 100MiB (104,857,600) = span of 204,800,
# so end would be 204800 + 128 - 1 = 204927.
# Line 1: 128, 204927,0x0C,*
#
# Partition 2: root  (id 0x83=Linux native)
#  original:  131200 is 0x20080
#      ours:  204928 is 0x32080. OK I think.
# Partition 3: Linux swap partition 512MB, type 0x82
#
# Get total # sectors from $SIZE ('bc' is Basic Calculator)
# For 32GB test card:
# "DISK SIZE – 31914983424 bytes
#  Sectors = 62333952  -- OK, this * 512 -- SIZE
#  CYLINDERS – 3880"
# One sector == 512 bytes.
#      subtract 512MB from sectors-1, that gives Part3's start
#      Part 2's end is then p3start-1...
# Remember that sectors 0-127 are 64KB MBR (no partition)
P1STARTSECTOR=128
P1ENDSECTOR=204927

P2STARTSECTOR=`echo $P1ENDSECTOR+1 | bc`

P3ENDSECTOR=`echo $SECTORS-1 | bc`
#echo "P3endSector = $P3ENDSECTOR"

# 1 MiB = 1048576 (1,048,576); *512 = 512 MB
P3STARTSECTOR=`echo $SECTORS-1048576 | bc`
#echo "P3StartSector = $P3STARTSECTOR"
P2ENDSECTOR=`echo $P3STARTSECTOR-1 | bc`
#echo "P2EndSector = $P2ENDSECTOR"
echo

# We will use P[n]SIZE (in sectors) for the sdisk's second parameter (start, SIZE, flags)

# bc does not seem to like parentheses...
# (end - start) + 1:
TMPSTART=$P1STARTSECTOR
TMPEND=$P1ENDSECTOR
TMP1=`echo $TMPEND-$TMPSTART | bc`
TMP2=`echo $TMP1+1 | bc`
P1SIZE=$TMP2
TMPSZA=`echo "$TMP2*512" | bc`
# 1GB = 1073741824
if [ "$TMPSZA" -gt 1073741800 ]; then
	TMPSZB=`echo "scale=3; $TMPSZA/1073741824" | bc`
	TMPSZ="$TMPSZB GiB"
else
	TMPSZB=`echo "scale=3; $TMPSZA/1048576" | bc`
	TMPSZ="$TMPSZB MiB"
fi
TMPSZP1=$TMPSZ

TMPSTART=$P2STARTSECTOR
TMPEND=$P2ENDSECTOR
TMP1=`echo $TMPEND-$TMPSTART | bc`
TMP2=`echo $TMP1+1 | bc`
P2SIZE=$TMP2
TMPSZA=`echo "$TMP2*512" | bc`
if [ "$TMPSZA" -gt 1073741800 ]; then
	TMPSZB=`echo "scale=3; $TMPSZA/1073741824" | bc`
	TMPSZ="$TMPSZB GiB"
else
	TMPSZB=`echo "scale=3; $TMPSZA/1048576" | bc`
	TMPSZ="$TMPSZB MiB"
fi
TMPSZP2=$TMPSZ

TMPSTART=$P3STARTSECTOR
TMPEND=$P3ENDSECTOR
TMP1=`echo $TMPEND-$TMPSTART | bc`
TMP2=`echo $TMP1+1 | bc`
P3SIZE=$TMP2
TMPSZA=`echo "$TMP2*512" | bc`
if [ "$TMPSZA" -gt 1073741800 ]; then
	TMPSZB=`echo "scale=3; $TMPSZA/1073741824" | bc`
	TMPSZ="$TMPSZB GiB"
else
	TMPSZB=`echo "scale=3; $TMPSZA/1048576" | bc`
	TMPSZ="$TMPSZB MiB"
fi
TMPSZP3=$TMPSZ

echo "Partition Sectors (MiB = 1024*1014, GiB = 1024MiB):"
printf "%-3s %-14s %-14s %-15s\n" "#" "Start" "End" "Size"
printf "%-3s %-14s %-14s %-15s\n" "1" "$P1STARTSECTOR" "$P1ENDSECTOR" "$TMPSZP1"
printf "%-3s %-14s %-14s %-15s\n" "2" "$P2STARTSECTOR" "$P2ENDSECTOR" "$TMPSZP2"
printf "%-3s %-14s %-14s %-15s\n" "3" "$P3STARTSECTOR" "$P3ENDSECTOR" "$TMPSZP3"

echo -e "\nOkay, here we go ...\n"
echo -e "=== Zeroing the MBR ===\n"
sudo dd if=/dev/zero of=$MYDRIVE bs=1024 count=1024

# OLD:
# Standard 2 partitions
# Sectors are 512 bytes
# 64 MB = 67108864 bytes = 131072 sectors
# 2 GB = 2147483648 bytes = 4194304 sectors
# MBR goes in first sector
# Next 127 sectors are empty to align first partition on a 128 sector boundary
# First partition starts at sector 128 and goes for 130944 sectors, FAT32
# Second partition starts at sector 131072 and goes to end of card, Linux
echo -e "\n=== Creating 3 partitions ===\n"
# ORIGINAL two partitions:
#{
#echo 128,130944,0x0C,*
#echo 131072,,,-
#} | sudo sfdisk --force -D -uS -H 255 -S 63 -C $CYLINDERS $MYDRIVE

# BRAD Param 2 is SIZE NOT I REPEAT NOT ENDSECTOR!!
# <start> <size> <id> <bootable> [omit <c,h,s>]
# 0x82: Linux swap, 0x83: Linux native.
echo "P1 START $P1STARTSECTOR  END  $P1ENDSECTOR  SIZE $P1SIZE"
echo "P2 START $P2STARTSECTOR  END  $P2ENDSECTOR  SIZE $P2SIZE"
echo "P3 START $P3STARTSECTOR  END  $P3ENDSECTOR  SIZE $P3SIZE"

$SFDISK_CMD $MYDRIVE << EOF
$P1STARTSECTOR,$P1SIZE,0x0C,*
$P2STARTSECTOR,$P2SIZE,0x83,-
$P3STARTSECTOR,,0x82;
EOF

sleep 1

echo -e "\nPartitioning complete!\n"
}
# ------------------- end of mkparts() ------------------

# set -e tells to exit this script if an error occurs
# Sometimes, the VM 'loses' the SD card (and Windows recaptures it)
# right in the middle of formatting the second partion and copying
# everything over. Usually need to reboot Ubuntu when this occurs.
set -e
# If a command might fail and we don't care use "set +e" to turn off exit-on-error
# but turn it back on right after... Use set +e sparingly.
# Alternate to set -e: "if ! command; then { echo "Command failed."; exit 1; } fi"

# Allow empty arrays (used in getSdName):
shopt -s nullglob

# For yocto 4.5 linux, use /home/osboxes/yocto45 and "4.5-r0" for these:
#YOCTO="/home/osboxes/yocto"
YOCTO="/home/office/yocto"
VERSION="3.18.21-r0"
VERSION_BASE="3.18.21"

# Check that these really exist. Once I got a dire warning from Bitbake
# "Wipe tmp now or you WILL have catastrophic problems perhaps even
#   several months from now!" OK but then rebuilding worked but these
#   weren't there yet...
FILE=${YOCTO}/build/tmp/work/cortexa8hf-vfp-neon-poky-linux-gnueabi/blink/1.0-r1/image/usr/bin/blink
if [ ! -f ${FILE} ]; then
	echo "File 'blink' not found, run: 'bitbake blink'"
	echo "'${FILE}'"
#	exit 1
fi

FILE="vmlinux-"${VERSION_BASE}"-custom"
FILE2="${YOCTO}/build/tmp/work/duovero-poky-linux-gnueabi/linux-gumstix/${VERSION}/image/boot/${FILE}"
if [ ! -f ${FILE2} ]; then
	echo "File '${FILE}' not found, run: 'bitbake virtual/kernel'"
#	exit 1
fi


echo "***************************************"
echo "******* Create ShadowX SD card. *******"
echo "***************************************"
echo "This is for creating ShadowX SD cards for Duovero Zephyr and Zephyr-Y boards."

read -p "Proceed (y or n)? " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
	echo "Goodbye"
	exit 0
fi

getSdName DRIVE

# DRIVE is now set, usu. to /dev/sdb (or exit if no SD card or user cancels).

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
# NOTE: SCRIPTDIR does NOT have ending '/'.

# Sanity checking:
#if [ "x${1}" = "x" ]; then
#	echo -e "\nUsage: ${0} <block device name, like 'sdb'>\n"
#	exit 0
#fi

# for BB reference, use (string compare): if [ "${YN}" = "y" ]; then
start_time="$(date -u +%s)"

#  UBOOT_ROOTFS_FOLDER="/home/srtg/yocto2_Jan19_2017/build/tmp/deploy/images/duovero/"
#UBOOT_ROOTFS_FOLDER="/home/srtg/yocto4/build/tmp/deploy/images/duovero/"
UBOOT_ROOTFS_FOLDER="${YOCTO}/build/tmp/deploy/images/duovero/"
if [ ! -d ${UBOOT_ROOTFS_FOLDER} ]; then
	echo "u-boot and rootfs source folder not found: ${UBOOT_ROOTFS_FOLDER}"
	exit 1
fi

# DRIVE="/dev/${1}" - original way - you had to enter 'makeSd.sh sdb'
# Now drive is chosen for you in getSdName()...
if [ ! -b ${DRIVE} ]; then
	echo "Cannot find drive / SD card: ${DRIVE}"
	exit 1
fi

# DRIVE="/dev/sdb" e/g... (No ending '/')
# DO NOT format or orverwrite /dev/sda!
# After a scary scare last week I now prevent myself from
# EVER doing anything with /dev/sda. getSdName prevents /dev/sda
# from ever being chosen, so we no longer have that possibility.

# getSdName prevents us from using /dev/sda
# Double check here from original days is OK too.
if [ "$DRIVE" = "/dev/sda" ] ; then
	echo "Danger! Will not format $DRIVE - it is primary drive!"
	exit 1
fi

#make sure that the SD card isn't mounted before we start
echo "Checking mount statuses..."
set +e
if [ -b ${DRIVE}1 ]; then
	# /dev/sdb1 exists and is a block-special file
	# does not mean it is mounted, which is why "set +e" is up there
	sudo umount ${DRIVE}1
fi
if [ -b ${DRIVE}2 ]; then
	sudo umount ${DRIVE}2
fi
if [ -b ${DRIVE}3 ]; then
	sudo umount ${DRIVE}3
fi
if [ -b ${DRIVE} ]; then
	sudo umount ${DRIVE}
fi
set -e
# We WANT THREE PARITIONS sted two.
read -p "Make the SD card partitions (y or n)? " -n 1 -r
echo
# If we made new partitions then always format them, else ask "Format?"
MKPARTITIONS="N"
if [[ $REPLY =~ ^[Yy]$ ]]
then
	mkparts ${DRIVE}
	MKPARTITIONS="Y"
fi

# DRIVE is (e.g.) /dev/sdb
DEV=${DRIVE}1
if [ ! -b $DEV ]; then
	echo "Block device '${DEV}' not found!"
	exit 1
fi

FORMAT_PARTS="N"
# If we just made new partitions, force re-format them...
if [ "${MKPARTITIONS}" = "Y" ]; then
	FORMAT_PARTS="Y"
else
	read -p "Format Partitions (y or n)? " -n 1 -r
	echo
	if [[ $REPLY =~ ^[Yy]$ ]]; then
		FORMAT_PARTS="Y"
	fi
fi

if [[ ${FORMAT_PARTS} = "Y" ]]; then
	echo "Formatting..."
	echo "Formatting FAT partition 1/3 (boot) on $DEV"
	sudo mkfs.vfat -F 32 ${DEV} -n BOOT

	echo "Format 1/3 complete, wait 10..."
	sleep 10

	# DRIVE is (e.g.) /dev/sdb
	DP2=${DRIVE}2
	echo "Formatting Partition 2/3 (rootfs) on $DP2..."
	sudo mkfs.ext4 -L ROOT ${DP2}

	echo "Format 2/3 complete, wait 10..."
	sleep 10

	DP3=${DRIVE}3
	echo "Formatting partition 3/3: linux swap partition on $DP3..."
	sudo mkswap ${DP3}

	echo "Format 3/3 complete, wait 10..."
	sleep 10
fi

DTOT=${DRIVE}
sudo parted ${DTOT} print

echo "Mounting $DEV"
# DEV is "/dev/sdb1" (boot partition)
sudo mount ${DEV} /media/card
# UBOOT_ROOTFS_FOLDER="/home/srtg/yocto2_Jan19_2017/build/tmp/deploy/images/duovero/" OLD
# UBOOT_ROOTFS_FOLDER="/home/osboxes/yocto2/build/tmp/deploy/images/duovero/" NEW
# boot partition has 4 files:
# ...sudo ls -l /media/card
# -rwxr-xr-x 1 root root    37312 Jan 20 16:07 MLO
# -rwxr-xr-x 1 root root 53580590 Jan 20 16:08 modules-duovero.tgz
# -rwxr-xr-x 1 root root   225880 Jan 20 16:08 u-boot.img
# -rwxr-xr-x 1 root root  4799232 Jan 20 16:10 zImage
#
# 2018: 4.9 FRANK. boot partition has ONLY MLO (43230) and
# u-boot.img  (281760 [Work Dell]  or 281776 [office PC])
# These are located in a different location as well:
# /home/office/yocto/build/tmp/work/duovero-poky-linux-gnueabi/u-boot/1_2017.01-r0/build
UBOOT_FOLDER="/home/office/yocto/build/tmp/work/duovero-poky-linux-gnueabi/u-boot/1_2017.01-r0/build/"
# MLO must be FIRST:
echo "Copying MLO"
# All file names are symlinks to latest built version...
sudo cp ${UBOOT_FOLDER}MLO /media/card/MLO
echo "Copying u-boot"
sudo cp ${UBOOT_FOLDER}u-boot.img /media/card/u-boot.img
# 2018: There IS a zImage (4.9.67) (in deploy/.../ folder) but we don't want it:
#   echo "Copying zImage"
#   sudo cp ${UBOOT_ROOTFS_FOLDER}zImage /media/card/zImage
# 2018 - Don't want modules-duovero.tgz either (but it does exist in deploy/.../ folder)
#   echo "Copying modules-duovero.tgz..."
#   sudo cp ${UBOOT_ROOTFS_FOLDER}modules-duovero.tgz /media/card/modules-duovero.tgz
echo "syncing..."
sync
# Frequently umount says "device busy", so wait until success:
echo "Unmounting ${DEV}"
RET=1
until [ ${RET} -eq 0 ]; do
    sudo umount ${DEV}
    RET=$?
    if [ ${RET} -ne 0 ]; then
        echo "unmount failed, will retry in 10..."
        sleep 10
    fi
done

# WARNING, have to wait awhile before formatting the
# next partition or the VM crashes horribly
# 30 seconds may be too long but it takes forever to restart
echo "Unmount complete; wait 20..."
sleep 20

echo "Partition 1 complete."
DEV=${DRIVE}2
if [ ! -b $DEV ]; then
	echo "Block device '${DEV}' not found!"
	exit 1
fi


# Partition 2 is already formatted.
echo "Mounting $DEV"
sudo mount $DEV /media/card

#  echo "Extracting: gumstix-console-image-duovero.tar.bz2 file system to /media/card/..."
#  sudo tar jxvf ${UBOOT_ROOTFS_FOLDER}gumstix-console-image-duovero.tar.bz2 -C /media/card

# 2018: File is .tar.xz sted bz2:
# ~/yocto/build/tmp/deploy/images/duovero$ ls -l
#   ...
#   -rw-r--r-- 2 office office 87712692 Jan 29 23:36 console-image-duovero-20180130011223.rootfs.tar.xz
# Symlink:
#   console-image-duovero.tar.xz -> console-image-duovero-20180130011223.rootfs.tar.xz
# Use tar flag "-xJf" for .tar.xz:
# We want bitbake shadowx-image (NOT console-image)
# File is:
# shadowx-image-duovero.tar.xz
echo "Extracting: shadowx-image-duovero.tar.xz file system to /media/card/..."
sudo tar xJvf ${UBOOT_ROOTFS_FOLDER}shadowx-image-duovero.tar.xz -C /media/card


echo "File system unpacked, syncing..."
sync

#copy_protobuf_libs -- already included along with shadowx, blink, etc.

# vmlinux can be opened in gdb (must untar it first)
# and you can find where something (bad) happened.
# 1. untar the file '/boot/xxx'
# 2. gdb vmlinux-3.18.21-custom
# 3. (gdb) list *( bq24190_probe+0x2e4)
#    [3. returns module (.c file name) and line number].
#
#  VERSION_BASE="3.18.21"
#  3.18.21 filename is "vmlinux-3.18.21-custom"
# (file is over 106,000,000 bytes, compressed is ~56,000,000)...
# 2018: SKIP FOR NOW...
#Un-tarring this gave have a huge folder depth, fix that on restoring:
#  VM_FILENAME="vmlinux-"${VERSION_BASE}"-custom"
#  THESRC="${YOCTO}/build/tmp/work/duovero-poky-linux-gnueabi/linux-gumstix/${VERSION}/image/boot/${VM_FILENAME}"
#  THEDEST="${YOCTO}/shadowx_files/${VM_FILENAME}"
#  cp ${THESRC} ${THEDEST}
#  tar -cvzf ${THEDEST}.tgz ${THEDEST}

#  sudo cp ${THEDEST}.tgz /media/card/boot

TARGET_HOSTNAME=duovero

echo "HOSTNAME: $TARGET_HOSTNAME"

echo "Writing hostname to /etc/hostname"
export TARGET_HOSTNAME
sudo -E bash -c 'echo ${TARGET_HOSTNAME} > /media/card/etc/hostname'

#TODO - copy shx files and setup (/etc/ files)...

SHX_FILES="${YOCTO}/shadowx_files"

# showbatt shows the ShadoxX battery value reading GPADC6 voltage:
sudo cp ${SHX_FILES}/showbatt.sh /media/card/home/root/showbatt.sh
# iw_channel_change show nl80211 channel change speed:
sudo cp ${SHX_FILES}/iw_channel_change.sh /media/card/home/root/iw_channel_change.sh

# Some files of stuff that is built on the ShadowX device and misc stuff I want to keep:
#SRC_F=/home/srtg/yocto4/shadowx_files


# RALINK didn't work right in the OTHER VM's DEV branch
# Shouldn't need to do this here
# WAS commented out but we DO need these files.
#    ifconfig wlan0 up:
#    ieee80211 phy0: rt2x00lib_request_firmware: Info - Loading firmware file 'rt2870.bin'
#    rt2800usb 3-1:1.0: Direct firmware load for rt2870.bin failed with error -2
#    ieee80211 phy0: rt2x00lib_request_firmware: Error - Failed to request Firmware
# AND rt2870.bin is *NOT* in /lib/firmware/ folder...
# Perhaps a kernel setting missing?

# this fstab has an entry for partition #3 (as "swap")
# and allows Linux to use the swap partition
sudo cp ${SHX_FILES}/etc/fstab /media/card/etc/

# 2018: SKIP THIS STUFF FOR NOW:
#  SRC_F=${YOCTO}/shadowx_files

#  sudo cp ${SRC_F}/etc/hostapd.conf /media/card/etc/hostapd.conf
# This is now called /etc/wpa_supplicant/wpa_supplicant-nl80211-wlan1.conf, copied below....
# sudo cp ${SRC_F}/etc/wpa_startup.conf /media/card/etc/wpa_startup.conf

echo "Copying ralink firmware..."

#sudo cp ${SRC_F}/LICENCE.Marvell /media/card/lib/firmware/LICENCE.Marvell
#sudo cp ${SRC_F}/LICENCE.ralink-firmware.txt /media/card/lib/firmware/LICENCE.ralink-firmware.txt
#sudo cp ${SRC_F}/rt2561.bin /media/card/lib/firmware/rt2561.bin
#sudo cp ${SRC_F}/rt2561s.bin /media/card/lib/firmware/rt2561s.bin

#sudo cp ${SRC_F}/rt2661.bin /media/card/lib/firmware/rt2661.bin
#sudo cp ${SRC_F}/rt2860.bin /media/card/lib/firmware/rt2860.bin

#sudo cp ${SRC_F}/rt2870.bin /media/card/lib/firmware/rt2870.bin
#sudo cp ${SRC_F}/rt3071.bin /media/card/lib/firmware/rt3071.bin

#sudo cp ${SRC_F}/rt3290.bin /media/card/lib/firmware/rt3290.bin
#sudo cp ${SRC_F}/rt73.bin /media/card/lib/firmware/rt73.bin

# These are symlinks to the other in the orig shx device.
#sudo cp ${SRC_F}/rt2870.bin /media/card/lib/firmware/rt3070.bin
#sudo cp ${SRC_F}/rt2860.bin /media/card/lib/firmware/rt3090.bin

# OK to not copy this, I am rebuilding the image now and want to
# keep this - it justs inet_pton for converting IP+netmask --> IP/24 (e.g.)
#sudo cp ${SRC_F}/test.c /media/card/home/root/test.c



# The factory "stock" gpsd gets input from ttyO0, but we will never
# plug in USB (which re-directs magically to ttyO0). This
# changes the input (in the ExecRun= line) to /dev/rfcomm0
# by setting GPS_DEVICES:
# rfcomm0 is auto created when bluetooth starts (or something)
#sudo cp ${SRC_F}/etc/default/gpsd.default /media/card/etc/default/

# Bluetooth firmware for TI Wilink8 wl18xx chipset:
# We HAVE TIInit_11.8.32.bts, however the driver looks
# for TIInit_11.8.*108*.bts:
#    Starting Attach WiLink8 Bluetooth Adapter hardwired to ttyO1...
#    Found a Texas Instruments' chip!
#    hciattach[1967]: Firmware file : /lib/firmware/ti-connectivity/TIInit_11.8.108.bts
#    hciattach[1967]: can't open firmware file: No such file or directory
#    hciattach[1967]: Warning: cannot find BTS file: /lib/firmware/ti-connectivity/TIInit_11.8.108.bts
# The 11.8.108 version is nowhere to be found in TI repo or linux-firmware repo or even Google
# so let us try the (older rev?) version that we DO have:
#sudo su -c "cd /media/card/lib/firmware/ti-connectivity/; ln -s TIInit_11.8.32.bts TIInit_11.8.108.bts;"

# This brings up Bluetooth interface hci0 at startup:
# This ALWAYS times out, let us wait a few minutes to manually bring up hci0 (FOR NOW).
#sudo cp ${SRC_F}/etc/udev/rules.d/10-local.rules /media/card/etc/udev/rules.d/

echo "(Skipping dhcpd.conf...)"
#sudo cp ${SRC_F}/dhcpd.conf /media/card/etc/dhcp

ETC_SYSD_NET=${SRC_F}/etc/systemd/network
#sudo cp ${ETC_SYSD_NET}/eth.network /media/card/etc/systemd/network
# This file is gumstix's way to force re-name mlan0 on the older boards
# to wlan0, we want it to be mlan0 for both the old and NEW 'Y' boards
#sudo rm /media/card/etc/systemd/network/85-mlan.link
# Start wifi0 as a client, ap0 as an AP and IP address. ShadowX will handle.
#sudo rm /media/card/etc/systemd/network/ap.network
#sudo rm /media/card/etc/systemd/network/eth.network
#sudo rm /media/card/etc/systemd/network/wifi.network

# We replace that with these files. For older boards:
#sudo cp ${ETC_SYSD_NET}/50-marvellwifi.link /media/card/etc/systemd/network
# For new 'Y' board:
#sudo cp ${ETC_SYSD_NET}/50-tiwifi.link /media/card/etc/systemd/network
# This ensures the RALINK USB wifi card always gets named wlan0:
#sudo cp ${ETC_SYSD_NET}/50-ralink.link /media/card/etc/systemd/network

# Assign eth0's network IP address and netmask:
#sudo cp ${ETC_SYSD_NET}/50-eth.network /media/card/etc/systemd/network

# Sets Access Point's IP address, DHCP server:
#sudo cp ${ETC_SYSD_NET}/50-ap0.network /media/card/etc/systemd/network

# usb0 and eth1 are mostly for production / testing use.
# usb0's IP address / netmask:
#sudo cp ${ETC_SYSD_NET}/50-usb0.network /media/card/etc/systemd/network

# This device appears when using USB<-->Ethernet adaptor.
#sudo cp ${ETC_SYSD_NET}/50-eth1.network /media/card/etc/systemd/network

#sudo cp ${ETC_SYSD_NET}/25-wireless.network /media/card/etc/systemd/network

# (reminder) SRC_F is ...yocto/shadowx_files folder.
# hostapd.service which waits for ap0 interface to be created.
# ETC_SYSD_NET=${SRC_F}/etc/systemd/network
#ETC_SYSD_SYS=${SRC_F}/etc/systemd/system
#echo "Copying [${ETC_SYSD_SYS}/hostapd.service]..."

#sudo cp ${ETC_SYSD_SYS}/hostapd.service /media/card/etc/systemd/system/
#sudo cp ${ETC_SYSD_SYS}/wpa_supplicant-nl80211@.service /media/card/etc/systemd/system/
# This version clears out IP addresses previously used, avoids confusion on eth0 IP addr:
#sudo cp ${ETC_SYSD_SYS}/systemd-networkd.service /media/card/etc/systemd/system/

# This runs bluetoothd in compatibility mode (so we have net socket at /var/run/sdp
# (else sdp_connect() returns "Host is down"
# this is a symlink to /lib/systemd/system/bluetooth.service, so remove the link:
#sudo rm /media/card/etc/systemd/system/dbus-org.bluez.service
#sudo cp ${ETC_SYSD_SYS}/dbus-org.bluez.service /media/card/etc/systemd/system/
# Well, it turns out that THIS is the service file that is starting bluetooth
# [NOT the one above ;( ]
# SRC_F=${YOCTO}/shadowx_files
#sudo cp ${SRC_F}/lib/systemd/system/bluetooth.service /media/card/lib/systemd/system/

#sudo cp ${YOCTO}/shadowx_files/etc/wpa_supplicant/wpa_supplicant-nl80211-mlan0.conf /media/card/etc/wpa_supplicant/
# eth1 - For prod testing USB<-->Ethernet adaptor

# Disable (mask) some systemd automatic services. Linking -->/dev/null
# prevents systemd from running (default in /lib/systemd/system/ folder)
# TEST - allow wpa_supplicant to start via "systemctl start wpa_supplicant@wlan1.service"
# vs manually controlling it. These [default] files are in /lib/systemd
# -- files in /etc/systemd/system will override the ones in /lib/systemd...
#sudo ln -s /dev/null /media/card/etc/systemd/system/wpa_supplicant@.service
#sudo ln -s /dev/null /media/card/etc/systemd/system/wpa_supplicant.service

# TEST - allow systemd to control the AP:
# This will be enabled but not run at boot (I think):
# We now let systemctl run hostapd for us.
#sudo ln -s /dev/null /media/card/etc/systemd/system/hostapd.service

# This is the service file for ShadowX - systemd will start for us:
#sudo cp ${YOCTO}/shadowx_files/etc/systemd/system/shadowx.service /media/card/etc/systemd/system/

# systemctl enable shadowx.service:
# "Created symlink from /etc/systemd/system/multi-user.target.wants/shadowx.service to /etc/systemd/system/shadowx.service."
# Now we see:
# /etc/systemd/system/multi-user.target.wants# ls -l:
# ...
# lrwxrwxrwx 1 root root 35 Apr  4 18:27 shadowx.service -> /etc/systemd/system/shadowx.service
# ...
#sudo su -c "cd /media/card/etc/systemd/system/multi-user.target.wants; ln -s ../shadowx.service shadowx.service;"


#sudo ln -s /dev/null /media/card/etc/systemd/system/ntpd.service

# This is the bluetoooth agent that gets DBUS messages from bluetooth
# and provides the GPS bluetooth device's pin code:
#sudo cp ${YOCTO}/shadowx_files/usr/bin/shxbtagent /media/card/usr/bin/

# Shortcut so I dont have to keep typing "systemctl" all the time, now 'sc' works...
#sudo cp ${YOCTO}/shadowx_files/usr/bin/sc /media/card/usr/bin/

# Disable the "avahi" daemon. This tools listens for discoverable devices
# (like printers, media players, blah blah blah) for zero-config setup
# However 'airmon check' complains this "could cause trouble"
# "systemctl disable avahi-daemon":
#   Removed symlink /etc/systemd/system/sockets.target.wants/avahi-daemon.socket.
#   Removed symlink /etc/systemd/system/multi-user.target.wants/avahi-daemon.service.
#   Removed symlink /etc/systemd/system/dbus-org.freedesktop.Avahi.service.
#sudo rm /media/card/etc/systemd/system/sockets.target.wants/avahi-daemon.socket
#sudo rm /media/card/etc/systemd/system/multi-user.target.wants/avahi-daemon.service
#sudo rm /media/card/etc/systemd/system/dbus-org.freedesktop.Avahi.service

# Production test app and scripts:
echo "TODO Production test app vee, go-vee, prod_test_start_eth1.sh, prod_test_stop_ap.sh..."
# How to make 'vee': 'bitbake go-vee' then cp it to ...shadowx_files
#sudo cp ${YOCTO}/shadowx_files/usr/bin/vee /media/card/usr/bin/
# These are also for production testing...
#sudo cp ${YOCTO}/shadowx_files/usr/bin/prod_test_start_eth1.sh /media/card/usr/bin/prod_test_start_eth1.sh
#sudo cp ${YOCTO}/shadowx_files/usr/bin/prod_test_stop_ap.sh /media/card/usr/bin/prod_test_stop_ap.sh

# shxbtagent needs this to run:
#sudo cp ${YOCTO}/shadowx_files/usr/bin/bluezutils.py /media/card/usr/bin/
# This is the MAC + pincode file that ShadowX write, shxbtagent reads:
#sudo cp ${YOCTO}/shadowx_files/home_root/bt_pins /media/card/home/root/

#sudo cp ${YOCTO}/shadowx_files/home_root/sqlite3 /media/card/home/root/

#sudo cp ${YOCTO}/shadowx_files/home_root/default-eth.network /media/card/home/root/
#sudo cp ${YOCTO}/shadowx_files/home_root/mkEthShadowx.sh /media/card/home/root/
#sudo cp ${YOCTO}/shadowx_files/home_root/mkEthSrt.sh /media/card/home/root/
#sudo cp ${YOCTO}/shadowx_files/home_root/myip.sh /media/card/home/root/
#sudo cp ${YOCTO}/shadowx_files/home_root/shadowxdhcp.network /media/card/home/root/
#sudo cp ${YOCTO}/shadowx_files/home_root/srt-eth.network /media/card/home/root/

# Here is the systemctl service file that starts / stops shxbtagent
#sudo cp ${YOCTO}/shadowx_files/etc/systemd/system/shxbtagent.service /media/card/etc/systemd/system/

# Make shxbtagent start on power-up (this is what 'systemctl enable shxbtagent.service' does):
#sudo su -c "cd /media/card/etc/systemd/system/multi-user.target.wants; ln -s ../shxbtagent.service shxbtagent.service;"

# We are still in the original folder here...

# This is the blasted auto-start where wlan0 keeps attaching to random APs at power on:
#sudo rm /media/card/etc/systemd/system/multi-user.target.wants/wpa_supplicant@wlan0.service

# ShadowX to handle:
#  Create interface ap0 (no longer automatic) NOTE IT IS NOW NAMED 'ap0' NOT 'uap0'!!
#  Start the Access Point
#  start wpa_supplicant so it doesn't crash


SRC_H=${SRC_F}/home_root

# home_root.tar.gz was created on the box and used for safekeeping
# -- moved to ~/yocto/safekeeping, don't use it anymore and its BIG
# -- contains protobuf master (3.1 IIRC) and shadowx src.

# echo "Copying home_root.tar.gz..."
# sudo cp ${SRC_H}/home_root.tar.gz /media/card/home/root/home_root.tar.gz


#sudo cp ${SRC_F}/blink /media/card/home/root/blink

#sudo cp ${YOCTO}/build/tmp/work/cortexa8hf-vfp-neon-poky-linux-gnueabi/blink/1.0-r1/image/usr/bin/blink /media/card/home/root/blink

echo "Almost done, copying etc/network/interfaces and wpa_supplicant-nl80211-wlan1.conf..."

# 2018 SKIP TEMP:
#SRC_E=${SRC_F}/etc
#SRC_N=${SRC_E}/network

# /etc/network/interfaces - this is now done by systemctl-network... (there is no interfaces file anymore)
# sudo cp ${SRC_N}/interfaces /media/card/etc/network/interfaces

echo "Copying /etc/wpa_supplicant/wpa_supplicant-nl80211-wlan1.conf..."
SRC_W=${SRC_E}/wpa_supplicant
#sudo cp ${SRC_W}/wpa_supplicant-nl80211-wlan1.conf /media/card/etc/wpa_supplicant/wpa_supplicant-nl80211-wlan1.conf
# ALSO WE ARE USING mlan0 (the built in chipset) instead of wlan1. On the new 'Y' boards,
# this is 2/5GHz (all channels). On old boards, mlan0 is only 13 channels - we will need to
# use wlan1 (the RALINK) on the old boards to connect to 5GHz channel APs.
# Could not get systemd to run wlan1 for some reason (still learning to cooperate)
# It started wpa_supplicant but didn't get timely assoc resp and tried a few times
# a few mins apart and never did associate / connect using wlan1.
# Shadowx still uses wlan1 to make the (larger on older boards) channel table for
# both mlan & wlan / hostapd / wpa_supplicant.

# Add the 'shadowx' user. (echo >> appends "text" to end of file + newline)
# "Permission Denied" ! -- added to gumstix-console-image.bb (meta-gumstix-extras/recipes-images/gumstix)
#sudo echo "shadowx:x:1000:1000::/home/shadowx:/bin/sh" >> /media/card/etc/passwd
# .. and the password which is 'shadowx':
#sudo echo "shadowx:$6$gq5orrro$J9PjC4iM4T1nLJOyBRglO9Lv3oo6.IzaHrfeWUrpm9OTOkv711OeQm3Yb22oVQ1tMmCw.OgezhJVjarOU1.XF0:17260:0:99999:7:::" >> /media/card/etc/shadow

echo "Please wait, syncing..."
sync
sleep 10
echo "Unmounting ${DEV}"
RET=1
until [ ${RET} -eq 0 ]; do
	set +e
    sudo umount ${DEV}
    RET=$?
    set -e
    if [ ${RET} -ne 0 ]; then
        echo "unmount failed (busy), will retry in 10..."
        sleep 10
    fi
done

# Note: swap partition (#3) is already formatted when we formatted the first two...

end_time="$(date -u +%s)"

elapsed="$(($end_time-$start_time))"
echo "================================================="
echo "SD card is READY."
echo "Completed in $elapsed seconds."
date +"%T"
echo "================================================="

