Mesh Bee
------------

![image](http://www.seeedstudio.com/wiki/images/6/6b/QQ20140327-1.png)

#### Abstract

MeshBee is a 2.4GHz wireless zigbee module from seeed studio. It uses microchip JN516x from NXP that enables
several different flvors of standards-based zigbee mesh networking. Our released firmware fully supports **ZigBee Pro** stack.
You can use MeshBee in two different ways: Master, Slave. In master mode,The factory firmware wraps the complicated ZigBee stack 
operation into a few easy to use serial commands(AT Command), Mesh Bee can also work as a transparent serial communication node that can be 
part of a simple point-to-point connection or a complex mesh network.In slave mode,user can control it through API frame.
 
Mesh Bee will bring you lots of fun.
 

#### 1.Software Architecture

##### 1.1 Schematic diagram

![image](https://raw.githubusercontent.com/CasyWang/Mesh_Bee/MeshBee_v1002/doc/MeshBeeArchitecture.jpg)

##### 1.2 Slave mode(Also called API mode)
Type AT command "ATAP" to enter API mode. In this mode,other MCU connects to MeshBee and sends API frame through UART.

###### 1.2.1 What's API mode
API is simplfy a set of standard interfaces created to allow other MCU to interact with MeshBee.For our purposes,API supports
local control and remote control.For instance,PC can send an API frame to Coordinator A, A received this frame,and 
execute sleeping command. The most important thing to note is that APIs are specifically engineered to enable MeshBee to talk 
efficiently to other MCU.

Every transfer of information requires a protocol. We defined the API frame format like this(structure defined in firmware_at_api.h):

![image](https://raw.githubusercontent.com/CasyWang/Mesh_Bee/MeshBee_v1002/doc/ApiSpec_Frame.jpg)

Each frame has a start delimiter for sync. 
Different types of frames contain different types of data structures,the Api Identifier tells us what type of API frame we are looking at.
Cmd name indicate which command you want to execute.

User can use our released library(On going) to package API frame. 

###### 1.2.2 Network Topology at API mode

In API mode,user's MCU which connected to the Coordinator, has the ability to access every node in network.

![image](https://raw.githubusercontent.com/CasyWang/Mesh_Bee/MeshBee_v1002/doc/MeshNetwork.jpg)

##### 1.3 Master mode(Also called MCU mode)
The most exciting thing to announce is that an arduino-ful user programming space(AUPS),by which you can treat MeshBee 
as a wireless arduino, was provided.You can develop a stand-alone application in AUPS. The user application consists of two arduino-style functions
at the top level: setup & loop, which act just like arduino's.
	Just typing "ATMC", MeshBee will enter MCU mode. the arduino_loop() function will execute periodically.

##### 1.4 AT console
	Typing "+++" will enter AT console no matter which mode you are in.
	refer to our AT command manual.
	
##### 1.5 Data mode(transparent data mode)
	Use "ATDT" to enter DATA mode, in that mode, two connected node can exchange data peer to peer. 

##### 1.6 What's a Suli  

We introduced Suli too. Suli means Seeed Unified Library Interface. We'll switch to Suli for our future module driver/library release. That means our suli-compatible library/driver will adapt all platforms that suli supporting. Please glance over https://github.com/Seeed-Studio/Suli for more information. 



#### 2. Usage

1. Install the SDK toolchain;
2. Download this repo and put all files into a directory A (example);
3. Move directory A to $TOOLCHAIN_INSTALL_DIR/Application;
4. Open eclipse of the toolchain, "File" -> "Import". 
5. Or you can build this repo by command line: Open "Jennic Bash Shell" -> change directory to "A/build", then execute "./build.sh" to build all targets.

The details are descripted at [this](http://www.seeedstudio.com/wiki/Mesh_Bee) wiki page, please launch there and find your need.

#### 3. Contribution

Contributing to this software is warmly welcomed. You can do this basically by
[forking](https://help.github.com/articles/fork-a-repo), committing modifications and then [pulling requests](https://help.github.com/articles/using-pull-requests) (follow the links above for operating guide). Adding change log and your contact into file header is encouraged.  

Thanks for your contribution.  

Want to know more about our firmware development process? Please check out this link: [http://seeed-studio.github.io/Mesh_Bee/](http://seeed-studio.github.io/Mesh_Bee/).

And more, you can post on our discussion group at [here](https://groups.google.com/forum/#!forum/seeedstudio-mesh-bee-discussion-group) and share your ideas.

    
----

This software is written by Jack Shao (xuguang.shao@seeedstudio.com) & Oliver Wang (long.wang@seeedstudio.com) for seeed studio<br>
and is licensed under [The MIT License](http://opensource.org/licenses/mit-license.php). Check License.txt for more information.<br>


Seeed Studio is an open hardware facilitation company based in Shenzhen, China. <br>
Benefiting from local manufacture power and convenient global logistic system, <br>
we integrate resources to serve new era of innovation. Seeed also works with <br>
global distributors and partners to push open hardware movement.<br>









[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/Seeed-Studio/mesh_bee/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

[![Analytics](https://ga-beacon.appspot.com/UA-46589105-3/Mesh_Bee)](https://github.com/igrigorik/ga-beacon)
