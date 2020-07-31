#!/bin/sh
#
######################################################################
#
# sample-postinstall.sh
#
# Example script for post-install hooks.
#
# Add this as a postinstall hook to your installer via
# the 'mkinstaller.py' command line:
#
# $ mkinstaller.py ... --postinstall-script sample-postinstall.sh ...
#
# At install time, this script will
#
# 1. be extracted into the working directory with the other installer
#    collateral
# 2. have the execute bit set
# 3. run in-place with the installer chroot directory passed
#    as the first command line parameter
#
# If the script fails (returns a non-zero exit code) then
# the install is aborted.
#
# This script is executed using the ONIE runtime (outside the chroot),
# after the actual installer (chrooted Python script) has finished.
#
# This script is run after the postinstall actions (e.g. proxy GRUB
# commands)
#
# At the time the script is run, the installer environment (chroot)
# is fully prepared, including filesystem mount-points.
# That is, the chroot mount points have not been unmounted yet.
#
######################################################################

rootdir=$1; shift

echo "Hello from postinstall"
echo "Chroot is $rootdir"


PATH_TMP='/tmp/os'
EFI_PATH_TMP='/tmp/efi'
DIAG_GRUB_DATA="function diag_bootcmd {
                \$diag_grub_custom 
            }"

			
diskpartition=$(df | grep "onie-boot" | awk '{print $1}')
diskname=${diskpartition:0:8}
echo "diskname:"$diskname
			
###  Delete ONL boot partition 
ONL_DIAG=$(efibootmgr | grep "Open Network Linux" | awk '{print $1}')
ONL_BOOT_NUM="${ONL_DIAG//Boot/}"
ONL_BOOT_NUM="${ONL_BOOT_NUM//\*/}"
# efibootmgr -b $ONL_BOOT_NUM -A # -B is inactive command
efibootmgr -b $ONL_BOOT_NUM -B  # -B is delete command

### Change parition name with -DIAG, The uninstall operation must not modify or remove this partiion.
sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-BOOT" | awk '{print $1}'):"ONL-BOOT-DIAG" $diskname
sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-CONFIG" | awk '{print $1}'):"ONL-CONFIG-DIAG" $diskname
sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-IMAGES" | awk ' {print $1}'):"ONL-IMAGES-DIAG" $diskname
sgdisk --change-name=$(sgdisk -p $diskname | grep "ONL-DATA" | awk '{print $1}'):"ONL-DATA-DIAG" $diskname

tune2fs -L "ONL-BOOT-DIAG" $diskname$(sgdisk -p $diskname | grep "ONL-BOOT" | awk '{print $1}')
tune2fs -L "ONL-CONFIG-DIAG" $diskname$(sgdisk -p $diskname | grep "ONL-CONFIG" | awk '{print $1}')
tune2fs -L "ONL-IMAGES-DIAG" $diskname$(sgdisk -p $diskname | grep "ONL-IMAGES" | awk '{print $1}')
tune2fs -L "ONL-DATA-DIAG" $diskname$(sgdisk -p $diskname | grep "ONL-DATA" | awk '{print $1}')

### Set GPT system partition attribute bit (bit 0)
sgdisk -A $(sgdisk -p $diskname | grep "ONL-BOOT" | awk '{print $1}'):set:0 $diskname
sgdisk -A $(sgdisk -p $diskname | grep "ONL-CONFIG" | awk '{print $1}'):set:0 $diskname
sgdisk -A $(sgdisk -p $diskname | grep "ONL-IMAGES" | awk '{print $1}'):set:0 $diskname
sgdisk -A $(sgdisk -p $diskname | grep "ONL-DATA" | awk '{print $1}'):set:0 $diskname

### Read grub config and set back to ONIE Diag grub.
mkdir -p $PATH_TMP
mount -v $diskname$(sgdisk -p $diskname | grep "ONL-BOOT" | awk '{print $1}') $PATH_TMP
ST_GRUB=$(cat $PATH_TMP/grub/grub.cfg | grep -n "menuentry \"Open Network Linux\" {" | head -n 1 | cut -d: -f1)
EN_GRUB=$(tail $PATH_TMP/grub/grub.cfg -n +$ST_GRUB | grep -n "}" |head -n 1 |cut -d: -f1)
EN_GRUB=$(($EN_GRUB+$ST_GRUB-1))

sed -n -e $(($ST_GRUB+1)),$(($EN_GRUB-1))p $PATH_TMP/grub/grub.cfg > /tmp/grub_tmp
sed -i "s/ONL-BOOT/ONL-BOOT-DIAG/g" /tmp/grub_tmp

# DIAG_GRUB="${DIAG_GRUB_DATA/"\$diag_grub_custom"/\"$DIAG_GRUB\"}"


# Reset onie grub remove ONL custom variable

#find "begin: serial console config" in grub.cfg
POS_LINE=$(cat $rootdir/mnt/onie-boot/grub/grub.cfg | grep -n "begin: serial console config" | head -n 1 | cut -d: -f1)
if [ $POS_LINE -gt 1 ]; then
  #echo "remove top variable from line 1 to $POS_LINE"
  sed $((1)),$(($POS_LINE-1))d $rootdir/mnt/onie-boot/grub/grub.cfg > /tmp/new_clean_grub_tmp
  cat /tmp/new_clean_grub_tmp > $rootdir/mnt/onie-boot/grub/grubNEW.cfg
else
  #echo "just copy to grubNEW.cfg"
  cp $rootdir/mnt/onie-boot/grub/grub.cfg $rootdir/mnt/onie-boot/grub/grubNEW.cfg
fi

# find&clean old diag boot command (This section was created after onie update onie'll copied diag-bootcmd.cfg to this section)
STARTEX_POS_LINE=$(cat $rootdir/mnt/onie-boot/grub/grubNEW.cfg | grep -n "# begin: diag boot command" | head -n 1 | cut -d: -f1)
LASTEX_POS_LINE=$(cat $rootdir/mnt/onie-boot/grub/grubNEW.cfg | grep -n "# end: diag boot command" | head -n 1 | cut -d: -f1)
if [ $STARTEX_POS_LINE -gt 1 ]; then
    RESULT=`expr $LASTEX_POS_LINE - $STARTEX_POS_LINE` || {
        echo "This case is created for fix bug from previous version please ignore above error message"
        RESULT=0
    }
    if [ $RESULT -gt 1 ]; then
        sed $(($STARTEX_POS_LINE+1)),$(($LASTEX_POS_LINE-1))d $rootdir/mnt/onie-boot/grub/grubNEW.cfg > /tmp/new_clean_grub_tmp
        cat /tmp/new_clean_grub_tmp > $rootdir/mnt/onie-boot/grub/grubNEW.cfg
    fi
fi






cp $rootdir/mnt/onie-boot/grub/grub.cfg $rootdir/mnt/onie-boot/grub/grub_backup.cfg
echo "Installing Diag OS grub to grub.cfg ....."
echo "$(echo "}" | cat - $rootdir/mnt/onie-boot/grub/grubNEW.cfg)" > $rootdir/mnt/onie-boot/grub/grubNEW.cfg
cat /tmp/grub_tmp | cat - $rootdir/mnt/onie-boot/grub/grubNEW.cfg > $rootdir/mnt/onie-boot/grub/grub.cfg
echo "$(echo "function diag_bootcmd {" | cat - $rootdir/mnt/onie-boot/grub/grub.cfg)" > $rootdir/mnt/onie-boot/grub/grub.cfg
echo "$(echo diag_menu=\"CLS Diag OS\" | cat - $rootdir/mnt/onie-boot/grub/grub.cfg)" > $rootdir/mnt/onie-boot/grub/grub.cfg
rm -f $rootdir/mnt/onie-boot/grub/grubNEW.cfg


# DIAG_GRUB="${DIAG_GRUB_DATA/"\$diag_grub_custom"/\"$DIAG_GRUB\"}"
cp $rootdir/mnt/onie-boot/onie/grub/grub_backup.cfg $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg 2> /dev/null || :
cp $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg $rootdir/mnt/onie-boot/onie/grub/grub_extra_backup.cfg

#Remove old CLS-DIAG-OS grub from old grub-extra.cfg
STARTEX_POS_LINE=$(cat $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg | grep -n "diag_menu=\"CLS Diag OS\"" | head -n 1 | cut -d: -f1)
LASTEX_POS_LINE=$(cat $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg | grep -n "ONIE_EXTRA_CMDLINE_LINUX" | head -n 1 | cut -d: -f1)
if [ $(($LASTEX_POS_LINE-2)) -gt 1 ]; then
  sed $(($STARTEX_POS_LINE-1)),$(($LASTEX_POS_LINE-2))d $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg > $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg
fi

#Create custom CLS-DIAG-OS option in grub-extra.cfg

cp $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg $rootdir/mnt/onie-boot/onie/grub/grubNEW.cfg
echo "Installing Diag OS grub grub-extra.cfg ....."
echo "$(echo "}" | cat - $rootdir/mnt/onie-boot/onie/grub/grubNEW.cfg)" > $rootdir/mnt/onie-boot/onie/grub/grubNEW.cfg
cat /tmp/grub_tmp | cat - $rootdir/mnt/onie-boot/onie/grub/grubNEW.cfg > $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg
echo "$(echo "function diag_bootcmd {" | cat - $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg)" > $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg
echo "$(echo diag_menu=\"CLS Diag OS\" | cat - $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg)" > $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg
echo "$(echo "## Begin CLS-DIAG-OS in grub-extra.cfg" | cat - $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg)" > $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg
rm -f $rootdir/mnt/onie-boot/onie/grub/grubNEW.cfg

#Get boot order before create new one.
# CURRENT_BOOT_ORDER=$(efibootmgr | grep BootOrder: | awk '{ print $2 }')
mkdir -p $EFI_PATH_TMP
mount -v $diskname$(sgdisk -p $diskname | grep "EFI System" | awk '{print $1}') $EFI_PATH_TMP
echo "Update EFI directory for ONL from /boot/efi/EFI/ONL to /boot/efi/EFI/ONL-DIAG"
if [ -d /tmp/efi/EFI/ONL-DIAG ]; then
    rm -r /tmp/efi/EFI/ONL-DIAG
fi
if [ -d /tmp/efi/EFI/ONL ]; then
    mv /tmp/efi/EFI/ONL /tmp/efi/EFI/ONL-DIAG
fi


# boot_num=$(efibootmgr -v | grep "CLS-DIAG-OS" | grep ')/File(' | tail -n 1 | awk '{ print $1 }')
# boot_num_len=${#boot_num}
# if [ $boot_num_len -eq 0 ]; then
#     efibootmgr -c -L "CLS-DIAG-OS" -l '\EFI\ONL-DIAG\grubx64.efi'
# fi

# #*Reorder* move CLS-DIAG-OS to back of list.
# boot_num=$(efibootmgr -v | grep "CLS-DIAG-OS" | grep ')/File(' | tail -n 1 | awk '{ print $1 }')
# boot_num=${boot_num#Boot}
# boot_num=${boot_num%\*}
# new_boot_order="$(echo -n $CURRENT_BOOT_ORDER | sed -e s/,$boot_num// -e s/$boot_num,// -e s/$boot_num//)"
# efibootmgr -o ${new_boot_order},${boot_num}


#Remove existing CLS-DIAG-OS from boot option following new requirement
boot_num=$(efibootmgr -v | grep "CLS-DIAG-OS" | grep ')/File(' | tail -n 1 | awk '{ print $1 }')
boot_num_len=${#boot_num}
if [ $boot_num_len -gt 0 ]; then
  boot_num=${boot_num#Boot}
  boot_num=${boot_num%\*}
  efibootmgr -b $boot_num -B
fi


echo "Copy grub-extra.cfg to diag-boocmd.cfg to prevent command disappear after Onie update ..."
cp $rootdir/mnt/onie-boot/onie/grub/grub-extra.cfg $rootdir/mnt/onie-boot/onie/grub/diag-bootcmd.cfg

echo "Create dummy partition for CLS Diag OS for prevent being destroy by onie-updater"
# DUMMY_PARTITION_NUMBER_POST=$(sgdisk -p $diskname | grep CLS-DIAG | awk '{print $1}')
# if [[ $DUMMY_PARTITION_NUMBER_POST -gt 0 ]]
# then
#     exit 0
# fi
START_POS=$(sgdisk -f $diskname)
END_POS=$(($START_POS+4096))
LAST_PARTITION_NUMBER=$(sgdisk -p $diskname | grep $(($START_POS-1)) | awk '{print $1}')
NEW_PARTITION_NUMBER=$((LAST_PARTITION_NUMBER+1))
sgdisk -n $NEW_PARTITION_NUMBER:$START_POS:$END_POS -t $NEW_PARTITION_NUMBER:0700 $diskname
sgdisk --change-name=$NEW_PARTITION_NUMBER:"CLS-DIAG" $diskname
sgdisk -A $(sgdisk -p $diskname | grep "CLS-DIAG" | awk '{print $1}'):set:0 $diskname
sync

partprobe $diskname
mkfs.ext4 -v -O ^huge_file -L CLS-DIAG $diskname$NEW_PARTITION_NUMBER || {
    echo "error using ext2 instead"
    mkfs.ext2 -v -L CLS-DIAG $diskname$NEW_PARTITION_NUMBER 
}
partprobe $diskname
exit 0

