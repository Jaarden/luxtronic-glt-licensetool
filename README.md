# BMS license creator
Creates an USB drive that unlocks the BMS functionality on an Luxtronic managed heatpump

```
	Usage: ./licensetool <usb_path> [command] [license_count]
	  usb_path: Path to USB device (e.g., /dev/sdb)
	  command: 'check', 'create', or 'decrement'
	  license_count: Number of licenses to create (only for 'create')

	Examples:
	  ./licensetool /dev/sdb check           # Check existing licenses
	  ./licensetool /dev/sdb create 5        # Create 5 licenses
	  ./licensetool /dev/sdb decrement       # Decrement license count
```

## Create license USB
```

	sudo ./licensetool  /dev/sda create 5678      
		Debug: Creating license with count 5678
		Debug: Checksum1: 68, Checksum2: 3, Checksum3: 7
		Debug: Total checksum: 78
		Successfully created license block with 5678 licenses
		License creation successful!
		-----------------------------
``` 


## Validate license USB
```
	sudo ./licensetool  /dev/sda check 
		Debug: Raw license count bytes: 0x2E 0x16
		Debug: License count: 5678
		Debug: Checksum1 (0x34): 68
		Debug: Checksum2 (0x65): 3
		Debug: Checksum3 (0x69): 7
		Debug: Total calculated: 78
		Debug: Stored checksum: 78
		Found 5678 valid licenses
		Licenses remaining: 5678
```


Afterwards, plug it in into a heatpump. In this case an Alpha Innotec and watch for the house symbol. It can take some trail and error to get it working. It took me 5-10 inserts to had it detect the USB properly.

![alt text](https://github.com/Jaarden/luxtronic-glt-licensetool/blob/main/images/screen1.png "Screen 1")


After that, you will see the following screen:


![alt text](https://github.com/Jaarden/luxtronic-glt-licensetool/blob/main/images/screen2.png "Screen 2")

Afterwards, BMS is unlocked

![alt text](https://github.com/Jaarden/luxtronic-glt-licensetool/blob/main/images/screen3.png "Screen 3")
