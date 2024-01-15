# !/bin/bash
#./build_all.sh ~/chef_bins

dest_dir=${1}

if [ ! -d $1 ]
then
   echo "$1 not exites, create it anyway."
   mkdir $1
fi


device_types=(
rootnode_airqualitysensor_e63187f6c9
rootnode_basicvideoplayer_0ff86e943b
rootnode_colortemperaturelight_hbUnzYVeyn
rootnode_contactsensor_lFAGG1bfRO
rootnode_dimmablelight_bCwGYSDpoe
rootnode_dishwasher_cc105034fe
rootnode_doorlock_aNKYAreMXE
rootnode_extendedcolorlight_8lcaaYJVAa
rootnode_fan_7N2TobIlOX
rootnode_flowsensor_1zVxHedlaV
rootnode_genericswitch_9866e35d0b
# rootnode_heatingcoolingunit_ncdGai1E5a
rootnode_humiditysensor_Xyj4gda6Hb
rootnode_laundrywasher_fb10d238c8
rootnode_lightsensor_lZQycTFcJK
rootnode_occupancysensor_iHyVgifZuo
rootnode_onofflight_bbs1b7IaOV
rootnode_onofflightswitch_FsPlMr090Q
rootnode_onoffpluginunit_Wtf8ss5EBY
rootnode_pressuresensor_s0qC9wLH4k
rootnode_pump_5f904818cc
rootnode_refrigerator_temperaturecontrolledcabinet_temperaturecontrolledcabinet_ffdb696680
rootnode_roboticvacuumcleaner_1807ff0c49
rootnode_roomairconditioner_9cf3607804
rootnode_smokecoalarm_686fe0dcb8
# rootnode_speaker_RpzeXdimqA
rootnode_temperaturesensor_Qy1zkNW7c3
rootnode_thermostat_bm3fb8dhYi
rootnode_windowcovering_RLCxaGi9Yx
)

for i in ${device_types[@]}
do
	./chef.py -br -d $i -t linux
	cp linux/out/$i $1

done

