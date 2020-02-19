E-series defined in IEC 60063 are a widly accepted system of prefered
numbers for electronic components.  Available values are approximately
equally spaced in a logarithmic scale.  Although E-series are used for
Zener-diodes, inductors and other components, this calculator is mainly
intended for resistors.

If your design requires a resistor value wich is not readily available, this
calculator will find a combination of standard E-series components to create
it.  You can enter the resistance required in almost any format: 

<center>1234,1234R, 1.234K, 1K234, M001234, 0M001234</center>

all represent the same resistance. Solutions are given in the following formats:

	R1 + R2 +...+ Rn	resistors in series
	R1 | R2 |...| Rn	resistors in parallel
	R1 + (R2|R3)...		any combination of the above

__Example:__ 24K5 resistance can be formed from resistors in the E6 or E3
range by 2K2 + 22K with -1.22% error, or 1K + ( 47K | 47K ) for an exact
match. Solutions using 3 or 4 resistors are given for E1/E3 series if a
better match can be found. 

__Warning:__ The user is always responsible to consider manufacturing tolerances
wich are not taken into account.  It is pointless to create a resistor
combination which matches a required resistance exactly when the resistors
being used have wide tolerances themselves.  Although a perfect match is
found, the tolerances of the actual resistors used in the solution could
make the result less perfect.

__Warning:__ Soldering several SMD components to a single PCB footprint in vertical or
horizontal stacked configuration should be limited to engineering samples
only. Such constructions are prone to crack for the reason of thermal expansion stress.
