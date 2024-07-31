# PixiePico


Ham radio FT8 experimental 28 MHz transceiver based on the famous Pixie 7 MHz transceiver kit.



![Alt Text](underconstruction.jpg?raw=true "PixiePico Project under construction")
 

# Introduction

Ham radio is a technical hobby enjoyed by millions of persons worldwide, despite different country specific definitions the overall characterization is about activities performed by individuals interested in:

* Understand telecomunications operating procedures.
* Perform technical studies.
* Experiment with electronics.

Ham radio operators uses communcations gear  both home made and commercially made alike. Depending on the technical skills of the operator she might be able to build from relatively simple to very sophisticated gear, the availblity of commercial equipment allows for persons with strong interest in the communications side of the activity to have access to sophisticated equipment as well.

As technology advances the ability for relatively novel people to craft their own equipment fades away as it requires a substantial footprint of knowledge, tools and lab equiptments.

Still there are opportunities, some of them offered as kits, of relatively simple to medium complexity equipments to be built by beginners.

As anybody with some mileage expended in the hobby might testify there is no thrill greater than to build and equipment with our own hands and *communicate* with it thereafter.

Ham radio communications spans over a large number of modes, both analog and digital, as well as a significant number of bands, essentially from very large long wave radio frequencies to very tiny microwave frequencies.

Still the frequencies commonly referred as *HF* brings the appeal of allowing very long distance communications with relatively simple antenas.

In particular, the 28 Mhz (10 meter band) allow a worldwide communication capability with antennas which can be acommodated even on very restricted facilities using relatively low powers (1W or less).

A new generation of small signal digital communication modes allows extremely very low power and relatively simple transceivers to allow state of the art digital communications. 

This project aim for the following goals:

* Create an extremely simple yet capable HF transceiver for the 28 MHz band able to withstand small signal digital communications in real conditions.
* Use as many low cost and readily available parts and kits to build it.
  * Pixie 7 MHz CW transceiver kit.
  * rp2040 breakout board.
  * Si5351 clock breakout board.
* Allow the builder to grow and extend technical skills in a number of domains while building it, such as:
  * Basic understanding of the design of the different stages and sub-systems of the transceiver.
  * Use of a ARM processor development environment and build chain
  * Operate the transceiver with state-of-the-art communications software such as WSJT-X and others.
  * Notions of 3D printing design techniques.
  * Simple radio station setup procedures.
  * Minimum test equipment and building techniques.  
* On-the-air evaluation of the transceiver and few operating techniques associated with it.




## Previous work

In a general way  over 50 years of ham radio activity leads to this project, but some of the past projects are more related to this project than others.
There are few, however, that provides quite a leverage in terms of previous experimentation, firmware libraries and the testing of quite a few constructions ideas, a short list of them are:

* **Pixie**
Many, many, implementations of the *Pixie* designs and countless variants over the years, many such design and variants can be seen at my blog sites [lu7dz](http://lu7did.blogspot.com) and [lu7hz](http://lu7hz.blogspot.com).

* **Pixino**
A project to implement a uSDX firmware porting but using the Pixie transceiver as the platform.

* **PixiePi**
A project to create a multipurpose 7 MHz transceiver for digital modes based on the 7 Mhz CW Pixie kit and the Raspberry Pi 3 board and Evariste's F5OEO [rpitx](https://github.com/F5OEO/rpitx) library.

* **OrangeThunder**
A project to create a multipurpose multimode HF transceiver based on the Raspberry Pi 3 board, the RTL-SDR SDR processor and the [rpitx](https://github.com/F5OEO/rpitx) library.

* **ADX-rp2040**
A porting of the [ADX](https://antrak.org.tr/blog/adx-arduino-digital-transceiver/) digital transceiver by Barb (WB2CBA) originally for the Arduino Nano (ATMEL 328p processor) into the ARM rp2040 architecture, this transceiver is able to operate with suitable software such as WSJT-X on different HF bands. 

* **RDX-rp2040**
A full implementation of a digital transceiver based on the ARM rp2040 architecture, this transceiver is able to be operated stand alone.



### Disclaimer

This is a pure, non-for-profit, project being performed in the pure ham radio spirit of experimentation, learning and sharing.

This project is original in conception and has a significant amount of code developed by me, it does also being built up on top of a giant effort of others to develop the base platform and libraries used.

Therefore this code should not be used outside the limits of the license provided and in particular for uses other than ham radio or similar experimental purposes.

No fit for any purpose is claimed nor guaranteed, use it at your own risk. The elements used are very common and safe, the skills required very basic and limited, but again, use it at your own risk.

Despite being a software enginering professional with access to technology, infrastructure and computing facilities of different sorts  I declare this project has been performed on my own time and equipment.


### Fair and educated warning

This project is aimed for ham radio operations and can transmit on bands allocated to that service, a license is required in most countries to transmit in these bands, so please ensure yourself to fulfill the requirements needed to operate this equipment.

This project is based on simple and cheap equipment, extra care is needed in order not to produce
unwanted behaviour that could be against your local communications laws as well as making interference
to your fellow local hams.

Additional filtering might help to improve the clean output.

Remember that most national regulations requires the armonics and other spurious outcome to be -30 dB below the fundamental, specially if other services are affected.


# System design

The overall transceiver involves four major subsystems as shown in the following figure

![Alt Text](PixiePico.jpg?raw=true "PixiePico Block Diagram")

* **RF modem**
Implemented with a modified Pixie HF 7 MHz transceiver kit.

* **rp2040 Zero board**
Implemented with a Raspberry Pico Zero ARM rp2040 board with suitable firmware.

* **DDS (Direct Digital Synthesis)**
Implemented with a Clock Generator Si5351 break out board.

* **Custom control board**
Custom ad-hoc board with the following functions
   * +9V to +5V regulator.
   * RX/TX Switch.
   * Audio In (LINE-IN) signal filtering and conditioning.

Each major block can be built and tested separately, the construction effort is really small as only is required 
by the custom ad-hoc control board.

## Pixie 7 MHz CW Transceiver kit

The Pixie QRPp Kit (very low power, less than 1W output) transceiver is a very popular DIY project among hams as it is very easy to build, test and operate from the electronic standpoint, yet able to perform some actual limited communications (QSO) over the air.

Although really small power can be enough to sustain communications over great distances and therefore not being a per se limiting factor there are other factors in the basic implementation which makes difficult to carry communications except on very favorable conditions.

An explanation of how the transceiver work can be found [here](http://w1sye.org/wp-content/uploads/2017/01/NCRC_PixieOperation.pdf).

This project starts with a working Pixie transceiver (a cheap kit bought at eBay or other sellers) and to integrate it with other systems to provide the signal generation and other control functionality.

Instead of a crystal based signal generation a Si5351 general purpose clock breakout board is used as a DDS, the actual operation of the board is performed by a Raspberry Pico controller among other control functions.

The rest of the code deals mostly with the user interface and operating features, among others:

* Receive on the selected sub-band depending on the mode.
* Transmission controlled by processor.
* Capable of operating other digital modes, specially FT4, JT8 and WSPR, with very small modifications.
* Open source firmware allow the adaptation to other simple DIY QRP/QRPp projects.




## Pixie transceiver kit

The Pixie transceiver has been implemented using countless strategies, it's actually very easy to implement it 
using a homemade approach with prototype techniques.

However, this project will start with a commercially available kit which can be sourced from many places:

* AliExpress
* Bangood
* eBay
* MercadoLibre


## Pixie kit building

The kit needs to be build using the instructions provided by the kit manufacturer, however a 7 MHz CW transceiver 
isn't the goal of the project but a 28 MHz FT8 one. Therefore, several mods are needed departing from the basic
kit building. A typical circuit for the kit might be:

![Alt Text](PixiePi_Schematics.jpg?raw=true "PixiePi Schematics")

This circuit is actually used as a starting point, but some modifications are needed to:

* Improve the basic operation.
* Prepare the kit to operate in FT8 rather than CW.
* Modify the operating frequency from 7 MHz to 28 MHz.
* Integrate with CPU & command board.

Some minor modifications are needed while building the Chinese DIY Pixie kit, other versions might vary.

## Components not used

The following components typically present in the most common chinese kits needs **not** to be placed when building the kit as there are not going to provide any functionality

```
* D2 - Diode 1N4001
* R6 - R 100K
* C8 - C 100nF
* W1 - R 47K (var)
* D3 - Diode 1N4148
* Y1 - Cristal 7.032 MHz

```

## Kit modifications

In order to adapt the kit to perform as the RF modem of this project the following modifications are needed

```
* Connect Cx=100 nF on the same place than Y1 on the kit.
* Connect hole of negative side of D3 diode to the interface board PTT line
* The KEY Cut trace from R5 to KEY socket, connect both keyer legs to KEY line of the control board.
* Assure all four boards (interface, Pixie, Raspberry Pico and Si5351) share a common ground.
* Extract +9V from the Pixie +9V socket, feed the control board's LM7805 with it, then feed the Raspberry Pico board with it.
```
![Alt Text](pixie_pcb.jpg?raw=true "PixiePi PCB mods")

All additional interface circuitry might be constructed on a prototype perfboard or using the Manhattan
technique.

Also, if during the build and test process the transistors of the kit results damaged they can be
replaced by common NPN low signal transistors such as:

* Replace Q1 9018 custom OEM part by a 2N3904 transistor (in need of replacement)
* Replace Q2 8050 custom OEM part by a 2N3904 transistor (in need of replacement).



## Circuit modifications

To transform the 7 MHz CW transceiver into a 28 MHz FT8 transceiver few modifications are needed to be implemented in the Pixie Kit.

* Modified final low pass filter.
* Modified audio filter.
* Improved PA heat management.
* Broadcast Interference (BCI) reduction.


### Modified final low pass filter

The original circuit comes with a $\pi$ low pass filter and impedance matching that needs to be redesigned for

$Z_{0}=50\Omega$
$f_{0}=28 MHz$

In such condition it should provide an RF power of

$P_{o}= \frac {V_{cc}^2}{2 \dot R_{L}} \approx 810 mW$

Calculating the new values for $C_{1}$ , $C_{2}$ and $L_{1}$ the circuit is left as
![Alt Text](PixiePico_LTSpice_PiFilter_Circuit.png?raw=true "Pi Filter Frequency Response")

The frequency response of the new Pi filter would be

![Alt Text](PixiePico_LTSpice_PiFilter_Response.png?raw=true "Pi Filter Frequency Response")

The new Pixie circuit would be

![Alt Text](PixiePico_LTSpice_TX_Circuit.png?raw=true "Pixie transmitter modified circuit")

The main values of signal in the circuit will be

![Alt Text](PixiePico_LTSpice_TX_SignalAnalysis.png?raw=true "Pixie transmitter signal analysis")

The frequency response by design would be

![Alt Text](PixiePico_LTSpice_TX_FFT.png?raw=true "Pixie transmitter harmonics")

The first harmonic is -20 dB below fundamental, a little bit higher than expected, it needs to be worked out to at least -30 dB, the remaining harmonics are quite adequate.


### Modified audio filter

The original kit is intended to operate in CW whereas this kit has to be optimized to operate in low signal
modes such as *FT-8*

An audio band pass filter needs to be added between the audio coming from the local computer (running WSJT-X) and the signal processing pin of the CPU

![Alt Text](PixiePico_LTSpice_AFFilter_Circuit.png?raw=true "AF in Band Pass Filter")
![Alt Text](PixiePico_LTSpice_AFFilter_SignalAnalysis.png?raw=true "AF in filter signal analysis")
![Alt Text](PixiePico_LTSpice_AFFilter_FreqAnalysis.png?raw=true "AF in filter frequency response")


### Audio limiter 

Also, in order for the firmware to detect the frequency with high accuracy an edge detector algorithm will be used,
therefore a crisp flank need to be present in the incoming signal.

![Alt Text](PixiePico_LTSpice_AFFilterLimiter_Circuit.png?raw=true "AF Circuit and limiter")
![Alt Text](PixiePico_LTSpice_AFFilterLimiter_SignalAnalysis.png?raw=true "AF Circuit and limiter")


### Improved PA heat management

FT8 requires a 15 secs key down of continuous transmission compared with few milliseconds of a repetitive Morse code clicking pattern. Even operating with a 9V supply the final transistor might become quite hot because of the lonw keydown and probably will burn out even on casual operation. WSPR is even worse as it transmit for over 120 secs at a time.


Therefore a small heatsink protecting **Q2** of the Pixie kit is mandatory.

Space is very limited on typical kits but a small piece of aluminum might be enough, be aware not to short either L1 nor L3 with it.

The keyer transistor **Q1** of the control board will benefit from a heat sink as well if long keying times are expected (do not confuse with Q1 of the Pixie kit, refer to the project overall schematic).

A cooler can be activated during the TX period using a **GPIO5** line thru the **Q3** transistor at the , feed the cooler not from the Raspberry Pico board but from the +5V regulator or external power supply if used.




### Broadcast Interference (BCI) reduction

At some locations the Chinese DIY Pixie kit might be subject to heavy BCI in presence of a nearby AM or FM commercial broadcast station or other powerful RF source, in order to minimize try to replace R3 from 1K to 47-100 Ohms.


### Increase power and other features

An interesting set of low cost modifications to increase the power, improve efficiency and other enhancements to the original DIY Pixie Kit can be found at [link](http://vtenn.com/Blog/?p=1348).



## Other alternatives

Even if the Pixie Kit is used for the project the software could be used directly with other DIY or homebrew popular designs, among others:

* [PY2OHH's Curumim](http://py2ohh.w2c.com.br/trx/curumim/curumim.htm)
* [NorCal 49'er](http://www.norcalqrp.org/files/49er.pdf)
* [Miss Mosquita](https://www.qrpproject.de/Media/pdf/Mosquita40Engl.pdf)
* [Mosquito](http://www.qrp.cat/ea3ghs/mosquito.pdf)
* [Jersey Fireball](http://www.njqrp.club/fireball40/rev_b/fb40b_manual.pdf)

Lots of good QRPp projects can be found at [link](http://www.ncqrpp.org/) or SPRAT magazine [link](http://www.gqrp.com/sprat.htm).

#### NOT really in the same level than

This project isn't a serious alternative to some excellent DIY Kits available in the market, which deliver tons of good features at reasonable cost. This project must be seen as a learning platform enabling a very cheap DIY kit with many intrisic limitations to be used as an experimentation platform.
In particular this kit is NOT a replacement nor an alternative to the following superb kits that I'm aware of:

* QRP Lab's (Hans Summers) OCX series.
* ADX (Barb WB2CBA) board (several variants).
* CRKits (Adam Rong's) D4D kit.
* QRPGuys's DSB Digital Transceiver.
* 4S QRP Group's Cricket 40.
* None of the uSDX transceiver architecture rigs
* mcHF transceiver.
* BitX transceiver.
* ... and other similar projects ...

For all of them do yourself a favor and buy a kit, assemble it and enjoy the experience.

# Main processor (ARM rp2040 board)

The project relies very heavily on the availability of a low cost, yet very powerful, controller such as the Raspberry Pico board.

With a dual core processors running at 100 MHz and over 2 MBytes of flash memory and 256 KBytes of static memory
this is a serious platform, outperforming in processing muscle the Arduino Nano platform.

The processor performs the following function:

* Audio decoding from the computer running WSJT-X (or other similar program) to produce a FSK control signal.
* Direct SSB synthesis control over the Si5351 board.
* PTT and other housekeeping controls.
* +3.3V generator for the Si5351 board.

## rp2040 datasheet

The full spec of the CPU to be used can be found at [rp2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)


## rp2040 pinout

![Alt Text](rp2040-pinout.webp?raw=true "rp2040 Zero pinout Diagram")
![Alt Text](rp2040Z.jpeg?raw=true "rp2040 Zero pinout Diagram")

Each GPIO line has several functions which can be seen in the diagram of the larger pinout
![Alt Text](pico-pinout.svg?raw=true "Raspberry pico pinout (larger)")


# Firmware

The processor perform several tasks controlled by the firmware such as:

* Overall housekeeping of the transceiver.
* Management of the configuration (EEPROM) and operating modes.
* Generation of FSK modulation thru the Si5351 clock generator.
* Provide external control means (CAT).


## Build chain

The development environment used is the Arduino IDE, even with some limitations it's far more easy to setup and operate than the Eclipse IDE alternative and present a much smoother transition from development for the Arduino environment into the rp2040 environment.

The usage of the Arduino IDE is based on the [arduino pico core libraries developed by Earle F. Philhower, III](https://github.com/earlephilhower/arduino-pico).
In order to install it a tutorial can be found [here](https://www.tomshardware.com/how-to/program-raspberry-pi-pico-with-arduino-ide) or
[here](https://www.upesy.com/blogs/tutorials/install-raspberry-pi-pico-on-arduino-ide-software).

Basic support

* Earl E Philhower rp2040 core porting to Arduino IDE (https://github.com/earlephilhower/arduino-pico)
* SI5351 Library by Jason Mildrum (NT7S) - https://github.com/etherkit/Si5351Arduino
* Arduino "Wire.h" I2C library(built-into arduino ide)
* Arduino "EEPROM.h" EEPROM Library(built-into arduino ide)

Code excerpts gathered from manyfold sources to recognize here, large pieces of code were extracted from former projects

* [PixiePi](https://github.com/lu7did/PixiePi).
* [Pixino](https://github.com/lu7did/Pixino).
* [OrangeThunder](https://github.com/lu7did/OrangeThunder).

# Package requirements

In order to develop for the rp2040 firmware a build chain with pre-requisite libraries needs to be created

```

```

## Code architecture

The main functionality is quite similar to the baseline ADX firmware used, there are three changes needed to adapt the firmware to the rp2040
architecture.


* **I/O**.
 A mapping between the Arduino I/O pin and the rp2040 GPIO ports has been made, symbolic names were adjusted and coding macros used to replace
the primitives to operate it. Proper initialization was initroduces for all ports used.


* **EEPROM**.
The rp2040 based Raspberry Pi Pico board used to host the porting doesn't have EEPROM available, however the arduino core is able to emulate it
on flash memory, however additional definitions to initialize and commit values is needed and thus included in the ported code.





* **Tone frequency counting**.
The rp2040 processor lacks the zero cross comparator interrupt used by the ADX transceiver and thus it has been replaced using a firmware
definition on one of the PIO of the processor (RISC processor).

The code port was made starting with the [ADX_UnO_V1.3](https://github.com/WB2CBA/ADX-UnO-V1.3) as available at the GitHub site by Nov,15th 2022,
no automatic synchronization mechanism has been established with it. The overall logic cycle of the firmware can be seen in the following figure.
![Alt Text](ADX-rp2040_activity.png "ADX-rp2040 Activity")

The main functionality is contained in the file ADX_rp2040.ino which is compiled by using the Arduino IDE supplied with the stated
libraries, different subsystems are made dependent on configuration directives (#define on the microcode, typically to signal the
introduction of a porting segment by means of the #define RP2040 directive) which mades the relevant code segments  to be included
or excluded from the build process.




## Code components

```
mandatory files
 
ADX_rp2040.ino

PIO programming (counting method)

freqPIO.cpp
freqPIO.pio
freqPIO.pio.h


```

## Transmission Algorithms

The ADX transceiver by Barb (WB2CBA) owes in part it's popularity to it's simplicity, and no small part of it derives from the very simple, yet effective, way
to process incoming audio signals to derive what is the tone being sent by the host program and direct the transmitter to operate at a base
frequency plus that tone frequency in order to achieve direct synthesis USB transmission with a very low overhead mechanism which involves little or no
DSP programming which can be very taxing for the original Arduino board the transceiver has been designed with.
The algorithm comes actually from Burkhard Kainka (DK7JD) [link](http://elektronik-labor.de/HF/SDRtxFSK2.html) cleverly uses the COMPA timer of the
ATMEGA328p processor architecture into a very effective frequency meassurement with very little overhead, hardly can be considered a DSP processing, then
the information of the frequencies is applied to the frequency output to achieve the digital modulation desired. This can be made because this is a digital
transceiver and it's assumed it's used to transmit some form of a weak signal mode such as WSPR, JS8, FT4 or FT8, it can be used for CW and in general with
some limitations for any mode where the amplitude carries no information.
Unfortunately the rp2040 doesn't have such a good resource, so the frequency needs to be measured using other strategy, this in turns become a great source
of experimentation.


A PIO processor (16 of them available with the rp2040 architecture) is configured to generate an interrupt with the rising flank of the signal, a high
resolution timer (capable of +/- 1 uSec resolution) is used to compute how many ticks can be counted between two sucessive flanks. There is actually no
"zero crossing" detection as the GPIO isn't capable to trigger interruptions with that condition, but it's triggered when the signal level achieves the
ON condition of the input pin. It's assumed the trigger level is the same for all cycles at it is not a signal subject to fading, therefore the frequency
can be counted by measuring the time elapsed between similar parts of sucessive cycles.
In order for the algorithm to work a signal conditioning needs to be performed by means of external hardware, this can be made either thru a MOSFET or an IC comparato$
circuit (see hardware for details).
The actual measurement accuracy is in the order of +/- 2 Hz in the worst case, which is accurate enough for digital modes.


```

Sources of error

The counting has +/- 2 Hz error as the start and end of the counting window might include or exclude the starting and ending cycles. Also
the trigger point might suffers some variations making the actual timing between sucessive cycles inaccurate. Finally the SNR of the signal
might present noises which trigger false counts.

```


# Custom control board


# Si5351 Clock generator (DDS)
![Alt Text](si5351.jpg?raw=true "Si5351 breakout board")


# Hardware

The following is the overall schematic for the project hardware

![Alt Text](PixiePico_Schematic.png?raw=true "Pixie Pico Schematic")




## Prototype (Under construction)

This is a snapshot of the current prototype used to develop and debug this project:

![Alt Text](PixiePi_buildprocess.jpg?raw=true "PixiePi Hardware Prototype during testing")
![Alt Text](PixiePi_Build_001.jpeg?raw=true "PixiePi Hardware Prototype")
![Alt Text](PixiePi_Build_001.jpeg?raw=true "PixiePi Hardware Prototype")

# Case 3D Design (under construction)

The preliminar 3D design for a project case (with LCD) can be seen as follows
[PixiePi 3D Case Design](https://www.thingiverse.com/thing:4153649)

**Warning**
3D STL file is an unfinished work in progress (see pending at issues)




# Release notes:

  * This project requires a hardware board combining:

```
     - Raspberry Pico.
     - Si5351 breakout board
     - Pixie Transceiver kit.
     - Glueware simple electronics to switch transmitter, connect keyer and others.
```

  This setup can be used with flrig as the front-end and CAT controller (**headless mode**), it should work with any
  other software supporting a Yaesu FT-817 model CAT command set.



# Operation (OTA)

## Headless operation (all controls thru CAT) 

The kit is designed to be used in headless mode, which is the entire operation setup is controlled by the
firmware with little or no modifications allowed in the box in order to simplify the mechanical mounting.

When used in Headless mode the transceiver behaviour can still be controlled by using CAT commands
see FLRig  and RigCtl usage in the incoming sections.


## WSPR

WSPR can be operated either as a monitoring station or as a beacon.

### Monitoring station

* Plug the PHONE exit to the LINE-IN entry of a soundcard.
* Start the PixiePi program with ./bash/PixiePi.sh (correct frequency to 7074000).
* Use WSJTX to monitor, select Mode as WSPR
* Ctl-C to terminate.

Your mileage might vary depending on local CONDX, antenna available and overall noise floor at your location,
some fairly strong signals can be decoded this way, after all it's just a very simple (tiniest imaginable)
double conversion receiver at work. Follows an example of a report captured from a WSPR Beacon in PY-Land some
500+ miles North of me.

![Alt Text](PixiePi_WSPR.jpg?raw=true "WSJT-X Program using PixiePi as the receiver for WSPR monitoring")


### Beacon station

* Run ./bash/PiWSPR.sh (replace your call, grid and power before executing the program).

This program will run once, so call it repeatedely with a timing of your choice, WSPR frames align and fire on even minutes.

Follows a screen capture of some reports given by the WSPRNet monitoring network[link](http://www.wsprnet.org), a fair distance has been achieved considered
the power output were in the order of 200 mW during the tests!

![Alt Text](PixiePi_Reports_WSPR.png?raw=true "Reports from WSPRNet during the test")
![Alt Text](PixiePi_WSPR_sample.jpg?raw=true "Reports from WSPRNet during the test")



## Operating FT8

FT8 can be operated either as a monitoring station or as a beacon or as a stand-alone USB transceiver for digital modes (see operating as an
USB transceiver below).

### Monitoring station

Same as WSPR monitoring station but selecting 7074000 as the frequency and FT8 at WSJTX, your mileage might vary
depending on local CONDX, antenna settings and overall noise floor at your locations, it's just a very basic
transceiver so probably relatively strong signals will be detected.

Sample of FT8 receiving:

![Alt Text](PixiePi_FT8_sample.jpg?raw=true "WSJT-X Program using PixiePi as the receiver for FT8 monitoring")

### Beacon station

Run pift8 from the rpitx package, simultaneous monitoring and beaconing will require a larger Raspberry Pi in order
to accomodate the extra power to run WSJT-X. Sample script can be found at bash/PiFT8.sh

Follows a screen capture of some reports given by the PSKReporter monitoring network[link](http://pskreporter.info), a fair distance has been achieved considered
the power output were in the order of 200 mW during the tests!

![Alt Text](PixiePi_Reports_FT8.png?raw=true "Reports from PSKReporter network during the test")
![Alt Text](PixiePi_FT8_sample.jpg?raw=true "Reports from PSKReporter network at 40 meters")




# CAT Control

CAT control can be performed over the programs of this project as they implement a limited subset of the Yaesu FT-817 CAT Command set.
Commands are carried thru an internal pipe (created using the socat package, see bash/pixie.sh script for details) which looks to the
controlling program just like a serial port (usually /tmp/tty0).

Operated in "headless" mode, with no tunning control or LCD display, this feature can be used to operate the transceiver.

Two alternatives has been tested:


## FLRig
FLRig can be built on the Raspberry Pi and used to control the PixiePi rig just configuring it as a FT-817 with the serial port as
/tmp/ttyv0. The PixiePi program must be up a running and the pipe established when this program is run.
Being a graphic X-Window app FLRig is somewhat taxing on resources and might not perform well when using a Raspberry Pi Zero W, however
it will run just fine on a Raspberry Pi 3 or higher.

![Alt Text](PixiePi_flrig.jpg?raw=true "Controlling a PixiePi instance using FLRIG")


## RigCtl

RigCtl is a companion program of the HamLib library, it's main advantage is  a console operation with a very low resources consumption,
therefore it can be used on a Raspberry Pi Zero W to control the rig. See bash/pixie.sh in order to understand the way to configure it.

The rigctl interface is a server running in the background (rigctld) which can be accessed using a telnet interface at localhost port 4532.

See the docs/rigctl_commands.txt for a summary of the commands or visit the Hamlib page for complete documentation, the script 
PixiePi/bash/pixie.sh has sample on how to implement a pipe based to enable rigctl.




# Other packages

In general the hardware can be used to implement modulation modes proposed by the [rpitx package](https://github.com/F5OEO/rpitx).
In some cases the RF chain after the Raspberry Pi needs to be activated, the hardware on this project uses the
GPIO12 line as the PTT, some programs might require this line to be activated or deactivated externally.





