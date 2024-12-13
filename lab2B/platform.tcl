# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\Users\mdsmith\workspace2\lab2B\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\Users\mdsmith\workspace2\lab2B\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {lab2B}\
-hw {C:\Users\mdsmith\Desktop\Lab1A\system_wrapper.xsa}\
-proc {microblaze_0} -os {standalone} -out {C:/Users/mdsmith/workspace2}

platform write
platform generate -domains 
platform active {lab2B}
bsp reload
bsp setdriver -ip sevenSeg_0 -driver generic -ver 3.0 3.1
bsp write
bsp reload
catch {bsp regenerate}
platform generate
platform generate
platform generate
platform generate
platform generate
platform active {lab2B}
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate
platform generate
platform active {lab2B}
bsp reload
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform active {lab2B}
bsp reload
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform generate
platform generate
platform active {lab2B}
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform generate -domains 
platform generate
platform active {lab2B}
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform active {lab2B}
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform active {lab2B}
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/Users/mdsmith/Desktop/test/system_wrapper.xsa}
platform generate -domains 
platform generate
platform generate
