from onl.platform.base import *
from onl.platform.celestica import *
import time

class OnlPlatform_x86_64_cel_ivystone_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_128x100):
    PLATFORM='x86-64-cel-ivystone-r0'
    MODEL="Ivystone"
    SYS_OBJECT_ID=".2060.1"

    def baseconfig(self):
        onlp_interval_time = 30 #second
        file_path = "/var/opt/interval_time.txt"
        qsfp_qty = 128
        qsfp_offset = 9

        print("Initialize Ivystone Platform driver")

        #Celestica Blacklist file
        blacklist_file_path="/etc/modprobe.d/celestica-blacklist.conf"
        #Blacklist the unuse module.
        if os.path.exists(blacklist_file_path):
            os.system("rm {0}".format(blacklist_file_path))

        os.system("touch {0}".format(blacklist_file_path))
        cel_paths = "/lib/modules/{0}/onl/celestica/".format(os.uname()[2])
        cel_dirs = os.listdir(cel_paths)
        for dir in cel_dirs:
            full_cel_path=cel_paths+dir
            if os.path.isdir(full_cel_path):
                modules=os.listdir(full_cel_path)
                for module in modules:
                    os.system("echo 'blacklist {0}' >> {1}".format(module[0:-3],blacklist_file_path))

        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/i2c/busses/i2c-ocores.ko")
        self.insmod("dimm-bus.ko")
        #self.insmod("i2c-imc.ko")
        os.system("insmod /lib/modules/`uname -r`/onl/celestica/x86-64-cel-ivystone/i2c-imc.ko allow_unsafe_access=1")
        self.insmod("baseboard_cpld.ko")
        self.insmod("switchboard_fpga.ko")
        self.insmod("mc24lc64t.ko")
        os.system("insmod /lib/modules/`uname -r`/onl/onl/common/optoe.ko")

        actual_port_num = 1

        for y in range(qsfp_qty):
            os.system("echo 'QSFP{1}' > /sys/bus/i2c/devices/{0}-0050/port_name".format(qsfp_offset+y+1,actual_port_num))
            actual_port_num += 1

        os.system("echo '3' > /proc/sys/kernel/printk")
        ###### new configuration for SDK support ########
        # os.system("insmod /lib/modules/`uname -r`/kernel/net/core/pktgen.ko")
        # os.system("insmod /lib/modules/`uname -r`/kernel/net/core/drop_monitor.ko")
        # os.system("insmod /lib/modules/`uname -r`/kernel/net/ipv4/tcp_probe.ko")

        ##### Config ma1 vlan interface to interact with OpenBMC #######
        os.system("ip link add link ma1 name ma1.4088 type vlan id 4088")
        os.system("ip addr add 240.1.1.2/30 dev ma1.4088")
        os.system("ip link set ma1.4088 up")

        #eeprom driver
        self.new_i2c_device('24lc64t', 0x56, 0)
        

        if os.path.exists(file_path):
            pass
        else:
            with open(file_path, 'w') as f:  
                f.write("{0}\r\n".format(onlp_interval_time))
            f.close()

        while os.system("ping -c 1 240.1.1.1")!=0:
            print "Initialize and Checking OpenBMC protocols , retry in 1 sec."
            time.sleep(1)
        print "OpenBMC Protocols ready ..."

        #initialize onlp cache files
        print("Initialize ONLP Cache files")
        os.system("curl -gs http://240.1.1.1:8080/api/sys/sensors |python -m json.tool > /tmp/onlp-sensor-cache.tmp; sync; rm -f /tmp/onlp-sensor-cache.txt; mv /tmp/onlp-sensor-cache.tmp /tmp/onlp-sensor-cache.txt")
        os.system("curl -gs http://240.1.1.1:8080/api/sys/fruid/psu | python -m json.tool > /tmp/onlp-psu-fru-cache.tmp; sync; rm -f /tmp/onlp-psu-fru-cache.txt; mv /tmp/onlp-psu-fru-cache.tmp /tmp/onlp-psu-fru-cache.txt")
        os.system("curl -gs http://240.1.1.1:8080/api/sys/fruid/fan | python -m json.tool > /tmp/onlp-fan-fru-cache.tmp; sync; rm -f /tmp/onlp-fan-fru-cache.txt; mv /tmp/onlp-fan-fru-cache.tmp /tmp/onlp-fan-fru-cache.txt")
        os.system("curl -gs http://240.1.1.1:8080/api/sys/fruid/sys | python -m json.tool > /tmp/onlp-sys-fru-cache.tmp; sync; rm -f /tmp/onlp-sys-fru-cache.txt; mv /tmp/onlp-sys-fru-cache.tmp /tmp/onlp-sys-fru-cache.txt")
        os.system("curl -gs http://240.1.1.1:8080/api/sys/fruid/status | python -m json.tool > /tmp/onlp-status-fru-cache.tmp; sync; rm -f /tmp/onlp-status-fru-cache.txt; mv /tmp/onlp-status-fru-cache.tmp /tmp/onlp-status-fru-cache.txt")
        #os.system("curl -ds \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/psu_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-psu-led-cache.tmp; sync; rm -f /tmp/onlp-psu-led-cache.txt; mv /tmp/onlp-psu-led-cache.tmp /tmp/onlp-psu-led-cache.txt")
        #os.system("curl -ds \'{\"data\":\"cat /sys/bus/i2c/devices/i2c-0/0-000d/fan_led_ctrl_en 2>/dev/null | head -n 1\"}\' http://240.1.1.1:8080/api/sys/raw | python -m json.tool > /tmp/onlp-fan-led-cache.tmp; sync; rm -f /tmp/onlp-fan-led-cache.txt; mv /tmp/onlp-fan-led-cache.tmp /tmp/onlp-fan-led-cache.txt")
        return True
