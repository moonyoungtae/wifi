# *parse_log* Data Instruction

In *parse_log*, each packet cell consists of:

`timestamp`, `csi_len`, `channel`, `phy_err`, `noise`, `rate`, `bandwidth`, `nc`, `nr`, `nt`, `rssi`, `rssi1`, `rssi2`, `rssi3`, `payload_len`, `csi`

### timestamp

A timestamp recorded by receiver hardware. Unit is microseconds.

### csi_len

The length of CSI data in bytes. `= 2.5 * nt * nr * nc`

`2.5` is 20 bits, each 10 bits for real and imagine number expression.

### channel

The center frequency of the **primary channel** in MHz.

### phy_err

PHY layer error code from ath9k driver. Look [here](https://github.com/wldh-g/BPI-R2-Atheros-CSITool/blob/5.4-main/drivers/net/wireless/ath/ath9k/mac.h#L193-L230) for detailed code information.

### noise

Currently unavailable in CSI tool. `= 0`.

### rate

[HT MCS](http://mcsindex.com/). If `csi_len` is 0, this can mean CCK or OFDM rate.

If you made the record using `logcsi` tool in this repository, it automatically removes HT flag (7th bit) to reveal the MCS number more clearly.

### bandwidth

+ `0` : 20MHz
+ `1` : 40MHz

### nc, nr, nt

+ `nc` : The number of sub*c*arriers
+ `nr` : The number of *r*eceive antennas
+ `nt` : The number of *t*ransmit antennas

### rssi, rssi1, rssi2, rssi3

+ `rssi`  : Combined RSSI (Sum of 3 decibel values below)
+ `rssi1` : RSSI of primary channel from Antenna 01
+ `rssi2` : RSSI of primary channel from Antenna 02
+ `rssi3` : RSSI of primary channel from Antenna 03

`128` in RSSI means "no data".

You should subtract `95` from all RSSI values excluding 128 to convert to dBm unit. It's because the atheros chip processes RSSI in that way.

### payload_len

The length of payload in bytes. `= injection packet size + 26`

Look [here](/receiver/logcsi.md#what-are-those-26-bytes) for why `+ 26`.

### csi

Your channel state information.
