echo -e "\nStarting compilation...\n"
# ENV
R=n
while read -p "Cam patch? (y/n)" bchoice; do
case "$bchoice" in
 n|N)
  break
 ;;
 y|Y)
  R=y
  git cp -s 3679d8fbfbf11151109c71eb4308a21d4fb854ab
  break
 ;;
 *)
  echo
  echo "Try again please!"
  echo
 ;;
esac
done

CONFIG=vendor/ginkgo-perf_defconfig
KERNEL_DIR=$(pwd)
PARENT_DIR="$(dirname "$KERNEL_DIR")"
KERN_IMG="$KERNEL_DIR/out/arch/arm64/boot/Image.gz-dtb"
#export KBUILD_BUILD_USER="hafizziq"
#export KBUILD_BUILD_HOST="ubuntu"
export KBUILD_BUILD_TIMESTAMP="$(TZ=Asia/Kuala_Lumpur date)"
export PATH="$HOME/gcc/gcc-arm64/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/gcc/gcc-arm64/lib:$LD_LIBRARY_PATH"
export KBUILD_COMPILER_STRING="$($HOME/gcc/gcc-arm64/bin/aarch64-elf-gcc --version | head -n 1 | cut -d ')' -f 2 | awk '{print $1}')"
export CROSS_COMPILE=$HOME/gcc/gcc-arm64/bin/aarch64-elf-
export CROSS_COMPILE_ARM32=$HOME/gcc/gcc-arm/bin/arm-eabi-
export out=out

# let's clean the output first before building
if [ -d $out ]; then
 echo -e "Cleaning out leftovers...\n"
 rm -rf $out
fi;

mkdir -p $out

# Functions
gcc_build () {
    make -j$(nproc --all) O=$out \
                          ARCH=arm64 \
                          CC="aarch64-elf-gcc" \
                          AR="llvm-ar" \
                          NM="llvm-nm" \
                          LD="ld.lld" \
                          AS="llvm-as" \
                          OBJCOPY="llvm-objcopy" \
                          OBJDUMP="llvm-objdump" \
                          CROSS_COMPILE=$CROSS_COMPILE \
                          CROSS_COMPILE_ARM32=$CROSS_COMPILE_ARM32
}

kernel="out/arch/arm64/boot/Image.gz-dtb"
dtbo="out/arch/arm64/boot/dtbo.img"

if [ ! -f "$kernel" ] || [ ! -f "$dtbo" ]; then
	echo -e "\nCompilation failed!"
	exit 1
fi

rm -rf AnyKernel3/Image.gz-dtb
rm -rf AnyKernel3/dtbo.img

cp $kernel AnyKernel3
cp $dtbo AnyKernel3

cd AnyKernel3
zip -r9 "../$ZIPNAME" * -x .git README.md *placeholder
cd ..
echo -e "\nCompleted in $((SECONDS / 60)) minute(s) and $((SECONDS % 60)) second(s) !"
echo "Zip: $ZIPNAME"
