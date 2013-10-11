#!/bin/sh

build_path="../build_s5pv210"
image_filename="$build_path/arch/arm/boot/zImage"
image_filename2="$build_path/arch/arm/boot/Image"
target_filename="/home/squake/tftp/zImage"
target_filename2="/home/squake/tftp/Image"

#build_tool="arm-linux-gnueabi-"
build_tool="arm-generic-linux-gnueabi-"


if [ -f .config ]; then
	echo ".....mrproper"
	make mrproper
fi

if [ ! -d $build_path ]; then
	mkdir $build_path
	chmod 777 $build_path
fi

if [ ! -f $build_path/.config ]; then
	echo ".....s5pv210 defconfig"	
	#CROSS_COMPILE=$build_tool ARCH=arm make O=$build_path ezs5pv210_defconfig 
	CROSS_COMPILE=$build_tool ARCH=arm make O=$build_path s5pv210_defconfig
fi


if [ "$1" = "" ]; then
   rm $image_filename
   rm $image_filename2
   rm $target_filename
   rm $target_filename2
	CROSS_COMPILE=$build_tool ARCH=arm make O=$build_path -j4 zImage 
else
	CROSS_COMPILE=$build_tool ARCH=arm make O=$build_path $1 $2 $3
fi


if [ -f $image_filename ]; then
   echo "copy from $image_filename to $target_filename"
   chmod 777 $image_filename
   cp -a $image_filename $target_filename
   echo "copy from $image_filename2 to $target_filename2"
   chmod 777 $image_filename2
   cp -a $image_filename2 $target_filename2
fi
