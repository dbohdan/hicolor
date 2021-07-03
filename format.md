# File format

* Magic: 7 bytes, "HiColor".
* Version: 1 byte, "5" for 15-bit color, "6" for 16-bit color.  Other versions may be added later.
* Width: 2 bytes: WB1, WB2.  Width = WB1 + 256×WB2.
* Height: 2 bytes: HB1, HB2.  Height = HB1 + 256×HB2.
* Data: 2×Width×Height bytes.  The data consists of Width×Height 2-byte values.

## Values

* Version "5":
    * 5 bits red, 5 bits green, 5 bits blue, 0.
* Version "6":
    * 5 bits red, 6 bits green, 5 bits blue.
