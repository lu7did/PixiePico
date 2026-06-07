---
jupyter:
  kernelspec:
    display_name: Python 3
    language: python
    name: python3
  language_info:
    codemirror_mode:
      name: ipython
      version: 3
    file_extension: .py
    mimetype: text/x-python
    name: python
    nbconvert_exporter: python
    pygments_lexer: ipython3
    version: 3.12.10
  nbformat: 4
  nbformat_minor: 5
  orig_nbformat: 4
---

-   [PixiePico](#pixiepico){#toc-pixiepico}
-   [Introduction](#introduction){#toc-introduction}
-   [Previous work](#previous-work){#toc-previous-work}
-   [Disclaimer](#disclaimer){#toc-disclaimer}
-   [Fair and educated
    warning](#fair-and-educated-warning){#toc-fair-and-educated-warning}
-   [System design](#system-design){#toc-system-design}
-   [Pixie 7 MHz CW Transceiver
    kit](#pixie-7-mhz-cw-transceiver-kit){#toc-pixie-7-mhz-cw-transceiver-kit}
-   [RF power
    discussion](#rf-power-discussion){#toc-rf-power-discussion}
-   [Pixie transceiver
    kit](#pixie-transceiver-kit){#toc-pixie-transceiver-kit}
-   [Pixie kit building](#pixie-kit-building){#toc-pixie-kit-building}
-   [Kit modifications](#kit-modifications){#toc-kit-modifications}
-   [Components not
    used](#components-not-used){#toc-components-not-used}
-   [Additional circuit
    modifications](#additional-circuit-modifications){#toc-additional-circuit-modifications}
-   [Improved PA heat
    management](#improved-pa-heat-management){#toc-improved-pa-heat-management}
-   [Broadcast Interference (BCI)
    reduction](#broadcast-interference-bci-reduction){#toc-broadcast-interference-bci-reduction}
-   [Increase power and other
    features](#increase-power-and-other-features){#toc-increase-power-and-other-features}
-   [Other alternatives](#other-alternatives){#toc-other-alternatives}
-   [High Performance RF
    Design](#high-performance-rf-design){#toc-high-performance-rf-design}
-   [PixiePico RF Specs](#pixiepico-rf-specs){#toc-pixiepico-rf-specs}
-   [Classic Pixie
    Hardware](#classic-pixie-hardware){#toc-classic-pixie-hardware}
-   [High Performance PixiePico RF
    Schematic](#high-performance-pixiepico-rf-schematic){#toc-high-performance-pixiepico-rf-schematic}
-   [Final stage class E
    design](#final-stage-class-e-design){#toc-final-stage-class-e-design}
-   [Low pass filter
    design](#low-pass-filter-design){#toc-low-pass-filter-design}
-   [Coupling and De-coupling
    components](#coupling-and-de-coupling-components){#toc-coupling-and-de-coupling-components}
-   [Driver design](#driver-design){#toc-driver-design}
    -   [$V_{GS}$ high enough](#v_gs-high-enough){#toc-v_gs-high-enough}
    -   [Maximum switching
        time](#maximum-switching-time){#toc-maximum-switching-time}
    -   [Driver current](#driver-current){#toc-driver-current}
    -   [Driver\'s power
        yield](#drivers-power-yield){#toc-drivers-power-yield}
    -   [Driver design](#driver-design-1){#toc-driver-design-1}
    -   [Driver design](#driver-design-2){#toc-driver-design-2}
-   [T/R Switch](#tr-switch){#toc-tr-switch}
-   [Audio Design](#audio-design){#toc-audio-design}
-   [Main processor (ARM rp2040
    board)](#main-processor-arm-rp2040-board){#toc-main-processor-arm-rp2040-board}
-   [rp2040 pinout](#rp2040-pinout){#toc-rp2040-pinout}
-   [rp2040Z pin
    assignment](#rp2040z-pin-assignment){#toc-rp2040z-pin-assignment}
-   [Firmware](#firmware){#toc-firmware}
-   [Custom control
    board](#custom-control-board){#toc-custom-control-board}
-   [Prototype (Under
    construction)](#prototype-under-construction){#toc-prototype-under-construction}
-   [Case 3D Design (under
    construction)](#case-3d-design-under-construction){#toc-case-3d-design-under-construction}
-   [Release notes:](#release-notes){#toc-release-notes}
-   [Operation (OTA)](#operation-ota){#toc-operation-ota}
-   [Headless operation (all controls thru
    CAT)](#headless-operation-all-controls-thru-cat){#toc-headless-operation-all-controls-thru-cat}
-   [WSPR](#wspr){#toc-wspr}
    -   [Monitoring
        station](#monitoring-station){#toc-monitoring-station}
    -   [Beacon station](#beacon-station){#toc-beacon-station}
-   [Operating FT8](#operating-ft8){#toc-operating-ft8}
    -   [Monitoring
        station](#monitoring-station){#toc-monitoring-station}
    -   [Beacon station](#beacon-station){#toc-beacon-station}
-   [CAT Control](#cat-control){#toc-cat-control}
-   [FLRig](#flrig){#toc-flrig}
-   [RigCtl](#rigctl){#toc-rigctl}
-   [Other packages](#other-packages){#toc-other-packages}

::: {#438e9f5d .cell .markdown}
# PixiePico

Ham radio FT8 experimental transceiver based on the famous Pixie 7 MHz
transceiver kit.

![Alt
Text](underconstruction.jpg?raw=true "PixiePico Project under construction")

Main features for the project are:

-   QRP $P_{0} \le 1W$ RF power.
-   +5V cc @ 2A cheap celullar power supply
-   Support for CW and digital modes (FT8, WSPR, etc).
-   RF modem for digital modes (to use with a computer running WSJTX).
-   Digital DDS implemented using a rp2040 processor.
-   Frequency agility within the band (optional).
-   LED display support (optional).
:::

::: {#209528e1 .cell .markdown}
# Introduction

Ham radio is a technical hobby enjoyed by millions of persons worldwide,
despite different country specific definitions the overall
characterization is about activities performed by individuals interested
in:

-   Understand telecomunications operating procedures.
-   Perform technical studies.
-   Experiment with electronics.

Ham radio operators uses communcations gear both home made and
commercially made alike. Depending on the technical skills of the
operator she might be able to build from relatively simple to very
sophisticated gear, the availblity of commercial equipment allows for
persons with strong interest in the communications side of the activity
to have access to sophisticated equipment as well.

As technology advances the ability for relatively novel people to craft
their own equipment fades away as it requires a substantial footprint of
knowledge, tools and lab equiptments.

Still there are opportunities, some of them offered as kits, of
relatively simple to medium complexity equipments to be built by
beginners.

As anybody with some mileage expended in the hobby might testify there
is no thrill greater than to build and equipment with our own hands and
*communicate* with it thereafter.

Ham radio communications spans over a large number of modes, both analog
and digital, as well as a significant number of bands, essentially from
very large long wave radio frequencies to very tiny microwave
frequencies.

Still the frequencies commonly referred as *HF* brings the appeal of
allowing very long distance communications with relatively simple
antenas.

Kits for the *Pixie* transceiver are common for the 7 MHz (40 mts band)
and 14 MHz (20 mts band), modifications to tune it for the 3.5 MHz (80
mts band) are somewhat easy to achieve as well.

These frequencies are a good compromise between several factors.

-   Can be operated with somewhat large but still workable portable
    antennas.
-   Materials sensitive to frequency usually aren\'t much of an issue.
-   Construction techniques are affordable for the beginner.
-   Simple and inexpensive lab equipment is available.

Extensions to other bands can be also addressed using similar hardware
as an evolution to the project.

In particular, the 28 Mhz (10 meter band) allow a worldwide
communication capability with antennas which can be acommodated even on
very restricted facilities using relatively low powers (1W or less).

A new generation of small signal digital communication modes allows
extremely very low power and relatively simple transceivers to allow
state of the art digital communications.

This project aim for the following goals:

-   Create an extremely simple yet capable HF transceiver able to
    withstand small signal digital communications in real conditions.
-   Use as many low cost and readily available parts and kits to build
    it.
    -   Pixie 7 MHz CW transceiver kit.
    -   rp2040 breakout board.
-   Introduce some additionals which, in the words of \[*VK3YE*\]
    (<https://youtu.be/roAc4c1a-a0?si=ma7CxaRRTaOutV2u)make> the Pixie
    operation \"slightly less appalling\".
-   Allow the builder to grow and extend technical skills in a number of
    domains while building it, such as:
    -   Basic understanding of the design of the different stages and
        sub-systems of the transceiver.
    -   Use of a ARM processor development environment and build chain
    -   Operate the transceiver with state-of-the-art communications
        software such as WSJT-X and others.
    -   Notions of 3D printing design techniques.
    -   Simple radio station setup procedures.
    -   Minimum test equipment and building techniques.\
-   On-the-air evaluation of the transceiver and few operating
    techniques associated with it.
:::

::: {#ec3bcc70 .cell .markdown}
## Previous work

In a general way over 50 years of ham radio activity leads to this
project, but some of the past projects are more related to this project
than others. There are few, however, that provides quite a leverage in
terms of previous experimentation, firmware libraries and the testing of
quite a few constructions ideas, a short list of them are:

-   **Pixie** Many, many, implementations of the *Pixie* designs and
    countless variants over the years, many such design and variants can
    be seen at my blog sites [lu7dz](http://lu7did.blogspot.com) and
    [lu7hz](http://lu7hz.blogspot.com).

-   **Pixino** A project to implement a uSDX firmware porting but using
    the Pixie transceiver as the platform.

-   **PixiePi** A project to create a multipurpose 7 MHz transceiver for
    digital modes based on the 7 Mhz CW Pixie kit and the Raspberry Pi 3
    board and Evariste\'s F5OEO [rpitx](https://github.com/F5OEO/rpitx)
    library.

-   **OrangeThunder** A project to create a multipurpose multimode HF
    transceiver based on the Raspberry Pi 3 board, the RTL-SDR SDR
    processor and the [rpitx](https://github.com/F5OEO/rpitx) library.

-   **ADX-rp2040** A porting of the
    [ADX](https://antrak.org.tr/blog/adx-arduino-digital-transceiver/)
    digital transceiver by Barb (WB2CBA) originally for the Arduino Nano
    (ATMEL 328p processor) into the ARM rp2040 architecture, this
    transceiver is able to operate with suitable software such as WSJT-X
    on different HF bands.

-   **RDX-rp2040** A full implementation of a digital transceiver based
    on the ARM rp2040 architecture, this transceiver is able to be
    operated stand alone.

-   **ADX-ddsPIO** A full implementation of a digital transceiver based
    on the ARM rp2040 architecture, this transceiver is able to be
    operated as a modem for computer generated digital modes (such as
    FT8, WSPR or FT4).
:::

::: {#c0dcaf80 .cell .markdown}
### Disclaimer

This is a non-profit project being performed in the pure ham radio
spirit of experimentation, learning and sharing.

This project is original in conception and has a significant amount of
code developed by me, it does also being built up on top of a giant
effort of others to develop the base platform and libraries used.

Therefore this code should not be used outside the limits of the license
provided and in particular for uses other than ham radio or similar
experimental purposes.

No fit for any purpose is claimed nor guaranteed, use it at your own
risk. The elements used are very common and safe, the skills required
very basic and limited, but again, use it at your own risk.

It\'s assumed the transmission functions will be used according with
applicable local communications laws by persons having the proper
clearance thru a license to do so.

The technical specifications are compatible with proper rules on most
countries, adequate operating practices are assumed in order not to
introduce harmful interference to other services or ham radio operators.

Despite being a software enginering professional with access to
technology, infrastructure and computing facilities of different sorts I
declare this project has been performed on my own time and equipment.
:::

::: {#cc805ab8 .cell .markdown}
### Fair and educated warning

This project is aimed for ham radio operations and can transmit on bands
allocated to that service, a license is required in most countries to
transmit in these bands, so please ensure yourself to fulfill the
requirements needed to operate this equipment.

This project is based on simple and cheap equipment, extra care is
needed in order not to produce unwanted behaviour that could be against
your local communications laws as well as making interference to your
fellow local hams.

Additional filtering might help to improve the clean output.

Remember that most national regulations requires the armonics and other
spurious outcome to be -30 dB below the fundamental, specially if other
services are affected.
:::

::: {#32572e3c .cell .markdown}
# System design

The overall transceiver involves three major subsystems as shown in the
following figure

\|
`<img src="PixiePico.png" alt="PixiePico Block Diagram" style="width:340px;height:auto;">`{=html}
\| *PixiePico System Block Diagram* \|

-   **RF modem** The project consider two alternatives to implement the
    RF chain

    -   Implement with a modified Pixie HF 7 MHz transceiver kit.
    -   Design and implement a new generation Pixie HF 7 MHz transceiver
        based on a high performance Class-E MOSFET amplifier.

-   **rp2040 Zero board** Implemented with a Raspberry Pico Zero ARM
    rp2040 board with suitable firmware.

-   **Custom control board** Custom ad-hoc board with the following
    functions

    -   +5V to +9V up-converter.
    -   RX/TX Switch.
    -   Audio In/Out filtering and conditioning.
    -   Fan control.

Each major block can be built and tested separately, the construction
effort is really small as only is required by the custom ad-hoc control
board.
:::

::: {#05d6feef .cell .markdown}
# Pixie 7 MHz CW Transceiver kit

The Pixie QRPp Kit (very low power, less than 1W output) transceiver is
a very popular DIY project among hams as it is very easy to build, test
and operate from the electronic standpoint, yet able to perform some
actual limited communications (QSO) over the air.

Although really small power can be enough to sustain communications over
great distances and therefore not being a per se limiting factor there
are other factors in the basic implementation which makes difficult to
carry communications except on very favorable conditions.

An explanation of how the transceiver work can be found
[here](http://w1sye.org/wp-content/uploads/2017/01/NCRC_PixieOperation.pdf).

This project starts with a working Pixie transceiver (a cheap kit bought
at eBay or other sellers) and to integrate it with other systems to
provide the signal generation and other control functionality.

Instead of a crystal based signal generation a Si5351 general purpose
clock breakout board is used as a DDS, the actual operation of the board
is performed by a Raspberry Pico controller among other control
functions.

The rest of the code deals mostly with the user interface and operating
features, among others:

-   Receive on the selected sub-band depending on the mode.
-   Transmission controlled by processor.
-   Capable of operating other digital modes, specially FT4, JT8 and
    WSPR, with very small modifications.
-   Open source firmware allow the adaptation to other simple DIY
    QRP/QRPp projects.
:::

::: {#45d89e3f .cell .markdown}
## RF power discussion

The **input** power in transmission mode can be estimated with:

$P_{o}=\frac{V_{cc}^2}{2R_{L}}$

With $V_{cc}$=9V and $R_{L}=50 \Omega$ then the output power would be
$P_{o}$=810 mW.

With $V_{cc}$=12V the output power would be $P_{o}$=1.44W, but even if
the power is higher the disipation (heat) on the output transitor,
specially during long key down periods, would be much higher.

Also with $V_{cc}$=5V and $R_{L}=50 \Omega$ the $P_{o}$=250 mW, however
even if the power output can be enough for local experimentation, the
main circuit will need to be modified for the +5V regulator to work
properly.

Operating using FT8 the mode has a signal-to-noise (SNR) advantage over
CW in the order of 6-10 dB, therefore even with a 9V supply iw will be
equivalent to 3.5W to 8W of output power in CW, more than adequate QRP
levels. Using the same reasoning FT8 would provide a SNR advantage over
SSB in the order of 20 dB, therefore 810 mW would provide a
communication capability equivalent to voice contact using 80W of power.

Assuming the operation will use $V_{cc}=9V$ and the conduction angle
$\theta$=180 degrees (class B) the effective output power will be

$P_{o}=\frac{v_{ef}^2}{2R_{L}}$

and the CC power will be

$P_{cc}=V_{cc}\hat{I_{cc}}= \frac{2V_{cc}^2}{\pi R_{L}}$

So the efficiency of the stage will be

$\eta=\frac {P_{o}}{P_{cc}}=\frac{\pi}{4}$=78.5%

As the conduction angle $\theta\leq$ 180 the $P_{cc}$ is reduced
according with the relation

$\eta=\frac{\theta-\sin{\theta}}{4\sin{\frac{\theta}{2}}-2\theta\cos{\frac{\theta}{2}}}$

The equilibrium between a reasonable efficiency and higher output power
is achieved when $\eta$=60% which means that about 40-50% of the output
power will be dissipated as heat.
:::

::: {#a67aa825 .cell .markdown}
## Pixie transceiver kit

The Pixie transceiver has been implemented using countless strategies,
it\'s actually very easy to implement it using a homemade approach with
prototype techniques.

However, this project will start with a commercially available kit which
can be sourced from many places such as (but not limited to):

-   AliExpress
-   Bangood
-   eBay
-   MercadoLibre
:::

::: {#16a4637b .cell .markdown}
## Pixie kit building

The kit needs to be built using the instructions provided by the kit
manufacturer. Starting from the original kit the circuit would be:

  ---------------------------------------------------------------------------------------------------------
   `<img src="PixiePi_Schematics.jpg" alt="Pixie kit schematics" style="width:640px;height:auto;">`{=html}
  ---------------------------------------------------------------------------------------------------------
                                        *Typical Pixie kit schematic*

  ---------------------------------------------------------------------------------------------------------

This circuit is actually used as a starting point, but some
modifications are needed to:

-   Improve the basic operation.
-   Prepare the kit to operate in FT8 rather than CW.
-   Integrate with CPU & command board.

The modifications to be evaluated are needed while building the Chinese
DIY Pixie kit, other versions might vary.
:::

::: {#d497b2bb .cell .markdown}
## Kit modifications

In order to adapt the kit to perform as the RF modem of this project the
following modifications are needed

-   *Do not* populate $C_3$ and $C_7$.
-   Connect Cx=100 nF (you can reuse $C_{8}$ from the kit) on the same
    place than $Y_{1}$ on the kit.
-   Connect either to the tip connector of KEY or the hole of negative
    side of D3 diode to the interface board PTT line
-   Connect the tip of the PHONE connector to the Audio line of the
    rp2040 Z.
-   Assure all four boards (interface, Pixie, rp2040 Zero and Si5351)
    share a common ground.
-   Extract +9V from the Pixie +9V socket, feed the control board\'s
    LM7805 with it, then feed the rp2040 zero board with it using the
    supply regulator shown in the PixiePico overall circuit.

  ----------------------------------------------------------------------------------------------
   `<img src="pixie_pcb.jpg" alt="Pixie kit PCB mods" style="width:480px;height:auto;">`{=html}
  ----------------------------------------------------------------------------------------------
                                       *Pixie kit PCB mods*

  ----------------------------------------------------------------------------------------------

The capacitor C8 (100nF) must be placed in the pins for the Y1 Crystal
(not used). All marks in **red** are kit components not populated,
whereas connections marked in **blue** are the signal points that needs
to be interfaced from the kit board to other places of the circuitry.

All additional interface circuitry might be constructed on a prototype
perfboard or using the Manhattan technique.

Also, if during the build and test process the transistors of the kit
results damaged they can be replaced by common NPN low signal
transistors such as:

-   Replace Q1 9018 custom OEM part by a 2N3904 transistor (in need of
    replacement)
-   Replace Q2 8050 custom OEM part by a 2N3904 transistor (in need of
    replacement).

When operation at 28 MHz is intended $L_{2}$, $C_{5}$ and $C_{6}$ needs
to be adjusted as per the values for 28MHz (modified final LPF) on top
of the firmware configuration changes needed for the *DDS* to operate at
28 MHz as well.
:::

::: {#4a2e97c6 .cell .markdown}
## Components not used

The following components typically present in the most common chinese
kits needs **not** to be placed when building the kit as there are not
going to provide any functionality

    * D2 - Diode 1N4001
    * R6 - R 100K
    * C8 - C 100nF
    * W1 - R 47K (var)
    * D3 - Diode 1N4148
    * Y1 - Cristal 7.032 MHz
:::

::: {#595a578f .cell .markdown}
## Additional circuit modifications

To transform the 7 MHz CW transceiver into a 28 MHz FT8 transceiver few
modifications are needed to be implemented in the Pixie Kit.

-   Modified audio filter.
-   Improved PA heat management.
-   Broadcast Interference (BCI) reduction.
:::

::: {#8215987e .cell .markdown}
### Improved PA heat management

FT8 requires a 15 secs key down of continuous transmission compared with
few milliseconds of a repetitive Morse code clicking pattern. Even
operating with a 9V supply the final transistor might become quite hot
because of the lonw keydown and probably will burn out even on casual
operation. WSPR is even worse as it transmit for over 120 secs at a
time.

Therefore a small heatsink protecting **Q2** of the Pixie kit is
mandatory.

Space is very limited on typical kits but a small piece of aluminum
might be enough, be aware not to short either L1 nor L3 with it.

The keyer transistor **Q1** of the control board will benefit from a
heat sink as well if long keying times are expected (do not confuse with
Q1 of the Pixie kit, refer to the project overall schematic).

A cooler can be activated during the TX period using a **GPIO5** line
thru the **Q3** transistor at the , feed the cooler not from the
Raspberry Pico board but from the +5V regulator or external power supply
if used.
:::

::: {#40e0fb3e .cell .markdown}
### Broadcast Interference (BCI) reduction

At some locations the Chinese DIY Pixie kit might be subject to heavy
BCI in presence of a nearby AM or FM commercial broadcast station or
other powerful RF source, in order to minimize try to replace R3 from 1K
to 47-100 Ohms.
:::

::: {#344429ef .cell .markdown}
### Increase power and other features

An interesting set of low cost modifications to increase the power,
improve efficiency and other enhancements to the original DIY Pixie Kit
can be found at [link](http://vtenn.com/Blog/?p=1348).
:::

::: {#11095219 .cell .markdown}
## Other alternatives

Even if the Pixie Kit is used for the project the software could be used
directly with other DIY or homebrew popular designs, among others:

-   [PY2OHH\'s
    Curumim](http://py2ohh.w2c.com.br/trx/curumim/curumim.htm)
-   [NorCal 49\'er](http://www.norcalqrp.org/files/49er.pdf)
-   [Miss
    Mosquita](https://www.qrpproject.de/Media/pdf/Mosquita40Engl.pdf)
-   [Mosquito](http://www.qrp.cat/ea3ghs/mosquito.pdf)
-   [Jersey
    Fireball](http://www.njqrp.club/fireball40/rev_b/fb40b_manual.pdf)

Lots of good QRPp projects can be found at
[link](http://www.ncqrpp.org/) or SPRAT magazine
[link](http://www.gqrp.com/sprat.htm).

#### Standard SSB transceiver

Obviously the Pixie Kit board can be replaced by **any** transceiver
capable to operate in SSB (USB) at the intended frequency, in fact in
any frequency the transceiver is capable of. Modifications needs to be
done to use the Si5351 signal to control the frequency of the
transceiver, the audio chain needs no modification.

#### NOT really in the same level than

This project isn\'t a serious alternative to some excellent DIY Kits
available in the market, which deliver tons of good features at
reasonable cost. This project must be seen as a learning platform
enabling a very cheap DIY kit with many intrisic limitations to be used
as an experimentation platform. In particular this kit is NOT a
replacement nor an alternative to the following superb kits that I\'m
aware of:

-   QRP Lab\'s (Hans Summers) OCX series.
-   ADX (Barb WB2CBA) board (several variants).
-   CRKits (Adam Rong\'s) D4D kit.
-   QRPGuys\'s DSB Digital Transceiver.
-   4S QRP Group\'s Cricket 40.
-   None of the uSDX transceiver architecture rigs
-   mcHF transceiver.
-   BitX transceiver.
-   \... and other similar projects \...

For all of them do yourself a favor and buy a kit, assemble it and enjoy
the experience.
:::

::: {#4d1ae8f0 .cell .markdown}
# High Performance RF Design

This alternative aims for the upgrade of the classical Pixie design to a
high performance, MOSFET class-E based, TX chain whose design details
and implementation are described in the following sections.
:::

::: {#86eacdef .cell .markdown}
## PixiePico RF Specs

The following specs will apply

$f_{out}=7 MHz$

$V_{DD}=+9V$

$P_{o}\leq1W$

$R_{L}=R_{ant}=50\Omega$

High efficiency requires a class E final amplifier.
:::

::: {#2ba8c4a3 .cell .markdown}
## Classic Pixie Hardware

The following is the overall hardware schematic for the project

  ------------------------------------------------------------------------------------------------------------------
   `<img src="PixiePico_Schematic.png" alt="PixiePico rp2040Z schematic" style="width:1024px;height:auto;">`{=html}
  ------------------------------------------------------------------------------------------------------------------
                                            *PixiePico Circuit schematics*

  ------------------------------------------------------------------------------------------------------------------
:::

::: {#c4773f51 .cell .markdown}
## High Performance PixiePico RF Schematic

`<img src="PixiePico_RF.png" alt="Pixie Pico Circuit" style="width:1000px;height:auto;">`{=html}
:::

::: {#41ea35e3 .cell .markdown}
## Final stage class E design

Adopting a *BS170* transistor with the following characteristics

$I_{Dmax}=500 mA$

$V_{Dmax}=60V$

$R_{DS}=1.2-1.8 \Omega (max 5\Omega)$

$V_{GS}=10V$

Output power is given in a MOSFET class E amplifier by

$P_{o}=0.577\times\frac{V_{DD}^2}{R_{L}}\approx0.577\times\frac{9^2V}{50\Omega}\approx0.93W$

Other inefficiencies will likely drive the output power to the range
\[0.5-0.8W\].

Under such conditions the maximum drain voltage $V_{DSmax}$ will be

$V_{DSpeak}\approx3.56\times{V_{DD}}\approx3.56\times9V\approx32V$

Which is safely below the maximum rating for the transistor
($V_{Dmax} \lt 60V$)

Follows the design of the $C_2$ and $L_2$ components to ensure the
proper class E operation

The actual paralell output capacitor needed $C_{sh}$ will be

$C_{sh}\approx\frac{0.1836}{\omega \times R}\approx\frac{0.1836}{2  \times \pi \times f \times R}\approx \frac{0.1836}{6.28 \times 7 Mhz \times 50 \Omega}\approx 83 pF$

Taking into the consideration the output capacitance of the transistor
(\$C\_{oss}) from the datasheet is 17pF to 30 pF therefore

$C_{2}\approx C_{sh} - C_{oss} \approx 83 pF - 17 pF \approx 66 pF$

it could either be 56 pF or 68 pF to use normalized values.

The shunt inductor $L_2$ will be

$L_{2} \approx \frac {1.1525 \times R}{2 \times \pi \times f } \approx \frac {1.1525 \times 50 \Omega}{6.28 \times \pi \times 7 MHz} \approx 1.3 \mu H$
:::

::: {#f971b198 .cell .markdown}
## Low pass filter design

The load to the transistor has to be approximately the antenna impedance
($50 \Omega$) therefore the output pass filter isn\'t required to
transform impedances, thus the main function will be to attenuate
undesired armonics

$f_{0}=7 MHz$

$f_{2}=21 MHz$

$f_{3}=21 MHz$

A 5-pole Chebyshev filter with 0,1 dB ripple and termination 50 $\Omega$
will be used, a $\pi$ configuration starting and ending with capacitors
will be used.

In the previous schematic the filter will be composed by
$C_4 , C_5 , C_6$ as well as $L_3 , L_4$.

The Chebyshev filter is an IIR type, therefore the configuration must be
carefully chosen in order to be stable, the standar design for such a
filter would use the following Chebychev coefficients:

$g_1=1.1468$

$g_2=1.3712$

$g_3=1.9750$

$g_4=1.3712$

$g_5=1.1468$

Implementing the Chebychev coefficients with components

$C_6=\frac{g_1}{2 \times \pi \times f_c \times R_{L}}$

$C_4=\frac{g_3}{2 \times \pi \times f_c \times R_{L}}$

$C_5=\frac{g_5}{2 \times \pi \times f_c \times R_{L}}$

$L_3=\frac{g_2 \times R_{L}} {2 \times \pi \times f_c }$

$L_4=\frac{g_4 \times R_{L}} {2 \times \pi \times f_c }$

In order to improve the frequency response a mid point between the max
frequency and the next armonic is taken, so $f_c=10 MHz$

Therefore the design will be

  Component   Value
  ----------- -------------
  $C_6$       390 pF
  $C_4$       680 pF
  $C_5$       390 pF
  $L_3$       1.1 $\mu$ H
  $L_4$       1.1 $\mu$ H
:::

::: {#c2ec80ad .cell .markdown}
## Coupling and De-coupling components

Several components will be used to couple and de-couple the RF circuit
with the CC circuit of the amplifier

The de-coupling capacitor $C_3$ must have a small reactance at the
operating frequency, the actual phase change is irrelevant therefore the
condition for the absolute value of the reactance $X_C\leq 5\Omega$ will
be taken. If $C_3=10nF$ is used

$X_C=\frac{1}{2 \times \pi \times f \times C}=\frac{1}{6.28 \times \pi \times 7 MHz \times 10nF}=2.26 \Omega$

The RF choke $L_1$ needs to present a high impedance to the RF in order
to avoid a path to ground thru the power supply, therefore

$X_L=2 \times \pi \times f \times L$

and

$X_L \gg Z_{load}$

The condition $X_L \approx 20 \times R_L$ is pursued

$L_1 \ge 22 \mu H$ para dar mayor margen de seguridad se toma $22 \mu H$
a $47 \mu H$

If the output power ($P_0 \approx 0.9 W$) and with an efficiency
(conservative) of $\Nu=0.7$ the power the inductor $L_1$ needs to
withstand is

$P_{CC} \approx \frac {P_0}{\Nu} \approx 1W$ and the maximum current
$I_{DD}$

$I_{DD}=\frac {P_{CC}}{V_{DD}}=\frac {1}{9}=111 mA$

to take a security margin $I_{DD} \approx 200 mA$

The RF choke $L_1$ inductor must be paired with a good power supply
decoupling

$C_7=10 \mu F$

$C_8=10 nF$

$C_9=100 nF$

will be used with that purpose.
:::

::: {#3b7bc1e5 .cell .markdown}
## Driver design

A MOSFET amplifier has a very high input impedance ($Z_in$) and
therefore very little power consumption, however the class E design
requires energy to charge, faster enough, the capacitance associated
with the MOSFET gate.

Preliminary the power needed will be

$P_{driver}\approx C_{iss} \times V_{g}^2 \times f$

The conservative case for a BS170 transistor is $C_{iss}\approx 40 pF$
so if $V_{g}=7V$

$P_{driver}\approx40 pF \times 7^2 \times 7 MHz \approx 14 mW$

To have an operational margin to account for inefficiencies and losses
20 mW will be taken.

Now, the criteria for a class E amplifier is to meet several conditions

### $V_{GS}$ high enough

The condition for a proper class E operation is for the amplifier to
switch between

$R_{DS}\approx \infty$

and

$R_{DS} \ll R_{L}$

A practical criteria might be to satisfy

$0.05 \le \frac{R_{DS_{on}}}{R_{L}} \le 0.10$

Given $R_{L}=50 \Omega$ then

$2.5 \Omega \le R_{DS_{on}} \le 5 \Omega$

The datasheet for the *BS170* MOSFET specify the $R_{DS_{on}}$ for
$V_{GS}=10V$ thus it\'s uncertain whether a lower voltage would achieve
the same performance.

### Maximum switching time

At $f=7 Mhz$ the period $T=142 nSec$, therefore for a fast operation the
condition

$\frac {T}{20} \le [t_{on},t_{off}] \le \frac {T}{10}$

must be verified

$[t_{on},t_{off}]\le$ 7 to 14 nSecs

### Driver current

The MOSFET gate is mostly a capacitance, therefore

$Q_{g}\approx C_{eq} \times V_{g}$

therefore the current to load the capacitor in a time $t_{on}$ (or
$t_{off}$ as it is a square signal)

$I_{driver}\approx \frac{Q_g}{t_{on}}$

Combining

$I_{driver}\approx \frac{C_{eq} \times V_{g}}{t_{on}}$

For the *BS170* $C_{eq}=40 pF$ (worst case) and $t_{on}=10 nSec$

The required current would be

  $V_{g}$   $I_{driver}$
  --------- -----------------
  3.3V      $\approx 14 mA$
  5.0V      $\approx 20 mA$
  7.0V      $\approx 28 mA$
  9.0V      $\approx 36 mA$

### Driver\'s power yield

The average power needed to charge and discharge the gate is given by

$P_{gate}\approx C_{eq} \times V_{g}^2 \times f$

For the *BS170* $C_{eq}=40 pF$ (worst case) and $f=7 MHz$

  $V_{g}$   $P_{gate}$
  --------- -----------------
  3.3V      $\approx 3 mW$
  5.0V      $\approx 7 mW$
  7.0V      $\approx 14 mW$
  9.0V      $\approx 23 mW$

### Driver design

If directly feed by the *GPIO* port the output voltage will be
$V_{pk-pk}=V_{g}=3.3V$ this would yield a maximum power of

$V_{rms}=\frac {V_{pk-pk}}{2}=1.65V$

$P_{gpio}=\frac {V_{rms}^2}{R_{in}}=\frac {1.65^2}{50K}=10 \mu W$

Which is way lower than needed

Additionally an efficiency consideration needs to be taken, under the
stated conditions

$I_{rms} \approx 0.2 A$

Then the losses on the internal MOSFET resistance will be

$P_{loss} \approx I_{rms}^2 \times R_{DS_{on}}$

  $R_{DS}$        $P_{loss}$
  --------------- -----------------
  $2.5 \Omega$    $\approx 10 mW$
  $5.0 \Omega$    $\approx 20 mW$
  $10.0 \Omega$   $\approx 40 mW$
  $15.0 \Omega$   $\approx 60 mW$

With any value higher than $R_{DS} \gt 2.5 \Omega$ the losses will be
way too high.

In order for the final stage to deliver with a low $R_{DS}$ value the
$V_{GS}$ needs to be as high as possible, as close to 9V will be taken.

So for the condition

$\frac{R_{DS_{on}} \times V_{GS}}{R_{L}} \ll 1$

A possible criteria would be

  Condition                               Interpretation
  --------------------------------------- ----------------
  $\frac{R_{DS_{on}}}{R_L}<0.05$          very good
  $0.05 < \frac{R_{DS_{on}}}{R_L}<0.10$   good
  $0.10 < \frac{R_{DS_{on}}}{R_L}<0.20$   marginal
  $\frac{R_{DS_{on}}}{R_L}>0.20$          poor

According with the *BS170* datasheet with $V_{GS}$ close to 10V the
$R_{DS}$ will be close to $2.5 \Omega$ meaning a very good level.

### Driver design

To design the driver the proper values of $R_1$ , $R_2$, $R_3$, $R_4$
and $R_5$ must be found.

The internal driver $C_{eq}$ capacitance is charged with a current
source formed by $R_3$ and $V_DD=9V$, the charging time must be
consistent with the switch time needed at 7 MHz.

$t_{on} \approx 2.2 \times R_3 \times C_{eq}$

with $t_{on}=10 nSec$ and $C_{eq}=40 pF$

$R_3 = \frac {t_{on}}{2.2 \times C_{eq}} \le 114 \Omega \approx 100 \Omega$

During the *ON* time

$I_{driver_{on}}\approx \frac{V_{DD}}{R_3} = \frac{9}{100}=90 mA$

With a 50% duty the power dissipation on $R_3$ would be

\$P\_{R\_{3}} \\approx \\frac {V\_{DD}\^2}{R_3}=0.8 mW

The coupling capacitor must be much larger than the $C_{eq}$ on the gate
for the next stage (40 pF)

thus $C_1 \ge 400 pF$ and 1 nF will be taken.

The value for $R_4$ must be choosen to avoid ringing but the loading
edge over $C_{eq}$ must be much smaller than the ramp up driven by
$R_3$, taking 10 times less $t_{on_{G}}\approx 2nSec$ therefore

$R_4\le \frac{2 nSec}{2.2 \times 40 pF} \approx 22 \Omega$ a range
between \[$10 \Omega, 22 \Omega$\] must be taken

$R_2$ and $R_5$ are selected to satisfy two conditions

-   Do not reduce the input impedance too much at the same time than
    biasing the gate.
-   Do not reduce the coupling between the stages together with $C_1$ at
    7 MHz.

The first criteria is met with

$R_2 = R_5 = 47 K \Omega$

The verification for the coupling is

$f_{c}=\frac{1}{2 \times \pi \times R_2 \times C_1}=\frac{1}{6.28 \times 47K \Omega \times 1 nF} \lt 4 KHz$

which is much lower than 7 MHz.

Finally, $R_1$ is selected based on the same criteria to help in the
load of the driver capacitance from the *GPIO* port.

Using $R_{1}=68 \Omega$ the switching time of the driver

$t_{on_{driver}}=2.2 \times 68 \Omega \times 40 pF \approx 6 nSec$

Which is deemed appropriate for 7 MHz.
:::

::: {#7b65254e .cell .markdown}
## T/R Switch

The previous design is to establish the TX chain, however in order to
operate as a transceiver the final stage needs to be biased for
operation at a much lower level.

To avoid the introduction of complex circuitry to implement the T/R
change the same strategy than the Pixie is ussed.

A resistor ($R_6$) is added during reception to the drain circuit, the
PTT switch by-pass it during transmission returning the circuit to the
previous design and making the output power to rise to the maximum
possible.

In order for the circuit to operate as a mixer the $I_{D}$ current needs
to be set to a minimum, to the point the transistor barely conducts
which biases the stage at the most non-linear part of the response.

With full signal from the driver the drain current needs to be limited
by a resistance, with $V_{D_{min}}=2V$ and $I_{D_{min}}=1 mA$

$R_{S}\approx \frac{9V-V_{D_{min}}}{I_{D_{min}}}\approx 7K \Omega$

For a lower current $R_{S}=10 K\Omega$ might be appropriate.

During transmission the *PTT* short circuits the $R_S$ and thus makes
the drain current to be the maximum.
:::

::: {#4bebf4ef .cell .markdown}
## Audio Design

The same audio design used in the original *Pixie transceiver* based on
the LM386 chip is used.
:::

::: {#865028a1 .cell .markdown}
# Main processor (ARM rp2040 board)

The project relies very heavily on the availability of a low cost, yet
very powerful, controller such as the *Raspberry Pico* board.

With a dual core processors running at 100 MHz and over 2 MBytes of
flash memory and 256 KBytes of static memory this is a serious platform,
outperforming in processing muscle the Arduino Nano platform.

The processor performs the following function:

-   Audio decoding from the computer running WSJT-X (or other similar
    program) to produce a FSK control signal.
-   Direct SSB synthesis using excerpts from Roman Piksaykin\'s
    libraries \[<piksaykin@gmail.com>\], R2BDY
    <https://www.qrz.com/db/r2bdycontrol>
-   PTT and other housekeeping controls.

In particular the *Waveshare rp2040-zero* board is adopted for this
project.
:::

::: {#c0f93e1c .cell .markdown}
## rp2040 pinout

  ------------------------------------------------------------------------------------------------------------
   `<img src="rp2040-pinout.webp" alt="rp2040 Zero pinout Diagram" style="width:1024px;height:auto;">`{=html}
  ------------------------------------------------------------------------------------------------------------
                                              *rp2040 Zero pinout*

  ------------------------------------------------------------------------------------------------------------

The full spec of the CPU to be used can be found at [rp2040
Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
:::

::: {#064d4965 .cell .markdown}
## rp2040Z pin assignment

   ![Alt Text](PixiePico_pinout.png?raw=true "rp2040Z pin assignment")
  ---------------------------------------------------------------------
                         *rp2040 pin assignment*
:::

::: {#1b14b4a1 .cell .markdown}
# Firmware

*Work in progress*

This project uses the firmware supported by the project
[ADX-rp2040-Z](http://www.github.com/lu7did/ADX-rp2040), proceed with
the installation and configuration instructions defined in the
repository for that project.
:::

::: {#71a46c0f .cell .markdown}
# Custom control board

Work in process
:::

::: {#83624716 .cell .markdown}
:::

::: {#cb7f58a3 .cell .markdown}
## Prototype (Under construction)

This is a snapshot of the current prototype used to develop and debug
this project:

![Alt
Text](PixiePi_buildprocess.jpg?raw=true "PixiePi Hardware Prototype during testing")
![Alt
Text](PixiePi_Build_001.jpeg?raw=true "PixiePi Hardware Prototype")
![Alt
Text](PixiePi_Build_001.jpeg?raw=true "PixiePi Hardware Prototype")
:::

::: {#e5775d5f .cell .markdown}
# Case 3D Design (under construction)

The preliminar 3D design for a project case (with LCD) can be seen as
follows [PixiePi 3D Case
Design](https://www.thingiverse.com/thing:4153649)

**Warning** 3D STL file is an unfinished work in progress (see pending
at issues)
:::

::: {#2caf5145 .cell .markdown}
# Release notes:

-   This project requires a hardware board combining:

<!-- -->

         - Raspberry Pico.
         - Pixie Transceiver kit.
         - Glueware simple electronics to switch transmitter, connect keyer and others.

This setup can be used with flrig as the front-end and CAT controller
(**headless mode**), it should work with any other software supporting a
Yaesu FT-817 model CAT command set.
:::

::: {#1fc4000b .cell .markdown}
# Operation (OTA)
:::

::: {#3771280f .cell .markdown}
## Headless operation (all controls thru CAT)

The kit is designed to be used in headless mode, which is the entire
operation setup is controlled by the firmware with little or no
modifications allowed in the box in order to simplify the mechanical
mounting.

When used in Headless mode the transceiver behaviour can still be
controlled by using CAT commands see FLRig and RigCtl usage in the
incoming sections.
:::

::: {#2089381b .cell .markdown}
## WSPR

WSPR can be operated either as a monitoring station or as a beacon.

### Monitoring station

-   Plug the PHONE exit to the LINE-IN entry of a soundcard.
-   Start the PixiePi program with ./bash/PixiePi.sh (correct frequency
    to 7074000).
-   Use WSJTX to monitor, select Mode as WSPR
-   Ctl-C to terminate.

Your mileage might vary depending on local CONDX, antenna available and
overall noise floor at your location, some fairly strong signals can be
decoded this way, after all it\'s just a very simple (tiniest
imaginable) double conversion receiver at work. Follows an example of a
report captured from a WSPR Beacon in PY-Land some 500+ miles North of
me.

![Alt
Text](PixiePi_WSPR.jpg?raw=true "WSJT-X Program using PixiePi as the receiver for WSPR monitoring")

### Beacon station

-   Run ./bash/PiWSPR.sh (replace your call, grid and power before
    executing the program).

This program will run once, so call it repeatedely with a timing of your
choice, WSPR frames align and fire on even minutes.

Follows a screen capture of some reports given by the WSPRNet monitoring
network[link](http://www.wsprnet.org), a fair distance has been achieved
considered the power output were in the order of 200 mW during the
tests!

![Alt
Text](PixiePi_Reports_WSPR.png?raw=true "Reports from WSPRNet during the test")
![Alt
Text](PixiePi_WSPR_sample.jpg?raw=true "Reports from WSPRNet during the test")
:::

::: {#041d6c12 .cell .markdown}
## Operating FT8

FT8 can be operated either as a monitoring station or as a beacon or as
a stand-alone USB transceiver for digital modes (see operating as an USB
transceiver below).

### Monitoring station {#monitoring-station}

Same as WSPR monitoring station but selecting 7074000 as the frequency
and FT8 at WSJTX, your mileage might vary depending on local CONDX,
antenna settings and overall noise floor at your locations, it\'s just a
very basic transceiver so probably relatively strong signals will be
detected.

Sample of FT8 receiving:

![Alt
Text](PixiePi_FT8_sample.jpg?raw=true "WSJT-X Program using PixiePi as the receiver for FT8 monitoring")

### Beacon station {#beacon-station}

Run pift8 from the rpitx package, simultaneous monitoring and beaconing
will require a larger Raspberry Pi in order to accomodate the extra
power to run WSJT-X. Sample script can be found at bash/PiFT8.sh

Follows a screen capture of some reports given by the PSKReporter
monitoring network[link](http://pskreporter.info), a fair distance has
been achieved considered the power output were in the order of 200 mW
during the tests!

![Alt
Text](PixiePi_Reports_FT8.png?raw=true "Reports from PSKReporter network during the test")
![Alt
Text](PixiePi_FT8_sample.jpg?raw=true "Reports from PSKReporter network at 40 meters")
:::

::: {#58dca86d .cell .markdown}
# CAT Control

CAT control can be performed over the programs of this project as they
implement a limited subset of the Yaesu FT-817 CAT Command set. Commands
are carried thru an internal pipe (created using the socat package, see
bash/pixie.sh script for details) which looks to the controlling program
just like a serial port (usually /tmp/tty0).

Operated in \"headless\" mode, with no tunning control or LCD display,
this feature can be used to operate the transceiver.

Two alternatives has been tested:
:::

::: {#8278b63e .cell .markdown}
## FLRig

FLRig can be built on the Raspberry Pi and used to control the PixiePi
rig just configuring it as a FT-817 with the serial port as /tmp/ttyv0.
The PixiePi program must be up a running and the pipe established when
this program is run. Being a graphic X-Window app FLRig is somewhat
taxing on resources and might not perform well when using a Raspberry Pi
Zero W, however it will run just fine on a Raspberry Pi 3 or higher.

![Alt
Text](PixiePi_flrig.jpg?raw=true "Controlling a PixiePi instance using FLRIG")
:::

::: {#d9251a5f .cell .markdown}
## RigCtl

RigCtl is a companion program of the HamLib library, it\'s main
advantage is a console operation with a very low resources consumption,
therefore it can be used on a Raspberry Pi Zero W to control the rig.
See bash/pixie.sh in order to understand the way to configure it.

The rigctl interface is a server running in the background (rigctld)
which can be accessed using a telnet interface at localhost port 4532.

See the docs/rigctl_commands.txt for a summary of the commands or visit
the Hamlib page for complete documentation, the script
PixiePi/bash/pixie.sh has sample on how to implement a pipe based to
enable rigctl.
:::

::: {#358832bf .cell .markdown}
# Other packages

In general the hardware can be used to implement modulation modes
proposed by the [rpitx package](https://github.com/F5OEO/rpitx). In some
cases the RF chain after the Raspberry Pi needs to be activated, the
hardware on this project uses the GPIO12 line as the PTT, some programs
might require this line to be activated or deactivated externally.
:::

::: {#8af202aa .cell .markdown}
[TOC Generator](https://luciopaiva.com/markdown-toc/) jupyter nbconvert
PixiePico.ipynb \--to markdown Convert online at
<https://markdownindexgenerator.netlify.app/>
:::

::: {#afcbd1ea .cell .markdown}
# Table of contents

# Index

-   [PixiePico](#pixiepico)
-   [Introduction](#introduction)
    -   [Previous work](#previous-work)
        -   [Disclaimer](#disclaimer)
        -   [Fair and educated warning](#fair-and-educated-warning)
-   [System design](#system-design)
    -   [Pixie 7 MHz CW Transceiver kit](#pixie-mhz-cw-transceiver-kit)
    -   [RF power discussion](#rf-power-discussion)
    -   [Pixie transceiver kit](#pixie-transceiver-kit)
    -   [Pixie kit building](#pixie-kit-building)
    -   [Components not used](#components-not-used)
        -   [Operating frequency changes](#operating-frequency-changes)
    -   [Modified frequency](#modified-frequency)
    -   [Kit modifications](#kit-modifications)
    -   [Additional circuit
        modifications](#additional-circuit-modifications)
        -   [Improved PA heat management](#improved-pa-heat-management)
        -   [Broadcast Interference (BCI)
            reduction](#broadcast-interference-bci-reduction)
        -   [Increase power and other
            features](#increase-power-and-other-features)
    -   [Other alternatives](#other-alternatives)
        -   [Standard SSB transceiver](#standard-ssb-transceiver)
        -   [NOT really in the same level
            than](#not-really-in-the-same-level-than)
-   [Main processor (ARM rp2040
    board)](#main-processor-arm-rp2040-board)
    -   [rp2040 datasheet](#rp2040-datasheet)
    -   [rp2040 pinout](#rp2040-pinout)
    -   [rp2040Z pin assignment](#rp2040z-pin-assignment)
-   [Firmware](#firmware)
-   [Custom control board](#custom-control-board)
-   [Si5351 Clock generator (DDS)](#si5351-clock-generator-dds)
-   [Hardware](#hardware)
    -   [Prototype (Under construction)](#prototype-under-construction)
-   [Case 3D Design (under
    construction)](#case-d-design-under-construction)
-   [Release notes:](#release-notes)
-   [Operation (OTA)](#operation-ota)
    -   [Headless operation (all controls thru
        CAT)](#headless-operation-all-controls-thru-cat)
    -   [WSPR](#wspr)
        -   [Monitoring station](#monitoring-station)
        -   [Beacon station](#beacon-station)
    -   [Operating FT8](#operating-ft8)
        -   [Monitoring station](#monitoring-station)
        -   [Beacon station](#beacon-station)
-   [CAT Control](#cat-control)
    -   [FLRig](#flrig)
    -   [RigCtl](#rigctl)
-   [Other packages](#other-packages)
-   [Table of contents](#table-of-contents)
:::
